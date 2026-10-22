// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import Combine

enum SearchScopeChoice: String, CaseIterable, Identifiable {
    case all = "All text", bounds = "Bounds", last = "Last results"
    var id: Self { self }
}
struct SearchHit: Identifiable, Sendable, Equatable {
    let id: Int
    let module: String
    let reference: String
}
// Presentation state only. Query preparation, search, bounds, complete retained
// scope, cancellation and configured reading all belong to the shared core.
@MainActor final class SearchModel: ObservableObject {
    @Published private(set) var modules: [InstalledModule] = []
    @Published var module = ""
    @Published var expression = ""
    @Published var mode: CoreSearchMode = .phrase
    @Published var caseSensitive = false
    @Published var scopeChoice: SearchScopeChoice = .all
    @Published var boundsModule = ""
    @Published var bounds = ""
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
    @Published var exportFormat: SearchExportFormat = .plainText
    @Published var exportImages = false
    @Published private(set) var message = "Enter text to search."
    var isBusy: Bool { isSearching || isPreviewLoading || isClearing || isExporting }
    var canSearch: Bool { !isBusy && !module.isEmpty && !expression.isEmpty }
    var canExportSelected: Bool {
        !closed && !isBusy && result?.generation.id != nil && result?.generation.id != 0 && hits.contains { $0.id == selection }
    }
    var canExportAll: Bool {
        !closed && !isBusy && !hits.isEmpty && result?.generation.id != nil && result?.generation.id != 0
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
        isSearching = false; isPreviewLoading = false; isClearing = false; isExporting = false
    }
    func stop() { if isSearching || isExporting { task?.cancel() } }
    func search() {
        guard !closed, canSearch, let worker, let tab else { return }
        let submittedModule = module, submittedExpression = expression, submittedMode = mode
        let sensitive = caseSensitive, last = scopeChoice == .last
        let range = scopeChoice == .bounds ? CoreSearchRange(module: boundsModule, expression: bounds) : nil
        revision &+= 1; let token = revision
        isSearching = true; progress = nil; message = "Searching…"
        task = Task { [weak self] in
            guard let self else { return }
            defer { if !self.closed && self.revision == token { self.isSearching = false; self.isPreviewLoading = false; self.task = nil } }
            do {
                let copied = try await worker.searchModule(tab: tab, module: submittedModule, expression: submittedExpression,
                    mode: submittedMode, caseSensitive: sensitive, range: range, withinLastResults: last,
                    returnCancelledResults: true, onProgress: { [weak self] progress in
                        await self?.receive(progress, token: token)
                    })
                guard !self.closed, self.revision == token else { return }
                self.scope = copied.scope
                self.isSearching = false
                // Pre-execution stop keeps the prior visible result and scope.
                if copied.executed || !copied.cancelled {
                    self.result = copied
                    self.hits = copied.references.enumerated().map { SearchHit(id: $0.offset, module: copied.module, reference: $0.element) }
                    self.selection = nil; self.preview = nil; self.previewSource = nil
                }
                if copied.cancelled { self.message = "Search stopped." }
                else if copied.limited { self.message = "Showing \(copied.references.count) passages. More results are available." }
                else { self.message = "\(copied.references.count) \(copied.references.count == 1 ? "passage" : "passages") found." }
                if (copied.executed || !copied.cancelled), let first = self.hits.first {
                    self.selection = first.id
                    try await self.loadPreview(first, worker: worker, tab: tab, token: token)
                }
            } catch {
                guard !self.closed, self.revision == token else { return }
                self.message = error.localizedDescription
                // Engine publication can precede C/Swift presentation failure.
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
        guard !closed, !isSearching, !isClearing, !isExporting, let worker, let tab, let hit = hits.first(where: { $0.id == id }) else { return }
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
        guard allMatches ? canExportAll : canExportSelected, let worker, let tab, let source = result else { return }
        let selected = selection, includeScripture = exportScripture, includeContext = exportSearchContext
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
                    let rows = allMatches ? [] : selected.map { [CoreSearchVisibleRow(group: 0, row: $0)] } ?? []
                    payload = .html(try await worker.exportSearchHTML(tab: tab, generation: source.generation.id,
                        selection: allMatches ? .allRetained : .visible, rows: rows, title: title,
                        includeScripture: includeScripture, includeSearchContext: includeContext, images: images, plainTextMaxBytes: plainTextFallback ? 1024 * 1024 : nil, returnCancelledResults: true, onProgress: callback))
                } else if allMatches {
                    payload = .text(try await worker.exportSearchAllText(tab: tab, generation: source.generation.id,
                        title: title, includeScripture: includeScripture, includeSearchContext: includeContext, returnCancelledResults: true, onProgress: callback))
                } else {
                    guard let selected else { return }
                    payload = .text(try await worker.exportSearchVisibleText(tab: tab, generation: source.generation.id,
                        rows: [CoreSearchVisibleRow(group: 0, row: selected)], title: title, includeScripture: includeScripture, includeSearchContext: includeContext,
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
                case .sourceIncomplete: self.message = "The search stopped or some modules failed. Export a selected passage or run the search again."
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
