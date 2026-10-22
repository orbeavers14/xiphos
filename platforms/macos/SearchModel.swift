// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import Combine

enum SearchScopeChoice: String, CaseIterable, Identifiable {
    case all = "All text", bounds = "Bounds", last = "Last results"
    var id: Self { self }
}
struct SearchHit: Identifiable, Sendable, Equatable {
    let id: Int
    let group: Int
    let row: Int
    let module: String
    let reference: String
}
// Presentation state only. Query preparation, search, bounds, complete retained
// scope, cancellation and configured reading all belong to the shared core.
@MainActor final class SearchModel: ObservableObject {
    @Published private(set) var modules: [InstalledModule] = []
    @Published var module = ""
    @Published var searchMultipleModules = false
    @Published var selectedModules: [String] = []
    @Published var expression = ""
    @Published var mode: CoreSearchMode = .phrase
    @Published var caseSensitive = false
    @Published var setAnnotationOptions = false
    @Published var searchStrongs = false
    @Published var searchMorphology = false
    @Published var searchFootnotes = false
    @Published var scopeChoice: SearchScopeChoice = .all
    @Published var boundsModule = ""
    @Published var bounds = ""
    @Published private(set) var savedSearchLists = SavedSearchLists(moduleLists: [], ranges: [])
    @Published private(set) var isLoadingSearchSetup = false
    @Published private(set) var batchResult: RenderedSearchBatch?
    private var batchExpression = ""
    @Published private(set) var result: RenderedSearchResult?
    @Published private(set) var hits: [SearchHit] = []
    @Published private(set) var selection: Int?
    @Published private(set) var preview: RenderedEntry?
    @Published private(set) var previewSource: SearchHit?
    @Published private(set) var progress: CoreSearchProgress?
    @Published private(set) var scope = CoreSearchScope(present: false, elementCount: 0)
    @Published private(set) var isSearching = false
    @Published private(set) var isPreviewLoading = false
    @Published private(set) var isClearing = false
    @Published private(set) var isExporting = false
    @Published var exportScripture = false
    @Published var exportSearchContext = false
    @Published var exportPartialSource = false
    @Published var exportFormat: SearchExportFormat = .plainText
    @Published var exportImages = false
    @Published private(set) var message = "Enter text to search."
    var isBusy: Bool { isSearching || isPreviewLoading || isClearing || isExporting || isLoadingSearchSetup }
    var canSearch: Bool {
        !isBusy && !expression.isEmpty && (searchMultipleModules ? (1...32).contains(selectedModules.count) : !module.isEmpty)
    }
    private var exportSource: (generation: CoreSearchGeneration, expression: String)? {
        if let batchResult { return (batchResult.generation, batchExpression) }
        if let result { return (result.generation, result.expression) }
        return nil
    }
    var hasIncompleteExportSource: Bool {
        guard let source = exportSource else { return false }
        return source.generation.cancelled || source.generation.failedGroups > 0 ||
            source.generation.groupCount != source.generation.requestedCount
    }
    func addModule() {
        guard !closed, !isBusy, selectedModules.count < 32, modules.contains(where: { $0.id == module }) else { return }
        selectedModules.append(module)
    }
    func removeModule(at index: Int) {
        guard !closed, !isBusy, selectedModules.indices.contains(index) else { return }
        selectedModules.remove(at: index)
    }
    func moveModule(at index: Int, by offset: Int) {
        guard !closed, !isBusy, [-1,1].contains(offset), selectedModules.indices.contains(index),
              selectedModules.indices.contains(index + offset) else { return }
        selectedModules.swapAt(index, index + offset)
    }
    func groupMessage(_ index: Int) -> String {
        guard let group = batchResult?.groups.first(where: { $0.requestIndex == index }) else { return "Not searched." }
        switch group.status {
        case .moduleNotFound: return "Module unavailable."
        case .invalidRequest: return "Search not supported for this request."
        case .engineError: return "Unable to search this module."
        case .success:
            if group.result?.cancelled == true { return "Search stopped; showing returned matches." }
            if group.result?.limited == true { return "Showing returned passages; more matches are available." }
            return group.result?.references.isEmpty == true ? "No returned matches." : ""
        }
    }
    var canExportSelected: Bool {
        !closed && !isBusy && exportSource?.generation.id != nil && exportSource?.generation.id != 0 && hits.contains { $0.id == selection }
    }
    var canExportAll: Bool {
        !closed && !isBusy && !hits.isEmpty && exportSource?.generation.id != nil && exportSource?.generation.id != 0
    }
    private var worker: CoreWorker?
    private var tab: UInt64?
    private var revision: UInt64 = 0
    private var closed = false
    private var task: Task<Void, Never>?
    deinit { task?.cancel() }
    func configure(worker: CoreWorker, tab: UInt64, modules: [InstalledModule], preferred: String?) {
        guard !closed, self.tab == nil else { return }
        self.worker = worker; self.tab = tab
        self.modules = modules.filter(\.hasVerseSearch)
        module = self.modules.first(where: { $0.id == preferred })?.id ?? self.modules.first?.id ?? ""
        boundsModule = module
        if self.modules.isEmpty { message = "No supported Bible or commentary modules are installed." }
    }
    func close() {
        guard !closed else { return }
        closed = true; revision &+= 1; task?.cancel(); task = nil
        isSearching = false; isPreviewLoading = false; isClearing = false; isExporting = false; isLoadingSearchSetup = false
    }
    func loadSearchLists(path: String) {
        guard !closed, !isBusy, let worker, let tab else { return }
        revision &+= 1; let token = revision
        isLoadingSearchSetup = true; message = "Loading saved search lists…"
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isLoadingSearchSetup = false; self.task = nil } }
            do {
                let copied = try await worker.readSearchLists(tab: tab, path: path)
                guard !self.closed, self.revision == token else { return }
                self.savedSearchLists = copied
                self.message = "Saved search lists loaded. Choose a module list or range to apply."
            } catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
    // The selected destination is explicit; only the verified returned snapshot
    // is published. Saving never applies a choice to the current search draft.
    func saveSearchList(path: String, edit: SavedSearchListEdit) {
        guard !closed, !isBusy, let worker, let tab else { return }
        revision &+= 1; let token = revision
        isLoadingSearchSetup = true; message = "Saving search list…"
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isLoadingSearchSetup = false; self.task = nil } }
            do {
                let copied = try await worker.saveSearchList(tab: tab, path: path, edit: edit)
                guard !self.closed, self.revision == token else { return }
                self.savedSearchLists = copied
                self.message = "Search list saved. Choose a module list or range to apply."
            } catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
    func savedSearchListEdit(kind: SavedSearchListKind, at index: Int) -> SavedSearchListEdit? {
        guard !closed, !isBusy else { return nil }
        let items = kind == .modules ? savedSearchLists.moduleLists : savedSearchLists.ranges
        guard items.indices.contains(index) else { return nil }
        let effective = items[index].effectiveIndex
        guard items.indices.contains(effective), let label = items[effective].label,
              let value = items[effective].value else { return nil }
        return SavedSearchListEdit(kind: kind, label: label, value: value)
    }
    func removeSearchList(path: String, kind: SavedSearchListKind, label: String) {
        guard !closed, !isBusy, let worker, let tab else { return }
        revision &+= 1; let token = revision
        isLoadingSearchSetup = true; message = "Removing saved search list…"
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isLoadingSearchSetup = false; self.task = nil } }
            do {
                let copied = try await worker.removeSearchList(tab: tab, path: path, kind: kind, label: label)
                guard !self.closed, self.revision == token else { return }
                self.savedSearchLists = copied
                self.message = "Saved list removal completed. Current search unchanged."
            } catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
    func applySavedModuleList(at index: Int) {
        guard !closed, !isBusy, let worker, let tab, savedSearchLists.moduleLists.indices.contains(index) else { return }
        let effective = savedSearchLists.moduleLists[index].effectiveIndex
        guard savedSearchLists.moduleLists.indices.contains(effective), let text = savedSearchLists.moduleLists[effective].value else {
            message = "This saved module list has no value."; return
        }
        revision &+= 1; let token = revision
        isLoadingSearchSetup = true; message = "Applying saved module list…"
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isLoadingSearchSetup = false; self.task = nil } }
            do {
                let names = try await worker.parseSearchModuleList(tab: tab, text: text)
                guard !self.closed, self.revision == token else { return }
                self.selectedModules = names; self.searchMultipleModules = true
                self.message = names.isEmpty ? "This saved module list is empty." : "Saved module list applied."
            } catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
    func applySavedRange(at index: Int) {
        guard !closed, !isBusy, savedSearchLists.ranges.indices.contains(index) else { return }
        let effective = savedSearchLists.ranges[index].effectiveIndex
        guard savedSearchLists.ranges.indices.contains(effective), let text = savedSearchLists.ranges[effective].value else {
            message = "This saved range has no value."; return
        }
        bounds = text; scopeChoice = .bounds
        message = "Saved range applied. Check the bounds module before searching."
    }
    func stop() { if isSearching || isExporting { task?.cancel() } }
    func search() {
        guard !closed, canSearch, let worker, let tab else { return }
        let submittedModule = module, submittedExpression = expression, submittedMode = mode
        let submittedModules = searchMultipleModules ? selectedModules : nil
        let sensitive = caseSensitive, last = scopeChoice == .last
        let annotations = setAnnotationOptions ? CoreSearchTextOptions(strongs: searchStrongs,
            morphology: searchMorphology, footnotes: searchFootnotes) : nil
        let range = scopeChoice == .bounds ? CoreSearchRange(module: boundsModule, expression: bounds) : nil
        revision &+= 1; let token = revision
        isSearching = true; progress = nil; message = "Searching…"
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isSearching = false; self.isPreviewLoading = false; self.task = nil } }
            do {
                let callback: @Sendable (CoreSearchProgress) async -> Void = { [weak self] progress in
                    await self?.receive(progress, token: token)
                }
                var published = false
                if let submittedModules {
                    let copied = try await worker.searchModules(tab: tab, modules: submittedModules,
                        expression: submittedExpression, mode: submittedMode, caseSensitive: sensitive,
                        range: range, withinLastResults: last, textOptions: annotations,
                        returnCancelledResults: true, onProgress: callback)
                    guard !self.closed, self.revision == token else { return }
                    self.scope = copied.scope
                    if copied.generationUpdated {
                        self.exportPartialSource = false
                        self.batchExpression = submittedExpression; self.batchResult = copied; self.result = nil
                        var hits: [SearchHit] = []
                        for group in copied.groups {
                            guard let result = group.result else { continue }
                            for (row, reference) in result.references.enumerated() {
                                hits.append(SearchHit(id: hits.count, group: group.requestIndex, row: row,
                                    module: result.module, reference: reference))
                            }
                        }
                        self.hits = hits; published = true
                    }
                    self.message = "\(copied.copiedResultCount) passages returned across \(copied.groups.count) of \(copied.requestedModules.count) module requests."
                    if copied.failureCount > 0 { self.message += " \(copied.failureCount) module requests failed." }
                    if copied.limited { self.message += " More results are available." }
                    if copied.groups.contains(where: { $0.result?.indexFallback == true }) { self.message += " Index unavailable for some modules; used multiword fallback." }
                    if copied.cancelled { self.message = "Search stopped. " + self.message }
                } else {
                    let copied = try await worker.searchModule(tab: tab, module: submittedModule, expression: submittedExpression,
                        mode: submittedMode, caseSensitive: sensitive, range: range, withinLastResults: last,
                        textOptions: annotations, returnCancelledResults: true, onProgress: callback)
                    guard !self.closed, self.revision == token else { return }
                    self.scope = copied.scope
                    if copied.executed || !copied.cancelled {
                        self.exportPartialSource = false
                        self.result = copied; self.batchResult = nil; self.batchExpression = ""
                        self.hits = copied.references.enumerated().map {
                            SearchHit(id: $0.offset, group: 0, row: $0.offset, module: copied.module, reference: $0.element)
                        }
                        published = true
                    }
                    if copied.cancelled { self.message = "Search stopped." }
                    else if copied.limited { self.message = "Showing \(copied.references.count) passages. More results are available." }
                    else { self.message = "\(copied.references.count) \(copied.references.count == 1 ? "passage" : "passages") found." }
                    if copied.indexFallback && !copied.cancelled { self.message = "Index unavailable; used multiword search. " + self.message }
                }
                self.isSearching = false
                if published {
                    self.selection = nil; self.preview = nil; self.previewSource = nil
                    if let first = self.hits.first {
                        self.selection = first.id
                        try await self.loadPreview(first, worker: worker, tab: tab, token: token)
                    }
                }
            } catch {
                guard !self.closed, self.revision == token else { return }
                self.message = error.localizedDescription
                if let current = try? await worker.lastSearchScope(tab: tab), !self.closed, self.revision == token { self.scope = current }
            }
        }
    }
    private func receive(_ copied: CoreSearchProgress, token: UInt64) {
        guard !closed, revision == token else { return }
        progress = copied
    }
    private func loadPreview(_ hit: SearchHit, worker: CoreWorker, tab: UInt64, token: UInt64) async throws {
        guard !closed, revision == token else { return }
        isPreviewLoading = true
        let entry = try await worker.readTabEntry(tab: tab, module: hit.module, reference: hit.reference)
        guard !closed, revision == token else { return }
        preview = entry; previewSource = hit; isPreviewLoading = false
    }
    func select(_ id: Int?) {
        guard !closed, !isSearching, !isClearing, !isExporting, !isLoadingSearchSetup, let worker, let tab, let hit = hits.first(where: { $0.id == id }) else { return }
        if selection == hit.id && previewSource == hit { return }
        task?.cancel()
        revision &+= 1; let token = revision
        selection = hit.id; isPreviewLoading = true
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isPreviewLoading = false; self.task = nil } }
            do { try await self.loadPreview(hit, worker: worker, tab: tab, token: token) }
            catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
    func exportText(allMatches: Bool, completion: @escaping @MainActor (RenderedSearchTextExport) -> Void) {
        exportPayload(allMatches: allMatches, format: .plainText, embedImages: false) {
            if case .text(let value) = $0 { completion(value) }
        }
    }
    func exportResults(allMatches: Bool, forClipboard: Bool = false, completion: @escaping @MainActor (SearchExportPayload) -> Void) {
        exportPayload(allMatches: allMatches, format: exportFormat, embedImages: exportImages, plainTextFallback: forClipboard, completion: completion)
    }
    private func exportPayload(allMatches: Bool, format: SearchExportFormat, embedImages: Bool, plainTextFallback: Bool = false,
                               completion: @escaping @MainActor (SearchExportPayload) -> Void) {
        guard allMatches ? canExportAll : canExportSelected, let worker, let tab, let source = exportSource else { return }
        let selected = hits.first(where: { $0.id == selection }), includeScripture = exportScripture, includeContext = exportSearchContext
        // The offered native partial-export choice requires shared context so
        // incomplete documents carry the core's source-disposition disclosure.
        let allowPartial = allMatches && hasIncompleteExportSource && exportPartialSource && includeContext
        // Capture displayed result identity and original query, never editable drafts
        // or a subsequently queried latest generation. Core rejects stale identity.
        let title = "Search results: \(source.expression)"
        revision &+= 1; let token = revision
        isExporting = true; progress = nil; message = format == .html ? "Preparing HTML document…" : "Preparing search text…"
        task = Task { [weak self] in
            guard let self, !self.closed else { return }
            defer { if !self.closed && self.revision == token { self.isExporting = false; self.task = nil } }
            do {
                let callback: @Sendable (CoreSearchProgress) async -> Void = { [weak self] progress in
                    await self?.receive(progress, token: token)
                }
                let payload: SearchExportPayload
                if format == .html {
                    var images = CoreHTMLImageOptions()
                    if embedImages && includeScripture { images.policy = .embeddedStaticPNG }
                    let rows = allMatches ? [] : selected.map { [CoreSearchVisibleRow(group: $0.group, row: $0.row)] } ?? []
                    payload = .html(try await worker.exportSearchHTML(tab: tab, generation: source.generation.id,
                        selection: allMatches ? .allRetained : .visible, rows: rows, title: title,
                        includeScripture: includeScripture, includeSearchContext: includeContext, allowPartialSource: allowPartial, images: images, plainTextMaxBytes: plainTextFallback ? 1024 * 1024 : nil, returnCancelledResults: true, onProgress: callback))
                } else if allMatches {
                    payload = .text(try await worker.exportSearchAllText(tab: tab, generation: source.generation.id,
                        title: title, includeScripture: includeScripture, includeSearchContext: includeContext, allowPartialSource: allowPartial, returnCancelledResults: true, onProgress: callback))
                } else {
                    guard let selected else { return }
                    payload = .text(try await worker.exportSearchVisibleText(tab: tab, generation: source.generation.id,
                        rows: [CoreSearchVisibleRow(group: selected.group, row: selected.row)], title: title, includeScripture: includeScripture, includeSearchContext: includeContext,
                        returnCancelledResults: true, onProgress: callback))
                }
                guard !self.closed, self.revision == token else { return }
                // A late completion may win a stop in the core. A stopped native
                // action still must not unexpectedly open a dialog/write clipboard.
                guard !Task.isCancelled else { self.message = "Export stopped."; return }
                switch payload.status {
                case .success:
                    self.message = format == .html ? "HTML document ready." : "Search text ready."
                    self.isExporting = false
                    completion(payload)
                case .cancelled: self.message = "Export stopped."
                case .stale: self.message = "Search results changed. Run the search again before exporting."
                case .invalidSelection: self.message = "Select a search passage to export."
                case .outputLimit: self.message = "This export is too large. Select a passage or turn off scripture."
                case .capacityExceeded: self.message = "Complete search results are unavailable. Export a selected passage instead."
                case .sourceIncomplete: self.message = "The search stopped or some modules failed. Export a selected passage, or enable search details and explicitly choose to export returned matches."
                case .moduleNotFound: self.message = "A searched module is no longer available. Run the search again."
                case .engineError: self.message = "Unable to prepare this export. Try the search again."
                case .unsupportedHTML:
                    if case .html(let value) = payload, value.issue == .resources {
                        self.message = "This passage contains images or resources that are not enabled or supported for HTML export."
                    } else { self.message = "This content cannot be exported in that format yet." }
                }
            } catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
    func exportDestinationMessage(_ text: String) { if !closed { message = text } }
    func resetSearchAnnotations() {
        guard !closed, !isBusy, let worker, let tab else { return }
        revision &+= 1; let token = revision; isClearing = true
        message = "Resetting search annotations…"
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isClearing = false; self.task = nil } }
            do {
                _ = try await worker.clearSearchTextOptions(tab: tab)
                guard !self.closed, self.revision == token else { return }
                self.setAnnotationOptions = false
                self.searchStrongs = false; self.searchMorphology = false; self.searchFootnotes = false
                self.message = "Search annotation settings reset."
            } catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
    func clearScope() {
        guard !closed, !isBusy, let worker, let tab else { return }
        revision &+= 1; let token = revision; isClearing = true
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isClearing = false; self.task = nil } }
            do {
                let copied = try await worker.clearSearchScope(tab: tab)
                guard !self.closed, self.revision == token else { return }
                self.scope = copied; self.message = "Previous search cleared."
            } catch { if !self.closed && self.revision == token { self.message = error.localizedDescription } }
        }
    }
}
