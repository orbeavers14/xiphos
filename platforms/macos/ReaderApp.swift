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
    @Published var canBack = false
    @Published var canForward = false
    private var reader: CoreReader?
    private var session: CoreSession?
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
            let chapter = try reader.readChapter(module: module, reference: entry.reference)
            html = chapter.html
            renderedReference = chapter.title
            message = chapter.verses.allSatisfy { $0.html.isEmpty } ? "This module has no text in this chapter." : ""
        } else {
            html = entry.html
            renderedReference = entry.reference
            message = entry.html.isEmpty ? "This module has no text at this reference." : ""
        }
    }
    func refreshDisplay() {
        guard let reader, let selection else { return }
        do { try display(reader.read(module: selection, reference: reference), module: selection) }
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
    func clearHistory() { session?.clearHistory(); updateHistoryButtons() }
}

struct PassageView: NSViewRepresentable {
    let html: String
    final class Coordinator: NSObject, WKNavigationDelegate {
        var loaded: String?
        func webView(_ webView: WKWebView, decidePolicyFor action: WKNavigationAction,
                     decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
            // Semantic link dispatch is a later parity slice. Keep this preview
            // offline and do not accidentally navigate away from the study pane.
            decisionHandler(action.navigationType == .other && action.request.url?.scheme == "about" ? .allow : .cancel)
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
        guard context.coordinator.loaded != html else { return }
        context.coordinator.loaded = html
        view.loadHTMLString("""
            <!doctype html><html><head><meta charset="utf-8">
            <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'">
            <style>:root { color-scheme: light dark; } body { font: 18px Georgia, serif; line-height: 1.55; margin: 24px; }</style>
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
                    Button("Previous chapter") { model.navigateChapter(-1) }.disabled(model.selection == nil)
                    Button("Next chapter") { model.navigateChapter(1) }.disabled(model.selection == nil)
                }.padding(.horizontal, 12).padding(.bottom, 8)
                Divider()
                if !model.renderedReference.isEmpty {
                    Text(model.renderedReference).font(.headline).padding(12)
                }
                PassageView(html: model.html)
                if !model.message.isEmpty {
                    Text(model.message).foregroundStyle(.secondary).padding(12)
                        .accessibilityLabel(model.message)
                }
                Divider()
                Text("Xiphos native preview • Chapter reading and history slice").font(.caption).foregroundStyle(.secondary).padding(8)
            }.frame(minWidth: 450)
        }.frame(minWidth: 700, minHeight: 450)
            .onChange(of: model.selection) { _ in model.read() }
    }
}

@main
struct XiphosNativePreview: App {
    var body: some Scene {
        WindowGroup("Xiphos Native Preview") { ReaderWindow() }
    }
}
