// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import SwiftUI
import WebKit

private struct ReadingOptionImportError: LocalizedError {
    var errorDescription: String? { "Unable to import reading options. Choose a readable Xiphos modops.conf file." }
}

private struct ReadingOptionSaveError: LocalizedError {
    var errorDescription: String? { "Unable to save reading options. Choose a writable file location." }
}

@MainActor
final class ReaderModel: ObservableObject {
    @Published var modules: [InstalledModule] = []
    @Published var selection: String?
    @Published var dictionarySelection: String?
    @Published var dictionaryKey = ""
    @Published private(set) var dictionaryEntry: RenderedDictionaryEntry?
    @Published private(set) var dictionarySource: PassageTarget?
    @Published var showLexiconPreferences = false
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
    @Published private(set) var referencePreview: RenderedReferenceList?
    @Published private(set) var lexicalPreview: RenderedLexicalEntry?
    @Published private(set) var studyNote: RenderedStudyNote?
    @Published private(set) var lexiconPreferences: LexiconPreferences?
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
            try await worker.enableLexiconPreferences(tab: opened)
            let lexicons = try await worker.lexiconPreferences(tab: opened)
            guard !closed else { return }
            lexiconPreferences = lexicons
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
    func applyLexiconPreferences(_ submitted: LexiconPreferences) {
        guard !closed, !isBusy, let worker, let tab, let previous = lexiconPreferences else { return }
        // Legacy "-- Select --" entries do not replace the existing preference.
        let replacement = LexiconPreferences(greek: submitted.greek ?? previous.greek,
            hebrew: submitted.hebrew ?? previous.hebrew,
            greekMorphNT: submitted.greekMorphNT ?? previous.greekMorphNT,
            greekMorphOT: submitted.greekMorphOT ?? previous.greekMorphOT,
            hebrewMorph: submitted.hebrewMorph ?? previous.hebrewMorph)
        let fields: [(String?, String?, LexiconRoles)] = [(replacement.greek, previous.greek, .greekDefinition),
            (replacement.hebrew, previous.hebrew, .hebrewDefinition),
            (replacement.greekMorphNT, previous.greekMorphNT, .greekMorphology),
            (replacement.greekMorphOT, previous.greekMorphOT, .greekMorphology),
            (replacement.hebrewMorph, previous.hebrewMorph, .hebrewMorphology)]
        guard fields.allSatisfy({ value, prior, role in
            value == prior || modules.contains(where: { $0.id == value && $0.isDictionary && $0.lexiconRoles.contains(role) })
        }) else { message = "Choose a dictionary from the corresponding lexicon list."; return }
        run { token in
            try await worker.setLexiconPreferences(tab: tab, preferences: replacement)
            let copied = try await worker.lexiconPreferences(tab: tab)
            guard token == self.revision && !self.closed else { return }
            self.lexiconPreferences = copied
            self.message = "Lexicon choices applied to this window."
        }
    }
    func selectionChanged() {
        guard !closed, !isBusy, selection != displayedModule else { return }
        if let chosen = selection, modules.contains(where: { $0.id == chosen && $0.isDictionary }) {
            selection = displayedModule
            dictionarySelection = chosen
            if !dictionaryKey.isEmpty { lookupDictionary() }
            return
        }
        read()
    }
    func lookupDictionary() {
        guard !closed, !isBusy, let worker, let tab, let module = dictionarySelection,
              modules.contains(where: { $0.id == module && $0.isDictionary }) else { return }
        let key = dictionaryKey
        guard !key.isEmpty else { message = "Enter a dictionary word or key."; return }
        let source = displayedModule.flatMap { id in displayedEntry.map { PassageTarget(module: id, reference: $0.reference) } }
        run { token in
            let entry = try await worker.readDictionary(tab: tab, module: module, key: key)
            guard token == self.revision && !self.closed else { return }
            self.dictionaryEntry = entry; self.dictionaryKey = entry.key; self.dictionarySource = source
            self.lexicalPreview = nil; self.message = ""
        }
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
        referencePreview = nil
        studyNote = nil
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
    func saveReadingOptions(path: String) {
        guard let worker, let tab else { return }
        run { token in
            do { try await worker.saveModulePreferences(tab: tab, path: path) }
            catch { throw ReadingOptionSaveError() }
            guard token == self.revision && !self.closed else { return }
            self.message = "Reading options saved."
        }
    }
    func importReadingOptions(path: String) {
        guard let worker, let tab else { return }
        let module = displayedModule, committed = displayedEntry
        run { token in
            do { try await worker.importModulePreferences(tab: tab, path: path) }
            catch { throw ReadingOptionImportError() }
            guard token == self.revision && !self.closed else { return }
            if let module, let committed {
                self.engineOptions = try await worker.moduleEngineOptions(tab: tab, module: module)
                let refreshed = try await worker.readTabEntry(tab: tab, module: module, reference: committed.reference)
                try await self.present(refreshed, module: module, token: token, updateInput: false)
            } else { self.message = "Reading options imported." }
        }
    }
    func selectVariant(_ reading: VariantReading) {
        guard engineOptions != nil, moduleOptions.declared.contains(.variantsPrimary),
              let worker, let tab, let module = displayedModule, let entry = displayedEntry else { return }
        run { token in
            try await worker.selectModuleVariant(tab: tab, module: module, reading: reading)
            let updated = try await worker.moduleEngineOptions(tab: tab, module: module)
            guard token == self.revision && !self.closed else { return }
            self.engineOptions = updated
            let refreshed = try await worker.readTabEntry(tab: tab, module: module, reference: entry.reference)
            try await self.present(refreshed, module: module, token: token, updateInput: false)
        }
    }
    func setReadingOption(_ flag: EngineOptionFlags, enabled: Bool) {
        let supported: EngineOptionFlags = [.wordsInRed, .strongs, .morphology, .footnotes, .greekAccents, .crossReferences, .hebrewVowels, .hebrewCantillation, .lemmas, .transliteratedForms, .glosses, .morphemeSegmentation]
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
    func followLink(_ uri: String, currentModule: String? = nil, currentReference: String? = nil) {
        guard let worker, let tab, let displayedModule, let displayedEntry else { return }
        let module = currentModule ?? displayedModule
        let context = currentReference ?? displayedEntry.reference
        run { token in
            switch try await worker.resolveStudyLinkAt(tab: tab, currentModule: module, currentReference: context, uri: uri) {
            case .note(let note):
                guard token == self.revision && !self.closed else { return }
                self.referencePreview = nil
                self.studyNote = note; self.message = ""
            case .references(let references):
                guard token == self.revision && !self.closed else { return }
                if currentReference == nil { self.studyNote = nil }
                self.referencePreview = references; self.message = ""
            case .lexical(let lexical):
                guard token == self.revision && !self.closed else { return }
                self.lexicalPreview = lexical
                self.dictionarySelection = lexical.entry.module; self.dictionaryKey = lexical.entry.key
                self.dictionaryEntry = lexical.entry
                self.dictionarySource = PassageTarget(module: lexical.module, reference: lexical.sourceReference)
                self.message = ""
            case .passage(let target):
                let entry = try await worker.visit(tab: tab, module: target.module, reference: target.reference)
                try await self.present(entry, module: target.module, token: token)
            }
        }
    }
    func showReferencedPassages() {
        guard let note = studyNote, !note.referenceList.isEmpty, let worker, let tab else { return }
        run { token in
            let references = try await worker.readReferenceList(tab: tab, module: note.module,
                expression: note.referenceList, context: note.reference)
            guard token == self.revision && !self.closed else { return }
            self.referencePreview = references; self.message = ""
        }
    }
    func showStudyNoteBody() { referencePreview = nil }
    func closeLexicalPreview() { guard !closed, !isBusy else { return }; lexicalPreview = nil; dictionaryEntry = nil; dictionarySource = nil; dictionarySelection = nil }
    func closeStudyNote() { referencePreview = nil; studyNote = nil }
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
            <base href="xiphos-preview://study/">
            <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'">
            <style>:root { color-scheme: light dark; } body { font: 18px Georgia, serif; line-height: 1.55; margin: 24px; }
            /* Existing Xiphos editor/template.h styles this SWORD class red. */
            span.wordsOfJesus { color: red; }
            .xc-reference { margin-bottom: 20px; } .xc-reference header { font-weight: bold; }
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

@MainActor private final class LexiconDraft: ObservableObject {
    @Published var preferences: LexiconPreferences
    init(preferences: LexiconPreferences) { self.preferences = preferences }
}

private struct LexiconPreferencesView: View {
    @Environment(\.dismiss) private var dismiss
    @ObservedObject var model: ReaderModel
    @StateObject private var draft: LexiconDraft
    init(model: ReaderModel, preferences: LexiconPreferences) {
        self.model = model; _draft = StateObject(wrappedValue: LexiconDraft(preferences: preferences))
    }
    private func picker(_ title: String, selection: Binding<String?>, role: LexiconRoles) -> some View {
        let choices = model.modules.filter { $0.isDictionary && $0.lexiconRoles.contains(role) }
        return Picker(title, selection: selection) {
            Text("-- Select --").tag(nil as String?)
            if let current = selection.wrappedValue, !choices.contains(where: { $0.id == current }) {
                Text(current.isEmpty ? "-- Select --" : "\(current) (unavailable)").tag(Optional(current))
            }
            ForEach(choices) { Text($0.id).tag(Optional($0.id)) }
        }
    }
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Lexicon dictionaries").font(.title2)
            Text("Choose the dictionaries used for original-language study in this window.")
            Form {
                picker("Greek lexicon", selection: $draft.preferences.greek, role: .greekDefinition)
                picker("Hebrew lexicon", selection: $draft.preferences.hebrew, role: .hebrewDefinition)
                picker("Greek morphology (New Testament)", selection: $draft.preferences.greekMorphNT, role: .greekMorphology)
                picker("Greek morphology (Old Testament)", selection: $draft.preferences.greekMorphOT, role: .greekMorphology)
                picker("Hebrew morphology", selection: $draft.preferences.hebrewMorph, role: .hebrewMorphology)
            }
            HStack {
                Spacer()
                Button("Cancel") { dismiss() }.keyboardShortcut(.cancelAction)
                Button("Apply") { model.applyLexiconPreferences(draft.preferences); dismiss() }.keyboardShortcut(.defaultAction)
            }
        }.padding(24).frame(width: 560).disabled(model.isBusy)
    }
}

struct ReaderWindow: View {
    @StateObject private var model: ReaderModel
    init(worker: CoreWorker?, startupError: String?) {
        _model = StateObject(wrappedValue: ReaderModel(worker: worker, startupError: startupError))
    }
    private func choosePreferenceFile() {
        let panel = NSOpenPanel()
        panel.title = "Import Xiphos reading options"
        panel.message = "Choose Xiphos's modops.conf to load your reading options."
        panel.prompt = "Import"
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            model.importReadingOptions(path: url.path)
        }
    }
    private func chooseSaveFile() {
        let panel = NSSavePanel()
        panel.title = "Save Xiphos reading options"
        panel.message = "Save your reading options in a Xiphos modops.conf file."
        panel.prompt = "Save"
        panel.nameFieldStringValue = "modops.conf"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            model.saveReadingOptions(path: url.path)
        }
    }
    @ViewBuilder private func readingOption(_ title: String, flag: EngineOptionFlags) -> some View {
        if model.moduleOptions.declared.contains(flag) {
            Toggle(title, isOn: Binding(
                get: { model.engineOptions?.contains(flag) ?? false },
                set: { model.setReadingOption(flag, enabled: $0) }))
        }
    }
    private func variantOption(_ title: String, flag: EngineOptionFlags, reading: VariantReading) -> some View {
        Toggle(title, isOn: Binding(get: { model.engineOptions?.contains(flag) ?? false },
                                   set: { _ in model.selectVariant(reading) }))
    }
    var body: some View {
        HSplitView {
            VSplitView {
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
                }.frame(minHeight: 120)
                    .disabled(model.isBusy)
                    .accessibilityLabel("Installed SWORD modules")
                if model.studyNote != nil || model.referencePreview != nil {
                    VStack(alignment: .leading, spacing: 0) {
                        HStack {
                            Text(model.referencePreview != nil ? "Referenced passages" : (model.studyNote?.type == "crossReference" ? "Cross-reference" : "Study note")).font(.headline)
                            Spacer()
                            Button("Close") { model.closeStudyNote() }.disabled(model.isBusy)
                        }.padding(8)
                        if let references = model.referencePreview {
                            Text("\(references.module) • \(references.context)").font(.caption).padding(.horizontal, 8)
                            HStack {
                                Text("\(references.entries.count) \(references.entries.count == 1 ? "passage" : "passages")").font(.caption)
                                Spacer()
                                if model.studyNote != nil {
                                    Button("Back to note") { model.showStudyNoteBody() }.disabled(model.isBusy)
                                }
                            }.padding(8)
                            if references.limited {
                                Text("Some ranges reached Xiphos's preview limit.").font(.caption).padding(.horizontal, 8)
                            }
                            if references.skipped > 0 {
                                Text("\(references.skipped) references could not be resolved.").font(.caption).padding(.horizontal, 8)
                            }
                            if references.entries.isEmpty {
                                Text("No referenced passages found.").foregroundStyle(.secondary).padding()
                            } else {
                                PassageView(html: references.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                            onLink: { model.followLink($0, currentModule: references.module, currentReference: references.context) })
                            }
                        } else if let note = model.studyNote {
                            Text("\(note.module) • \(note.reference)").font(.caption).padding(.horizontal, 8)
                            if !note.referenceList.isEmpty {
                                Button("Show referenced passages") { model.showReferencedPassages() }.disabled(model.isBusy).padding(8)
                            }
                            PassageView(html: note.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                        onLink: { model.followLink($0, currentModule: note.module, currentReference: note.reference) })
                        }
                    }.frame(minHeight: 140, idealHeight: 220)
                }
            }.frame(minWidth: 190, idealWidth: 230, maxWidth: 320)
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
                        readingOption("Cross-references", flag: .crossReferences)
                        readingOption("Greek Accents", flag: .greekAccents)
                        readingOption("Hebrew Vowel Points", flag: .hebrewVowels)
                        readingOption("Hebrew Cantillation", flag: .hebrewCantillation)
                        readingOption("Lemmas", flag: .lemmas)
                        readingOption("Transliterated Forms", flag: .transliteratedForms)
                        readingOption("Glosses", flag: .glosses)
                        readingOption("Morpheme Segmentation", flag: .morphemeSegmentation)
                        if model.moduleOptions.declared.contains(.variantsPrimary) {
                            Menu("Textual Variants") {
                                variantOption("Primary Reading", flag: .variantsPrimary, reading: .primary)
                                variantOption("Secondary Reading", flag: .variantsSecondary, reading: .secondary)
                                variantOption("All Readings", flag: .variantsAll, reading: .all)
                            }
                        }
                    }.disabled(model.engineOptions == nil || model.moduleOptions.declared.intersection(
                        [.wordsInRed, .strongs, .morphology, .footnotes, .greekAccents, .crossReferences, .hebrewVowels, .hebrewCantillation, .lemmas, .transliteratedForms, .glosses, .morphemeSegmentation, .variantsPrimary]).isEmpty)
                }.padding(.horizontal, 12).padding(.bottom, 8).disabled(model.isBusy)
                HStack {
                    Menu("Preferences") {
                        Button("Import reading options…") { choosePreferenceFile() }
                        Button("Save reading options…") { chooseSaveFile() }
                        Divider()
                        Button("Lexicon dictionaries…") { model.showLexiconPreferences = true }.disabled(model.lexiconPreferences == nil)
                    }.disabled(model.modules.isEmpty)
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
            if model.dictionarySelection != nil {
                VStack(alignment: .leading, spacing: 0) {
                    HStack {
                        Text("Dictionary").font(.headline)
                        Spacer()
                        Button("Close") { model.closeLexicalPreview() }.disabled(model.isBusy)
                    }.padding(8)
                    Picker("Dictionary module", selection: $model.dictionarySelection) {
                        ForEach(model.modules.filter(\.isDictionary)) { module in
                            Text(module.id).tag(Optional(module.id))
                        }
                    }.padding(8).disabled(model.isBusy)
                    HStack {
                        TextField("Dictionary word or key", text: $model.dictionaryKey).onSubmit { model.lookupDictionary() }
                            .accessibilityLabel("Dictionary word or key")
                        Button("Look up") { model.lookupDictionary() }
                    }.padding(8).disabled(model.isBusy)
                    Divider()
                    if let entry = model.dictionaryEntry {
                        Text("\(entry.module) • \(entry.key)").font(.caption).padding(8)
                        PassageView(html: entry.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                    onLink: { model.followLink($0, currentModule: model.dictionarySource?.module, currentReference: model.dictionarySource?.reference) })
                    } else {
                        Text("Enter a word or key to look up.").foregroundStyle(.secondary).padding()
                        Spacer()
                    }
                }.frame(minWidth: 230, idealWidth: 300)
            }
        }.frame(minWidth: 700, minHeight: 450)
            .onChange(of: model.selection) { _ in model.selectionChanged() }
            .onChange(of: model.verseNumbers) { _ in model.refreshDisplay() }
            .onChange(of: model.highlightSelected) { _ in model.refreshDisplay() }
            .sheet(isPresented: $model.showLexiconPreferences) {
                if let preferences = model.lexiconPreferences { LexiconPreferencesView(model: model, preferences: preferences) }
            }
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
