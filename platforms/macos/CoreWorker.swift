// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore

struct DictionaryLocation: Sendable, Equatable { let module: String; let key: String }
struct CoreDictionaryHistoryState: Sendable, Equatable {
    let entries: [DictionaryLocation]
    let cursor: Int
    let canBack: Bool, canForward: Bool
}
struct CoreHistoryState: Sendable {
    let canBack: Bool
    let canForward: Bool
    let count: Int
    let module: String?
    let commentary: String?
}
// Only transfers a uniquely owned handle into background shutdown. All API
// submissions/polls otherwise stay actor-isolated; the C++ core owns scheduling.
private struct ShutdownHandle: @unchecked Sendable { let value: OpaquePointer }
actor CoreWorker {
    private var handle: ShutdownHandle?
    init(root: String? = nil) throws {
        var opened: OpaquePointer?
        let status = root.map { path in path.withCString { xc_worker_open($0, &opened) } }
            ?? xc_worker_open(nil, &opened)
        guard status == XC_OK, let opened else { throw CoreError(status: status) }
        handle = ShutdownHandle(value: opened)
    }
    deinit {
        if let handle {
            DispatchQueue.global(qos: .utility).async { xc_worker_close(handle.value) }
        }
    }
    func shutdown() async {
        guard let owned = handle else { return }
        handle = nil
        await withCheckedContinuation { continuation in
            DispatchQueue.global(qos: .utility).async {
                xc_worker_close(owned.value)
                continuation.resume()
            }
        }
    }
    private func worker() throws -> OpaquePointer {
        guard let handle else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        return handle.value
    }
    private func searchProgressValue(_ request: OpaquePointer) -> CoreSearchProgress? {
        var info = xc_search_progress()
        guard xc_request_search_progress(request, &info) == XC_OK,
              let phase = CoreSearchPhase(rawValue: info.phase) else { return nil }
        return CoreSearchProgress(phase: phase, percent: info.percent, cancelRequested: info.cancel_requested != 0)
    }
    private func wait(_ status: xc_status, _ request: OpaquePointer?, cancellingSearch: Bool = false,
                      returnCancelledSearch: Bool = false,
                      onSearchProgress: (@Sendable (CoreSearchProgress) async -> Void)? = nil) async throws -> OpaquePointer {
        guard status == XC_OK, let request else { throw CoreError(status: status) }
        // Accepted searches drain even after stop. Optional progress contains
        // copied scalars only; callbacks never receive a request or SDK pointer.
        // Other accepted actions retain their original cancellation behavior.
        var delivered: CoreSearchProgress?
        var result = XC_PENDING
        repeat {
            if cancellingSearch && Task.isCancelled { _ = xc_request_cancel_search(request) }
            result = xc_request_poll(request)
            if let callback = onSearchProgress, let progress = searchProgressValue(request), progress != delivered {
                delivered = progress
                await callback(progress)
            }
            if result == XC_PENDING {
                await withCheckedContinuation { continuation in
                    DispatchQueue.global(qos: .utility).asyncAfter(deadline: .now() + .milliseconds(5)) {
                        continuation.resume()
                    }
                }
            }
        } while result == XC_PENDING
        guard result == XC_OK else {
            xc_request_release(request)
            throw CoreError(status: result)
        }
        if cancellingSearch && Task.isCancelled && !returnCancelledSearch {
            xc_request_release(request)
            throw CancellationError()
        }
        return request
    }
    func resolveModuleName(tab: UInt64, requested: String) async throws -> ModuleNameResolution {
        guard !requested.isEmpty, !requested.utf8.contains(0), requested.utf8.count <= 4096 else {
            throw CoreError(status: XC_INVALID_ARGUMENT)
        }
        let handle = try worker()
        var request: OpaquePointer?
        let status = requested.withCString { xc_worker_resolve_module_name(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_module_resolution(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_module_resolution_release(result) }
        guard let original = xc_module_resolution_requested(result) else { throw CoreError(status: XC_ENGINE_ERROR) }
        return ModuleNameResolution(requested: String(cString: original),
            module: xc_module_resolution_module(result).map { String(cString: $0) },
            viaAbbreviation: xc_module_resolution_via_abbreviation(result) != 0)
    }
    func modules() async throws -> [InstalledModule] {
        var request: OpaquePointer?
        let status = xc_worker_modules(try worker(), &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let catalog = xc_request_take_catalog(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_catalog_release(catalog) }
        return (0..<xc_catalog_count(catalog)).map { index in
            InstalledModule(id: String(cString: xc_catalog_id(catalog, index)!),
                description: String(cString: xc_catalog_description(catalog, index)!),
                type: String(cString: xc_catalog_type(catalog, index)!),
                language: String(cString: xc_catalog_language(catalog, index)!),
                options: ModuleOptionInfo(declared: EngineOptionFlags(rawValue: xc_catalog_declared_engine_flags(catalog, index)),
                    hasHeadings: xc_catalog_has_headings(catalog, index) != 0,
                    transliterationAvailable: xc_catalog_transliteration_available(catalog, index) != 0),
                isDictionary: xc_catalog_is_dictionary(catalog, index) != 0,
                isRegularCommentary: xc_catalog_is_regular_commentary(catalog, index) != 0,
                lexiconRoles: LexiconRoles(rawValue: xc_catalog_lexicon_roles(catalog, index)),
                hasBookTree: xc_catalog_has_book_tree(catalog, index) != 0,
                hasVerseSearch: xc_catalog_has_verse_search(catalog, index) != 0)
        }
    }
    func bookTree(module: String, offset: UInt64) async throws -> BookTree {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_book_tree(handle, $0, offset, &request) }
        return try await bookTreeValue(status, request, optional: false)!
    }
    func bookSelection(tab: UInt64) async throws -> BookTree? {
        var request: OpaquePointer?
        let status = xc_worker_book_selection(try worker(), tab, &request)
        return try await bookTreeValue(status, request, optional: true)
    }
    private func bookTreeValue(_ status: xc_status, _ request: OpaquePointer?, optional: Bool) async throws -> BookTree? {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        if optional && xc_request_book_selection_present(completed) == 0 { return nil }
        guard let result = xc_request_take_book_tree(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_book_tree_release(result) }
        return CoreValues.bookTree(result)
    }
    func selectBook(tab: UInt64, module: String, offset: UInt64) async throws -> RenderedBookEntry {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_select_book(handle, tab, $0, offset, &request) }
        return try await bookEntryValue(status, request, optional: false)!
    }
    func navigateBook(tab: UInt64, direction: BookNavigation) async throws -> RenderedBookEntry? {
        var request: OpaquePointer?
        let status = xc_worker_navigate_book(try worker(), tab, direction.rawValue, &request)
        return try await bookEntryValue(status, request, optional: true)
    }
    func restoreBookProfile(tab: UInt64, path: String, mode: BookRestoreMode) async throws -> RenderedBookEntry? {
        guard !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_restore_book_profile(handle, tab, $0, mode.rawValue, &request) }
        return try await bookEntryValue(status, request, optional: true)
    }
    func restoreBookSelection(tab: UInt64, saved: SavedBookSelection, mode: BookRestoreMode) async throws -> RenderedBookEntry? {
        var request: OpaquePointer?
        let handle = try worker()
        let status = try CoreValues.withSavedBookSelection(saved) {
            xc_worker_restore_book_selection(handle, tab, $0, mode.rawValue, &request)
        }
        return try await bookEntryValue(status, request, optional: true)
    }
    private func bookEntryValue(_ status: xc_status, _ request: OpaquePointer?, optional: Bool) async throws -> RenderedBookEntry? {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        if optional && xc_request_book_selection_present(completed) == 0 { return nil }
        guard let result = xc_request_take_book_entry(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_book_entry_release(result) }
        return CoreValues.bookEntry(result)
    }
    func bookNoteLink(tab: UInt64, module: String, offset: UInt64, uri: String, renderBody: Bool = true) async throws -> RenderedBookNote {
        guard ![module, uri].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            uri.withCString { xc_worker_book_note_link_at(handle, tab, id, offset, $0, renderBody ? 1 : 0, &request) }
        }
        return try await bookNoteValue(status, request)
    }
    func readBookDocument(tab: UInt64, module: String, offset: UInt64) async throws -> RenderedBookDocument {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_read_book_document(handle, tab, $0, offset, &request) }
        return try await bookDocumentValue(status, request, optional: false)!
    }
    func selectBookDocument(tab: UInt64, module: String, offset: UInt64) async throws -> RenderedBookDocument {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_select_book_document(handle, tab, $0, offset, &request) }
        return try await bookDocumentValue(status, request, optional: false)!
    }
    func bookDocumentSelection(tab: UInt64) async throws -> RenderedBookDocument? {
        var request: OpaquePointer?
        let status = xc_worker_book_document_selection(try worker(), tab, &request)
        return try await bookDocumentValue(status, request, optional: true)
    }
    func refreshBookDocument(tab: UInt64) async throws -> RenderedBookDocument? {
        var request: OpaquePointer?
        let status = xc_worker_refresh_book_document(try worker(), tab, &request)
        return try await bookDocumentValue(status, request, optional: true)
    }
    func navigateBookDocument(tab: UInt64, direction: BookNavigation) async throws -> RenderedBookDocument? {
        var request: OpaquePointer?
        let status = xc_worker_navigate_book_document(try worker(), tab, direction.rawValue, &request)
        return try await bookDocumentValue(status, request, optional: true)
    }
    func restoreBookDocumentSelection(tab: UInt64, saved: SavedBookSelection, mode: BookRestoreMode) async throws -> RenderedBookDocument? {
        var request: OpaquePointer?
        let handle = try worker()
        let status = try CoreValues.withSavedBookSelection(saved) {
            xc_worker_restore_book_document_selection(handle, tab, $0, mode.rawValue, &request)
        }
        return try await bookDocumentValue(status, request, optional: true)
    }
    func restoreBookDocumentProfile(tab: UInt64, path: String, mode: BookRestoreMode) async throws -> RenderedBookDocument? {
        guard !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_restore_book_document_profile(handle, tab, $0, mode.rawValue, &request) }
        return try await bookDocumentValue(status, request, optional: true)
    }
    private func bookDocumentValue(_ status: xc_status, _ request: OpaquePointer?, optional: Bool) async throws -> RenderedBookDocument? {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        if optional && xc_request_book_selection_present(completed) == 0 { return nil }
        guard let result = xc_request_take_book_document(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_book_document_release(result) }
        return CoreValues.bookDocument(result)
    }
    func bookDocumentNoteLink(tab: UInt64, uri: String, renderBody: Bool = true) async throws -> RenderedBookNote {
        guard !uri.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = uri.withCString { xc_worker_book_document_note_link(handle, tab, $0, renderBody ? 1 : 0, &request) }
        return try await bookNoteValue(status, request)
    }
    private func bookNoteValue(_ status: xc_status, _ request: OpaquePointer?) async throws -> RenderedBookNote {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_book_note(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_book_note_release(result) }
        return RenderedBookNote(tree: CoreValues.bookTree(xc_book_note_tree(result)!),
            content: noteValue(xc_book_note_content(result)!))
    }
    func searchModule(tab: UInt64, module: String, expression: String, mode: CoreSearchMode = .phrase,
                      caseSensitive: Bool = false, maxResults: Int = 1000,
                      maxReferenceBytes: Int = 1024 * 1024, range: CoreSearchRange? = nil,
                      withinLastResults: Bool = false, textOptions: CoreSearchTextOptions? = nil, returnCancelledResults: Bool = false,
                      onProgress: (@Sendable (CoreSearchProgress) async -> Void)? = nil) async throws -> RenderedSearchResult {
        let inputs = [module, expression] + (range.map { [$0.module, $0.expression] } ?? [])
        guard !inputs.contains(where: { $0.utf8.contains(0) }), maxResults > 0, maxReferenceBytes > 0,
              !(withinLastResults && range != nil) else {
            throw CoreError(status: XC_INVALID_ARGUMENT)
        }
        var request: OpaquePointer?
        let handle = try worker()
        let status = withSearchQuery(module: module, expression: expression, mode: mode, caseSensitive: caseSensitive,
            maxResults: maxResults, maxReferenceBytes: maxReferenceBytes, range: range,
            withinLastResults: withinLastResults, textOptions: textOptions) {
                xc_worker_search(handle, tab, $0, &request)
            }
        let completed = try await wait(status, request, cancellingSearch: true,
                                       returnCancelledSearch: returnCancelledResults, onSearchProgress: onProgress)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_search(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_release(result) }
        return CoreValues.search(result)
    }
    // Copies one common advanced query across ordered modules; no frontend
    // scope chaining, engine mode selection or search-option state lives here.
    func searchModules(tab: UInt64, modules: [String], expression: String, mode: CoreSearchMode = .phrase,
                       caseSensitive: Bool = false, maxResultsPerModule: Int = 1000,
                       maxReferenceBytesPerModule: Int = 1024 * 1024,
                       maxResults: Int = 10000, maxReferenceBytes: Int = 8 * 1024 * 1024,
                       range: CoreSearchRange? = nil, withinLastResults: Bool = false,
                       textOptions: CoreSearchTextOptions? = nil, returnCancelledResults: Bool = false,
                       onProgress: (@Sendable (CoreSearchProgress) async -> Void)? = nil) async throws -> RenderedSearchBatch {
        let inputs = modules + [expression] + (range.map { [$0.module, $0.expression] } ?? [])
        guard (1...32).contains(modules.count), !inputs.contains(where: { $0.utf8.contains(0) }),
              maxResultsPerModule > 0, maxReferenceBytesPerModule > 0, maxResults > 0, maxReferenceBytes > 0,
              !(withinLastResults && range != nil) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let handle = try worker()
        var total = xc_search_batch_limits(max_results: maxResults, max_reference_bytes: maxReferenceBytes)
        let (status, request) = withModuleStrings(modules) { ids in
            withSearchQuery(module: nil, expression: expression, mode: mode, caseSensitive: caseSensitive,
                maxResults: maxResultsPerModule, maxReferenceBytes: maxReferenceBytesPerModule,
                range: range, withinLastResults: withinLastResults, textOptions: textOptions) { query in
                    var request: OpaquePointer?
                    let status = xc_worker_search_modules(handle, tab, ids.baseAddress, ids.count, query, &total, &request)
                    return (status, request)
                }
        }
        let completed = try await wait(status, request, cancellingSearch: true,
            returnCancelledSearch: returnCancelledResults, onSearchProgress: onProgress)
        defer { xc_request_release(completed) }
        guard let batch = xc_request_take_search_batch(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_batch_release(batch) }
        return CoreValues.searchBatch(batch)
    }
    private func withModuleStrings<T>(_ values: [String], _ body: (UnsafeBufferPointer<UnsafePointer<CChar>?>) -> T) -> T {
        var pointers: [UnsafePointer<CChar>?] = []
        // Nested withCString scopes retain every buffer until synchronous copying
        // returns. This bounded recursion never crosses an await or escapes.
        func append(_ index: Int) -> T {
            if index == values.count { return pointers.withUnsafeBufferPointer(body) }
            return values[index].withCString { pointer in
                pointers.append(pointer); defer { pointers.removeLast() }
                return append(index + 1)
            }
        }
        return append(0)
    }
    private func withSearchQuery<T>(module: String?, expression: String, mode: CoreSearchMode, caseSensitive: Bool,
                                 maxResults: Int, maxReferenceBytes: Int, range: CoreSearchRange?,
                                 withinLastResults: Bool, textOptions: CoreSearchTextOptions?,
                                 _ body: (UnsafePointer<xc_search_query>) -> T) -> T {
        var options = xc_search_options()
        xc_search_options_default(&options)
        options.mode = mode.rawValue; options.case_sensitive = caseSensitive ? 1 : 0
        options.max_results = maxResults; options.max_reference_bytes = maxReferenceBytes
        var annotations = xc_search_text_options()
        xc_search_text_options_default(&annotations)
        if let textOptions {
            annotations.strongs = textOptions.strongs ? 1 : 0
            annotations.morphology = textOptions.morphology ? 1 : 0
            annotations.footnotes = textOptions.footnotes ? 1 : 0
        }
        func queue(_ id: UnsafePointer<CChar>?) -> T {
            expression.withCString { expression in
                func bounded(_ scopeModule: UnsafePointer<CChar>?, _ bounds: UnsafePointer<CChar>?) -> T {
                    var query = xc_search_query()
                    xc_search_query_default(&query)
                    query.module = id; query.expression = expression
                    query.scope = withinLastResults ? XC_SEARCH_SCOPE_LAST : range != nil ? XC_SEARCH_SCOPE_BOUNDS : XC_SEARCH_SCOPE_ALL
                    query.range_module = scopeModule; query.range_expression = bounds
                    return withUnsafePointer(to: &options) { base in
                        withUnsafePointer(to: &annotations) { flags in
                            query.options = base; query.text_options = textOptions == nil ? nil : flags
                            return withUnsafePointer(to: &query, body)
                        }
                    }
                }
                if let range {
                    return range.module.withCString { scope in
                        range.expression.withCString { bounds in bounded(scope, bounds) }
                    }
                }
                return bounded(nil, nil)
            }
        }
        if let module { return module.withCString(queue) }
        return queue(nil)
    }
    func exportSearchHTML(tab: UInt64, generation: UInt64, selection: CoreSearchExportSelection = .visible,
                          rows: [CoreSearchVisibleRow] = [], title: String = "Search results", includeScripture: Bool = false, includeSearchContext: Bool = false,
                          maxBytes: Int = 1024 * 1024, maxRows: Int = 10_000, allowPartialSource: Bool = false,
                          images: CoreHTMLImageOptions = CoreHTMLImageOptions(), plainTextMaxBytes: Int? = nil, returnCancelledResults: Bool = false,
                          onProgress: (@Sendable (CoreSearchProgress) async -> Void)? = nil) async throws -> RenderedSearchHTMLExport {
        guard (selection == .visible ? !rows.isEmpty : rows.isEmpty), rows.count <= 10_000,
              rows.allSatisfy({ $0.group >= 0 && $0.row >= 0 }), !title.utf8.contains(0), title.utf8.count <= 32768,
              maxBytes > 0, maxRows > 0, images.maxImages > 0, images.maxOriginalBytes > 0,
              images.maxPixels > 0 else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        guard plainTextMaxBytes == nil || (plainTextMaxBytes! > 0 && plainTextMaxBytes! <= 8 * 1024 * 1024) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let handle = try worker()
        let copied = rows.map { xc_search_visible_row(group: $0.group, row: $0.row) }
        let queued = title.withCString { label in copied.withUnsafeBufferPointer { buffer in
            var request: OpaquePointer?
            var input = xc_search_html_export_request(generation: generation, selection: selection.rawValue,
                rows: selection == .visible ? buffer.baseAddress : nil, row_count: buffer.count, title: label,
                include_scripture: includeScripture ? 1 : 0, max_bytes: maxBytes, max_rows: maxRows,
                allow_partial_source: allowPartialSource ? 1 : 0)
            var imageOptions = xc_search_html_image_options(policy: images.policy.rawValue, max_images: images.maxImages,
                max_original_bytes: images.maxOriginalBytes, max_pixels: images.maxPixels)
            let status = xc_worker_export_search_rich_html_with_context(handle, tab, &input, &imageOptions, plainTextMaxBytes ?? 0, includeSearchContext ? 1 : 0, &request)
            return (status, request)
        } }
        let completed = try await wait(queued.0, queued.1, cancellingSearch: true,
            returnCancelledSearch: returnCancelledResults, onSearchProgress: onProgress)
        defer { xc_request_release(completed) }
        guard let owned = xc_request_take_search_html_export(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_html_export_release(owned) }
        var info = xc_search_html_export_info(), generation = xc_search_generation_info(), imageInfo = xc_search_html_image_info()
        guard xc_search_html_export_get_info(owned, &info) == XC_OK,
              xc_search_html_export_get_images(owned, &imageInfo) == XC_OK,
              xc_search_html_export_get_generation(owned, &generation) == XC_OK,
              let status = CoreSearchExportStatus(rawValue: info.disposition),
              let selection = CoreSearchExportSelection(rawValue: info.selection),
              let issue = CoreHTMLExportIssue(rawValue: info.issue) else { throw CoreError(status: XC_ENGINE_ERROR) }
        let html = String(cString: xc_search_html_export_html(owned)!)
        precondition(html.utf8.count == xc_search_html_export_size(owned))
        let plainText = xc_search_html_export_plain_text(owned).map { String(cString: $0) }
        precondition((plainText?.utf8.count ?? 0) == xc_search_html_export_plain_text_size(owned))
        return RenderedSearchHTMLExport(selection: selection, status: status, generation: CoreValues.searchGeneration(generation),
            rowCount: info.row_count, failedRow: info.failed_row_present != 0 ? info.failed_row : nil,
            includesScripture: info.includes_scripture != 0, issue: issue,
            images: CoreHTMLImageStatistics(count: imageInfo.count, uniqueCount: imageInfo.unique_count,
                originalBytes: imageInfo.original_bytes, pixels: imageInfo.pixels), html: html, plainText: plainText)
    }
    func exportSearchVisibleText(tab: UInt64, generation: UInt64, rows: [CoreSearchVisibleRow],
                                 title: String = "Search results", includeScripture: Bool = false, includeSearchContext: Bool = false,
                                 maxBytes: Int = 1024 * 1024, returnCancelledResults: Bool = false,
                                 onProgress: (@Sendable (CoreSearchProgress) async -> Void)? = nil) async throws -> RenderedSearchTextExport {
        guard !rows.isEmpty, rows.count <= 10_000, rows.allSatisfy({ $0.group >= 0 && $0.row >= 0 }),
              !title.utf8.contains(0), title.utf8.count <= 32768, maxBytes > 0 else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let handle = try worker()
        let copiedRows = rows.map { xc_search_visible_row(group: $0.group, row: $0.row) }
        let queued = title.withCString { label in copiedRows.withUnsafeBufferPointer { buffer in
            var request: OpaquePointer?
            var input = xc_search_text_export_request(generation: generation, rows: buffer.baseAddress,
                row_count: buffer.count, title: label, include_scripture: includeScripture ? 1 : 0, max_bytes: maxBytes)
            let status = xc_worker_export_search_visible_text_with_context(handle, tab, &input, includeSearchContext ? 1 : 0, &request)
            return (status, request)
        } }
        return try await searchTextExportValue(queued.0, queued.1, returnCancelledResults: returnCancelledResults, onProgress: onProgress)
    }
    func exportSearchAllText(tab: UInt64, generation: UInt64, title: String = "Search results",
                             includeScripture: Bool = false, includeSearchContext: Bool = false, maxBytes: Int = 1024 * 1024, maxRows: Int = 10_000,
                             allowPartialSource: Bool = false, returnCancelledResults: Bool = false,
                             onProgress: (@Sendable (CoreSearchProgress) async -> Void)? = nil) async throws -> RenderedSearchTextExport {
        guard !title.utf8.contains(0), title.utf8.count <= 32768, maxBytes > 0, maxRows > 0 else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let handle = try worker()
        let queued = title.withCString { label in
            var request: OpaquePointer?
            var input = xc_search_all_text_export_request(generation: generation, title: label,
                include_scripture: includeScripture ? 1 : 0, max_bytes: maxBytes, max_rows: maxRows,
                allow_partial_source: allowPartialSource ? 1 : 0)
            let status = xc_worker_export_search_all_text_with_context(handle, tab, &input, includeSearchContext ? 1 : 0, &request)
            return (status, request)
        }
        return try await searchTextExportValue(queued.0, queued.1, returnCancelledResults: returnCancelledResults, onProgress: onProgress)
    }
    private func searchTextExportValue(_ status: xc_status, _ request: OpaquePointer?, returnCancelledResults: Bool,
                                       onProgress: (@Sendable (CoreSearchProgress) async -> Void)?) async throws -> RenderedSearchTextExport {
        let completed = try await wait(status, request, cancellingSearch: true,
            returnCancelledSearch: returnCancelledResults, onSearchProgress: onProgress)
        defer { xc_request_release(completed) }
        guard let owned = xc_request_take_search_text_export(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_text_export_release(owned) }
        var info = xc_search_text_export_info(), generation = xc_search_generation_info()
        guard xc_search_text_export_get_info(owned, &info) == XC_OK,
              xc_search_text_export_get_generation(owned, &generation) == XC_OK,
              let selection = CoreSearchExportSelection(rawValue: xc_search_text_export_selection(owned)),
              let disposition = CoreSearchExportStatus(rawValue: info.disposition) else { throw CoreError(status: XC_ENGINE_ERROR) }
        let text = String(cString: xc_search_text_export_text(owned)!)
        precondition(text.utf8.count == xc_search_text_export_size(owned))
        return RenderedSearchTextExport(selection: selection, status: disposition, generation: CoreValues.searchGeneration(generation),
            rowCount: info.row_count, failedRow: info.failed_row_present != 0 ? info.failed_row : nil,
            includesScripture: info.includes_scripture != 0, text: text)
    }
    func searchGeneration(tab: UInt64) async throws -> CoreSearchGeneration {
        var request: OpaquePointer?
        let status = xc_worker_search_generation(try worker(), tab, &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var info = xc_search_generation_info()
        let copied = xc_request_search_generation(completed, &info)
        guard copied == XC_OK else { throw CoreError(status: copied) }
        return CoreValues.searchGeneration(info)
    }
    func searchRetentionLimits(tab: UInt64) async throws -> CoreSearchRetentionLimits {
        var request: OpaquePointer?
        let status = xc_worker_search_retention_limits(try worker(), tab, &request)
        return try await searchRetentionValue(status, request)
    }
    func setSearchRetentionLimits(tab: UInt64, limits: CoreSearchRetentionLimits) async throws -> CoreSearchRetentionLimits {
        guard limits.maxElements > 0, limits.maxKeyBytes > 0 else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        var copied = xc_search_retention_limits(max_elements: limits.maxElements, max_key_bytes: limits.maxKeyBytes)
        let status = xc_worker_set_search_retention_limits(try worker(), tab, &copied, &request)
        return try await searchRetentionValue(status, request)
    }
    private func searchRetentionValue(_ status: xc_status, _ request: OpaquePointer?) async throws -> CoreSearchRetentionLimits {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var info = xc_search_retention_limits()
        let copied = xc_request_search_retention_limits(completed, &info)
        guard copied == XC_OK else { throw CoreError(status: copied) }
        return CoreSearchRetentionLimits(maxElements: info.max_elements, maxKeyBytes: info.max_key_bytes)
    }
    func readSearchGeneration(tab: UInt64, id: UInt64, group: Int, source: CoreSearchReferenceSource = .complete,
                              maxResults: Int = 1000, maxReferenceBytes: Int = 1024 * 1024) async throws -> RenderedSearchGenerationRead {
        guard group >= 0, maxResults > 0, maxReferenceBytes > 0 else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        var options = xc_search_generation_read_options(source: source.rawValue, max_results: maxResults, max_reference_bytes: maxReferenceBytes)
        let status = xc_worker_read_search_generation(try worker(), tab, id, group, &options, &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var info = xc_search_generation_read_info(), generation = xc_search_generation_info()
        guard xc_request_search_generation_read(completed, &info) == XC_OK,
              xc_request_search_generation(completed, &generation) == XC_OK,
              let disposition = CoreSearchReadStatus(rawValue: info.disposition),
              let failure = CoreSearchGroupStatus(rawValue: info.failure.rawValue) else { throw CoreError(status: XC_ENGINE_ERROR) }
        var metadata = xc_search_query_metadata()
        let query: CoreSearchRequestedQuery?
        if xc_request_search_generation_query(completed, &metadata) == XC_OK {
            guard let mode = CoreSearchMode(rawValue: metadata.mode) else { throw CoreError(status: XC_ENGINE_ERROR) }
            query = CoreSearchRequestedQuery(module: String(cString: metadata.module!), expression: String(cString: metadata.expression!),
                mode: mode, caseSensitive: metadata.case_sensitive != 0, withinLastResults: metadata.within_last_results != 0,
                range: metadata.range_module.map { CoreSearchRange(module: String(cString: $0), expression: String(cString: metadata.range_expression!)) },
                textOptions: CoreValues.searchTextOptions(metadata.text_options))
        } else { query = nil }
        let result: RenderedSearchResult?
        if info.result_present != 0 {
            guard let owned = xc_request_take_search(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
            defer { xc_search_release(owned) }
            result = CoreValues.search(owned)
        } else { result = nil }
        return RenderedSearchGenerationRead(status: disposition, generation: CoreValues.searchGeneration(generation),
            query: query, failure: failure, result: result)
    }
    func lastSearchScope(tab: UInt64) async throws -> CoreSearchScope {
        var request: OpaquePointer?
        let status = xc_worker_last_search_scope(try worker(), tab, &request)
        return try await searchScopeValue(status, request)
    }
    func clearSearchScope(tab: UInt64) async throws -> CoreSearchScope {
        var request: OpaquePointer?
        let status = xc_worker_clear_search_scope(try worker(), tab, &request)
        return try await searchScopeValue(status, request)
    }
    private func searchScopeValue(_ status: xc_status, _ request: OpaquePointer?) async throws -> CoreSearchScope {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var info = xc_search_scope_info()
        let copied = xc_request_search_scope(completed, &info)
        guard copied == XC_OK else { throw CoreError(status: copied) }
        return CoreSearchScope(present: info.present != 0, elementCount: info.element_count)
    }
    func searchTextOptions(tab: UInt64) async throws -> CoreSearchTextOptions? {
        var request: OpaquePointer?
        let status = xc_worker_search_text_options(try worker(), tab, &request)
        return try await searchTextStateValue(status, request)
    }
    func clearSearchTextOptions(tab: UInt64) async throws -> CoreSearchTextOptions? {
        var request: OpaquePointer?
        let status = xc_worker_clear_search_text_options(try worker(), tab, &request)
        return try await searchTextStateValue(status, request)
    }
    private func searchTextStateValue(_ status: xc_status, _ request: OpaquePointer?) async throws -> CoreSearchTextOptions? {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var info = xc_search_text_info()
        let copied = xc_request_search_text_options(completed, &info)
        guard copied == XC_OK else { throw CoreError(status: copied) }
        return CoreValues.searchTextOptions(info)
    }
    // The shared core owns source decoding, family/root policy and byte reads.
    // Accepted file reads drain; this does not promise interruption of file I/O.
    func readVerseInlineImage(module: String, source: String, maxBytes: Int = 16 * 1024 * 1024) async throws -> RenderedVerseImage {
        guard !module.isEmpty, module.utf8.count <= 4096, !module.utf8.contains(0),
              !source.isEmpty, source.utf8.count <= 32768, !source.utf8.contains(0),
              maxBytes > 0, maxBytes <= 16 * 1024 * 1024 else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let handle = try worker()
        let submitted: (xc_status, OpaquePointer?) = module.withCString { id in source.withCString { uri in
            var request: OpaquePointer?
            let status = xc_worker_read_verse_inline_image(handle, id, uri, maxBytes, &request)
            return (status, request)
        } }
        let completed = try await wait(submitted.0, submitted.1)
        defer { xc_request_release(completed) }
        guard let image = xc_request_take_verse_image(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_verse_image_release(image) }
        return CoreValues.verseImage(image)
    }
    func followBookLink(tab: UInt64, uri: String) async throws -> RenderedBookLink {
        guard !uri.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = uri.withCString { xc_worker_follow_book_link(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        if let note = xc_request_take_book_note(completed) {
            defer { xc_book_note_release(note) }
            return .note(RenderedBookNote(tree: CoreValues.bookTree(xc_book_note_tree(note)!),
                content: noteValue(xc_book_note_content(note)!)))
        }
        guard let image = xc_request_take_book_image_data(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_book_image_data_release(image) }
        return .image(CoreValues.bookImage(image))
    }
    func followBookPaneLink(tab: UInt64, mainModule: String, mainReference: String, uri: String) async throws -> RenderedBookPaneLink {
        try admitStudyLink(module: mainModule, reference: mainReference, uri: uri)
        var request: OpaquePointer?
        let handle = try worker()
        let status = mainModule.withCString { module in mainReference.withCString { reference in uri.withCString {
            xc_worker_follow_book_pane_link_at(handle, tab, module, reference, $0, &request)
        } } }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        if let note = xc_request_take_book_note(completed) {
            defer { xc_book_note_release(note) }
            return .book(.note(RenderedBookNote(tree: CoreValues.bookTree(xc_book_note_tree(note)!), content: noteValue(xc_book_note_content(note)!))))
        }
        if let image = xc_request_take_book_image_data(completed) {
            defer { xc_book_image_data_release(image) }
            return .book(.image(CoreValues.bookImage(image)))
        }
        return .study(try contextualLinkValue(completed))
    }
    func bookChildren(module: String, parent: UInt64, after: UInt64? = nil, limit: UInt32 = 64) async throws -> BookChildren {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_book_children(handle, $0, parent, after == nil ? 0 : 1, after ?? 0, limit, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_book_children(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_book_children_release(result) }
        var parent = xc_book_node(), next: UInt64 = 0
        precondition(xc_book_children_parent(result, &parent) == XC_OK)
        let nodes = (0..<xc_book_children_count(result)).map { index in
            var value = xc_book_node()
            precondition(xc_book_children_get_node(result, index, &value) == XC_OK)
            return CoreValues.bookNode(value)!
        }
        let more = xc_book_children_next_after(result, &next) != 0
        return BookChildren(module: String(cString: xc_book_children_module(result)!),
            parent: CoreValues.bookNode(parent)!, nodes: nodes, nextAfter: more ? next : nil)
    }
    func createTab() async throws -> UInt64 {
        var request: OpaquePointer?
        let status = xc_worker_create_tab(try worker(), &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        let tab = xc_request_tab(completed)
        guard tab != 0 else { throw CoreError(status: XC_ENGINE_ERROR) }
        return tab
    }
    func closeTab(_ tab: UInt64) async throws {
        var request: OpaquePointer?
        let status = xc_worker_close_tab(try worker(), tab, &request)
        xc_request_release(try await wait(status, request))
    }
    func setLexicalLayoutEnabled(tab: UInt64, enabled: Bool) async throws {
        var request: OpaquePointer?
        let status = xc_worker_set_lexical_layout_enabled(try worker(), tab, enabled ? 1 : 0, &request)
        xc_request_release(try await wait(status, request))
    }
    func setHeadingNoteLinksEnabled(tab: UInt64, enabled: Bool) async throws {
        var request: OpaquePointer?
        let status = xc_worker_set_heading_note_links_enabled(try worker(), tab, enabled ? 1 : 0, &request)
        xc_request_release(try await wait(status, request))
    }
    func setNoteNumberingEnabled(tab: UInt64, enabled: Bool) async throws {
        var request: OpaquePointer?
        let status = xc_worker_set_note_numbering_enabled(try worker(), tab, enabled ? 1 : 0, &request)
        xc_request_release(try await wait(status, request))
    }
    func clearHistory(_ tab: UInt64) async throws {
        var request: OpaquePointer?
        let status = xc_worker_clear_history(try worker(), tab, &request)
        xc_request_release(try await wait(status, request))
    }
    func setEngineOptions(tab: UInt64, options: EngineOptionFlags) async throws {
        var request: OpaquePointer?
        let status = xc_worker_set_engine_options(try worker(), tab, options.rawValue, &request)
        xc_request_release(try await wait(status, request))
    }
    func saveModulePreferences(tab: UInt64, path: String) async throws {
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_save_module_preferences(handle, tab, $0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func importModulePreferences(tab: UInt64, path: String) async throws {
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_import_module_preferences(handle, tab, $0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func enableModulePreferences(tab: UInt64) async throws {
        var request: OpaquePointer?
        let status = xc_worker_enable_module_preferences(try worker(), tab, &request)
        xc_request_release(try await wait(status, request))
    }
    func selectModuleVariant(tab: UInt64, module: String, reading: VariantReading) async throws {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_select_module_variant(handle, tab, $0, reading.rawValue, &request) }
        xc_request_release(try await wait(status, request))
    }
    func setModuleEngineOptions(tab: UInt64, module: String, options: EngineOptionFlags) async throws {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_set_module_engine_options(handle, tab, $0, options.rawValue, &request) }
        xc_request_release(try await wait(status, request))
    }
    func clearModuleEngineOptions(tab: UInt64, module: String) async throws {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_clear_module_engine_options(handle, tab, $0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func moduleEngineOptions(tab: UInt64, module: String) async throws -> EngineOptionFlags? {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_module_engine_options(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var flags: UInt32 = 0
        var configured: Int32 = 0
        let copied = xc_request_engine_options(completed, &flags, &configured)
        guard copied == XC_OK else { throw CoreError(status: copied) }
        return configured != 0 ? EngineOptionFlags(rawValue: flags) : nil
    }
    func clearEngineOptions(tab: UInt64) async throws {
        var request: OpaquePointer?
        let status = xc_worker_clear_engine_options(try worker(), tab, &request)
        xc_request_release(try await wait(status, request))
    }
    func enableLexiconPreferences(tab: UInt64) async throws {
        var request: OpaquePointer?
        let status = xc_worker_enable_lexicon_preferences(try worker(), tab, &request)
        xc_request_release(try await wait(status, request))
    }
    func setLexiconPreferences(tab: UInt64, preferences: LexiconPreferences) async throws {
        let values = [preferences.greek, preferences.hebrew, preferences.greekMorphNT, preferences.greekMorphOT, preferences.hebrewMorph]
        guard !values.compactMap({ $0 }).contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = CoreValues.withLexiconNames(preferences) { xc_worker_set_lexicon_preferences(handle, tab, $0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func lexiconPreferences(tab: UInt64) async throws -> LexiconPreferences? {
        var request: OpaquePointer?
        let status = xc_worker_lexicon_preferences(try worker(), tab, &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_lexicons(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_lexicons_release(result) }
        return CoreValues.lexicons(result)
    }
    func importLexiconProfile(tab: UInt64, path: String) async throws -> LexiconPreferences {
        guard !path.isEmpty, !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_import_lexicon_profile(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_lexicons(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_lexicons_release(result) }
        guard let copied = CoreValues.lexicons(result) else { throw CoreError(status: XC_ENGINE_ERROR) }
        return copied
    }
    func readSearchLists(tab: UInt64, path: String) async throws -> SavedSearchLists {
        guard !path.isEmpty, !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_read_search_lists(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_search_lists(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_lists_release(result) }
        return try CoreValues.searchLists(result)
    }
    func saveSearchList(tab: UInt64, path: String, edit: SavedSearchListEdit) async throws -> SavedSearchLists {
        let fields = [path, edit.label, edit.value]
        guard !path.isEmpty, fields.allSatisfy({ !$0.utf8.contains(0) && $0.utf8.count <= 16 * 1024 * 1024 }) else {
            throw CoreError(status: XC_INVALID_ARGUMENT)
        }
        let handle = try worker()
        let queued = path.withCString { file in edit.label.withCString { label in edit.value.withCString { value in
            var request: OpaquePointer?
            var input = xc_saved_search_list_edit(kind: edit.kind.rawValue, label: label, value: value)
            let status = xc_worker_save_search_list(handle, tab, file, &input, &request)
            return (status, request)
        } } }
        // Saves are accepted FIFO mutations. Observer cancellation does not undo
        // a write; wait drains the accepted operation and returns its staged value.
        let completed = try await wait(queued.0, queued.1)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_search_lists(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_lists_release(result) }
        return try CoreValues.searchLists(result)
    }
    func removeSearchList(tab: UInt64, path: String, kind: SavedSearchListKind, label: String) async throws -> SavedSearchLists {
        guard !path.isEmpty, [path, label].allSatisfy({ !$0.utf8.contains(0) && $0.utf8.count <= 16 * 1024 * 1024 }) else {
            throw CoreError(status: XC_INVALID_ARGUMENT)
        }
        let handle = try worker()
        let queued = path.withCString { file in label.withCString { name in
            var request: OpaquePointer?
            let status = xc_worker_remove_search_list(handle, tab, file, kind.rawValue, name, &request)
            return (status, request)
        } }
        // Accepted removals drain even if the observer is cancelled.
        let completed = try await wait(queued.0, queued.1)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_search_lists(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_lists_release(result) }
        return try CoreValues.searchLists(result)
    }
    func parseSearchModuleList(tab: UInt64, text: String) async throws -> [String] {
        guard !text.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = text.withCString { xc_worker_parse_search_module_list(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_search_module_names(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_search_module_names_release(result) }
        return (0..<xc_search_module_names_count(result)).map { String(cString: xc_search_module_names_at(result, $0)!) }
    }
    func saveLexiconProfile(tab: UInt64, path: String) async throws {
        guard !path.isEmpty, !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_save_lexicon_profile(handle, tab, $0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func readLexical(tab: UInt64, module: String, reference: String, uri: String, origin: LexiconOrigin = .main) async throws -> RenderedLexicalEntry {
        try await lexicalLookup(tab: tab, module: module, reference: reference, uri: uri, origin: origin, record: false)
    }
    func visitLexical(tab: UInt64, module: String, reference: String, uri: String, origin: LexiconOrigin = .main) async throws -> RenderedLexicalEntry {
        try await lexicalLookup(tab: tab, module: module, reference: reference, uri: uri, origin: origin, record: true)
    }
    private func lexicalLookup(tab: UInt64, module: String, reference: String, uri: String, origin: LexiconOrigin, record: Bool) async throws -> RenderedLexicalEntry {
        guard ![module, reference, uri].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in reference.withCString { context in
            uri.withCString { value in
                record ? xc_worker_visit_lexical(handle, tab, id, context, value, origin.rawValue, &request)
                       : xc_worker_read_lexical(handle, tab, id, context, value, origin.rawValue, &request)
            }
        } }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_lexical_entry(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_lexical_entry_release(result) }
        return CoreValues.lexicalEntry(result)
    }
    func navigateDictionary(tab: UInt64, module: String, key: String, direction: Int32) async throws -> RenderedDictionaryEntry {
        guard (direction == -1 || direction == 1), ![module, key].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in key.withCString { xc_worker_navigate_dictionary(handle, tab, id, $0, direction, &request) } }
        return try await dictionaryValue(status, request)
    }
    func readDictionary(tab: UInt64, module: String, key: String) async throws -> RenderedDictionaryEntry {
        guard ![module, key].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in key.withCString { xc_worker_read_dictionary(handle, tab, id, $0, &request) } }
        return try await dictionaryValue(status, request)
    }
    private func dictionaryValue(_ status: xc_status, _ request: OpaquePointer?) async throws -> RenderedDictionaryEntry {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_dictionary_entry(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_dictionary_entry_release(result) }
        return CoreValues.dictionaryEntry(result)
    }
    func visitDictionary(tab: UInt64, module: String, key: String) async throws -> RenderedDictionaryEntry {
        guard ![module, key].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in key.withCString { xc_worker_visit_dictionary(handle, tab, id, $0, &request) } }
        return try await dictionaryValue(status, request)
    }
    func navigateDictionaryHistory(tab: UInt64, direction: Int32) async throws -> RenderedDictionaryEntry {
        guard direction == -1 || direction == 1 else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let status = xc_worker_navigate_dictionary_history(try worker(), tab, direction, &request)
        return try await dictionaryValue(status, request)
    }
    func clearDictionaryHistory(tab: UInt64) async throws {
        var request: OpaquePointer?
        let status = xc_worker_clear_dictionary_history(try worker(), tab, &request)
        xc_request_release(try await wait(status, request))
    }
    func dictionaryChoices(tab: UInt64, module: String, key: String) async throws -> DictionaryChoices {
        guard ![module, key].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in key.withCString { xc_worker_dictionary_choices(handle, tab, id, $0, &request) } }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_dictionary_choices(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_dictionary_choices_release(result) }
        return DictionaryChoices(module: String(cString: xc_dictionary_choices_module(result)!),
            key: String(cString: xc_dictionary_choices_key(result)!),
            keys: (0..<xc_dictionary_choices_count(result)).map { String(cString: xc_dictionary_choices_at(result, $0)!) })
    }
    func dictionaryHistory(tab: UInt64) async throws -> CoreDictionaryHistoryState {
        var request: OpaquePointer?
        let status = xc_worker_dictionary_history(try worker(), tab, &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_dictionary_history(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_dictionary_history_release(result) }
        let entries = (0..<xc_dictionary_history_count(result)).map { index in
            DictionaryLocation(module: String(cString: xc_dictionary_history_module(result, index)!),
                               key: String(cString: xc_dictionary_history_key(result, index)!))
        }
        return CoreDictionaryHistoryState(entries: entries, cursor: xc_dictionary_history_cursor(result),
            canBack: xc_dictionary_history_can_back(result) != 0, canForward: xc_dictionary_history_can_forward(result) != 0)
    }
    func readReferenceList(tab: UInt64, module: String, expression: String, context: String) async throws -> RenderedReferenceList {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            expression.withCString { input in
                context.withCString { xc_worker_read_reference_list(handle, tab, id, input, $0, &request) }
            }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_reference_list(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_reference_list_release(result) }
        return CoreValues.referenceList(result)
    }
    func resolveStudyLinkAt(tab: UInt64, currentModule: String, currentReference: String, uri: String) async throws -> ContextualStudyLink {
        try await contextualStudyLinkAt(tab: tab, currentModule: currentModule, currentReference: currentReference, uri: uri, record: false)
    }
    func crossReferencePreviewList(tab: UInt64) async throws -> Bool {
        var request: OpaquePointer?
        let status = xc_worker_cross_reference_preview_list(try worker(), tab, &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var enabled: Int32 = 0
        let copied = xc_request_cross_reference_preview_list(completed, &enabled)
        guard copied == XC_OK else { throw CoreError(status: copied) }
        return enabled != 0
    }
    func setCrossReferencePreviewList(tab: UInt64, enabled: Bool) async throws {
        var request: OpaquePointer?
        let status = xc_worker_set_cross_reference_preview_list(try worker(), tab, enabled ? 1 : 0, &request)
        xc_request_release(try await wait(status, request))
    }
    private func admitStudyLink(module: String, reference: String = "", uri: String, navigation: String? = nil) throws {
        let contextLimit = Int(XC_STUDY_LINK_MAX_CONTEXT_BYTES)
        guard module.utf8.count <= contextLimit, reference.utf8.count <= contextLimit,
              uri.utf8.count <= Int(XC_STUDY_LINK_MAX_URI_BYTES),
              navigation.map({ $0.utf8.count <= contextLimit }) ?? true,
              ![module, reference, uri].contains(where: { $0.utf8.contains(0) }),
              navigation?.utf8.contains(0) != true else { throw CoreError(status: XC_INVALID_ARGUMENT) }
    }
    func hoverStudyLinkAt(tab: UInt64, currentModule: String, currentReference: String, uri: String) async throws -> HoveredStudyLink {
        try admitStudyLink(module: currentModule, reference: currentReference, uri: uri)
        var request: OpaquePointer?
        let handle = try worker()
        let status = currentModule.withCString { module in
            currentReference.withCString { reference in
                uri.withCString { xc_worker_hover_study_link_at(handle, tab, module, reference, $0, &request) }
            }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        if xc_request_no_presentation(completed) != 0 { return .noPresentation }
        if let result = xc_request_take_dictionary_entry(completed) {
            defer { xc_dictionary_entry_release(result) }
            return .dictionary(CoreValues.dictionaryEntry(result))
        }
        if let result = xc_request_take_reference_list(completed) {
            defer { xc_reference_list_release(result) }
            return .references(CoreValues.referenceList(result))
        }
        if let result = xc_request_take_note(completed) {
            defer { xc_note_release(result) }
            return .note(noteValue(result))
        }
        guard let result = xc_request_take_lexical_entry(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_lexical_entry_release(result) }
        return .lexical(CoreValues.lexicalEntry(result))
    }
    func followStudyLinkAt(tab: UInt64, currentModule: String, currentReference: String, uri: String, navigationModule: String? = nil) async throws -> ContextualStudyLink {
        try await contextualStudyLinkAt(tab: tab, currentModule: currentModule, currentReference: currentReference, uri: uri, record: true, navigationModule: navigationModule)
    }
    private func contextualStudyLinkAt(tab: UInt64, currentModule: String, currentReference: String, uri: String, record: Bool, navigationModule: String? = nil) async throws -> ContextualStudyLink {
        try admitStudyLink(module: currentModule, reference: currentReference, uri: uri, navigation: navigationModule)
        var request: OpaquePointer?
        let handle = try worker()
        let status = currentModule.withCString { id in
            currentReference.withCString { context in
                uri.withCString { value in
                    if let navigationModule {
                        return navigationModule.withCString { navigation in
                            xc_worker_follow_pane_link_at(handle, tab, navigation, id, context, value, &request)
                        }
                    }
                    return record ? xc_worker_follow_study_link_at(handle, tab, id, context, value, &request)
                                  : xc_worker_resolve_study_link_at(handle, tab, id, context, value, &request)
                }
            }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        return try contextualLinkValue(completed)
    }
    private func contextualLinkValue(_ completed: OpaquePointer) throws -> ContextualStudyLink {
        if let result = xc_request_take_book_uri_target(completed) {
            defer { xc_book_uri_target_release(result) }
            return .book(CoreValues.bookUriTarget(result))
        }
        if let result = xc_request_take_dictionary_entry(completed) {
            defer { xc_dictionary_entry_release(result) }
            return .dictionary(CoreValues.dictionaryEntry(result))
        }
        if let result = xc_request_take_reference_list(completed) {
            defer { xc_reference_list_release(result) }
            return .references(CoreValues.referenceList(result))
        }
        if let result = xc_request_take_note(completed) {
            defer { xc_note_release(result) }
            return .note(noteValue(result))
        }
        if let result = xc_request_take_lexical_entry(completed) {
            defer { xc_lexical_entry_release(result) }
            return .lexical(CoreValues.lexicalEntry(result))
        }
        if let result = xc_request_take_verse_image(completed) {
            defer { xc_verse_image_release(result) }
            return .image(CoreValues.verseImage(result))
        }
        guard let result = xc_request_take_link(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_link_release(result) }
        return .passage(PassageTarget(module: String(cString: xc_link_module(result)!),
            reference: String(cString: xc_link_reference(result)!), anchor: xc_link_anchor(result).map { String(cString: $0) }))
    }
    func resolveStudyLink(tab: UInt64, currentModule: String, uri: String) async throws -> ResolvedStudyLink {
        try admitStudyLink(module: currentModule, uri: uri)
        var request: OpaquePointer?
        let handle = try worker()
        let status = currentModule.withCString { id in
            uri.withCString { xc_worker_resolve_study_link(handle, tab, id, $0, &request) }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        if let result = xc_request_take_note(completed) {
            defer { xc_note_release(result) }
            return .note(noteValue(result))
        }
        guard let result = xc_request_take_link(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_link_release(result) }
        return .passage(PassageTarget(module: String(cString: xc_link_module(result)!),
            reference: String(cString: xc_link_reference(result)!), anchor: xc_link_anchor(result).map { String(cString: $0) }))
    }
    func studyNoteLink(tab: UInt64, currentModule: String, uri: String) async throws -> RenderedStudyNote {
        try admitStudyLink(module: currentModule, uri: uri)
        var request: OpaquePointer?
        let handle = try worker()
        let status = currentModule.withCString { id in
            uri.withCString { xc_worker_study_note_link(handle, tab, id, $0, &request) }
        }
        return try await note(status, request)
    }
    func studyNote(tab: UInt64, module: String, reference: String, identifier: String) async throws -> RenderedStudyNote {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            reference.withCString { ref in
                identifier.withCString { xc_worker_study_note(handle, tab, id, ref, $0, &request) }
            }
        }
        return try await note(status, request)
    }
    private func note(_ status: xc_status, _ request: OpaquePointer?) async throws -> RenderedStudyNote {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_note(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_note_release(result) }
        return noteValue(result)
    }
    private func noteValue(_ result: OpaquePointer) -> RenderedStudyNote {
        RenderedStudyNote(module: String(cString: xc_note_module(result)!), reference: String(cString: xc_note_reference(result)!),
            identifier: String(cString: xc_note_identifier(result)!), type: String(cString: xc_note_type(result)!),
            label: String(cString: xc_note_label(result)!), html: String(cString: xc_note_html(result)!),
            referenceList: String(cString: xc_note_reference_list(result)!))
    }
    func readTabEntry(tab: UInt64, module: String, reference: String) async throws -> RenderedEntry {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            reference.withCString { xc_worker_read_tab_entry(handle, tab, id, $0, &request) }
        }
        return try await entry(status, request)
    }
    private func chapter(_ status: xc_status, _ request: OpaquePointer?) async throws -> RenderedChapter {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_chapter(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_chapter_release(result) }
        return CoreValues.chapter(result)
    }
    func contentDisplayOptions(tab: UInt64, module: String) async throws -> ContentDisplayOptions {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_module_presentation_options(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        var images: Int32 = -1, fonts: Int32 = -1
        let result = xc_request_content_options(completed, &images, &fonts)
        guard result == XC_OK else { throw CoreError(status: result) }
        guard (-1...1).contains(images), (-1...1).contains(fonts) else { throw CoreError(status: XC_ENGINE_ERROR) }
        let spacing = xc_request_double_space(completed)
        guard (0...1).contains(spacing) else { throw CoreError(status: XC_ENGINE_ERROR) }
        var numbers: Int32 = 0, available: Int32 = 0
        let noteStatus = xc_request_note_numbers(completed, &numbers, &available)
        guard noteStatus == XC_OK else { throw CoreError(status: noteStatus) }
        guard (0...1).contains(numbers), (0...1).contains(available) else { throw CoreError(status: XC_ENGINE_ERROR) }
        return ContentDisplayOptions(images: images == -1 ? nil : images != 0, fontFaces: fonts == -1 ? nil : fonts != 0, doubleSpace: spacing != 0, noteNumbers: available == 0 ? nil : numbers != 0)
    }
    func setNoteNumbers(tab: UInt64, module: String, enabled: Bool) async throws {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_set_note_numbers(handle, tab, $0, enabled ? 1 : 0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func setDoubleSpace(tab: UInt64, module: String, enabled: Bool) async throws {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_set_double_space(handle, tab, $0, enabled ? 1 : 0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func setImageContent(tab: UInt64, module: String, enabled: Bool) async throws {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_set_image_content(handle, tab, $0, enabled ? 1 : 0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func setRespectFontFaces(tab: UInt64, module: String, enabled: Bool) async throws {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_set_respect_font_faces(handle, tab, $0, enabled ? 1 : 0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func commentaryChapterMode(tab: UInt64, module: String) async throws -> Bool {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_module_presentation_options(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        let mode = xc_request_commentary_chapter_mode(completed)
        guard mode >= 0 else { throw CoreError(status: XC_ENGINE_ERROR) }
        return mode != 0
    }
    func setCommentaryChapterMode(tab: UInt64, module: String, enabled: Bool) async throws {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_set_commentary_chapter_mode(handle, tab, $0, enabled ? 1 : 0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func headingDisplay(tab: UInt64, module: String) async throws -> Bool {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_module_presentation_options(handle, tab, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        let mode = xc_request_headings(completed)
        guard mode >= 0 else { throw CoreError(status: XC_ENGINE_ERROR) }
        return mode != 0
    }
    func setHeadings(tab: UInt64, module: String, enabled: Bool) async throws {
        guard !module.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { xc_worker_set_headings(handle, tab, $0, enabled ? 1 : 0, &request) }
        xc_request_release(try await wait(status, request))
    }
    func readTabCommentaryChapter(tab: UInt64, module: String, reference: String, verseNumbers: Bool, highlightSelected: Bool) async throws -> RenderedChapter {
        guard ![module, reference].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let flags = (verseNumbers ? UInt32(XC_CHAPTER_VERSE_NUMBERS) : 0) |
                    (highlightSelected ? UInt32(XC_CHAPTER_HIGHLIGHT_SELECTED) : 0)
        let status = module.withCString { id in
            reference.withCString { xc_worker_read_tab_commentary_chapter(handle, tab, id, $0, flags, &request) }
        }
        return try await chapter(status, request)
    }
    func readTabChapter(tab: UInt64, module: String, reference: String, verseNumbers: Bool, highlightSelected: Bool) async throws -> RenderedChapter {
        var request: OpaquePointer?
        let handle = try worker()
        let flags = (verseNumbers ? UInt32(XC_CHAPTER_VERSE_NUMBERS) : 0) |
                    (highlightSelected ? UInt32(XC_CHAPTER_HIGHLIGHT_SELECTED) : 0)
        let status = module.withCString { id in
            reference.withCString { xc_worker_read_tab_chapter(handle, tab, id, $0, flags, &request) }
        }
        return try await chapter(status, request)
    }
    func history(_ tab: UInt64) async throws -> CoreHistoryState {
        var request: OpaquePointer?
        let status = xc_worker_history(try worker(), tab, &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_history(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_history_release(result) }
        return CoreHistoryState(canBack: xc_history_can_back(result) != 0,
            canForward: xc_history_can_forward(result) != 0, count: xc_history_count(result),
            module: xc_history_module(result).map { String(cString: $0) },
            commentary: xc_history_commentary(result).map { String(cString: $0) })
    }
    private func entry(_ status: xc_status, _ request: OpaquePointer?) async throws -> RenderedEntry {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_entry(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_entry_release(result) }
        return CoreValues.entry(result)
    }
    private func passage(_ status: xc_status, _ request: OpaquePointer?) async throws -> RenderedStudyPassage {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_passage(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_passage_release(result) }
        let commentary = xc_passage_has_commentary(result) != 0 ? RenderedEntry(
            reference: String(cString: xc_passage_commentary_reference(result)!),
            html: String(cString: xc_passage_commentary_html(result)!),
            headingHTML: String(cString: xc_passage_commentary_heading_html(result)!),
            cleanupDiagnostics: CoreValues.cleanupDiagnostics(count: xc_passage_commentary_cleanup_count(result),
                kind: { xc_passage_commentary_cleanup_kind(result, $0) }, suffix: { xc_passage_commentary_cleanup_suffix(result, $0) }),
            lexical: CoreValues.lexicalLayout { xc_passage_commentary_lexical_layout(result, $0) }) : nil
        return RenderedStudyPassage(module: String(cString: xc_passage_module(result)!),
            commentaryModule: String(cString: xc_passage_commentary_module(result)!),
            text: RenderedEntry(reference: String(cString: xc_passage_reference(result)!),
                                html: String(cString: xc_passage_html(result)!),
                                cleanupDiagnostics: CoreValues.cleanupDiagnostics(count: xc_passage_cleanup_count(result),
                                    kind: { xc_passage_cleanup_kind(result, $0) }, suffix: { xc_passage_cleanup_suffix(result, $0) }),
                                lexical: CoreValues.lexicalLayout { xc_passage_lexical_layout(result, $0) }), commentary: commentary,
            anchor: xc_passage_anchor(result).map { RenderedPassageAnchor(module: String(cString: xc_passage_anchor_module(result)!), text: String(cString: $0)) })
    }
    func studyPassage(tab: UInt64, module: String, commentary: String, reference: String, record: Bool = true) async throws -> RenderedStudyPassage {
        guard ![module, commentary, reference].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in commentary.withCString { notes in reference.withCString { ref in
            record ? xc_worker_visit_passage(handle, tab, id, notes, ref, &request)
                   : xc_worker_read_passage(handle, tab, id, notes, ref, &request)
        } } }
        return try await passage(status, request)
    }
    func visitLinkedStudyPassage(tab: UInt64, bible: String, commentary: String, target: PassageTarget) async throws -> RenderedStudyPassage {
        guard [bible, commentary, target.module].allSatisfy({ $0.utf8.count <= Int(XC_STUDY_LINK_MAX_CONTEXT_BYTES) && !$0.utf8.contains(0) }),
              target.reference.utf8.count <= Int(XC_STUDY_LINK_MAX_URI_BYTES), !target.reference.utf8.contains(0),
              target.anchor.map({ $0.utf8.count <= Int(XC_STUDY_LINK_MAX_URI_BYTES) && !$0.utf8.contains(0) }) != false else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = bible.withCString { main in commentary.withCString { notes in target.module.withCString { module in target.reference.withCString { reference in
            CoreValues.withOptionalCString(target.anchor) { anchor in
                xc_worker_visit_linked_passage_with_anchor(handle, tab, main, notes, module, reference, anchor, &request)
            }
        } } } }
        return try await passage(status, request)
    }
    func navigateStudyPassage(tab: UInt64, direction: Int32) async throws -> RenderedStudyPassage {
        var request: OpaquePointer?
        let status = xc_worker_navigate_passage(try worker(), tab, direction, &request)
        return try await passage(status, request)
    }
    func navigateStudyChapter(tab: UInt64, module: String, commentary: String, reference: String, direction: Int32) async throws -> RenderedStudyPassage {
        guard ![module, commentary, reference].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in commentary.withCString { notes in reference.withCString { ref in
            xc_worker_navigate_passage_chapter(handle, tab, id, notes, ref, direction, &request)
        } } }
        return try await passage(status, request)
    }
    func visit(tab: UInt64, module: String, reference: String) async throws -> RenderedEntry {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            reference.withCString { xc_worker_visit(handle, tab, id, $0, &request) }
        }
        return try await entry(status, request)
    }
    func navigate(tab: UInt64, direction: Int32) async throws -> RenderedEntry {
        var request: OpaquePointer?
        let status = xc_worker_navigate(try worker(), tab, direction, &request)
        return try await entry(status, request)
    }
    func navigateChapter(tab: UInt64, module: String, reference: String, direction: Int32) async throws -> RenderedEntry {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            reference.withCString { xc_worker_navigate_chapter(handle, tab, id, $0, direction, &request) }
        }
        return try await entry(status, request)
    }
    func readChapter(module: String, reference: String, verseNumbers: Bool, highlightSelected: Bool) async throws -> RenderedChapter {
        var request: OpaquePointer?
        let handle = try worker()
        let flags = (verseNumbers ? UInt32(XC_CHAPTER_VERSE_NUMBERS) : 0) |
                    (highlightSelected ? UInt32(XC_CHAPTER_HIGHLIGHT_SELECTED) : 0)
        let status = module.withCString { id in
            reference.withCString { xc_worker_read_chapter(handle, id, $0, flags, &request) }
        }
        return try await chapter(status, request)
    }
    func referenceChoices(module: String, reference: String) async throws -> ReferenceChoices {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            reference.withCString { xc_worker_reference_choices(handle, id, $0, &request) }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_choices(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_choices_release(result) }
        return CoreValues.choices(result)
    }
    func resolveLink(_ uri: String, currentModule: String) async throws -> PassageTarget {
        try admitStudyLink(module: currentModule, uri: uri)
        var request: OpaquePointer?
        let handle = try worker()
        let status = currentModule.withCString { module in
            uri.withCString { xc_worker_resolve_link(handle, module, $0, &request) }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_link(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_link_release(result) }
        return CoreValues.link(result)
    }
}
