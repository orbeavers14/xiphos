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
                    transliterationAvailable: xc_catalog_transliteration_available(catalog, index) != 0),
                isDictionary: xc_catalog_is_dictionary(catalog, index) != 0,
                lexiconRoles: LexiconRoles(rawValue: xc_catalog_lexicon_roles(catalog, index)))
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
    func hoverStudyLinkAt(tab: UInt64, currentModule: String, currentReference: String, uri: String) async throws -> HoveredStudyLink {
        guard ![currentModule, currentReference, uri].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
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
    func followStudyLinkAt(tab: UInt64, currentModule: String, currentReference: String, uri: String) async throws -> ContextualStudyLink {
        try await contextualStudyLinkAt(tab: tab, currentModule: currentModule, currentReference: currentReference, uri: uri, record: true)
    }
    private func contextualStudyLinkAt(tab: UInt64, currentModule: String, currentReference: String, uri: String, record: Bool) async throws -> ContextualStudyLink {
        guard ![currentModule, currentReference, uri].contains(where: { $0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var request: OpaquePointer?
        let handle = try worker()
        let status = currentModule.withCString { id in
            currentReference.withCString { context in
                uri.withCString { value in
                    record ? xc_worker_follow_study_link_at(handle, tab, id, context, value, &request)
                           : xc_worker_resolve_study_link_at(handle, tab, id, context, value, &request)
                }
            }
        }
        let completed = try await wait(status, request)
        defer { xc_request_release(completed) }
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
        guard let result = xc_request_take_link(completed) else { throw CoreError(status: XC_ENGINE_ERROR) }
        defer { xc_link_release(result) }
        return .passage(PassageTarget(module: String(cString: xc_link_module(result)!),
            reference: String(cString: xc_link_reference(result)!)))
    }
    func resolveStudyLink(tab: UInt64, currentModule: String, uri: String) async throws -> ResolvedStudyLink {
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
            reference: String(cString: xc_link_reference(result)!)))
    }
    func studyNoteLink(tab: UInt64, currentModule: String, uri: String) async throws -> RenderedStudyNote {
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
            html: String(cString: xc_passage_commentary_html(result)!)) : nil
        return RenderedStudyPassage(module: String(cString: xc_passage_module(result)!),
            commentaryModule: String(cString: xc_passage_commentary_module(result)!),
            text: RenderedEntry(reference: String(cString: xc_passage_reference(result)!),
                                html: String(cString: xc_passage_html(result)!)), commentary: commentary)
    }
    func studyPassage(tab: UInt64, module: String, commentary: String, reference: String, record: Bool = true) async throws -> RenderedStudyPassage {
        var request: OpaquePointer?
        let handle = try worker()
        let status = module.withCString { id in commentary.withCString { notes in reference.withCString { ref in
            record ? xc_worker_visit_passage(handle, tab, id, notes, ref, &request)
                   : xc_worker_read_passage(handle, tab, id, notes, ref, &request)
        } } }
        return try await passage(status, request)
    }
    func navigateStudyPassage(tab: UInt64, direction: Int32) async throws -> RenderedStudyPassage {
        var request: OpaquePointer?
        let status = xc_worker_navigate_passage(try worker(), tab, direction, &request)
        return try await passage(status, request)
    }
    func navigateStudyChapter(tab: UInt64, module: String, commentary: String, reference: String, direction: Int32) async throws -> RenderedStudyPassage {
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
