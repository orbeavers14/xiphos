// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import SwiftUI
import WebKit

@MainActor
final class ReaderModel: ObservableObject {
    @Published var modules: [InstalledModule] = []
    @Published var selection: String?
    @Published var reference = "John 3:16"
    @Published var renderedReference = ""
    @Published var html = ""
    @Published var message = ""
    @Published var showChapter = false
    @Published var verseNumbers = true
    @Published var highlightSelected = true
    @Published var renderedAnchor: String?
    @Published var canBack = false
    @Published var canForward = false
    private var reader: CoreReader?
    private var session: CoreSession?
    private var displayedEntry: RenderedEntry?
    private var displayedModule: String?
    init() {
        do {
            reader = try CoreReader(root: ProcessInfo.processInfo.environment["XIPHOS_MODULE_ROOT"])
            session = try CoreSession(reader: reader!)
            modules = reader!.modules
            selection = modules.first(where: { $0.type == "Biblical Texts" })?.id
            if modules.isEmpty { message = "No installed SWORD modules found. Install modules with the existing Xiphos application, then reopen this preview." }
            else if selection == nil { message = "Select a Bible or commentary module to read a verse." }
            else { read() }
        } catch { message = error.localizedDescription }
    }
    func read() {
        guard let session, let selection else { return }
        do {
            let entry = try session.visit(module: selection, reference: reference)
            updateHistoryButtons()
            try display(entry, module: selection)
        } catch {
            html = ""
            renderedReference = ""
            renderedAnchor = nil
            displayedEntry = nil
            displayedModule = nil
            message = error.localizedDescription
        }
    }
    private func updateHistoryButtons() {
        canBack = session?.canBack ?? false
        canForward = session?.canForward ?? false
    }
    func navigate(_ direction: Int32) {
        guard let session else { return }
        do {
            let entry = try session.navigate(direction)
            selection = session.historyModule
            reference = entry.reference
            try display(entry, module: selection ?? "")
            updateHistoryButtons()
        } catch { message = error.localizedDescription }
    }
    private func display(_ entry: RenderedEntry, module: String) throws {
        if showChapter, let reader {
            let chapter = try reader.readChapter(module: module, reference: entry.reference,
                                                 verseNumbers: verseNumbers, highlightSelected: highlightSelected)
            html = chapter.html
            renderedAnchor = chapter.selectedAnchor
            renderedReference = chapter.title
            message = chapter.verses.allSatisfy { $0.html.isEmpty } ? "This module has no text in this chapter." : ""
        } else {
            html = entry.html
            renderedAnchor = nil
            renderedReference = entry.reference
            message = entry.html.isEmpty ? "This module has no text at this reference." : ""
        }
        displayedEntry = entry
        displayedModule = module
    }
    func refreshDisplay() {
        guard let entry = displayedEntry, let module = displayedModule else { return }
        do { try display(entry, module: module) }
        catch { message = error.localizedDescription }
    }
    func navigateChapter(_ direction: Int32) {
        guard let session, let selection else { return }
        do {
            let entry = try session.navigateChapter(module: selection, reference: reference, direction: direction)
            reference = entry.reference
            try display(entry, module: selection)
            updateHistoryButtons()
        } catch { message = "The adjacent chapter is unavailable. Check the reference and module." }
    }
    func followLink(_ uri: String) {
        guard let reader, let session, let current = displayedModule else { return }
        do {
            let target = try reader.resolveLink(uri, currentModule: current)
            let entry = try session.visit(module: target.module, reference: target.reference)
            // Unsupported links/errors keep the committed passage and history.
            reference = entry.reference
            try display(entry, module: target.module)
            selection = target.module
            updateHistoryButtons()
        } catch { message = "This link is unavailable in this preview. Passage ranges, notes and other study actions are still pending." }
    }
    func clearHistory() { session?.clearHistory(); updateHistoryButtons() }
}

struct PassageView: NSViewRepresentable {
    let html: String
    let anchor: String?
    var onScrollFailure: (String) -> Void
    var onLink: (String) -> Void
    final class Coordinator: NSObject, WKNavigationDelegate {
        var loaded: String?
        var anchor: String?
        var navigation: WKNavigation?
        var onScrollFailure: ((String) -> Void)?
        var onLink: ((String) -> Void)?
        func scrollToAnchor(_ webView: WKWebView) {
            guard let anchor,
                  let data = try? JSONSerialization.data(withJSONObject: anchor, options: [.fragmentsAllowed]),
                  let literal = String(data: data, encoding: .utf8) else { return }
            // Trusted host code runs in an isolated world. Module scripts stay
            // disabled and are never interpolated into this expression.
            webView.evaluateJavaScript("document.getElementById(\(literal))?.scrollIntoView({block: 'start'});",
                in: nil, in: .defaultClient) { [weak self] result in
                    if case .failure = result { self?.onScrollFailure?("Unable to scroll to the selected verse.") }
                }
        }
        func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
            guard navigation === self.navigation else { return }
            scrollToAnchor(webView)
        }
        func webView(_ webView: WKWebView, decidePolicyFor action: WKNavigationAction,
                     decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
            if action.navigationType == .linkActivated, let url = action.request.url {
                decisionHandler(.cancel)
                // Cancel browser navigation first; only the core interprets commands.
                onLink?(url.absoluteString)
            } else {
                decisionHandler(action.navigationType == .other && action.request.url?.scheme == "about" ? .allow : .cancel)
            }
        }
    }
    func makeCoordinator() -> Coordinator { Coordinator() }
    func makeNSView(context: Context) -> WKWebView {
        let config = WKWebViewConfiguration()
        config.websiteDataStore = .nonPersistent()
        config.defaultWebpagePreferences.allowsContentJavaScript = false
        let view = WKWebView(frame: .zero, configuration: config)
        view.navigationDelegate = context.coordinator
        return view
    }
    func updateNSView(_ view: WKWebView, context: Context) {
        context.coordinator.onScrollFailure = onScrollFailure
        context.coordinator.onLink = onLink
        let changedAnchor = context.coordinator.anchor != anchor
        context.coordinator.anchor = anchor
        guard context.coordinator.loaded != html else {
            if changedAnchor { context.coordinator.scrollToAnchor(view) }
            return
        }
        context.coordinator.loaded = html
        context.coordinator.navigation = view.loadHTMLString("""
            <!doctype html><html><head><meta charset="utf-8">
            <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'">
            <style>:root { color-scheme: light dark; } body { font: 18px Georgia, serif; line-height: 1.55; margin: 24px; }
            .xc-verse { scroll-margin-top: 12px; }
            .xc-verse.xc-current { background: Highlight; color: HighlightText; }
            .xc-verse sup { font-size: 0.7em; padding-right: 4px; }
            </style>
            </head><body>\(html)</body></html>
            """, baseURL: nil)
    }
}

struct ReaderWindow: View {
    @StateObject private var model = ReaderModel()
    var body: some View {
        HSplitView {
            List(selection: $model.selection) {
                ForEach(Array(Set(model.modules.map(\.type))).sorted(), id: \.self) { type in
                    Section(type) {
                        ForEach(model.modules.filter { $0.type == type }) { module in
                            VStack(alignment: .leading, spacing: 3) {
                                Text(module.id)
                                Text(module.description).font(.caption).foregroundStyle(.secondary).lineLimit(2)
                            }.tag(module.id)
                        }
                    }
                }
            }.frame(minWidth: 190, idealWidth: 230, maxWidth: 320)
                .accessibilityLabel("Installed SWORD modules")
            VStack(alignment: .leading, spacing: 0) {
                HStack {
                    Button("Back") { model.navigate(-1) }.disabled(!model.canBack)
                    Button("Forward") { model.navigate(1) }.disabled(!model.canForward)
                    Menu("History") {
                        Button("Clear History") { model.clearHistory() }
                    }
                    TextField("Passage reference", text: $model.reference).onSubmit { model.read() }
                        .accessibilityLabel("Passage reference")
                    Button("Read") { model.read() }.disabled(model.selection == nil)
                }.padding(12)
                HStack {
                    Toggle("Whole chapter", isOn: $model.showChapter)
                        .onChange(of: model.showChapter) { _ in model.refreshDisplay() }
                    Menu("Display") {
                        Toggle("Verse numbers", isOn: $model.verseNumbers)
                        Toggle("Highlight selected verse", isOn: $model.highlightSelected)
                    }.disabled(!model.showChapter)
                    Button("Previous chapter") { model.navigateChapter(-1) }.disabled(model.selection == nil)
                    Button("Next chapter") { model.navigateChapter(1) }.disabled(model.selection == nil)
                }.padding(.horizontal, 12).padding(.bottom, 8)
                Divider()
                if !model.renderedReference.isEmpty {
                    Text(model.renderedReference).font(.headline).padding(12)
                }
                PassageView(html: model.html, anchor: model.renderedAnchor,
                            onScrollFailure: { model.message = $0 }, onLink: { model.followLink($0) })
                if !model.message.isEmpty {
                    Text(model.message).foregroundStyle(.secondary).padding(12)
                        .accessibilityLabel(model.message)
                }
                Divider()
                Text("Xiphos native preview • Chapter reading and history slice").font(.caption).foregroundStyle(.secondary).padding(8)
            }.frame(minWidth: 450)
        }.frame(minWidth: 700, minHeight: 450)
            .onChange(of: model.selection) { _ in model.read() }
            .onChange(of: model.verseNumbers) { _ in model.refreshDisplay() }
            .onChange(of: model.highlightSelected) { _ in model.refreshDisplay() }
    }
}

@main
struct XiphosNativePreview: App {
    var body: some Scene {
        WindowGroup("Xiphos Native Preview") { ReaderWindow() }
    }
}
