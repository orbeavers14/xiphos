// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore

struct CoreHistoryState: Sendable {
    let canBack: Bool
    let canForward: Bool
    let count: Int
    let module: String?
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
    private func wait(_ status: xc_status, _ request: OpaquePointer?) async throws -> OpaquePointer {
        guard status == XC_OK, let request else { throw CoreError(status: status) }
        // Accepted core actions are not canceled by dropping a Swift task. Keep
        // observing until completion so tab IDs/results can always be released.
        // Hosts guard stale presentation separately. This sleep yields the actor
        // and never waits on an engine future or blocks a UI thread.
        while xc_request_poll(request) == XC_PENDING {
            await withCheckedContinuation { continuation in
                DispatchQueue.global(qos: .utility).asyncAfter(deadline: .now() + .milliseconds(5)) {
                    continuation.resume()
                }
            }
        }
        let result = xc_request_poll(request)
        guard result == XC_OK else {
            xc_request_release(request)
            throw CoreError(status: result)
        }
        return request
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
                    transliterationAvailable: xc_catalog_transliteration_available(catalog, index) != 0))
        }
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
    func readTabEntry(tab: UInt64, module: String, reference: String) async throws -> RenderedEntry {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in
            reference.withCString { xc_worker_read_tab_entry(handle, tab, id, $0, &request) }
        }
        return try await entry(status, request)
    }
    func readTabChapter(tab: UInt64, module: String, reference: String, verseNumbers: Bool, highlightSelected: Bool) async throws -> RenderedChapter {
        var request: OpaquePointer?
        let handle = try worker()
        let flags = (verseNumbers ? UInt32(XC_CHAPTER_VERSE_NUMBERS) : 0) |
                    (highlightSelected ? UInt32(XC_CHAPTER_HIGHLIGHT_SELECTED) : 0)
        let status = module.withCString { id in
            reference.withCString { xc_worker_read_tab_chapter(handle, tab, id, $0, flags, &request) }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_chapter(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_chapter_release(result) }
        return CoreValues.chapter(result)
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
            module: xc_history_module(result).map { String(cString: $0) })
    }
    private func entry(_ status: xc_status, _ request: OpaquePointer?) async throws -> RenderedEntry {
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_entry(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_entry_release(result) }
        return CoreValues.entry(result)
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
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
        guard let result = xc_request_take_chapter(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_chapter_release(result) }
        return CoreValues.chapter(result)
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
