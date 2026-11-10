// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import SwiftUI
import WebKit
import Darwin

// Match legacy main.c before starting SWORD workers. Locale stays host-owned;
// changing it while engines run is unsupported. Invalid environment retains
// the process's existing locale, as in the legacy startup call.
@MainActor
func initializePreviewLocale() -> String? {
    guard let locale = setlocale(LC_ALL, "") else { return nil }
    return String(cString: locale)
}

private struct BookSelectionError: LocalizedError {
    var errorDescription: String? { "Unable to read this book section. Check the module and key or path." }
}

private struct BookLinkError: LocalizedError {
    var errorDescription: String? { "Unable to open this link in the displayed book." }
}

private struct ReadingImageError: LocalizedError {
    var errorDescription: String? { "Unable to display this image." }
}

private struct ReadingOptionImportError: LocalizedError {
    var errorDescription: String? { "Unable to import reading options. Choose a readable Xiphos modops.conf file." }
}

private struct ReadingOptionSaveError: LocalizedError {
    var errorDescription: String? { "Unable to save reading options. Choose a writable file location." }
}
private struct LexiconProfileImportError: LocalizedError {
    var errorDescription: String? { "Unable to import lexicon choices. Choose a readable Xiphos settings file." }
}
private struct LexiconProfileSaveError: LocalizedError {
    var errorDescription: String? { "Unable to save lexicon choices. Choose a writable Xiphos settings file or a new location." }
}

@MainActor
final class ReaderModel: ObservableObject {
    let search = SearchModel()
    @Published private(set) var parallelChapter: CoreParallelChapter?
    @Published private(set) var showingParallel = false
    var parallelProfilePath: String?
    @Published private(set) var parallelSourceNotice=""
    @Published private(set) var commentaryDecoration: CoreDecoratedReading?
    private func prepareParallel(_ raw: CoreParallelChapter) async -> CoreParallelChapter {
        parallelSourceNotice="";guard readingSources.enabled,let worker,let tab else {return raw}
        do {let value=try await worker.decoratedParallel(tab:tab,sources:readingSources)
            if value.annotationIncomplete {parallelSourceNotice="Some parallel bookmark or note records could not be displayed."}
            if value.invalidColors>0 {parallelSourceNotice += " Invalid colors were omitted."};return value
        } catch {parallelSourceNotice="Parallel notes or bookmarks could not be loaded. Reading remains available.";return raw}
    }
    func openParallel(path: String? = nil,key: String? = nil) {
        guard !closed,!isBusy else { return }
        if path == nil,key == nil,parallelChapter != nil { showingParallel=true;return }
        guard let worker,let tab,let path=path ?? parallelProfilePath else { return }
        let reference=displayedEntry?.reference ?? self.reference
        run { token in
            var chapter=try await worker.visitParallelProfile(tab:tab,path:path,key:key,reference:reference,flags:self.engineOptions?.rawValue ?? 0)
            chapter=await self.prepareParallel(chapter)
            guard token==self.revision,!self.closed else { return };self.parallelChapter=chapter;self.showingParallel=true
        }
    }
    func navigateParallel(_ direction: Int32) {
        guard let worker,let tab else { return }
        run { token in let raw=try await worker.navigateParallel(tab:tab,direction:direction);let chapter=await self.prepareParallel(raw);guard token==self.revision,!self.closed else { return };self.parallelChapter=chapter }
    }
    func closeParallel() { guard !isBusy else { return };showingParallel=false }
    @Published var readingSources = CoreReadingSources()
    @Published private(set) var readingSourceNotice = ""
    private var sourceChange: UInt64 = 0
    func applyReadingSources(_ sources: CoreReadingSources) {
        guard sources.valid,!closed else {return};readingSources=sources;sourceChange &+= 1;let serial=sourceChange
        if !isBusy {refreshDisplay();return}
        Task {while isBusy {guard !closed,serial==sourceChange else {return};try? await Task.sleep(nanoseconds:10_000_000)};guard !closed,serial==sourceChange else {return};refreshDisplay()}
    }
    @Published var modules: [InstalledModule] = []
    @Published private(set) var libraryOrganization = CoreLibraryOrganization.empty
    @Published private(set) var libraryOrganizationNotice = ""
    func applyLibraryOrganization(_ value: CoreLibraryOrganization, notice: String = "") { libraryOrganization=value;libraryOrganizationNotice=notice }
    func isFavorite(_ module: String) -> Bool { libraryOrganization.preference(provider:"sword",item:module)?.favorite ?? false }
    var sidebarModules: [InstalledModule] {
        modules.filter { libraryOrganization.preference(provider:"sword",item:$0.id)?.hidden != true }.sorted {
            let first=isFavorite($0.id),second=isFavorite($1.id);return first == second ? $0.id < $1.id : first
        }
    }
    func sidebarSection(_ module: InstalledModule) -> String {
        let group=libraryOrganization.preference(provider:"sword",item:module.id)?.group ?? ""
        return group.isEmpty ? module.type : "Group: " + group
    }
    struct SidebarGroup: Identifiable { let id: String, modules: [InstalledModule] }
    var sidebarGroups: [SidebarGroup] {
        let grouped=Dictionary(grouping:sidebarModules,by:sidebarSection)
        return grouped.keys.sorted().map { SidebarGroup(id:$0,modules:grouped[$0]!) }
    }

    @Published var selection: String?
    @Published private(set) var commentarySelection = ""
    @Published private(set) var commentaryEntry: RenderedEntry?
    @Published private(set) var commentaryChapter: RenderedChapter?
    @Published private(set) var showCommentaryChapter = false
    @Published private(set) var commentaryHeadings = true
    @Published private(set) var contentOptions = ContentDisplayOptions.unknown
    @Published private(set) var commentaryContentOptions = ContentDisplayOptions.unknown
    @Published private(set) var showingBook = false
    @Published private(set) var bookImage: BookImagePreview?
    @Published private(set) var verseImage: VerseImagePreview?
    @Published private(set) var bookNote: RenderedBookNote?
    @Published private(set) var bookDocument: RenderedBookDocument?
    @Published private(set) var bookAnchor: String?
    @Published private(set) var bookAnchorRevision: UInt64 = 0
    var bookEntry: RenderedBookEntry? { bookDocument?.sections.first?.entry }
    @Published private(set) var bookBrowser: BookTree?
    @Published private(set) var bookChildren: BookChildren?
    @Published var bookKey = ""
    @Published var dictionarySelection: String?
    @Published var dictionaryKey = ""
    @Published private(set) var dictionaryEntry: RenderedDictionaryEntry?
    @Published private(set) var dictionaryAnchorRevision: UInt64 = 0
    @Published private(set) var dictionaryChoices: DictionaryChoices?
    @Published private(set) var dictionaryHistory: CoreDictionaryHistoryState?
    @Published private(set) var dictionarySource: PassageTarget?
    @Published var showLexiconPreferences = false
    @Published var reference = "John 3:16"
    @Published var renderedReference = ""
    @Published var html = ""
    @Published private(set) var lexicalPresentation: RenderedLexicalPresentation = .plain
    @Published var message = "Loading installed modules…"
    @Published var showChapter = false
    @Published var verseNumbers = true
    @Published var highlightSelected = true
    @Published var renderedAnchor: String?
    @Published private(set) var commentaryRenderedAnchor: String?
    @Published private(set) var renderedAnchorRevision: UInt64 = 0
    @Published private(set) var commentaryAnchorRevision: UInt64 = 0
    @Published var choices: ReferenceChoices?
    @Published var canBack = false
    @Published var canForward = false
    @Published var isBusy = true
    @Published private(set) var referencePreview: RenderedReferenceList?
    @Published private(set) var lexicalPreview: RenderedLexicalEntry?
    @Published private(set) var informationPreview: RenderedLexicalEntry?
    @Published private(set) var informationDictionary: RenderedDictionaryEntry?
    @Published private(set) var informationNote: RenderedStudyNote?
    @Published private(set) var crossReferencesInVerseList = true
    @Published private(set) var isPreviewLoading = false
    @Published private(set) var studyNote: RenderedStudyNote?
    @Published private(set) var lexiconPreferences: LexiconPreferences?
    @Published private(set) var engineOptions: EngineOptionFlags?
    @Published private(set) var moduleOptions = ModuleOptionInfo.unknown
    private let worker: CoreWorker?
    private var tab: UInt64?
    var onPassageCommitted: ((PassageTarget,Bool) -> Void)?
    var annotationEditingTarget: PassageTarget? { guard let module=displayedModule,let entry=displayedEntry else {return nil};return PassageTarget(module:module,reference:entry.reference) }
    var syncTarget: PassageTarget? { guard !showingBook,let module = displayedModule,let entry = displayedEntry else { return nil };return PassageTarget(module:module,reference:entry.reference) }
    @discardableResult func receiveSync(_ navigation: CoreSyncNavigation) -> Bool {
        guard !closed,!isBusy,tab != nil else { return false }
        run { token in try await self.visitAndPresent(module:navigation.module,reference:navigation.reference,token:token,received:true) };return true
    }
    @discardableResult func reviewSync(_ navigation: CoreSyncNavigation) -> Bool {
        guard !closed,!isBusy,let worker,let tab,modules.contains(where: { $0.id == navigation.module }) else { return false }
        run { token in
            let references = try await worker.readReferenceList(tab:tab,module:navigation.module,expression:navigation.reference,context:self.displayedEntry?.reference ?? self.reference)
            guard token == self.revision,!self.closed else { return }
            self.bookNote = nil;self.referencePreview = references;self.message = "Received references for review."
        };return true
    }
    private var displayedEntry: RenderedEntry?
    var readingRightToLeft: Bool? { modules.first(where: {$0.id == displayedModule})?.rightToLeft }
    private var displayedModule: String?
    private var revision: UInt64 = 0
    private var closed = false
    private let preferredBible: String
    private let preferredCommentary: String
    init(worker: CoreWorker?, startupError: String?, autostart: Bool = true, preferredBible: String = "", preferredCommentary: String = "") {
        self.worker = worker
        self.preferredBible = preferredBible; self.preferredCommentary = preferredCommentary
        guard worker != nil else {
            message = startupError ?? "Unable to start the reading engine."
            isBusy = false
            return
        }
        if autostart { Task { [weak self] in await self?.start() } }
    }
    deinit {
        if let worker, let tab { Task { try? await worker.closeTab(tab) } }
    }
    func close() {
        guard !closed else { return }
        closed = true
        search.close()
        revision &+= 1
        hoverRevision &+= 1
        hoverTask?.cancel(); hoverTask = nil
        isPreviewLoading = false
        informationPreview = nil
        informationNote = nil
        informationDictionary = nil
        bookImage = nil; verseImage = nil
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
            try await worker.setLexicalLayoutEnabled(tab: opened, enabled: true)
            try await worker.setNoteNumberingEnabled(tab: opened, enabled: true)
            try await worker.setHeadingNoteLinksEnabled(tab: opened, enabled: true)
            try await worker.enableModulePreferences(tab: opened)
            try await worker.enableLexiconPreferences(tab: opened)
            let lexicons = try await worker.lexiconPreferences(tab: opened)
            let previewList = try await worker.crossReferencePreviewList(tab: opened)
            guard !closed else { return }
            lexiconPreferences = lexicons
            crossReferencesInVerseList = previewList
            modules = discovered
            selection = modules.first(where: { $0.type == "Biblical Texts" && $0.id == preferredBible })?.id ?? modules.first(where: { $0.type == "Biblical Texts" })?.id
            commentarySelection = modules.first(where: { $0.type == "Commentaries" && $0.id == preferredCommentary })?.id ?? ""
            search.configure(worker: worker, tab: opened, modules: discovered, preferred: selection)
            if modules.isEmpty { message = "No installed modules found. Open Module Library from the File menu to install modules." }
            else if let selection {
                try await visitAndPresent(module: selection, reference: reference, token: revision)
            } else { message = "Select a Bible or commentary module to read a verse." }
        } catch { if !closed { message = error.localizedDescription } }
    }
    var moduleEditingTarget: (String,String)? {
        if showingBook, let book = bookDocument { return (book.tree.module,book.tree.node.path) }
        if let reference = displayedEntry?.reference {
            if modules.contains(where: { $0.id == commentarySelection && $0.type == "Commentaries" && !$0.isRegularCommentary }) { return (commentarySelection,reference) }
            if let module = displayedModule, modules.contains(where: { $0.id == module && $0.type == "Commentaries" && !$0.isRegularCommentary }) { return (module,reference) }
        }
        return nil
    }
    func refreshModuleLibrary() {
        guard let worker, let tab, !closed else { return }
        Task {
            do { let discovered = try await worker.modules();guard !closed else { return };modules = discovered;search.configure(worker:worker,tab:tab,modules:discovered,preferred:selection);refreshAfterModuleEdit() }
            catch { if !closed { message = error.localizedDescription } }
        }
    }
    func refreshAfterModuleEdit() {
        guard let worker, let tab else { return }
        search.moduleContentChanged()
        run { token in
            let wasBook = self.showingBook
            if let module = self.displayedModule, let entry = self.displayedEntry {
                let fresh = try await worker.readTabEntry(tab:tab,module:module,reference:entry.reference)
                try await self.present(fresh,module:module,token:token,updateInput:false)
            }
            if let document = try await worker.bookDocumentSelection(tab:tab) { self.presentBook(document,updateInput:false) }
            self.showingBook = wasBook && self.bookDocument != nil
            if let raw=try await worker.restoredParallel(tab:tab) {self.parallelChapter=await self.prepareParallel(raw)}
            self.showingParallel = self.showingParallel && self.parallelChapter != nil
        }
    }
    var sessionTab: UInt64? { tab }
    func restoreWindow(_ saved: CoreSavedWindow, tab restoredTab: UInt64, host: SessionHostState?, parallelShowing: Bool = false) async throws {
        guard let worker, !closed, tab == nil else { throw SessionHostError(message: "This reading window cannot be restored.") }
        tab = restoredTab
        defer { isBusy = false }
        modules = try await worker.modules()
        lexiconPreferences = try await worker.lexiconPreferences(tab: restoredTab)
        crossReferencesInVerseList = try await worker.crossReferencePreviewList(tab: restoredTab)
        if let host {
            readingSources=host.readingSources ?? CoreReadingSources()
            showChapter = host.showChapter; verseNumbers = host.verseNumbers; highlightSelected = host.highlightSelected
        }
        selection = saved.module
        commentarySelection = saved.commentary ?? ""
        search.configure(worker: worker, tab: restoredTab, modules: modules, preferred: saved.module)
        if let module = saved.module, let reference = saved.reference {
            let entry = try await worker.readTabEntry(tab: restoredTab, module: module, reference: reference)
            try await present(entry, module: module, token: revision)
        } else { message = "Select a module to read." }
        if host == nil, let module = saved.dictionaryModule, let key = saved.dictionaryKey {
            let entry = try await worker.readDictionary(tab: restoredTab, module: module, key: key)
            let history = try await worker.dictionaryHistory(tab: restoredTab)
            try await presentDictionary(entry, source: nil, history: history, token: revision)
        }
        if let module = host?.dictionaryDisplayModule, let key = host?.dictionaryDisplayKey {
            let entry = try await worker.readDictionary(tab: restoredTab, module: module, key: key)
            let history = try await worker.dictionaryHistory(tab: restoredTab)
            let source = host?.dictionarySourceModule.flatMap { module in
                host?.dictionarySourceReference.map { PassageTarget(module: module, reference: $0) }
            }
            try await presentDictionary(entry, source: source, history: history, token: revision)
        }
        if saved.bookDocument, let document = try await worker.bookDocumentSelection(tab: restoredTab) {
            presentBook(document)
            bookBrowser = document.tree
            bookChildren = try await worker.bookChildren(module: document.tree.module, parent: document.tree.node.offset)
        }
        if let raw=try await worker.restoredParallel(tab:restoredTab) {parallelChapter=await prepareParallel(raw)}
        showingParallel = parallelShowing && parallelChapter != nil
        if let host {
            showingBook = host.showingBook && bookDocument != nil
            reference = host.referenceDraft; dictionaryKey = host.dictionaryDraft; bookKey = host.bookDraft
            host.search.apply(to: search)
        } else { showingBook = saved.module == nil && bookDocument != nil }
    }
    private func run(_ action: @escaping @MainActor (UInt64) async throws -> Void) {
        guard !closed, !isBusy, tab != nil else { return }
        hoverRevision &+= 1
        hoverTask?.cancel(); hoverTask = nil; isPreviewLoading = false
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
    private var hoverRevision: UInt64 = 0
    private var hoverTask: Task<Void, Never>?
    func previewStudyLink(_ uri: String) {
        guard !closed, !isBusy, let worker, let tab, uri.utf8.count <= 32_768 else { return }
        // Legacy main hover policy uses the live committed main context, even
        // when the information/sidebar content was produced by an older view.
        guard let sourceModule = displayedModule, let sourceReference = displayedEntry?.reference else { return }
        hoverRevision &+= 1
        let hoverToken = hoverRevision, displayToken = revision
        hoverTask?.cancel()
        isPreviewLoading = true
        hoverTask = Task { [weak self] in
            defer {
                if let self, self.hoverRevision == hoverToken { self.isPreviewLoading = false }
            }
            do {
                let result = try await worker.hoverStudyLinkAt(tab: tab, currentModule: sourceModule, currentReference: sourceReference, uri: uri)
                guard let self, !Task.isCancelled, !self.closed, self.hoverRevision == hoverToken,
                      self.revision == displayToken else { return }
                switch result {
                case .noPresentation: break
                case .dictionary(let entry):
                    self.informationDictionary = entry; self.informationPreview = nil; self.informationNote = nil
                case .lexical(let entry):
                    self.informationPreview = entry; self.informationNote = nil; self.informationDictionary = nil
                case .note(let note):
                    self.informationNote = note; self.informationPreview = nil; self.informationDictionary = nil
                case .references(let references):
                    self.bookNote = nil; self.referencePreview = references; self.studyNote = nil
                }
            } catch { /* Unsupported links/failed hovers retain the last preview. */ }
        }
    }
    func setCrossReferencesInVerseList(_ enabled: Bool) {
        guard let worker, let tab else { return }
        run { token in
            try await worker.setCrossReferencePreviewList(tab: tab, enabled: enabled)
            let copied = try await worker.crossReferencePreviewList(tab: tab)
            guard token == self.revision && !self.closed else { return }
            self.crossReferencesInVerseList = copied
            self.message = "Cross-reference preview updated."
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
    func importLexiconProfile(path: String) {
        guard let worker, let tab else { return }
        run { token in
            let imported: LexiconPreferences
            do { imported = try await worker.importLexiconProfile(tab: tab, path: path) }
            catch { throw LexiconProfileImportError() }
            guard token == self.revision && !self.closed else { return }
            self.lexiconPreferences = imported
            self.message = "Lexicon choices imported for this window."
        }
    }
    func saveLexiconProfile(path: String) {
        guard let worker, let tab, lexiconPreferences != nil else { return }
        run { token in
            do { try await worker.saveLexiconProfile(tab: tab, path: path) }
            catch { throw LexiconProfileSaveError() }
            guard token == self.revision && !self.closed else { return }
            self.message = "Lexicon choices saved."
        }
    }
    func selectionChanged() {
        guard !closed, !isBusy, selection != displayedModule else { return }
        if let chosen = selection, modules.contains(where: { $0.id == chosen && $0.hasBookTree }) {
            selection = displayedModule
            chooseBook(chosen)
            return
        }
        if let chosen = selection, modules.contains(where: { $0.id == chosen && $0.isDictionary }) {
            selection = displayedModule
            dictionarySelection = chosen
            showingBook = false
            if !dictionaryKey.isEmpty { lookupDictionary() }
            return
        }
        if let chosen = selection, modules.contains(where: { $0.id == chosen && $0.type == "Commentaries" }),
           let displayedModule, modules.contains(where: { $0.id == displayedModule && $0.type == "Biblical Texts" }) {
            selection = displayedModule
            chooseCommentary(chosen)
            return
        }
        read()
    }
    func chooseCommentary(_ module: String) {
        guard let worker, let tab, let main = displayedModule, displayedEntry != nil,
              modules.contains(where: { $0.id == main && $0.type == "Biblical Texts" }) else { return }
        run { token in
            let pair = try await worker.selectCommentary(tab:tab,module:module)
            try await self.present(pair.text, module: main, token: token, updateInput: false, passage: pair)
        }
    }
    func setCommentaryChapterMode(_ enabled: Bool) {
        guard let worker, let tab, let main = displayedModule, let entry = displayedEntry, !commentarySelection.isEmpty else { return }
        let module = commentarySelection
        run { token in
            try await worker.setCommentaryChapterMode(tab: tab, module: module, enabled: enabled)
            try await self.present(entry, module: main, token: token, updateInput: false)
        }
    }
    func setNoteNumbers(_ enabled: Bool, commentary: Bool = false) {
        guard let worker, let tab, let main = displayedModule, let entry = displayedEntry else { return }
        let module = commentary ? commentarySelection : main
        let options = commentary ? commentaryContentOptions : contentOptions
        guard !module.isEmpty, options.noteNumbers != nil else { return }
        run { token in
            try await worker.setNoteNumbers(tab: tab, module: module, enabled: enabled)
            let refreshed = try await worker.readTabEntry(tab: tab, module: main, reference: entry.reference)
            try await self.present(refreshed, module: main, token: token, updateInput: false)
        }
    }
    func setDoubleSpace(_ enabled: Bool, commentary: Bool = false) {
        guard let worker, let tab, let main = displayedModule, let entry = displayedEntry else { return }
        let module = commentary ? commentarySelection : main
        let options = commentary ? commentaryContentOptions : contentOptions
        guard !module.isEmpty, options.doubleSpace != nil else { return }
        run { token in
            try await worker.setDoubleSpace(tab: tab, module: module, enabled: enabled)
            let refreshed = try await worker.readTabEntry(tab: tab, module: main, reference: entry.reference)
            try await self.present(refreshed, module: main, token: token, updateInput: false)
        }
    }
    func setImageContent(_ enabled: Bool, commentary: Bool = false) {
        setContentOption(enabled, commentary: commentary, fontFaces: false)
    }
    func setRespectFontFaces(_ enabled: Bool, commentary: Bool = false) {
        setContentOption(enabled, commentary: commentary, fontFaces: true)
    }
    private func setContentOption(_ enabled: Bool, commentary: Bool, fontFaces: Bool) {
        guard let worker, let tab, let main = displayedModule, let entry = displayedEntry else { return }
        let module = commentary ? commentarySelection : main
        let options = commentary ? commentaryContentOptions : contentOptions
        guard !module.isEmpty, (fontFaces ? options.fontFaces : options.images) != nil else { return }
        run { token in
            if fontFaces { try await worker.setRespectFontFaces(tab: tab, module: module, enabled: enabled) }
            else { try await worker.setImageContent(tab: tab, module: module, enabled: enabled) }
            let refreshed = try await worker.readTabEntry(tab: tab, module: main, reference: entry.reference)
            try await self.present(refreshed, module: main, token: token, updateInput: false)
        }
    }
    func setCommentaryHeadings(_ enabled: Bool) {
        guard let worker, let tab, let main = displayedModule, let entry = displayedEntry, !commentarySelection.isEmpty else { return }
        let module = commentarySelection
        run { token in
            try await worker.setHeadings(tab: tab, module: module, enabled: enabled)
            try await self.present(entry, module: main, token: token, updateInput: false)
        }
    }
    private func visitAndPresent(module: String, reference: String, token: UInt64, received: Bool = false) async throws {
        guard let worker, let tab else { return }
        if modules.contains(where: { $0.id == module && $0.type == "Biblical Texts" }) {
            let pair = try await worker.studyPassage(tab: tab, module: module, commentary: commentarySelection, reference: reference)
            try await present(pair.text, module: module, token: token, passage: pair, received:received)
        } else {
            let entry = try await worker.visit(tab: tab, module: module, reference: reference)
            try await present(entry, module: module, token: token, received:received)
        }
    }
    private func presentDictionary(_ entry: RenderedDictionaryEntry, source: PassageTarget?,
                                   lexical: RenderedLexicalEntry? = nil, history: CoreDictionaryHistoryState? = nil, token: UInt64) async throws {
        guard let worker, let tab else { return }
        let choices = try await worker.dictionaryChoices(tab: tab, module: entry.module, key: entry.key)
        guard token == revision && !closed else { return }
        showingBook = false
        dictionaryEntry = entry; dictionaryKey = entry.key; dictionarySelection = entry.module
        if entry.anchor != nil { dictionaryAnchorRevision &+= 1 }
        dictionarySource = source; dictionaryChoices = choices; lexicalPreview = lexical
        if let history { dictionaryHistory = history }
        message = ""
    }
    func chooseBook(_ module: String) {
        guard !closed, !isBusy, let worker, let tab,
              modules.contains(where: { $0.id == module && $0.hasBookTree }) else { return }
        if bookEntry?.tree.module == module { showingBook = true; return }
        run { token in
            let browser = try await worker.bookTree(module: module, offset: 0)
            let children = try await worker.bookChildren(module: module, parent: 0)
            let entry = try await worker.selectBookDocument(tab: tab, module: module, offset: 0)
            guard token == self.revision && !self.closed else { return }
            self.bookBrowser = browser; self.bookChildren = children
            self.presentBook(entry)
        }
    }
    private func presentBook(_ entry: RenderedBookDocument, updateInput: Bool = true) {
        bookNote = nil
        bookDocument = entry
        bookAnchor = nil
        if updateInput { bookKey = entry.tree.node.path }
        showingBook = true; message = ""
    }
    private enum BookDisplayOption { case images, fontFaces, doubleSpace, noteNumbers }
    func setBookNoteNumbers(_ enabled: Bool) { refreshBookDisplay(option: .noteNumbers, enabled: enabled) }
    func setBookDoubleSpace(_ enabled: Bool) { refreshBookDisplay(option: .doubleSpace, enabled: enabled) }
    func setBookImageContent(_ enabled: Bool) { refreshBookDisplay(option: .images, enabled: enabled) }
    func setBookRespectFontFaces(_ enabled: Bool) { refreshBookDisplay(option: .fontFaces, enabled: enabled) }
    func refreshBookDisplay() { refreshBookDisplay(option: nil, enabled: false) }
    private func refreshBookDisplay(option: BookDisplayOption?, enabled: Bool) {
        guard !closed, !isBusy, let worker, let tab, let document = bookDocument else { return }
        if let option {
            let options = document.presentation?.options
            let available: Bool?
            switch option {
            case .images: available = options?.images
            case .fontFaces: available = options?.fontFaces
            case .doubleSpace: available = options?.doubleSpace
            case .noteNumbers: available = document.presentation?.noteNumbers
            }
            guard available != nil else { return }
        }
        run { token in
            switch option {
            case .images: try await worker.setImageContent(tab: tab, module: document.tree.module, enabled: enabled)
            case .fontFaces: try await worker.setRespectFontFaces(tab: tab, module: document.tree.module, enabled: enabled)
            case .doubleSpace: try await worker.setDoubleSpace(tab: tab, module: document.tree.module, enabled: enabled)
            case .noteNumbers: try await worker.setNoteNumbers(tab: tab, module: document.tree.module, enabled: enabled)
            case nil: break
            }
            let refreshed = try await worker.refreshBookDocument(tab: tab)
            guard token == self.revision && !self.closed, let refreshed else { return }
            self.presentBook(refreshed, updateInput: false)
        }
    }
    func showBookNoteLink(_ uri: String) { followBookLink(uri) }
    func closeVerseImage() { guard !closed, !isBusy else { return }; verseImage = nil }
    func closeBookImage() { guard !closed, !isBusy else { return }; bookImage = nil }
    func followBookLink(_ uri: String) {
        guard !closed, !isBusy, let worker, let tab, bookDocument != nil else { return }
        run { token in
            let link: RenderedBookPaneLink
            do { link = try await worker.followBookPaneLink(tab: tab, mainModule: self.displayedModule ?? "",
                mainReference: self.displayedEntry?.reference ?? "", uri: uri) }
            catch { throw BookLinkError() }
            try await self.presentBookPaneLink(link,token:token)
        }
    }
    private func presentBookPaneLink(_ link: RenderedBookPaneLink, token: UInt64) async throws {
            guard token == self.revision && !self.closed else { return }
            if case .study(let target) = link {
                try await self.presentStudyLink(target, mainModule: self.displayedModule, clearNote: false, token: token)
                return
            }
            guard case .book(let bookLink) = link else { return }
            switch bookLink {
            case .note(let note):
                self.studyNote = nil; self.referencePreview = nil
                self.bookNote = note; self.message = ""
            case .image(let image):
                let preview: BookImagePreview
                do { preview = try await BookImagePreview.decode(image) }
                catch { throw BookLinkError() }
                guard token == self.revision && !self.closed else { return }
                self.bookImage = preview; self.message = ""
            }
    }
    func closeBook() { guard !closed, !isBusy else { return }; showingBook = false }
    func readBookKey() {
        guard !closed, !isBusy, let worker, let tab, let source = bookEntry?.tree else { return }
        let key = bookKey
        run { token in
            let entry: RenderedBookDocument?
            do {
                entry = try await worker.restoreBookDocumentSelection(tab: tab,
                    saved: .init(module: source.module, key: key, offsetText: nil), mode: .displayKey)
            } catch { throw BookSelectionError() }
            guard token == self.revision && !self.closed, let entry else { return }
            self.presentBook(entry)
        }
    }
    func navigateBook(_ direction: BookNavigation) {
        guard !closed, !isBusy, let worker, let tab, bookEntry != nil else { return }
        run { token in
            let entry = try await worker.navigateBookDocument(tab: tab, direction: direction)
            guard token == self.revision && !self.closed, let entry else { return }
            self.presentBook(entry)
        }
    }
    func chooseBookNode(_ node: BookNode) {
        guard !closed, !isBusy, let worker, let tab, let page = bookChildren,
              page.nodes.contains(node) else { return }
        run { token in
            let entry = try await worker.selectBookDocument(tab: tab, module: page.module, offset: node.offset)
            guard token == self.revision && !self.closed else { return }
            self.presentBook(entry)
        }
    }
    func browseBook(_ offset: UInt64) {
        guard !closed, !isBusy, let worker, let module = bookEntry?.tree.module else { return }
        run { token in
            let browser = try await worker.bookTree(module: module, offset: offset)
            let children = try await worker.bookChildren(module: module, parent: offset)
            guard token == self.revision && !self.closed else { return }
            self.bookBrowser = browser; self.bookChildren = children; self.message = ""
        }
    }
    func moreBookChildren() {
        guard !closed, !isBusy, let worker, let page = bookChildren, let after = page.nextAfter else { return }
        run { token in
            let next = try await worker.bookChildren(module: page.module, parent: page.parent.offset, after: after)
            guard token == self.revision && !self.closed else { return }
            self.bookChildren = BookChildren(module: next.module, parent: next.parent,
                nodes: page.nodes + next.nodes, nextAfter: next.nextAfter)
        }
    }
    func chooseDictionaryKey(_ key: String) {
        guard !closed, !isBusy, let choices = dictionaryChoices,
              dictionarySelection == choices.module, choices.keys.contains(key) else { return }
        dictionaryKey = key; lookupDictionary()
    }
    func navigateDictionaryHistory(_ direction: Int32) {
        guard !closed, !isBusy, let worker, let tab,
              (direction == -1 && dictionaryHistory?.canBack == true) || (direction == 1 && dictionaryHistory?.canForward == true) else { return }
        let source = displayedModule.flatMap { id in displayedEntry.map { PassageTarget(module: id, reference: $0.reference) } }
        run { token in
            let entry = try await worker.navigateDictionaryHistory(tab: tab, direction: direction)
            let history = try await worker.dictionaryHistory(tab: tab)
            try await self.presentDictionary(entry, source: source, history: history, token: token)
        }
    }
    func clearDictionaryHistory() {
        guard let worker, let tab else { return }
        run { token in
            try await worker.clearDictionaryHistory(tab: tab)
            let history = try await worker.dictionaryHistory(tab: tab)
            guard token == self.revision && !self.closed else { return }
            self.dictionaryHistory = history
        }
    }
    func navigateDictionary(_ direction: Int32) {
        guard !closed, !isBusy, direction == -1 || direction == 1, let worker, let tab,
              let current = dictionaryEntry, dictionarySelection == current.module else { return }
        run { token in
            let entry = try await worker.navigateDictionary(tab: tab, module: current.module, key: current.key, direction: direction)
            try await self.presentDictionary(entry, source: self.dictionarySource, token: token)
        }
    }
    func lookupDictionary() {
        guard !closed, !isBusy, let worker, let tab, let module = dictionarySelection,
              modules.contains(where: { $0.id == module && $0.isDictionary }) else { return }
        let key = dictionaryKey
        guard !key.isEmpty else { message = "Enter a dictionary word or key."; return }
        let source = displayedModule.flatMap { id in displayedEntry.map { PassageTarget(module: id, reference: $0.reference) } }
        run { token in
            let entry = try await worker.readDictionary(tab: tab, module: module, key: key)
            try await self.presentDictionary(entry, source: source, token: token)
        }
    }
    func read() {
        guard worker != nil, tab != nil, let selection else { return }
        let submitted = reference
        run { token in
            try await self.visitAndPresent(module: selection, reference: submitted, token: token)
        }
    }
    private func present(_ entry: RenderedEntry, module: String, token: UInt64, updateInput: Bool = true, passage: RenderedStudyPassage? = nil, received: Bool = false) async throws {
        guard let worker, let tab else { return }
        let sources=readingSources
        let pair: RenderedStudyPassage?
        if let passage { pair = passage }
        else if modules.contains(where: { $0.id == module && $0.type == "Biblical Texts" }) {
            pair = try await worker.studyPassage(tab: tab, module: module, commentary: commentarySelection,
                                                reference: entry.reference, record: false)
        } else { pair = nil }
        let chapterMode = showChapter
        let numbers = verseNumbers, highlight = highlightSelected
        let chapter = chapterMode ? try await worker.readTabChapter(tab: tab, module: module, reference: entry.reference,
                                                                  verseNumbers: numbers, highlightSelected: highlight) : nil
        let commentaryMode = if let pair, pair.commentary != nil {
            try await worker.commentaryChapterMode(tab: tab, module: pair.commentaryModule)
        } else { false }
        let headings = if let pair, pair.commentary != nil {
            try await worker.headingDisplay(tab: tab, module: pair.commentaryModule)
        } else { true }
        let commentaryChapter: RenderedChapter?
        if commentaryMode, let pair, let commentary = pair.commentary {
            commentaryChapter = try await worker.readTabCommentaryChapter(tab: tab, module: pair.commentaryModule,
                reference: commentary.reference, verseNumbers: numbers, highlightSelected: highlight)
        } else { commentaryChapter = nil }
        let contentOptions = try await worker.contentDisplayOptions(tab: tab, module: module)
        let commentaryContentOptions = if let pair, pair.commentary != nil {
            try await worker.contentDisplayOptions(tab: tab, module: pair.commentaryModule)
        } else { ContentDisplayOptions.unknown }
        var decorated: CoreDecoratedReading?,sourceNotice=""
        if sources.enabled {
            do {decorated=try await worker.decoratedReading(tab:tab,sources:sources,module:module,reference:entry.reference,chapter:chapterMode,numbers:numbers,highlight:highlight)
                if decorated?.incomplete == true {sourceNotice="Some bookmark or note records could not be displayed.";if (decorated?.invalidColors ?? 0)>0 {sourceNotice += " Invalid colors were omitted."}}
            } catch {sourceNotice="Reading notes or bookmarks could not be loaded. The passage is still available. Reload the selected files or clear reading notes."}
        }
        var sideDecoration: CoreDecoratedReading?
        if sources.enabled,let pair,let side=pair.commentary {
            do {sideDecoration=try await worker.decoratedReading(tab:tab,sources:sources,module:pair.commentaryModule,reference:side.reference,chapter:commentaryMode,numbers:numbers,highlight:highlight,commentary:true)
                if sideDecoration?.incomplete == true {sourceNotice += " Some commentary bookmark or note records could not be displayed."}
            } catch {sourceNotice += " Commentary notes or bookmarks could not be loaded. Reading remains available."}
        }
        var refreshedParallel: CoreParallelChapter?
        if parallelChapter != nil,let raw=try await worker.restoredParallel(tab:tab) {refreshedParallel=await prepareParallel(raw)}
        let selectors = try await worker.referenceChoices(module: module, reference: entry.reference)
        let history = try await worker.history(tab)
        let options = try await worker.moduleEngineOptions(tab: tab, module: module)
        guard token == revision && !closed else { return }
        referencePreview = nil
        studyNote = nil
        bookNote = nil
        commentarySelection = pair?.commentaryModule ?? ""
        commentaryEntry = pair?.commentary
        self.commentaryChapter = commentaryChapter
        commentaryRenderedAnchor = if let anchor = pair?.anchor, anchor.module == pair?.commentaryModule { anchor.text }
            else { commentaryChapter?.selectedAnchor }
        if let anchor = pair?.anchor, anchor.module == pair?.commentaryModule { commentaryAnchorRevision &+= 1 }
        showCommentaryChapter = commentaryMode
        commentaryHeadings = headings
        self.contentOptions = contentOptions
        self.commentaryContentOptions = commentaryContentOptions
        engineOptions = options
        moduleOptions = modules.first(where: { $0.id == module })?.options ?? .unknown
        if let chapter {
            html = chapter.html; renderedAnchor = chapter.selectedAnchor; renderedReference = chapter.title
            lexicalPresentation = chapter.lexical
            message = chapter.verses.allSatisfy { $0.html.isEmpty } ? "This module has no text in this chapter." : ""
        } else {
            html = entry.html; renderedAnchor = nil; renderedReference = entry.reference
            lexicalPresentation = entry.lexical
            message = entry.html.isEmpty ? "This module has no text at this reference." : ""
        }
        if sources==readingSources {
            readingSourceNotice=sourceNotice
            commentaryDecoration=sideDecoration
            if let refreshedParallel {parallelChapter=refreshedParallel}
            if let decorated {html=decorated.html;lexicalPresentation=RenderedLexicalPresentation(css:decorated.css,requested:lexicalPresentation.requested,applied:lexicalPresentation.applied)}
        }
        if let anchor = pair?.anchor, anchor.module == module {
            renderedAnchor = anchor.text; renderedAnchorRevision &+= 1
        }
        choices = selectors
        if updateInput { reference = entry.reference; selection = module }
        let changed = displayedModule != module || displayedEntry?.reference != entry.reference
        displayedEntry = entry; displayedModule = module
        canBack = history.canBack; canForward = history.canForward
        if changed { onPassageCommitted?(PassageTarget(module:module,reference:entry.reference),received) }
    }
    func navigate(_ direction: Int32) {
        guard let worker, let tab else { return }
        run { token in
            let pair = try await worker.navigateStudyPassage(tab: tab, direction: direction)
            try await self.present(pair.text, module: pair.module, token: token, passage: pair)
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
            if self.modules.contains(where: { $0.id == module && $0.type == "Biblical Texts" }) {
                let pair = try await worker.navigateStudyChapter(tab: tab, module: module, commentary: self.commentarySelection,
                                                               reference: entry.reference, direction: direction)
                try await self.present(pair.text, module: module, token: token, passage: pair)
            } else {
                let next = try await worker.navigateChapter(tab: tab, module: module, reference: entry.reference, direction: direction)
                try await self.present(next, module: module, token: token)
            }
        }
    }
    var canOpenSearchResult: Bool { worker != nil && tab != nil }
    func openSearchResult(_ hit: SearchHit) {
        guard let worker, let tab else { return }
        if hit.keyKind == .dictionary {
            let source = displayedModule.flatMap { id in displayedEntry.map { PassageTarget(module: id, reference: $0.reference) } }
            run { token in
                let entry = try await worker.visitDictionaryExact(tab: tab,module: hit.module,key: hit.reference)
                let history = try await worker.dictionaryHistory(tab: tab)
                try await self.presentDictionary(entry,source: source,history: history,token: token)
            }
            return
        }
        if hit.keyKind == .generalBook {
            run { token in
                let entry = try await worker.selectBookDocumentPath(tab: tab,module: hit.module,path: hit.reference)
                let children = try await worker.bookChildren(module: hit.module,parent: entry.tree.node.offset)
                guard token == self.revision && !self.closed else { return }
                self.bookBrowser = entry.tree; self.bookChildren = children; self.presentBook(entry)
            }
            return
        }
        guard let main = displayedModule else { return }
        let target = PassageTarget(module: hit.module, reference: hit.reference)
        let commentary = commentarySelection
        let paired = modules.contains(where: { $0.id == main && $0.type == "Biblical Texts" })
        run { token in
            if paired {
                let pair = try await worker.visitLinkedStudyPassage(tab: tab, bible: main,
                    commentary: commentary, target: target)
                try await self.present(pair.text, module: pair.module, token: token, passage: pair)
            } else {
                try await self.visitAndPresent(module: target.module, reference: target.reference, token: token)
            }
        }
    }
    func followSearchLink(_ uri: String, hit: SearchHit) {
        if hit.keyKind == .verse { followLink(uri,currentModule:hit.module,currentReference:hit.reference); return }
        guard !closed, !isBusy, let worker, let tab else { return }
        run { token in
            if hit.keyKind == .dictionary {
                // Revalidate captured source identity before interpreting its links.
                _ = try await worker.previewKeyedSearch(tab:tab,module:hit.module,key:hit.reference,kind:hit.keyKind)
                let link = try await worker.followStudyLinkAt(tab:tab,currentModule:hit.module,currentReference:self.displayedEntry?.reference ?? "",uri:uri)
                try await self.presentStudyLink(link,mainModule:self.displayedModule,clearNote:false,token:token)
            } else {
                let entry = try await worker.selectBookDocumentPath(tab:tab,module:hit.module,path:hit.reference)
                let children = try await worker.bookChildren(module:hit.module,parent:entry.tree.node.offset)
                guard token == self.revision && !self.closed else { return }
                self.bookBrowser = entry.tree; self.bookChildren = children; self.presentBook(entry)
                let link = try await worker.followBookPaneLink(tab:tab,mainModule:self.displayedModule ?? "",
                    mainReference:self.displayedEntry?.reference ?? "",uri:uri)
                try await self.presentBookPaneLink(link,token:token)
            }
        }
    }
    func chooseReference(_ target: String) {
        guard worker != nil, tab != nil, let module = displayedModule else { return }
        run { token in
            try await self.visitAndPresent(module: module, reference: target, token: token)
        }
    }
    func followCommentaryLink(_ uri: String) {
        guard let displayedModule, let entry = commentaryEntry else { return }
        followLink(uri, currentModule: commentarySelection, currentReference: entry.reference, navigationModule: displayedModule)
    }
    func followLink(_ uri: String, currentModule: String? = nil, currentReference: String? = nil, navigationModule: String? = nil) {
        guard let worker, let tab, let module = currentModule ?? displayedModule, let context = currentReference ?? displayedEntry?.reference else { return }
        let mainModule = displayedModule
        run { token in
            let link = try await worker.followStudyLinkAt(tab: tab, currentModule: module, currentReference: context, uri: uri, navigationModule: navigationModule)
            try await self.presentStudyLink(link, mainModule: mainModule, clearNote: currentReference == nil, token: token)
        }
    }
    private func presentStudyLink(_ link: ContextualStudyLink, mainModule: String?, clearNote: Bool, token: UInt64) async throws {
        guard let worker, let tab else { return }
        switch link {
            case .book(let target):
                let children = try await worker.bookChildren(module: target.tree.module, parent: target.tree.node.offset)
                guard let entry = try await worker.restoreBookDocumentSelection(tab: tab,
                    saved: .init(module: target.tree.module, key: target.key, offsetText: nil), mode: .displayKey) else { throw BookSelectionError() }
                guard token == self.revision && !self.closed else { return }
                self.bookBrowser = entry.tree; self.bookChildren = children
                self.presentBook(entry); self.bookAnchor = target.anchor
                if target.anchor != nil { self.bookAnchorRevision &+= 1 }
            case .image(let source):
                let preview: VerseImagePreview
                do { preview = try await VerseImagePreview.decode(source) }
                catch { throw ReadingImageError() }
                guard token == self.revision && !self.closed else { return }
                self.verseImage = preview; self.message = ""
            case .note(let note):
                guard token == self.revision && !self.closed else { return }
                self.referencePreview = nil
                self.bookNote = nil; self.studyNote = note; self.message = ""
            case .references(let references):
                guard token == self.revision && !self.closed else { return }
                if clearNote { self.studyNote = nil }
                self.bookNote = nil; self.referencePreview = references; self.message = ""
            case .dictionary(let entry):
                let history = try await worker.dictionaryHistory(tab: tab)
                try await self.presentDictionary(entry, source: nil, history: history, token: token)
            case .lexical(let lexical):
                let history = try await worker.dictionaryHistory(tab: tab)
                try await self.presentDictionary(lexical.entry,
                    source: PassageTarget(module: lexical.module, reference: lexical.sourceReference),
                    lexical: lexical, history: history, token: token)
            case .passage(let target):
                if let displayedModule = mainModule, self.modules.contains(where: { $0.id == displayedModule && $0.type == "Biblical Texts" }) {
                    let pair = try await worker.visitLinkedStudyPassage(tab: tab, bible: displayedModule,
                        commentary: self.commentarySelection, target: target, commentaryVisible: !self.showingBook)
                    try await self.present(pair.text, module: pair.module, token: token, passage: pair)
                    if token == self.revision && !self.closed, target.module == pair.commentaryModule { self.showingBook = false }
                } else {
                    try await self.visitAndPresent(module: target.module, reference: target.reference, token: token)
                    if token == self.revision && !self.closed, let anchor = target.anchor {
                        self.renderedAnchor = anchor; self.renderedAnchorRevision &+= 1
                    }
                }
        }
    }
    func showReferencedPassages() {
        guard let note = studyNote, !note.referenceList.isEmpty, let worker, let tab else { return }
        run { token in
            let references = try await worker.readReferenceList(tab: tab, module: note.module,
                expression: note.referenceList, context: note.reference)
            guard token == self.revision && !self.closed else { return }
            self.bookNote = nil; self.referencePreview = references; self.message = ""
        }
    }
    func showStudyNoteBody() { referencePreview = nil }
    func closeLexicalPreview() { guard !closed, !isBusy else { return }; lexicalPreview = nil; dictionaryEntry = nil; dictionarySource = nil; dictionarySelection = nil; dictionaryChoices = nil }
    func closeStudyNote() { referencePreview = nil; studyNote = nil; bookNote = nil }
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
    var rightToLeft: Bool? = nil
    var css: String = ""
    let anchor: String?
    var anchorRevision: UInt64 = 0
    var onScrollFailure: (String) -> Void
    var onLink: (String) -> Void
    var onLinkHover: ((String) -> Void)? = nil
    var allowsInlineImages = false
    var onContextLink: ((String,String,String) -> Void)? = nil
    @MainActor final class Coordinator: NSObject, WKNavigationDelegate, WKScriptMessageHandler {
        var loaded: String?
        var loadedCSS: String?
        var loadedInlineImages = false
        var loadedRightToLeft: Bool?
        var anchor: String?
        var anchorRevision: UInt64 = 0
        var navigation: WKNavigation?
        var onScrollFailure: ((String) -> Void)?
        var onLink: ((String) -> Void)?
        var onLinkHover: ((String) -> Void)?
        var onContextLink: ((String,String,String) -> Void)?
        func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
            if message.name == "xiphosContextLink",message.frameInfo.isMainFrame,
               let fields=message.body as? [String:String],let uri=fields["uri"],let module=fields["module"],let reference=fields["reference"],
               uri.utf8.count<=32768,module.utf8.count<=4096,reference.utf8.count<=32768 {
                onContextLink?(uri,module,reference);return
            }
            guard message.name == "xiphosHover", message.frameInfo.isMainFrame,
                  let uri = message.body as? String, uri.utf8.count <= 32_768 else { return }
            onLinkHover?(uri)
        }
        func scrollToAnchor(_ webView: WKWebView) {
            guard let anchor,
                  let data = try? JSONSerialization.data(withJSONObject: anchor, options: [.fragmentsAllowed]),
                  let literal = String(data: data, encoding: .utf8) else { return }
            // Trusted host code runs in an isolated world. Module scripts stay
            // disabled and are never interpolated into this expression.
            let script = anchor.isEmpty ? "window.scrollTo(0, 0); true;" : "(() => { const node = document.getElementById(\(literal)) || document.getElementsByName(\(literal))[0]; if (!node) return false; node.scrollIntoView({block: 'start'}); return true; })()"
            let revision = anchorRevision, navigation = self.navigation
            webView.evaluateJavaScript(script,
                in: nil, in: .defaultClient) { [weak self] result in
                    guard let self, self.anchor == anchor, self.anchorRevision == revision,
                          self.navigation === navigation else { return }
                    switch result {
                    case .success(let value) where value as? Bool == true: break
                    default: self.onScrollFailure?("Unable to scroll to the selected passage.")
                    }
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
        // Trusted transport runs in an isolated world; module scripts remain
        // disabled. The core interprets the URI, never this event script.
        config.userContentController.add(context.coordinator, contentWorld: .defaultClient, name: "xiphosHover")
        config.userContentController.add(context.coordinator, contentWorld: .defaultClient, name: "xiphosContextLink")
        config.userContentController.addUserScript(WKUserScript(source: """
            (() => {
                let previous = null;
                document.addEventListener('click', event => {
                    const link = event.target instanceof Element ? event.target.closest('a[href]') : null;
                    const cell = link ? link.closest('td[data-xc-module][data-xc-reference]') : null;
                    if (!cell || !cell.dataset.xcModule) return;
                    if (link.href.length > 32768 || cell.dataset.xcModule.length > 4096 || cell.dataset.xcReference.length > 32768) { event.preventDefault(); return; }
                    event.preventDefault();
                    window.webkit.messageHandlers.xiphosContextLink.postMessage({uri: link.href, module: cell.dataset.xcModule, reference: cell.dataset.xcReference});
                }, true);
                document.addEventListener('mouseover', event => {
                    const link = event.target instanceof Element ? event.target.closest('a[href]') : null;
                    const uri = link ? link.href : null;
                    if (uri === previous) return;
                    previous = uri;
                    if (uri && uri.length <= 32768) window.webkit.messageHandlers.xiphosHover.postMessage(uri);
                }, true);
                document.addEventListener('mouseout', event => {
                    const next = event.relatedTarget instanceof Element ? event.relatedTarget.closest('a[href]') : null;
                    if (!next) previous = null;
                }, true);
            })();
            """, injectionTime: .atDocumentEnd, forMainFrameOnly: true, in: .defaultClient))
        let view = WKWebView(frame: .zero, configuration: config)
        view.navigationDelegate = context.coordinator
        return view
    }
    func updateNSView(_ view: WKWebView, context: Context) {
        context.coordinator.onScrollFailure = onScrollFailure
        context.coordinator.onLink = onLink
        context.coordinator.onLinkHover = onLinkHover
        context.coordinator.onContextLink = onContextLink
        let changedAnchor = context.coordinator.anchor != anchor || context.coordinator.anchorRevision != anchorRevision
        context.coordinator.anchor = anchor
        context.coordinator.anchorRevision = anchorRevision
        guard context.coordinator.loaded != html || context.coordinator.loadedCSS != css || context.coordinator.loadedInlineImages != allowsInlineImages || context.coordinator.loadedRightToLeft != rightToLeft else {
            if changedAnchor { context.coordinator.scrollToAnchor(view) }
            return
        }
        context.coordinator.loaded = html
        context.coordinator.loadedCSS = css
        context.coordinator.loadedInlineImages = allowsInlineImages
        context.coordinator.loadedRightToLeft = rightToLeft
        context.coordinator.navigation = view.loadHTMLString(Self.documentHTML(html: html, css: css, allowsInlineImages: allowsInlineImages, rightToLeft: rightToLeft), baseURL: nil)
    }
    static func documentHTML(html: String, css: String, allowsInlineImages: Bool = false, rightToLeft: Bool? = nil) -> String {
        """
            <!doctype html><html><head><meta charset="utf-8">
            <base href="xiphos-preview://study/">
            <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; img-src \(allowsInlineImages ? "data:" : "'none'")">
            <style>:root { color-scheme: light dark; } body { font: 18px Georgia, serif; line-height: 1.55; margin: 24px; }
            /* Existing Xiphos editor/template.h styles this SWORD class red. */
            span.wordsOfJesus { color: red; }
            :root { --xc-highlight-fg:#ffffff; --xc-highlight-bg:#174ea6; --xc-current-verse:LinkText; }
            .xc-reading-annotation {color:var(--xc-highlight-bg);background-color:var(--xc-highlight-fg);}
            .xc-reading-current-highlight {color:var(--xc-highlight-fg);background-color:var(--xc-highlight-bg);}
            .xc-reading-current-text {color:var(--xc-current-verse);}
            .xc-reading-tag {padding-inline-start:4px;}
            .xc-reading-tag a {color:inherit;}
            .xc-personal-note {margin:6px 0;padding:4px 8px;border-inline-start:3px solid GrayText;}
            .xc-personal-note summary {font:small-caption;cursor:pointer;}

            .xc-reference { margin-bottom: 20px; } .xc-reference header { font-weight: bold; }
            .xc-verse, .xc-commentary-section { scroll-margin-top: 12px; }
            .xc-verse.xc-current, .xc-commentary-section.xc-current { background: Highlight; color: HighlightText; }
            .xc-current a { color: inherit; }
            .xc-verse sup { font-size: 0.7em; padding-inline-end: 4px; }
            \(css)
            </style>
            </head><body dir="\(rightToLeft.map { $0 ? "rtl" : "ltr" } ?? "auto")">\(html)</body></html>
            """
    }
    static func dismantleNSView(_ view: WKWebView, coordinator: Coordinator) {
        view.configuration.userContentController.removeScriptMessageHandler(forName: "xiphosHover", contentWorld: .defaultClient)
        view.navigationDelegate = nil
        view.configuration.userContentController.removeScriptMessageHandler(forName: "xiphosContextLink", contentWorld: .defaultClient)
        coordinator.onLink = nil; coordinator.onLinkHover = nil; coordinator.onContextLink = nil
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

struct LexiconPreferencesView: View {
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
    private var closeOnDisappear = true
    init(model: ReaderModel) {
        _model = StateObject(wrappedValue: model); closeOnDisappear = false
    }
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
    private func chooseLexiconImport() {
        let panel = NSOpenPanel()
        panel.title = "Import Xiphos lexicon choices"
        panel.message = "Choose a Xiphos settings file to import lexicon dictionaries for this window."
        panel.prompt = "Import"
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            model.importLexiconProfile(path: url.path)
        }
    }
    private func chooseLexiconSave() {
        let panel = NSSavePanel()
        panel.title = "Save Xiphos lexicon choices"
        panel.message = "Save this window’s lexicon choices. Other settings in an existing file are kept."
        panel.prompt = "Save"
        panel.nameFieldStringValue = "settings.xml"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            model.saveLexiconProfile(path: url.path)
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
                TabView {
                List(selection: $model.selection) {
                    ForEach(model.sidebarGroups) { group in
                        Section(group.id) {
                            ForEach(group.modules) { module in
                                VStack(alignment: .leading, spacing: 3) {
                                    Text((model.isFavorite(module.id) ? "★ " : "") + module.id).accessibilityLabel(module.id + (model.isFavorite(module.id) ? ", favorite" : ""))
                                    Text(module.description).font(.caption).foregroundStyle(.secondary).lineLimit(2)
                                }.tag(module.id)
                            }
                        }
                    }
                }.frame(minHeight: 120)
                    .disabled(model.isBusy)
                    .accessibilityLabel("Installed SWORD modules")
                    .safeAreaInset(edge:.bottom) { if !model.libraryOrganizationNotice.isEmpty { Text(model.libraryOrganizationNotice).font(.caption).padding(6) } }
                    .tabItem { Text("Modules") }
                    SearchPane(model: model.search, hostBusy: model.isBusy, canRead: model.canOpenSearchResult,
                        onRead: model.openSearchResult,
                        onLink: { uri, hit in model.followSearchLink(uri, hit: hit) })
                        .tabItem { Text("Search") }
                }
                if model.studyNote != nil || model.referencePreview != nil || model.bookNote != nil {
                    VStack(alignment: .leading, spacing: 0) {
                        HStack {
                            Text(model.bookNote != nil ? "Book note" : model.referencePreview != nil ? "Referenced passages" : (model.studyNote?.type == "crossReference" ? "Cross-reference" : "Study note")).font(.headline)
                            Spacer()
                            Button("Close") { model.closeStudyNote() }.disabled(model.isBusy)
                        }.padding(8)
                        if let note = model.bookNote {
                            Text("\(note.tree.module) • \(note.tree.node.path)").font(.caption).padding(.horizontal, 8)
                            PassageView(html: note.content.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                onLink: { model.followBookLink($0) })
                        } else if let references = model.referencePreview {
                            Text("\(references.module) • \(references.context)").font(.caption).padding(.horizontal, 8)
                            HStack {
                                Text("\(references.entries.count) \(references.entries.count == 1 ? "passage" : "passages")").font(.caption)
                                Spacer()
                                if model.studyNote != nil {
                                    Button("Back to note") { model.showStudyNoteBody() }.disabled(model.isBusy)
                                }
                            }.padding(8)
                            if references.limited {
                                Text("This preview reached Xiphos's passage limit.").font(.caption).padding(.horizontal, 8)
                            }
                            if references.skipped > 0 {
                                Text("\(references.skipped) references could not be resolved.").font(.caption).padding(.horizontal, 8)
                            }
                            if references.entries.isEmpty {
                                Text("No referenced passages found.").foregroundStyle(.secondary).padding()
                            } else {
                                PassageView(html: references.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                            onLink: { model.followLink($0, currentModule: references.module, currentReference: references.context) },
                                            onLinkHover: { model.previewStudyLink($0) })
                            }
                        } else if let note = model.studyNote {
                            Text("\(note.module) • \(note.reference)").font(.caption).padding(.horizontal, 8)
                            if !note.referenceList.isEmpty {
                                Button("Show referenced passages") { model.showReferencedPassages() }.disabled(model.isBusy).padding(8)
                            }
                            PassageView(html: note.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                        onLink: { model.followLink($0, currentModule: note.module, currentReference: note.reference) },
                                        onLinkHover: { model.previewStudyLink($0) })
                        }
                    }.frame(minHeight: 140, idealHeight: 220)
                }
                if let preview = model.informationPreview {
                    VStack(alignment: .leading, spacing: 0) {
                        HStack {
                            Text("Information Preview").font(.headline)
                            Spacer()
                            if model.isPreviewLoading { ProgressView().controlSize(.small) }
                        }.padding(8)
                        Text("\(preview.kind == .strongs ? "Strong’s" : "Morphology"): \(preview.value)")
                            .font(.caption).padding(.horizontal, 8)
                        Text(preview.entry.module).font(.caption).foregroundStyle(.secondary).padding(.horizontal, 8)
                        PassageView(html: preview.entry.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                    onLink: { model.followLink($0, currentModule: preview.module, currentReference: preview.sourceReference) },
                                    onLinkHover: { model.previewStudyLink($0) })
                    }.frame(minHeight: 140, idealHeight: 220)
                } else if let entry = model.informationDictionary {
                    VStack(alignment: .leading, spacing: 0) {
                        HStack {
                            Text("Information Preview").font(.headline)
                            Spacer()
                            if model.isPreviewLoading { ProgressView().controlSize(.small) }
                        }.padding(8)
                        Text("\(entry.module) • \(entry.key)").font(.caption).foregroundStyle(.secondary).padding(.horizontal, 8)
                        PassageView(html: entry.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                    onLink: { model.followLink($0, currentModule: entry.module, currentReference: entry.key) },
                                    onLinkHover: { model.previewStudyLink($0) })
                    }.frame(minHeight: 140, idealHeight: 220)
                } else if let note = model.informationNote {
                    VStack(alignment: .leading, spacing: 0) {
                        HStack {
                            Text("Information Preview").font(.headline)
                            Spacer()
                            if model.isPreviewLoading { ProgressView().controlSize(.small) }
                        }.padding(8)
                        Text(note.type == "crossReference" ? "Cross-references" : "Study note")
                            .font(.caption).padding(.horizontal, 8)
                        Text("\(note.module) • \(note.reference)").font(.caption).foregroundStyle(.secondary).padding(.horizontal, 8)
                        PassageView(html: note.html, anchor: nil, onScrollFailure: { model.message = $0 },
                                    onLink: { model.followLink($0, currentModule: note.module, currentReference: note.reference) },
                                    onLinkHover: { model.previewStudyLink($0) })
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
                        .onChange(of: model.showChapter) { model.refreshDisplay() }
                    Menu("Display") {
                        Toggle("Verse numbers", isOn: $model.verseNumbers)
                        Toggle("Highlight selected verse", isOn: $model.highlightSelected)
                    }.disabled(!model.showChapter && !model.showCommentaryChapter)
                    Menu("Reading options") {
                        if let numbers = model.contentOptions.noteNumbers {
                            Toggle("Footnote and cross-reference numbers", isOn: Binding(get: { numbers }, set: { model.setNoteNumbers($0) }))
                        }
                        if let spacing = model.contentOptions.doubleSpace {
                            Toggle("Double space", isOn: Binding(get: { spacing }, set: { model.setDoubleSpace($0) }))
                        }
                        if let images = model.contentOptions.images {
                            Toggle("Image content", isOn: Binding(get: { images }, set: { model.setImageContent($0) }))
                        }
                        if let fonts = model.contentOptions.fontFaces {
                            Toggle("Respect font faces", isOn: Binding(get: { fonts }, set: { model.setRespectFontFaces($0) }))
                        }
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
                    }.disabled(model.engineOptions == nil || (model.contentOptions.noteNumbers == nil && model.contentOptions.doubleSpace == nil && model.contentOptions.images == nil && model.contentOptions.fontFaces == nil && model.moduleOptions.declared.intersection(
                        [.wordsInRed, .strongs, .morphology, .footnotes, .greekAccents, .crossReferences, .hebrewVowels, .hebrewCantillation, .lemmas, .transliteratedForms, .glosses, .morphemeSegmentation, .variantsPrimary]).isEmpty))
                }.padding(.horizontal, 12).padding(.bottom, 8).disabled(model.isBusy)
                HStack {
                    Menu("Preferences") {
                        Button("Reload Reading Notes") {model.refreshDisplay()}.disabled(!model.readingSources.enabled)
                        Button("Clear Reading Notes") {model.applyReadingSources(CoreReadingSources())}.disabled(!model.readingSources.enabled)
                        Button("Parallel Reading") { model.openParallel() }.disabled(model.parallelProfilePath == nil)
                        Button("Import reading options…") { choosePreferenceFile() }
                        Button("Save reading options…") { chooseSaveFile() }
                        Divider()
                        Button("Lexicon dictionaries…") { model.showLexiconPreferences = true }.disabled(model.lexiconPreferences == nil)
                        Button("Import lexicon choices…") { chooseLexiconImport() }
                        Button("Save lexicon choices…") { chooseLexiconSave() }.disabled(model.lexiconPreferences == nil)
                        Divider()
                        Toggle("Cross-references in verse list", isOn: Binding(
                            get: { model.crossReferencesInVerseList },
                            set: { model.setCrossReferencesInVerseList($0) }))
                    }.disabled(model.modules.isEmpty)
                    Button("Previous chapter") { model.navigateChapter(-1) }.disabled(model.selection == nil)
                    Button("Next chapter") { model.navigateChapter(1) }.disabled(model.selection == nil)
                }.padding(.horizontal, 12).padding(.bottom, 8).disabled(model.isBusy)
                Divider()
                if !model.renderedReference.isEmpty {
                    Text(model.renderedReference).font(.headline).padding(12)
                }
                PassageView(html: model.html, rightToLeft: model.readingRightToLeft, css: model.lexicalPresentation.css, anchor: model.renderedAnchor, anchorRevision: model.renderedAnchorRevision,
                            onScrollFailure: { model.message = $0 }, onLink: { model.followLink($0) },
                            onLinkHover: { model.previewStudyLink($0) })
                if !model.readingSourceNotice.isEmpty {Text(model.readingSourceNotice).font(.caption).foregroundStyle(.secondary).padding(8)}
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
            if model.modules.contains(where: { $0.type == "Commentaries" }) || model.dictionarySelection != nil || model.showingBook || model.showingParallel {
                VSplitView {
                    if model.showingParallel,let chapter=model.parallelChapter { NativeParallelChapterPane(model:model,chapter:chapter) }
                    if model.modules.contains(where: { $0.type == "Commentaries" }) {
                        VStack(alignment: .leading, spacing: 0) {
                            HStack {
                                Text("Commentary").font(.headline)
                                Spacer()
                                if !model.commentarySelection.isEmpty {
                                    Button("Close") { model.chooseCommentary("") }.disabled(model.isBusy)
                                }
                            }.padding(8)
                            Picker("Commentary module", selection: Binding(
                                get: { model.commentarySelection }, set: { model.chooseCommentary($0) })) {
                                Text("Select a commentary").tag("")
                                ForEach(model.modules.filter { $0.type == "Commentaries" }) { module in
                                    Text(module.id).tag(module.id)
                                }
                            }.padding(8).disabled(model.isBusy)
                            Toggle("Whole commentary chapter", isOn: Binding(
                                get: { model.showCommentaryChapter }, set: { model.setCommentaryChapterMode($0) }))
                                .padding(.horizontal, 8).padding(.bottom, 8).disabled(model.isBusy || model.commentaryEntry == nil ||
                                    model.modules.first(where: { $0.id == model.commentarySelection })?.isRegularCommentary != true)
                            Toggle("Commentary headings", isOn: Binding(
                                get: { model.commentaryHeadings }, set: { model.setCommentaryHeadings($0) }))
                                .padding(.horizontal, 8).padding(.bottom, 8).disabled(model.isBusy || model.commentaryEntry == nil ||
                                    model.modules.first(where: { $0.id == model.commentarySelection })?.options.hasHeadings != true)
                            if model.commentaryContentOptions.noteNumbers != nil || model.commentaryContentOptions.doubleSpace != nil || model.commentaryContentOptions.images != nil || model.commentaryContentOptions.fontFaces != nil {
                                Menu("Commentary options") {
                                    if let numbers = model.commentaryContentOptions.noteNumbers {
                                        Toggle("Footnote and cross-reference numbers", isOn: Binding(get: { numbers }, set: { model.setNoteNumbers($0, commentary: true) }))
                                    }
                                    if let spacing = model.commentaryContentOptions.doubleSpace {
                                        Toggle("Double space", isOn: Binding(get: { spacing }, set: { model.setDoubleSpace($0, commentary: true) }))
                                    }
                                    if let images = model.commentaryContentOptions.images {
                                        Toggle("Image content", isOn: Binding(get: { images }, set: { model.setImageContent($0, commentary: true) }))
                                    }
                                    if let fonts = model.commentaryContentOptions.fontFaces {
                                        Toggle("Respect font faces", isOn: Binding(get: { fonts }, set: { model.setRespectFontFaces($0, commentary: true) }))
                                    }
                                }.padding(.horizontal, 8).padding(.bottom, 8).disabled(model.isBusy)
                            }
                            if let entry = model.commentaryEntry {
                                Text(model.commentaryChapter?.title ?? entry.reference).font(.caption).padding(8)
                                PassageView(html: model.commentaryDecoration?.html ?? model.commentaryChapter?.html ?? (entry.headingHTML + entry.html), rightToLeft: model.modules.first(where: {$0.id == model.commentarySelection})?.rightToLeft, css: model.commentaryDecoration?.css ?? (model.commentaryChapter?.lexical ?? entry.lexical).css, anchor: model.commentaryRenderedAnchor, anchorRevision: model.commentaryAnchorRevision, onScrollFailure: { model.message = $0 },
                                    onLink: { model.followCommentaryLink($0) },
                                    onLinkHover: { model.previewStudyLink($0) })
                                if model.commentaryChapter?.verses.allSatisfy({ $0.html.isEmpty && $0.headingHTML.isEmpty }) ?? (entry.html.isEmpty && entry.headingHTML.isEmpty) {
                                    Text("This commentary has no text at this reference.").foregroundStyle(.secondary).padding(8)
                                }
                            } else {
                                Text("Select a commentary to follow the Bible passage.").foregroundStyle(.secondary).padding(8)
                                Spacer()
                            }
                        }.frame(minHeight: 140)
                    }
            if model.showingBook {
                BookPane(model: model)
            }
            if model.dictionarySelection != nil && !model.showingBook {
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
                    }.labelsHidden().accessibilityLabel("Dictionary module").padding(8).disabled(model.isBusy)
                    HStack {
                        TextField("Dictionary word or key", text: $model.dictionaryKey).onSubmit { model.lookupDictionary() }
                            .accessibilityLabel("Dictionary word or key")
                        Menu {
                            if let choices = model.dictionaryChoices {
                                ForEach(Array(choices.keys.enumerated()), id: \.offset) { _, key in
                                    Button(key) { model.chooseDictionaryKey(key) }
                                }
                            }
                        } label: { Image(systemName: "list.bullet") }
                            .accessibilityLabel("Nearby dictionary keys").help("Nearby dictionary keys")
                            .disabled(model.dictionaryChoices == nil || model.dictionarySelection != model.dictionaryChoices?.module)
                        Button("Look up") { model.lookupDictionary() }
                    }.padding(8).disabled(model.isBusy)
                    HStack {
                        Button("Back") { model.navigateDictionaryHistory(-1) }
                            .disabled(model.dictionaryHistory?.canBack != true).accessibilityLabel("Dictionary history Back")
                        Button("Forward") { model.navigateDictionaryHistory(1) }
                            .disabled(model.dictionaryHistory?.canForward != true).accessibilityLabel("Dictionary history Forward")
                        Menu {
                            Button("Clear dictionary history") { model.clearDictionaryHistory() }
                                .disabled(model.dictionaryHistory?.entries.isEmpty != false)
                        } label: { Image(systemName: "clock.arrow.circlepath") }
                            .accessibilityLabel("Dictionary history").help("Dictionary history")
                    }.padding(.horizontal, 8).padding(.bottom, 8).disabled(model.isBusy)
                    HStack {
                        Button("Previous key") { model.navigateDictionary(-1) }
                        Button("Next key") { model.navigateDictionary(1) }
                    }.padding(.horizontal, 8).padding(.bottom, 8)
                        .disabled(model.isBusy || model.dictionaryEntry == nil || model.dictionarySelection != model.dictionaryEntry?.module)
                    Divider()
                    if let entry = model.dictionaryEntry {
                        Text("\(entry.module) • \(entry.key)").font(.caption).padding(8)
                        PassageView(html: entry.html, rightToLeft: model.modules.first(where: {$0.id == entry.module})?.rightToLeft, anchor: entry.anchor, anchorRevision: model.dictionaryAnchorRevision, onScrollFailure: { model.message = $0 },
                                    onLink: { model.followLink($0, currentModule: model.dictionarySource?.module, currentReference: model.dictionarySource?.reference) },
                                    onLinkHover: { model.previewStudyLink($0) })
                    } else {
                        Text("Enter a word or key to look up.").foregroundStyle(.secondary).padding()
                        Spacer()
                    }
                }.frame(minWidth: 230, idealWidth: 300)
            }
                }.frame(minWidth: 230, idealWidth: 300)
            }
        }.frame(minWidth: 700, minHeight: 450)
            .onChange(of: model.selection) { model.selectionChanged() }
            .onChange(of: model.verseNumbers) { model.refreshDisplay() }
            .onChange(of: model.highlightSelected) { model.refreshDisplay() }
            .sheet(item: Binding(get: { model.verseImage }, set: { if $0 == nil { model.closeVerseImage() } })) { image in
                VerseImagePreviewView(image: image, close: model.closeVerseImage)
            }
            .sheet(isPresented: $model.showLexiconPreferences) {
                if let preferences = model.lexiconPreferences { LexiconPreferencesView(model: model, preferences: preferences) }
            }
            .onDisappear { if closeOnDisappear { model.close() } }
    }
}

#if !XIPHOS_PREVIEW_TEST
@main
struct XiphosNativePreview: App {
    @NSApplicationDelegateAdaptor(SessionAppDelegate.self) private var delegate
    var body: some Scene {
        Settings { NativeSettingsView(delegate: delegate) }
        .commands {
            CommandGroup(replacing: .newItem) {
                Button("New Reading Window") { delegate.openNewWindow() }.keyboardShortcut("n")
                Button("New Reading Tab") { delegate.openNewTab() }.keyboardShortcut("t")
                Button("Move Reading Tab to New Window") { delegate.detachReadingTab() }
                Button("Save") { delegate.saveSession() }.keyboardShortcut("s")
                Button("Save As…") { delegate.saveSessionAs() }.keyboardShortcut("s",modifiers:[.command,.shift])
                Divider()
                Button("Import Legacy Settings and Session…") { delegate.openLegacyMigration() }
                Button("BibleSync…") { delegate.openSync() }
                Button("Bookmarks and Imports…") { delegate.openBookmarks() }
                Button("Personal Verse Notes…") { delegate.openPersonalNotes() }.keyboardShortcut("n", modifiers: [.command,.shift])
                Button("Saved Parallel Reading Sets…") { delegate.openParallelSets() }.keyboardShortcut("p", modifiers: [.command,.shift])
                Button("Module Library…") { delegate.openModuleLibrary() }
                Button("Choose Module Library Folder…") { delegate.chooseModuleLibrary() }
                Button("New Module Library…") { delegate.chooseModuleLibrary(create:true) }
                Divider()
                Button("New Journal or Prayer Section…") { delegate.newBookSection() }
                Button("New Study Document") { delegate.newEditorDocument() }
                Button("Open Study Document…") { delegate.openEditorDocument() }.keyboardShortcut("o")
                Button("Edit Personal Commentary or Book…") { delegate.editCurrentModule() }
            }
        }
    }
}
#endif
