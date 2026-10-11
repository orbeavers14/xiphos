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
    @Published var message = "Loading installed modules…"
    @Published var showChapter = false
    @Published var verseNumbers = true
    @Published var highlightSelected = true
    @Published var renderedAnchor: String?
    @Published var choices: ReferenceChoices?
    @Published var canBack = false
    @Published var canForward = false
    @Published var isBusy = true
    @Published private(set) var engineOptions: EngineOptionFlags?
    @Published private(set) var moduleOptions = ModuleOptionInfo.unknown
    private let worker: CoreWorker?
    private var tab: UInt64?
    private var displayedEntry: RenderedEntry?
    private var displayedModule: String?
    private var revision: UInt64 = 0
    private var closed = false
    init(worker: CoreWorker?, startupError: String?) {
        self.worker = worker
        guard worker != nil else {
            message = startupError ?? "Unable to start the reading engine."
            isBusy = false
            return
        }
        Task { [weak self] in await self?.start() }
    }
    deinit {
        if let worker, let tab { Task { try? await worker.closeTab(tab) } }
    }
    func close() {
        guard !closed else { return }
        closed = true
        revision &+= 1
        if let worker, let tab {
            self.tab = nil
            Task { try? await worker.closeTab(tab) }
        }
    }
    private func start() async {
        guard let worker else { return }
        defer { if !closed { isBusy = false } }
        do {
            let discovered = try await worker.modules()
            guard !closed else { return }
            let opened = try await worker.createTab()
            guard !closed else { try? await worker.closeTab(opened); return }
            tab = opened
            try await worker.enableModulePreferences(tab: opened)
            guard !closed else { return }
            modules = discovered
            selection = modules.first(where: { $0.type == "Biblical Texts" })?.id
            if modules.isEmpty { message = "No installed SWORD modules found. Install modules with the existing Xiphos application, then reopen this preview." }
            else if let selection {
                let entry = try await worker.visit(tab: opened, module: selection, reference: reference)
                try await present(entry, module: selection, token: revision)
            } else { message = "Select a Bible or commentary module to read a verse." }
        } catch { if !closed { message = error.localizedDescription } }
    }
    private func run(_ action: @escaping @MainActor (UInt64) async throws -> Void) {
        guard !closed, !isBusy, tab != nil else { return }
        revision &+= 1
        let token = revision
        isBusy = true
        Task {
            defer { if token == revision && !closed { isBusy = false } }
            guard token == revision && !closed else { return }
            do { try await action(token) }
            catch {
                guard token == revision && !closed else { return }
                message = error.localizedDescription
                if let worker, let tab, let history = try? await worker.history(tab) {
                    guard token == revision && !closed else { return }
                    canBack = history.canBack; canForward = history.canForward
                }
            }
        }
    }
    func selectionChanged() {
        guard selection != displayedModule else { return }
        read()
    }
    func read() {
        guard let worker, let tab, let selection else { return }
        let submitted = reference
        run { token in
            let entry = try await worker.visit(tab: tab, module: selection, reference: submitted)
            try await self.present(entry, module: selection, token: token)
        }
    }
    private func present(_ entry: RenderedEntry, module: String, token: UInt64, updateInput: Bool = true) async throws {
        guard let worker, let tab else { return }
        let chapterMode = showChapter
        let numbers = verseNumbers, highlight = highlightSelected
        let chapter = chapterMode ? try await worker.readTabChapter(tab: tab, module: module, reference: entry.reference,
                                                                  verseNumbers: numbers, highlightSelected: highlight) : nil
        let selectors = try await worker.referenceChoices(module: module, reference: entry.reference)
        let history = try await worker.history(tab)
        let options = try await worker.moduleEngineOptions(tab: tab, module: module)
        guard token == revision && !closed else { return }
        engineOptions = options
        moduleOptions = modules.first(where: { $0.id == module })?.options ?? .unknown
        if let chapter {
            html = chapter.html; renderedAnchor = chapter.selectedAnchor; renderedReference = chapter.title
            message = chapter.verses.allSatisfy { $0.html.isEmpty } ? "This module has no text in this chapter." : ""
        } else {
            html = entry.html; renderedAnchor = nil; renderedReference = entry.reference
            message = entry.html.isEmpty ? "This module has no text at this reference." : ""
        }
        choices = selectors
        if updateInput { reference = entry.reference; selection = module }
        displayedEntry = entry; displayedModule = module
        canBack = history.canBack; canForward = history.canForward
    }
    func navigate(_ direction: Int32) {
        guard let worker, let tab else { return }
        run { token in
            let entry = try await worker.navigate(tab: tab, direction: direction)
            let history = try await worker.history(tab)
            try await self.present(entry, module: history.module ?? "", token: token)
        }
    }
    func refreshDisplay() {
        guard let entry = displayedEntry, let module = displayedModule else { return }
        run { token in try await self.present(entry, module: module, token: token, updateInput: false) }
    }
    func setReadingOption(_ flag: EngineOptionFlags, enabled: Bool) {
        let supported: EngineOptionFlags = [.wordsInRed, .strongs, .morphology, .footnotes]
        guard supported.contains(flag), flag.rawValue.nonzeroBitCount == 1,
              moduleOptions.declared.contains(flag), var options = engineOptions,
              let worker, let tab, let module = displayedModule, let entry = displayedEntry else { return }
        if enabled { options.insert(flag) } else { options.remove(flag) }
        let updated = options
        run { token in
            try await worker.setModuleEngineOptions(tab: tab, module: module, options: updated)
            guard token == self.revision && !self.closed else { return }
            self.engineOptions = updated
            let refreshed = try await worker.readTabEntry(tab: tab, module: module, reference: entry.reference)
            try await self.present(refreshed, module: module, token: token, updateInput: false)
        }
    }
    func navigateChapter(_ direction: Int32) {
        guard let worker, let tab, let module = displayedModule, let entry = displayedEntry else { return }
        run { token in
            let next = try await worker.navigateChapter(tab: tab, module: module, reference: entry.reference, direction: direction)
            try await self.present(next, module: module, token: token)
        }
    }
    func chooseReference(_ target: String) {
        guard let worker, let tab, let module = displayedModule else { return }
        run { token in
            let entry = try await worker.visit(tab: tab, module: module, reference: target)
            try await self.present(entry, module: module, token: token)
        }
    }
    func followLink(_ uri: String) {
        guard let worker, let tab, let module = displayedModule else { return }
        run { token in
            let target = try await worker.resolveLink(uri, currentModule: module)
            let entry = try await worker.visit(tab: tab, module: target.module, reference: target.reference)
            try await self.present(entry, module: target.module, token: token)
        }
    }
    func clearHistory() {
        guard let worker, let tab else { return }
        run { token in
            try await worker.clearHistory(tab)
            let history = try await worker.history(tab)
            guard token == self.revision && !self.closed else { return }
            self.canBack = history.canBack; self.canForward = history.canForward
        }
    }
}

struct PassageView: NSViewRepresentable {
    let html: String
    let anchor: String?
    var onScrollFailure: (String) -> Void
    var onLink: (String) -> Void
    @MainActor final class Coordinator: NSObject, WKNavigationDelegate {
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
                     decisionHandler: @escaping @MainActor (WKNavigationActionPolicy) -> Void) {
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
            /* Existing Xiphos editor/template.h styles this SWORD class red. */
            span.wordsOfJesus { color: red; }
            .xc-verse { scroll-margin-top: 12px; }
            .xc-verse.xc-current { background: Highlight; color: HighlightText; }
            .xc-verse sup { font-size: 0.7em; padding-right: 4px; }
            </style>
            </head><body>\(html)</body></html>
            """, baseURL: nil)
    }
}

struct ReferenceMenu: View {
    let title: String
    let label: String
    let choices: [ReferenceChoice]
    let select: (String) -> Void
    var body: some View {
        Menu("\(title): \(label)") {
            ForEach(Array(choices.enumerated()), id: \.offset) { _, choice in
                Button(choice.label) { select(choice.reference) }
            }
        }.accessibilityLabel("\(title) selector")
    }
}

struct ReaderWindow: View {
    @StateObject private var model: ReaderModel
    init(worker: CoreWorker?, startupError: String?) {
        _model = StateObject(wrappedValue: ReaderModel(worker: worker, startupError: startupError))
    }
    @ViewBuilder private func readingOption(_ title: String, flag: EngineOptionFlags) -> some View {
        if model.moduleOptions.declared.contains(flag) {
            Toggle(title, isOn: Binding(
                get: { model.engineOptions?.contains(flag) ?? false },
                set: { model.setReadingOption(flag, enabled: $0) }))
        }
    }
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
                .disabled(model.isBusy)
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
                }.padding(12).disabled(model.isBusy)
                if let choices = model.choices {
                    HStack {
                        ReferenceMenu(title: "Book", label: choices.bookLabel, choices: choices.books, select: model.chooseReference)
                        ReferenceMenu(title: "Chapter", label: choices.chapterLabel, choices: choices.chapters, select: model.chooseReference)
                        ReferenceMenu(title: "Verse", label: choices.verseLabel, choices: choices.verses, select: model.chooseReference)
                    }.padding(.horizontal, 12).padding(.bottom, 8).disabled(model.isBusy)
                }
                HStack {
                    Toggle("Whole chapter", isOn: $model.showChapter)
                        .onChange(of: model.showChapter) { _ in model.refreshDisplay() }
                    Menu("Display") {
                        Toggle("Verse numbers", isOn: $model.verseNumbers)
                        Toggle("Highlight selected verse", isOn: $model.highlightSelected)
                    }.disabled(!model.showChapter)
                    Menu("Reading options") {
                        readingOption("Words of Christ in Red", flag: .wordsInRed)
                        readingOption("Strong's Numbers", flag: .strongs)
                        readingOption("Morphological Tags", flag: .morphology)
                        readingOption("Footnotes", flag: .footnotes)
                    }.disabled(model.engineOptions == nil || model.moduleOptions.declared.intersection(
                        [.wordsInRed, .strongs, .morphology, .footnotes]).isEmpty)
                }.padding(.horizontal, 12).padding(.bottom, 8).disabled(model.isBusy)
                HStack {
                    Button("Previous chapter") { model.navigateChapter(-1) }.disabled(model.selection == nil)
                    Button("Next chapter") { model.navigateChapter(1) }.disabled(model.selection == nil)
                }.padding(.horizontal, 12).padding(.bottom, 8).disabled(model.isBusy)
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
                if model.isBusy {
                    HStack { ProgressView().controlSize(.small); Text("Loading…").font(.caption) }.padding(8)
                }
                Divider()
                Text("Xiphos native preview • Chapter reading and history slice").font(.caption).foregroundStyle(.secondary).padding(8)
            }.frame(minWidth: 450)
        }.frame(minWidth: 700, minHeight: 450)
            .onChange(of: model.selection) { _ in model.selectionChanged() }
            .onChange(of: model.verseNumbers) { _ in model.refreshDisplay() }
            .onChange(of: model.highlightSelected) { _ in model.refreshDisplay() }
            .onDisappear { model.close() }
    }
}

#if !XIPHOS_PREVIEW_TEST
@main
struct XiphosNativePreview: App {
    private let worker: CoreWorker?
    private let startupError: String?
    init() {
        do {
            worker = try CoreWorker(root: ProcessInfo.processInfo.environment["XIPHOS_MODULE_ROOT"])
            startupError = nil
        } catch { worker = nil; startupError = error.localizedDescription }
    }
    var body: some Scene {
        WindowGroup("Xiphos Native Preview") { ReaderWindow(worker: worker, startupError: startupError) }
    }
}

#endif
