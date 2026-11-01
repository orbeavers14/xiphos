// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore
import Darwin

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
private final class SessionCStrings {
    private var owned: [UnsafeMutablePointer<CChar>] = []
    func copy(_ value: String) throws -> UnsafePointer<CChar> {
        guard !value.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        guard let pointer = strdup(value) else { throw CoreError(status: XC_ENGINE_ERROR) }
        owned.append(pointer); return UnsafePointer(pointer)
    }
    deinit { for pointer in owned { free(pointer) } }
}
struct CoreWindowCapture: Sendable {
    var tab: UInt64
    var key: String
    var hostState: String = ""
    var texts = true, preview = true, commentaries = true, dictionaries = true
    var commentaryShowing = false, parallelShowing = false
}
struct CoreSavedWindow: Sendable {
    let key: String
    let module: String?, reference: String?, commentary: String?
    let dictionaryModule: String?, dictionaryKey: String?
    let bookDocument: Bool
    let hostState: String
}
struct CoreNativeSessionFile: Sendable {
    let xml: String
    let windows: [CoreSavedWindow]
    let activeKey: String?
}
struct CoreRestoredWindow: Sendable {
    let key: String
    let tab: UInt64
    var capture: CoreWindowCapture
}
struct CoreRestoredSession: Sendable {
    let windows: [CoreRestoredWindow]
    let activeTab: UInt64?
}
struct CoreRecoveryDraft: Sendable, Equatable {
    var id: String, title: String
    var target: Int32 = 0
    var path = "", module = "", reference = ""
    var originRevision: String?
    var savedText: String, text: String
    var selectionStart = 0, selectionEnd = 0
}
struct CoreRecoveryFile: Sendable {
    let session: CoreNativeSessionFile
    let drafts: [CoreRecoveryDraft]
    let revision: String
    let previous: Bool
}
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
    func readNativeSession(path: String) async throws -> CoreNativeSessionFile {
        guard !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = path.withCString { xc_worker_read_native_session(handle, $0, &request) }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_native_session(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_native_session_release(result) }
        return try nativeSessionValue(result)
    }
    private func nativeSessionValue(_ result: OpaquePointer) throws -> CoreNativeSessionFile {
        var text: OpaquePointer?
        guard xc_native_session_serialize(result, &text) == XC_OK, let text else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_native_session_text_release(text) }
        let windows = (0..<xc_native_session_tab_count(result)).map { row in
            func field(_ kind: xc_native_session_field_kind) -> String? {
                xc_native_session_field(result, row, Int32(kind.rawValue)).map { String(cString: $0) }
            }
            return CoreSavedWindow(key: String(cString: xc_native_session_tab_key(result, row)!),
                module: field(XC_SESSION_READING_MODULE), reference: field(XC_SESSION_READING_REFERENCE),
                commentary: field(XC_SESSION_READING_COMMENTARY), dictionaryModule: field(XC_SESSION_DICTIONARY_MODULE),
                dictionaryKey: field(XC_SESSION_DICTIONARY_KEY), bookDocument: xc_native_session_book_document(result, row) != 0,
                hostState: field(XC_SESSION_HOST_STATE) ?? "")
        }
        return CoreNativeSessionFile(xml: String(cString: xc_native_session_text_data(text)!), windows: windows,
            activeKey: xc_native_session_active_key(result).map { String(cString: $0) })
    }
    func restoreNativeSession(_ saved: CoreNativeSessionFile) async throws -> CoreRestoredSession {
        guard !saved.xml.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var input: OpaquePointer?
        let parsed = saved.xml.withCString { xc_native_session_parse($0, &input) }
        guard parsed == XC_OK, let input else { throw CoreError(status: parsed) }
        defer { xc_native_session_release(input) }
        var capabilities = xc_session_capabilities(lexical_layout: 1, note_numbering: 1, heading_note_links: 1)
        var request: OpaquePointer?
        let status = xc_worker_restore_native_session(try worker(), input, &capabilities, &request)
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_restored_session(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_restored_session_release(result) }
        let rows = try (0..<xc_restored_session_tab_count(result)).map { row in
            let key = String(cString: xc_restored_session_tab_key(result, row)!)
            let tab = xc_restored_session_tab_id(result, row)
            var visibility = xc_tab_visibility(), commentary: Int32 = 0, parallel: Int32 = 0
            guard xc_restored_session_visibility(result, row, &visibility, &commentary, &parallel) == XC_OK else { throw CoreError(status: XC_ENGINE_ERROR) }
            let capture = CoreWindowCapture(tab: tab, key: key, texts: visibility.texts != 0, preview: visibility.preview != 0,
                commentaries: visibility.commentaries != 0, dictionaries: visibility.dictionaries != 0,
                commentaryShowing: commentary != 0, parallelShowing: parallel != 0)
            return CoreRestoredWindow(key: key, tab: tab, capture: capture)
        }
        let active = xc_restored_session_active_tab(result)
        return CoreRestoredSession(windows: rows, activeTab: active == 0 ? nil : active)
    }
    func saveNativeSession(path: String, windows: [CoreWindowCapture], activeKey: String?) async throws {
        guard !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        // Admission copies all C strings before this synchronous submission ends.
        let strings = SessionCStrings()
        var rows: [xc_native_tab_capture] = []
        for window in windows {
            var row = xc_native_tab_capture(); xc_native_tab_capture_default(&row)
            row.tab = window.tab; row.key = try strings.copy(window.key); row.host_state = try strings.copy(window.hostState)
            row.visibility = xc_tab_visibility(texts: window.texts ? 1 : 0, preview: window.preview ? 1 : 0,
                commentaries: window.commentaries ? 1 : 0, dictionaries: window.dictionaries ? 1 : 0)
            row.commentary_showing = window.commentaryShowing ? 1 : 0; row.parallel_showing = window.parallelShowing ? 1 : 0
            rows.append(row)
        }
        let active = try activeKey.map { try strings.copy($0) }
        let handle = try worker()
        var request: OpaquePointer?
        let status = withExtendedLifetime(strings) {
            rows.withUnsafeBufferPointer { buffer in
                path.withCString { xc_worker_save_native_session(handle, $0, buffer.baseAddress, buffer.count, active, &request) }
            }
        }
        xc_request_release(try await wait(status, request))
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
            reference: String(cString: xc_link_reference(result)!), anchor: xc_link_anchor(result).map { String(cString: $0) }, anchorMainFallback: xc_link_anchor_main_fallback(result) != 0))
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
            reference: String(cString: xc_link_reference(result)!), anchor: xc_link_anchor(result).map { String(cString: $0) }, anchorMainFallback: xc_link_anchor_main_fallback(result) != 0))
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
    func visitLinkedStudyPassage(tab: UInt64, bible: String, commentary: String, target: PassageTarget, commentaryVisible: Bool = true) async throws -> RenderedStudyPassage {
        guard [bible, commentary, target.module].allSatisfy({ $0.utf8.count <= Int(XC_STUDY_LINK_MAX_CONTEXT_BYTES) && !$0.utf8.contains(0) }),
              target.reference.utf8.count <= Int(XC_STUDY_LINK_MAX_URI_BYTES), !target.reference.utf8.contains(0),
              target.anchor.map({ $0.utf8.count <= Int(XC_STUDY_LINK_MAX_URI_BYTES) && !$0.utf8.contains(0) }) != false else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = bible.withCString { main in commentary.withCString { notes in target.module.withCString { module in target.reference.withCString { reference in
            CoreValues.withOptionalCString(target.anchor) { anchor in
                xc_worker_visit_linked_passage_with_anchor_policy(handle, tab, main, notes, module, reference, anchor, target.anchorMainFallback ? 1 : 0, commentaryVisible ? 1 : 0, &request)
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
extension CoreWorker {
    func openEditorFile(tab: UInt64, path: String) async throws -> EditorSource {
        guard !path.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?; let handle = try worker()
        let status = path.withCString { xc_worker_read_editable_document(handle, tab, $0, &request) }
        let completed = try await wait(status, request); defer { xc_request_release(completed) }
        guard let result = xc_request_take_editable_document(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_editable_document_release(result) }
        return EditorSource(text: String(cString: xc_editable_document_text(result)!), revision: String(cString: xc_editable_document_revision(result)!))
    }
    func saveEditorFile(tab: UInt64, path: String, revision: String?, text: String) async throws -> EditorSource {
        guard !path.utf8.contains(0), !text.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let strings = SessionCStrings(); let expected = try revision.map { try strings.copy($0) }
        var request: OpaquePointer?; let handle = try worker()
        let status = withExtendedLifetime(strings) { path.withCString { path in text.withCString { xc_worker_save_editable_document(handle, tab, path, expected, $0, &request) } } }
        let completed = try await wait(status, request); defer { xc_request_release(completed) }
        guard let result = xc_request_take_editable_document(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_editable_document_release(result) }
        return EditorSource(text: String(cString: xc_editable_document_text(result)!), revision: String(cString: xc_editable_document_revision(result)!))
    }
    func openModuleEditor(tab: UInt64, module: String, reference: String) async throws -> EditorModuleSource {
        var request: OpaquePointer?; let handle = try worker()
        guard !module.utf8.contains(0), !reference.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let status = module.withCString { module in reference.withCString { xc_worker_read_module_edit(handle, tab, module, $0, &request) } }
        return try await moduleEditorValue(status, request)
    }
    func saveModuleEditor(tab: UInt64, module: String, reference: String, revision: String, text: String, erase: Bool = false) async throws -> EditorModuleSource {
        guard [module,reference,revision,text].allSatisfy({ !$0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?; let handle = try worker()
        let status = module.withCString { module in reference.withCString { reference in revision.withCString { revision in text.withCString { xc_worker_save_module_edit(handle, tab, module, reference, revision, $0, erase ? 1 : 0, &request) } } } }
        return try await moduleEditorValue(status, request)
    }
    private func moduleEditorValue(_ status: xc_status, _ request: OpaquePointer?) async throws -> EditorModuleSource {
        let completed = try await wait(status, request); defer { xc_request_release(completed) }
        guard let result = xc_request_take_module_edit(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_module_edit_release(result) }
        return EditorModuleSource(module: String(cString: xc_module_edit_module(result)!), reference: String(cString: xc_module_edit_reference(result)!), driver: String(cString: xc_module_edit_driver(result)!), sourceType: String(cString: xc_module_edit_source_type(result)!), text: String(cString: xc_module_edit_text(result)!), revision: String(cString: xc_module_edit_revision(result)!))
    }
}

extension CoreWorker {
    func captureNativeSession(windows: [CoreWindowCapture], activeKey: String?) async throws -> CoreNativeSessionFile {
        let strings = SessionCStrings()
        var rows: [xc_native_tab_capture] = []
        for window in windows {
            var row = xc_native_tab_capture(); xc_native_tab_capture_default(&row)
            row.tab = window.tab; row.key = try strings.copy(window.key); row.host_state = try strings.copy(window.hostState)
            row.visibility = xc_tab_visibility(texts: window.texts ? 1 : 0, preview: window.preview ? 1 : 0, commentaries: window.commentaries ? 1 : 0, dictionaries: window.dictionaries ? 1 : 0)
            row.commentary_showing = window.commentaryShowing ? 1 : 0; row.parallel_showing = window.parallelShowing ? 1 : 0; rows.append(row)
        }
        let active = try activeKey.map { try strings.copy($0) }; var request: OpaquePointer?
        let handle = try worker()
        let status = withExtendedLifetime(strings) { rows.withUnsafeBufferPointer { xc_worker_capture_native_session(handle,$0.baseAddress,$0.count,active,&request) } }
        let completed = try await wait(status,request); defer { xc_request_release(completed) }
        guard let owned = xc_request_take_native_session(completed) else { throw CoreError(status:XC_ENGINE_ERROR) }
        defer { xc_native_session_release(owned) }; return try nativeSessionValue(owned)
    }
    private func recoveryValue(_ owned: OpaquePointer) throws -> CoreRecoveryFile {
        var session: OpaquePointer?
        let status = xc_recovery_snapshot_copy_session(owned,&session)
        guard status == XC_OK, let session else { throw CoreError(status:status) }
        defer { xc_native_session_release(session) }
        let drafts = try (0..<xc_recovery_snapshot_count(owned)).map { index in
            var d = xc_recovery_draft()
            guard xc_recovery_snapshot_draft(owned,index,&d) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
            return CoreRecoveryDraft(id:String(cString:d.id!),title:String(cString:d.title!),target:d.target,
                path:String(cString:d.path!),module:String(cString:d.module!),reference:String(cString:d.reference!),
                originRevision:d.origin_revision.map { String(cString:$0) },savedText:String(cString:d.saved_text!),text:String(cString:d.text!),selectionStart:d.selection_start,selectionEnd:d.selection_end)
        }
        return CoreRecoveryFile(session:try nativeSessionValue(session),drafts:drafts,revision:String(cString:xc_recovery_snapshot_revision(owned)!),previous:xc_recovery_snapshot_from_previous(owned) != 0)
    }
    func readRecovery(path: String) async throws -> CoreRecoveryFile {
        guard !path.utf8.contains(0) else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        let handle = try worker();var request: OpaquePointer?
        let status = path.withCString { xc_worker_read_recovery(handle,$0,&request) }
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let owned = xc_request_take_recovery_snapshot(completed) else { throw CoreError(status:XC_ENGINE_ERROR) }
        defer { xc_recovery_snapshot_release(owned) };return try recoveryValue(owned)
    }
    func saveRecovery(path: String, revision: String?, session: CoreNativeSessionFile, drafts: [CoreRecoveryDraft]) async throws -> CoreRecoveryFile {
        guard !path.utf8.contains(0), !session.xml.utf8.contains(0) else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        var native: OpaquePointer?;let parsed = session.xml.withCString { xc_native_session_parse($0,&native) }
        guard parsed == XC_OK, let native else { throw CoreError(status:parsed) };defer { xc_native_session_release(native) }
        let strings = SessionCStrings();var rows: [xc_recovery_draft] = []
        for d in drafts {
            rows.append(xc_recovery_draft(id:try strings.copy(d.id),title:try strings.copy(d.title),target:d.target,
                path:try strings.copy(d.path),module:try strings.copy(d.module),reference:try strings.copy(d.reference),origin_revision:try d.originRevision.map { try strings.copy($0) },
                saved_text:try strings.copy(d.savedText),text:try strings.copy(d.text),selection_start:d.selectionStart,selection_end:d.selectionEnd))
        }
        var snapshot: OpaquePointer?
        let created = withExtendedLifetime(strings) { rows.withUnsafeBufferPointer { xc_recovery_snapshot_create(native,$0.baseAddress,$0.count,&snapshot) } }
        guard created == XC_OK, let snapshot else { throw CoreError(status:created) };defer { xc_recovery_snapshot_release(snapshot) }
        let expected = try revision.map { try strings.copy($0) };let handle = try worker();var request: OpaquePointer?
        let status = withExtendedLifetime(strings) { path.withCString { xc_worker_save_recovery(handle,$0,expected,snapshot,&request) } }
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let owned = xc_request_take_recovery_snapshot(completed) else { throw CoreError(status:XC_ENGINE_ERROR) }
        defer { xc_recovery_snapshot_release(owned) };return try recoveryValue(owned)
    }
}

struct CoreLibraryChoice: Identifiable, Sendable {
    let provider: String, module: String, title: String, version: String, language: String, type: String
    var decision: Int32 = -1
    var license = "", attribution = ""
    var id: String { provider + "|" + module }
}
struct CoreRepositoryEndpoint: Sendable, Equatable {
    var id: String, host: String, catalog: String, packages: String
    var port: UInt32 = 443
}
struct CoreRepositoryRow: Identifiable, Sendable {
    let endpoint: CoreRepositoryEndpoint
    let name: String, enabled: Bool, cached: Bool, fetchedAt: UInt64
    var id: String { endpoint.id }
}
struct CoreRepositoryProfile: Sendable {
    let bytes: Data
    let revision: String?
    let rows: [CoreRepositoryRow]
}
struct CoreLocalInstallOffer: Sendable {
    let source: String, module: String, sourceRevision: String, destinationRevision: String, version: String, installedVersion: String
    let decision: Int32
}
struct CoreMaintenanceOffer: Sendable {
    let module: String, revision: String
    let cipher: Bool, indexed: Bool
}
struct CoreTransferProgress: Sendable {
    let phase: Int32
    let downloaded: UInt64, total: UInt64, expanded: UInt64
}
struct CoreRemoteInstallOutcome: Sendable {
    let profile: CoreRepositoryProfile?
    let message: String
}
extension CoreWorker {
    private func libraryChoice(_ item: xc_library_item) -> CoreLibraryChoice {
        CoreLibraryChoice(provider:String(cString:item.provider!),module:String(cString:item.item!),title:String(cString:item.title!),version:String(cString:item.version!),language:String(cString:item.language!),type:String(cString:item.type!),license:String(cString:item.license!),attribution:String(cString:item.attribution!))
    }
    private func libraryValues(_ catalog: OpaquePointer) throws -> [CoreLibraryChoice] {
        try (0..<xc_library_catalog_count(catalog)).map { index in
            var item = xc_library_item();guard xc_library_catalog_item(catalog,index,&item) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) };return libraryChoice(item)
        }
    }
    func libraryInventory(source: String? = nil) async throws -> [CoreLibraryChoice] {
        var request: OpaquePointer?;let handle = try worker()
        let status: xc_status
        if let source { guard !source.utf8.contains(0) else { throw CoreError(status:XC_INVALID_ARGUMENT) };status = source.withCString { xc_worker_local_library_catalog(handle,$0,&request) } }
        else { status = xc_worker_library_catalog(handle,&request) }
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let catalog = xc_request_take_library_catalog(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_library_catalog_release(catalog) }
        return try libraryValues(catalog)
    }
    private func repositoryHandle(_ profile: CoreRepositoryProfile?) throws -> OpaquePointer {
        var result: OpaquePointer?;let status: xc_status
        if let profile { status = profile.bytes.withUnsafeBytes { bytes in xc_repository_snapshot_parse(bytes.bindMemory(to:CChar.self).baseAddress,bytes.count,&result) } }
        else { status = xc_repository_snapshot_create(&result) }
        guard status == XC_OK, let result else { throw CoreError(status:status) };return result
    }
    private func repositoryValue(_ result: OpaquePointer) throws -> CoreRepositoryProfile {
        var count = 0;guard xc_repository_snapshot_serialize(result,nil,0,&count) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
        var bytes = Data(count:count);let status = bytes.withUnsafeMutableBytes { xc_repository_snapshot_serialize(result,$0.bindMemory(to:CChar.self).baseAddress,$0.count,&count) }
        guard status == XC_OK else { throw CoreError(status:status) }
        let rows = try (0..<xc_repository_snapshot_count(result)).map { index in
            var row = xc_saved_repository();guard xc_repository_snapshot_get(result,index,&row) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
            return CoreRepositoryRow(endpoint:CoreRepositoryEndpoint(id:String(cString:row.id!),host:String(cString:row.host!),catalog:String(cString:row.catalog_directory!),packages:String(cString:row.package_directory!),port:row.port),name:String(cString:row.name!),enabled:row.enabled != 0,cached:row.cached != 0,fetchedAt:row.fetched_at)
        }
        let revision = String(cString:xc_repository_snapshot_revision(result)!)
        return CoreRepositoryProfile(bytes:bytes,revision:revision.isEmpty ? nil : revision,rows:rows)
    }
    func readRepositories(path: String) async throws -> CoreRepositoryProfile {
        guard !path.utf8.contains(0) else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        if !FileManager.default.fileExists(atPath:path) { let input = try repositoryHandle(nil);defer { xc_repository_snapshot_release(input) };return try repositoryValue(input) }
        let handle = try worker();var request: OpaquePointer?;let status = path.withCString { xc_worker_read_repositories(handle,$0,&request) }
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let result = xc_request_take_repository_snapshot(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_repository_snapshot_release(result) };return try repositoryValue(result)
    }
    private func saveRepositoryHandle(path: String, expected: String?, input: OpaquePointer) async throws -> CoreRepositoryProfile {
        let strings = SessionCStrings();let file = try strings.copy(path), revision = try expected.map { try strings.copy($0) };var request: OpaquePointer?
        try FileManager.default.createDirectory(at:URL(fileURLWithPath:path).deletingLastPathComponent(),withIntermediateDirectories:true)
        let handle = try worker()
        let status = withExtendedLifetime(strings) { xc_worker_save_repositories(handle,file,revision,input,&request) }
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let result = xc_request_take_repository_snapshot(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_repository_snapshot_release(result) };return try repositoryValue(result)
    }
    // All edits execute in the shared repository service; Swift only transports fields.
    func editRepository(path: String, profile: CoreRepositoryProfile, endpoint: CoreRepositoryEndpoint? = nil, name: String = "", enabled: Bool = true, remove: String? = nil, move: (String,Int)? = nil) async throws -> CoreRepositoryProfile {
        let input = try repositoryHandle(profile);defer { xc_repository_snapshot_release(input) }
        let strings = SessionCStrings();let status: xc_status
        if let endpoint {
            var r = xc_https_repository(id:try strings.copy(endpoint.id),host:try strings.copy(endpoint.host),catalog_directory:try strings.copy(endpoint.catalog),package_directory:try strings.copy(endpoint.packages),port:endpoint.port)
            status = withExtendedLifetime(strings) { name.withCString { xc_repository_snapshot_put(input,&r,$0,enabled ? 1 : 0) } }
        } else if let remove { status = remove.withCString { xc_repository_snapshot_remove(input,$0) } }
        else if let move { guard move.1 >= 0 else { throw CoreError(status:XC_INVALID_ARGUMENT) };status = move.0.withCString { xc_repository_snapshot_move(input,$0,move.1) } }
        else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        guard status == XC_OK else { throw CoreError(status:status) }
        return try await saveRepositoryHandle(path:path,expected:profile.revision,input:input)
    }
    func repositoryChoices(profile: CoreRepositoryProfile,index: Int,planning selectedID: String? = nil) async throws -> [CoreLibraryChoice] {
        guard index >= 0 else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        let input = try repositoryHandle(profile);defer { xc_repository_snapshot_release(input) };var offer: OpaquePointer?
        guard xc_repository_snapshot_copy_catalog(input,index,&offer) == XC_OK, let offer else { return [] };defer { xc_remote_catalog_release(offer) }
        guard let catalog = xc_remote_catalog_library(offer) else { throw CoreError(status:XC_ENGINE_ERROR) }
        var values = try libraryValues(catalog)
        guard let selectedID, let i = values.firstIndex(where: { $0.id == selectedID }) else { return values }
        var request: OpaquePointer?;let status = xc_worker_library_catalog(try worker(),&request)
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let inventory = xc_request_take_library_catalog(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_library_catalog_release(inventory) }
        do {
            var plan: OpaquePointer?;let status = values[i].provider.withCString { p in values[i].module.withCString { xc_library_plan_install(catalog,inventory,p,$0,&plan) } }
            guard status == XC_OK, let plan else { throw CoreError(status:status) };defer { xc_library_install_plan_release(plan) }
            var value = xc_library_plan();guard xc_library_install_plan_get(plan,&value) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) };values[i].decision = value.decision
        }
        return values
    }
    private func waitTransfer(_ status: xc_status,_ request: OpaquePointer?,progress: (@Sendable (CoreTransferProgress) async -> Void)?) async throws -> OpaquePointer {
        guard status == XC_OK, let request else { throw CoreError(status:status) }
        guard let control = xc_request_take_transfer_control(request) else { xc_request_release(request);throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_transfer_control_release(control) }
        while true {
            if Task.isCancelled { _ = xc_transfer_control_cancel(control) }
            let status = xc_request_poll(request)
            var value = xc_transfer_progress()
            if xc_transfer_control_progress(control,&value) == XC_OK { await progress?(CoreTransferProgress(phase:value.phase,downloaded:value.downloaded,total:value.download_total,expanded:value.expanded)) }
            if status != XC_PENDING { guard status == XC_OK else { xc_request_release(request);throw CoreError(status:status) };return request }
            await withCheckedContinuation { continuation in DispatchQueue.global(qos:.utility).asyncAfter(deadline:.now() + .milliseconds(50)) { continuation.resume() } }
        }
    }
    func refreshRepository(path: String,profile: CoreRepositoryProfile,index: Int,workspace: String,caBundle: String? = nil,progress: (@Sendable (CoreTransferProgress) async -> Void)? = nil) async throws -> CoreRepositoryProfile {
        guard profile.rows.indices.contains(index) else { throw CoreError(status:XC_INVALID_ARGUMENT) };let row = profile.rows[index];guard row.enabled else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        guard try await readRepositories(path:path).revision == profile.revision else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        let strings = SessionCStrings();var repository = xc_https_repository(id:try strings.copy(row.id),host:try strings.copy(row.endpoint.host),catalog_directory:try strings.copy(row.endpoint.catalog),package_directory:try strings.copy(row.endpoint.packages),port:row.endpoint.port)
        var options = xc_transfer_options();xc_transfer_options_default(&options);options.workspace = try strings.copy(workspace);options.ca_bundle = try caBundle.map { try strings.copy($0) };options.remote_access_acknowledged = 1
        var request: OpaquePointer?;let handle = try worker()
        let status = withExtendedLifetime(strings) { xc_worker_fetch_remote_catalog(handle,&repository,&options,&request) }
        let completed = try await waitTransfer(status,request,progress:progress);defer { xc_request_release(completed) }
        guard let offer = xc_request_take_remote_catalog(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_remote_catalog_release(offer) }
        let input = try repositoryHandle(profile);defer { xc_repository_snapshot_release(input) }
        guard xc_repository_snapshot_cache(input,offer,UInt64(Date().timeIntervalSince1970)) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
        return try await saveRepositoryHandle(path:path,expected:profile.revision,input:input)
    }
    func installRemote(path: String,profile: CoreRepositoryProfile,index: Int,module: String,replace: Bool,workspace: String,caBundle: String? = nil,progress: (@Sendable (CoreTransferProgress) async -> Void)? = nil) async throws -> CoreRemoteInstallOutcome {
        guard profile.rows.indices.contains(index), profile.rows[index].enabled else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        guard try await readRepositories(path:path).revision == profile.revision else { throw CoreError(status:XC_INVALID_ARGUMENT) }
        let input = try repositoryHandle(profile);defer { xc_repository_snapshot_release(input) };var offer: OpaquePointer?
        guard xc_repository_snapshot_copy_catalog(input,index,&offer) == XC_OK, let offer else { throw CoreError(status:XC_INVALID_ARGUMENT) };defer { xc_remote_catalog_release(offer) }
        let strings = SessionCStrings();let name = try strings.copy(module);var options = xc_transfer_options();xc_transfer_options_default(&options);options.workspace = try strings.copy(workspace);options.ca_bundle = try caBundle.map { try strings.copy($0) };options.remote_access_acknowledged = 1
        var request: OpaquePointer?;let handle = try worker()
        let status = withExtendedLifetime(strings) { xc_worker_install_remote_module(handle,offer,name,replace ? 1 : 0,&options,&request) }
        let completed = try await waitTransfer(status,request,progress:progress);defer { xc_request_release(completed) }
        guard let result = xc_request_take_remote_install_result(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_remote_install_result_release(result) }
        do {
            guard let next = xc_remote_install_result_next_catalog(result), xc_repository_snapshot_cache(input,next,UInt64(Date().timeIntervalSince1970)) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
            return CoreRemoteInstallOutcome(profile:try await saveRepositoryHandle(path:path,expected:profile.revision,input:input),message:"Module installed.")
        } catch { return CoreRemoteInstallOutcome(profile:nil,message:"Module installed. Source cache could not be saved; reload sources before another installation.") }
    }
    func prepareLocalInstall(source: String,module: String) async throws -> CoreLocalInstallOffer {
        let strings = SessionCStrings();let folder = try strings.copy(source), name = try strings.copy(module);var request: OpaquePointer?
        let handle = try worker()
        let status = withExtendedLifetime(strings) { xc_worker_prepare_local_install(handle,folder,name,&request) }
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let result = xc_request_take_local_install_preview(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_local_install_preview_release(result) }
        var info = xc_local_install_info();guard xc_local_install_preview_get(result,&info) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
        return CoreLocalInstallOffer(source:source,module:String(cString:info.module!),sourceRevision:String(cString:info.source_revision!),destinationRevision:String(cString:info.destination_revision!),version:String(cString:info.source_version!),installedVersion:String(cString:info.installed_version!),decision:info.decision)
    }
    func installLocal(_ offer: CoreLocalInstallOffer,replace: Bool) async throws {
        let strings = SessionCStrings();let source = try strings.copy(offer.source), module = try strings.copy(offer.module), from = try strings.copy(offer.sourceRevision), to = try strings.copy(offer.destinationRevision);var request: OpaquePointer?
        let handle = try worker()
        let status = withExtendedLifetime(strings) { xc_worker_install_local_module(handle,source,module,from,to,replace ? 1 : 0,&request) };xc_request_release(try await wait(status,request))
    }
    func maintenanceOffer(module: String) async throws -> CoreMaintenanceOffer {
        guard !module.utf8.contains(0) else { throw CoreError(status:XC_INVALID_ARGUMENT) };var request: OpaquePointer?;let handle = try worker()
        let status = module.withCString { xc_worker_preview_module_maintenance(handle,$0,&request) };let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let result = xc_request_take_module_maintenance_preview(completed) else { throw CoreError(status:XC_ENGINE_ERROR) };defer { xc_module_maintenance_preview_release(result) }
        var info = xc_module_maintenance_info();guard xc_module_maintenance_preview_get(result,&info) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
        return CoreMaintenanceOffer(module:String(cString:info.module!),revision:String(cString:info.revision!),cipher:info.cipher_declared != 0,indexed:info.indexed != 0)
    }
    func maintainModule(_ offer: CoreMaintenanceOffer,action: Int32,key: String = "") async throws {
        let strings = SessionCStrings();let name = try strings.copy(offer.module), revision = try strings.copy(offer.revision), cipher = try strings.copy(key);var request: OpaquePointer?
        let handle = try worker()
        let status = withExtendedLifetime(strings) { xc_worker_maintain_module(handle,name,revision,action,cipher,&request) };xc_request_release(try await wait(status,request))
    }
}

extension CoreWorker {
    func releaseModuleEditor(tab: UInt64) async throws {
        var request: OpaquePointer?;let status = xc_worker_release_module_editor(try worker(),tab,&request)
        xc_request_release(try await wait(status,request))
    }
}

struct CoreSyncSettings: Sendable, Equatable {
    var mode: Int32 = 0, listen: Int32 = 0
    var privatePersonal = true, direct = true
    var group = "", domain = "BIBLE-VERSE"
    var speakers: [String] = []
}
struct CoreSyncNavigation: Sendable, Equatable {
    var speaker: String, module: String, reference: String
    var group = "1", domain = "BIBLE-VERSE"
}
struct CoreSyncDecision: Sendable {
    let route: Int32
    let navigation: CoreSyncNavigation?
}
struct CoreSyncEvent: Sendable { let kind: Int8, navigation: CoreSyncNavigation }
struct CoreSyncBatch: Sendable { let active: Bool, overflow: Bool, events: [CoreSyncEvent] }
extension CoreWorker {
    func syncAvailable() -> Bool { xc_sync_network_available() != 0 }
    private func syncValue(_ navigation: xc_sync_navigation) -> CoreSyncNavigation {
        func text(_ pointer: UnsafePointer<CChar>?) -> String { pointer.map { String(cString:$0) } ?? "" }
        return CoreSyncNavigation(speaker:text(navigation.speaker),module:text(navigation.module),reference:text(navigation.reference),group:text(navigation.group),domain:text(navigation.domain))
    }
    private func syncInput(_ navigation: CoreSyncNavigation, strings: SessionCStrings) throws -> xc_sync_navigation {
        try xc_sync_navigation(speaker:strings.copy(navigation.speaker),module:strings.copy(navigation.module),reference:strings.copy(navigation.reference),group:strings.copy(navigation.group),domain:strings.copy(navigation.domain))
    }
    func configureSync(_ settings: CoreSyncSettings, user: String = "", phrase: String = "", acknowledged: Bool = false, network: Bool = true) async throws {
        let handle = try worker(), strings = SessionCStrings()
        let speakers = try settings.speakers.map { Optional(try strings.copy($0)) }
        var config = try xc_sync_settings(mode:settings.mode,listen:settings.listen,private_personal:settings.privatePersonal ? 1:0,direct_navigation:settings.direct ? 1:0,group_tabs:0,group:strings.copy(settings.group),domain:strings.copy(settings.domain),allowed_speakers:nil,speaker_count:speakers.count)
        var connection = try xc_sync_connection(application:strings.copy("Xiphos Native Preview"),version:strings.copy("1"),user:strings.copy(user),passphrase:strings.copy(phrase),network_acknowledged:acknowledged ? 1:0)
        var request: OpaquePointer?
        let status = speakers.withUnsafeBufferPointer { pointers in
            config.allowed_speakers = pointers.baseAddress
            return network && settings.mode != 0 ? xc_worker_configure_sync_network(handle,&config,&connection,&request):xc_worker_configure_sync(handle,&config,&request)
        }
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
    }
    func pollSync() async throws -> CoreSyncBatch {
        let handle = try worker();var request: OpaquePointer?
        let status = xc_worker_poll_sync_network(handle,&request)
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let owned = xc_request_take_sync_network_events(completed) else { throw CoreError(status:XC_ENGINE_ERROR) }
        defer { xc_sync_network_events_release(owned) }
        var events: [CoreSyncEvent] = []
        for index in 0..<xc_sync_network_event_count(owned) {
            var kind: CChar = 0, navigation = xc_sync_navigation()
            guard xc_sync_network_event_get(owned,index,&kind,&navigation) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
            events.append(CoreSyncEvent(kind:kind,navigation:syncValue(navigation)))
        }
        return CoreSyncBatch(active:xc_sync_network_events_active(owned) != 0,overflow:xc_sync_network_events_overflow(owned) != 0,events:events)
    }
    func planSync(_ navigation: CoreSyncNavigation) async throws -> CoreSyncDecision {
        let handle = try worker(), strings = SessionCStrings();var input = try syncInput(navigation,strings:strings);var request: OpaquePointer?
        let status = xc_worker_plan_incoming_sync(handle,&input,&request)
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        guard let owned = xc_request_take_sync_decision(completed) else { throw CoreError(status:XC_ENGINE_ERROR) }
        defer { xc_sync_decision_release(owned) }
        var result = xc_sync_result()
        guard xc_sync_decision_get(owned,&result) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
        return CoreSyncDecision(route:result.route,navigation:result.route == 0 ? nil:syncValue(result.navigation))
    }
    func transmitSync(_ navigation: CoreSyncNavigation, received: Bool = false) async throws -> Bool {
        let handle = try worker(), strings = SessionCStrings();var input = try syncInput(navigation,strings:strings);var request: OpaquePointer?
        let status = xc_worker_transmit_sync_network(handle,&input,received ? 1:0,&request)
        let completed = try await wait(status,request);defer { xc_request_release(completed) }
        var sent: Int32 = 0
        guard xc_request_sync_transmitted(completed,&sent) == XC_OK else { throw CoreError(status:XC_ENGINE_ERROR) }
        return sent != 0
    }
}
