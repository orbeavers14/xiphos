// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore

// Typed C ABI values only; eligibility and render policy remain in C++.
struct EngineOptionFlags: OptionSet, Sendable {
    let rawValue: UInt32
    static let strongs = Self(rawValue: UInt32(XC_ENGINE_STRONGS.rawValue))
    static let morphology = Self(rawValue: UInt32(XC_ENGINE_MORPHOLOGY.rawValue))
    static let footnotes = Self(rawValue: UInt32(XC_ENGINE_FOOTNOTES.rawValue))
    static let greekAccents = Self(rawValue: UInt32(XC_ENGINE_GREEK_ACCENTS.rawValue))
    static let lemmas = Self(rawValue: UInt32(XC_ENGINE_LEMMAS.rawValue))
    static let crossReferences = Self(rawValue: UInt32(XC_ENGINE_CROSS_REFERENCES.rawValue))
    static let hebrewVowels = Self(rawValue: UInt32(XC_ENGINE_HEBREW_VOWELS.rawValue))
    static let hebrewCantillation = Self(rawValue: UInt32(XC_ENGINE_HEBREW_CANTILLATION.rawValue))
    static let italicHeadings = Self(rawValue: UInt32(XC_ENGINE_ITALIC_HEADINGS.rawValue))
    static let wordsInRed = Self(rawValue: UInt32(XC_ENGINE_WORDS_IN_RED.rawValue))
    static let transliteratedForms = Self(rawValue: UInt32(XC_ENGINE_TRANSLITERATED_FORMS.rawValue))
    static let enumerations = Self(rawValue: UInt32(XC_ENGINE_ENUMERATIONS.rawValue))
    static let glosses = Self(rawValue: UInt32(XC_ENGINE_GLOSSES.rawValue))
    static let morphemeSegmentation = Self(rawValue: UInt32(XC_ENGINE_MORPHEME_SEGMENTATION.rawValue))
    static let transliteration = Self(rawValue: UInt32(XC_ENGINE_TRANSLITERATION.rawValue))
    static let variantsPrimary = Self(rawValue: UInt32(XC_ENGINE_VARIANTS_PRIMARY.rawValue))
    static let variantsSecondary = Self(rawValue: UInt32(XC_ENGINE_VARIANTS_SECONDARY.rawValue))
    static let variantsAll = Self(rawValue: UInt32(XC_ENGINE_VARIANTS_ALL.rawValue))
}
// ABI values match XiphosVariantReading; flag transitions stay in the core.
enum VariantReading: Int32, Sendable { case primary = 0, secondary = 1, all = 2 }
struct ModuleOptionInfo: Sendable {
    let declared: EngineOptionFlags
    let hasHeadings: Bool
    let transliterationAvailable: Bool
    static let unknown = Self(declared: [], hasHeadings: false, transliterationAvailable: false)
}
// Nil carries the core's unknown content state; no display defaults live here.
struct ContentDisplayOptions: Sendable, Equatable {
    let images: Bool?, fontFaces: Bool?, doubleSpace: Bool?, noteNumbers: Bool?
    init(images: Bool?, fontFaces: Bool?, doubleSpace: Bool? = nil, noteNumbers: Bool? = nil) {
        self.images = images; self.fontFaces = fontFaces; self.doubleSpace = doubleSpace; self.noteNumbers = noteNumbers
    }
    static let unknown = Self(images: nil, fontFaces: nil)
}
struct InstalledModule: Identifiable, Sendable {
    let id: String
    let description: String
    let type: String
    let language: String
    let options: ModuleOptionInfo
    let isDictionary: Bool
    let isRegularCommentary: Bool
    let lexiconRoles: LexiconRoles
    let hasVerseSearch: Bool
    let hasBookTree: Bool
    init(id: String, description: String, type: String, language: String, options: ModuleOptionInfo = .unknown, isDictionary: Bool = false, isRegularCommentary: Bool = false, lexiconRoles: LexiconRoles = [], hasBookTree: Bool = false, hasVerseSearch: Bool = false) {
        self.id = id; self.description = description; self.type = type; self.language = language; self.options = options
        self.isDictionary = isDictionary; self.isRegularCommentary = isRegularCommentary; self.lexiconRoles = lexiconRoles; self.hasBookTree = hasBookTree; self.hasVerseSearch = hasVerseSearch
    }
}
struct LexiconRoles: OptionSet, Sendable {
    let rawValue: UInt32
    static let greekDefinition = Self(rawValue: UInt32(XIPHOS_LEXICON_GREEK_DEF.rawValue))
    static let hebrewDefinition = Self(rawValue: UInt32(XIPHOS_LEXICON_HEBREW_DEF.rawValue))
    static let greekMorphology = Self(rawValue: UInt32(XIPHOS_LEXICON_GREEK_PARSE.rawValue))
    static let hebrewMorphology = Self(rawValue: UInt32(XIPHOS_LEXICON_HEBREW_PARSE.rawValue))
}
enum LexiconOrigin: Int32, Sendable { case main = 0, dialog = 1 }
enum LexicalKind: Int32, Sendable { case strongs = 0, morphology = 1 }
struct LexiconPreferences: Sendable, Equatable {
    var greek: String?, hebrew: String?, greekMorphNT: String?, greekMorphOT: String?, hebrewMorph: String?
}
struct RenderedLexicalEntry: Sendable {
    let kind: LexicalKind
    let module: String, type: String, value: String, passage: String, sourceReference: String
    let entry: RenderedDictionaryEntry
}
// Owned values copied before releasing C results; no SDK handles cross actors.
struct BookNode: Sendable, Equatable, Identifiable {
    let offset: UInt64
    let path: String, title: String
    let hasChildren: Bool
    var id: UInt64 { offset }
}
struct BookTree: Sendable, Equatable {
    let module: String
    let node: BookNode
    let parent: BookNode?, firstChild: BookNode?, previousSibling: BookNode?, nextSibling: BookNode?
}
struct RenderedBookEntry: Sendable, Equatable {
    let tree: BookTree
    let html: String
    let hasEntry: Bool
}
struct RenderedBookDocumentSection: Sendable, Equatable {
    let entry: RenderedBookEntry
    let depth: UInt32
    let rendered: Bool
}
struct RenderedBookImageIssue: Sendable, Equatable { let source: String; let message: String }
struct RenderedBookInlineImages: Sendable, Equatable {
    let html: String
    let embedded: Int
    let issues: [RenderedBookImageIssue]
}
struct RenderedBookStylesheet: Sendable, Equatable { let filename: String; let css: String; let inlineCSS: String }
struct RenderedBookPresentation: Sendable, Equatable {
    let html: String
    let lexical: RenderedLexicalPresentation
    let options: ContentDisplayOptions
    let cleanupDiagnostics: [RenderedCleanupDiagnostic]
    var inlineImages: RenderedBookInlineImages? = nil
    var stylesheet: RenderedBookStylesheet? = nil
    var stylesheetIssue: String? = nil
    var noteNumbers: Bool? = nil
}
struct RenderedBookNoteSource: Sendable, Equatable { let reportedLabel: String; let lastRenderedOffset: UInt64? }
struct RenderedBookDocument: Sendable, Equatable {
    let tree: BookTree
    let displayLevel: UInt32
    let sections: [RenderedBookDocumentSection]
    let html: String
    let presentation: RenderedBookPresentation?
    var noteSource: RenderedBookNoteSource? = nil
    var displayHTML: String { presentation?.html ?? html }
    var lexical: RenderedLexicalPresentation { presentation?.lexical ?? .plain }
    var resourceHTML: String { presentation?.inlineImages?.html ?? displayHTML }
    var resourceCSS: String { lexical.css + "\n" + (presentation?.stylesheet?.inlineCSS ?? "") }
}
enum CoreSearchMode: Int32, Sendable { case regularExpression=0, phrase=1, words=2, lemma=3, morphology=4, footnote=5, multiword=6, indexed=7 }
enum CoreSearchPhase: Int32, Sendable { case queued = 0, running, complete, cancelled, failed }
struct CoreSearchProgress: Sendable, Equatable { let phase: CoreSearchPhase; let percent: UInt32; let cancelRequested: Bool }
struct CoreSearchTextOptions: Sendable, Equatable { var strongs = false; var morphology = false; var footnotes = false }
struct CoreSearchScope: Sendable, Equatable { let present: Bool; let elementCount: UInt64 }
struct CoreSearchRange: Sendable, Equatable { let module: String; let expression: String }
// IDs are ephemeral within the originating live worker/tab. Keep that origin
// with a selection; never substitute the latest ID after a stale response.
enum CoreSearchRetention: Int32, Sendable { case none=0, complete=1, capacityExceeded=2 }
struct CoreSearchGeneration: Sendable, Equatable {
    let id: UInt64
    let retention: CoreSearchRetention
    let requestedCount: Int, groupCount: Int, failedGroups: Int, visibleResults: Int
    let matchedElements: UInt64, retainedElements: UInt64
    let accountedKeyBytes: Int
    let cancelled: Bool
    static let empty = CoreSearchGeneration(id: 0, retention: .none, requestedCount: 0, groupCount: 0,
        failedGroups: 0, visibleResults: 0, matchedElements: 0, retainedElements: 0, accountedKeyBytes: 0, cancelled: false)
}
struct CoreSearchRetentionLimits: Sendable, Equatable {
    var maxElements = 100_000
    var maxKeyBytes = 16 * 1024 * 1024
}
enum CoreSearchReferenceSource: Int32, Sendable { case visible=0, complete=1 }
enum CoreSearchReadStatus: Int32, Sendable { case success=0, stale=1, invalidGroup=2, notAttempted=3, failedGroup=4, capacityExceeded=5 }
struct CoreSearchRequestedQuery: Sendable, Equatable {
    let module: String, expression: String
    let mode: CoreSearchMode
    let caseSensitive: Bool, withinLastResults: Bool
    let range: CoreSearchRange?
    let textOptions: CoreSearchTextOptions?
}
struct RenderedSearchGenerationRead: Sendable, Equatable {
    let status: CoreSearchReadStatus
    let generation: CoreSearchGeneration
    let query: CoreSearchRequestedQuery?
    let failure: CoreSearchGroupStatus
    let result: RenderedSearchResult?
}
struct CoreSearchVisibleRow: Sendable, Equatable {let group: Int, row: Int}
enum CoreSearchExportStatus: Int32, Sendable {case success=0, stale=1, invalidSelection=2, outputLimit=3, cancelled=4, moduleNotFound=5, engineError=6, capacityExceeded=7, sourceIncomplete=8, unsupportedHTML=9}
enum CoreSearchExportSelection: Int32, Sendable {case visible=0, allRetained=1}
enum CoreHTMLExportIssue: Int32, Sendable {case none=0, resources=1, markup=2, styles=3, links=4}
struct RenderedSearchHTMLExport: Sendable, Equatable {
    let selection: CoreSearchExportSelection
    let status: CoreSearchExportStatus
    let generation: CoreSearchGeneration
    let rowCount: Int
    let failedRow: Int?
    let includesScripture: Bool
    let issue: CoreHTMLExportIssue
    let html: String
}
struct RenderedSearchTextExport: Sendable, Equatable {
    let selection: CoreSearchExportSelection
    let status: CoreSearchExportStatus
    let generation: CoreSearchGeneration
    let rowCount: Int
    let failedRow: Int?
    let includesScripture: Bool
    let text: String
}
struct RenderedSearchResult: Sendable, Equatable {
    let module: String, expression: String, effectiveExpression: String
    let mode: CoreSearchMode
    let engineType: Int32
    let caseSensitive: Bool, limited: Bool, cancelled: Bool, executed: Bool
    let totalResults: UInt64
    let references: [String]
    var range: CoreSearchRange? = nil
    var withinLastResults = false
    var scope = CoreSearchScope(present: false, elementCount: 0)
    var scopeUpdated = false
    var requestedTextOptions: CoreSearchTextOptions? = nil
    var effectiveTextOptions: CoreSearchTextOptions? = nil
    var textOptionsUpdated = false
    var indexFallback = false
    var generation = CoreSearchGeneration.empty
    var generationUpdated = false
}
enum CoreSearchGroupStatus: UInt32, Sendable { case success=0, invalidRequest=1, moduleNotFound=2, engineError=3 }
struct RenderedSearchGroup: Sendable, Equatable {
    let requestIndex: Int
    let status: CoreSearchGroupStatus
    let result: RenderedSearchResult?
}
struct RenderedSearchBatch: Sendable, Equatable {
    let requestedModules: [String]
    let groups: [RenderedSearchGroup]
    let copiedResultCount: Int, copiedReferenceBytes: Int, failureCount: Int
    let limited: Bool, cancelled: Bool, scopeUpdated: Bool, textOptionsUpdated: Bool
    let scope: CoreSearchScope
    let textOptions: CoreSearchTextOptions?
    var generation = CoreSearchGeneration.empty
    var generationUpdated = false
}
struct RenderedBookImage: Sendable, Equatable {
    let module: String, filename: String, mediaType: String
    let data: Data
}
enum RenderedBookLink: Sendable { case note(RenderedBookNote), image(RenderedBookImage) }
struct RenderedBookNote: Sendable {
    let tree: BookTree
    let content: RenderedStudyNote
}
struct BookChildren: Sendable, Equatable {
    let module: String
    let parent: BookNode
    let nodes: [BookNode]
    let nextAfter: UInt64?
}
enum BookNavigation: Int32, Sendable { case parent = 0, firstChild = 1, previousSibling = 2, nextSibling = 3 }
enum BookRestoreMode: Int32, Sendable { case displayKey = 0, initialWindow = 1 }
struct SavedBookSelection: Sendable {
    var module: String?, key: String?, offsetText: String?
}
struct DictionaryChoices: Sendable, Equatable { let module: String; let key: String; let keys: [String] }
struct RenderedDictionaryEntry: Sendable { let module: String; let key: String; let html: String }
struct RenderedReferenceList: Sendable {
    let module: String
    let context: String
    let expression: String
    let entries: [RenderedEntry]
    let html: String
    let limited: Bool
    let skipped: UInt32
}
struct RenderedStudyNote: Sendable {
    let module: String
    let reference: String
    let identifier: String
    let type: String
    let label: String
    let html: String
    let referenceList: String
}
struct RenderedStudyPassage: Sendable {
    let module: String
    let commentaryModule: String
    let text: RenderedEntry
    let commentary: RenderedEntry?
}
struct RenderedCleanupDiagnostic: Sendable, Equatable {
    enum Kind: Int32, Sendable { case image = 0, fontFace = 1 }
    let kind: Kind
    let suffix: String
}
struct RenderedLexicalPresentation: Sendable, Equatable {
    let css: String
    let requested: Bool
    let applied: Bool
    static let plain = Self(css: "", requested: false, applied: false)
}
struct RenderedEntry: Sendable {
    let reference: String
    let html: String
    let headingHTML: String
    let cleanupDiagnostics: [RenderedCleanupDiagnostic]
    let lexical: RenderedLexicalPresentation
    init(reference: String, html: String, headingHTML: String = "", cleanupDiagnostics: [RenderedCleanupDiagnostic] = [], lexical: RenderedLexicalPresentation = .plain) {
        self.reference = reference; self.html = html; self.headingHTML = headingHTML
        self.cleanupDiagnostics = cleanupDiagnostics
        self.lexical = lexical
    }
}
struct RenderedChapter: Sendable {
    let reference: String
    let title: String
    let verses: [RenderedEntry]
    let verseOrdinals: [Int]
    let selectedVerse: Int
    let selectedAnchor: String
    let html: String
    let lexical: RenderedLexicalPresentation
    init(reference: String, title: String, verses: [RenderedEntry], verseOrdinals: [Int], selectedVerse: Int, selectedAnchor: String, html: String, lexical: RenderedLexicalPresentation = .plain) {
        self.reference = reference; self.title = title; self.verses = verses; self.verseOrdinals = verseOrdinals
        self.selectedVerse = selectedVerse; self.selectedAnchor = selectedAnchor; self.html = html; self.lexical = lexical
    }
}
struct ReferenceChoice: Sendable { let label: String; let reference: String }
struct ReferenceChoices: Sendable {
    let books: [ReferenceChoice]; let chapters: [ReferenceChoice]; let verses: [ReferenceChoice]
    let bookLabel: String; let chapterLabel: String; let verseLabel: String
}
struct PassageTarget: Sendable { let module: String; let reference: String }
enum ContextualStudyLink: Sendable {
    case passage(PassageTarget)
    case note(RenderedStudyNote)
    case references(RenderedReferenceList)
    case lexical(RenderedLexicalEntry)
}
enum HoveredStudyLink: Sendable {
    case noPresentation
    case note(RenderedStudyNote)
    case references(RenderedReferenceList)
    case lexical(RenderedLexicalEntry)
}
enum ResolvedStudyLink: Sendable {
    case passage(PassageTarget)
    case note(RenderedStudyNote)
}
struct CoreError: LocalizedError, Sendable {
    let status: xc_status
    var errorDescription: String? {
        switch status {
        case XC_INVALID_ARGUMENT: return "Check the module folder and reference. This preview supports verse-key modules."
        case XC_MODULE_NOT_FOUND: return "The selected module is not installed."
        default: return "The SWORD engine could not complete the request."
        }
    }
}
// Presentation adapter only: no SWORD manager, canon tables, navigation or
// rendering policy in Swift. Use this instance serially, including destruction.
final class CoreReader {
    fileprivate let handle: OpaquePointer
    let modules: [InstalledModule]
    init(root: String? = nil) throws {
        var opened: OpaquePointer?
        let status: xc_status
        if let root { status = root.withCString { xc_open($0, &opened) } }
        else { status = xc_open(nil, &opened) }
        guard status == XC_OK, let opened else { throw CoreError(status: status) }
        handle = opened
        modules = (0..<xc_module_count(opened)).map { index in
            InstalledModule(id: String(cString: xc_module_id(opened, index)!),
                description: String(cString: xc_module_description(opened, index)!),
                type: String(cString: xc_module_type(opened, index)!),
                language: String(cString: xc_module_language(opened, index)!),
                isDictionary: xc_module_is_dictionary(opened, index) != 0,
                isRegularCommentary: xc_module_is_regular_commentary(opened, index) != 0,
                lexiconRoles: LexiconRoles(rawValue: xc_module_lexicon_roles(opened, index)),
                hasBookTree: xc_module_has_book_tree(opened, index) != 0,
                hasVerseSearch: xc_module_has_verse_search(opened, index) != 0)
        }
    }
    deinit { xc_close(handle) }
    func referenceChoices(module: String, reference: String) throws -> ReferenceChoices {
        var result: OpaquePointer?
        let status = module.withCString { id in
            reference.withCString { xc_reference_choices(handle, id, $0, &result) }
        }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_choices_release(result) }
        return CoreValues.choices(result)
    }
    func resolveLink(_ uri: String, currentModule: String) throws -> PassageTarget {
        var result: OpaquePointer?
        let status = currentModule.withCString { module in
            uri.withCString { xc_resolve_link(handle, module, $0, &result) }
        }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_link_release(result) }
        return CoreValues.link(result)
    }
    func readChapter(module: String, reference: String, verseNumbers: Bool = true, highlightSelected: Bool = true) throws -> RenderedChapter {
        var result: OpaquePointer?
        let flags = (verseNumbers ? UInt32(XC_CHAPTER_VERSE_NUMBERS) : 0) |
                    (highlightSelected ? UInt32(XC_CHAPTER_HIGHLIGHT_SELECTED) : 0)
        let status = module.withCString { id in
            reference.withCString { ref in xc_read_chapter_with_options(handle, id, ref, flags, &result) }
        }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_chapter_release(result) }
        return CoreValues.chapter(result)
    }
    func read(module: String, reference: String) throws -> RenderedEntry {
        var result: OpaquePointer?
        let status = module.withCString { id in
            reference.withCString { key in xc_read(handle, id, key, &result) }
        }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_entry_release(result) }
        return CoreValues.entry(result)
    }
}

// One session per tab/window. All navigation/history policy lives in C++.
final class CoreSession {
    private let handle: OpaquePointer
    init(reader: CoreReader) throws {
        var opened: OpaquePointer?
        let status = xc_session_open(reader.handle, &opened)
        guard status == XC_OK, let opened else { throw CoreError(status: status) }
        handle = opened
    }
    deinit { xc_session_close(handle) }
    var canBack: Bool { xc_session_can_back(handle) != 0 }
    var canForward: Bool { xc_session_can_forward(handle) != 0 }
    var historyCount: Int { xc_session_history_count(handle) }
    var historyModule: String? { xc_session_module(handle).map { String(cString: $0) } }
    func clearHistory() { xc_session_clear_history(handle) }
    private func copyResult(_ status: xc_status, _ result: OpaquePointer?) throws -> RenderedEntry {
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_entry_release(result) }
        return CoreValues.entry(result)
    }
    func visit(module: String, reference: String) throws -> RenderedEntry {
        var result: OpaquePointer?
        let status = module.withCString { id in
            reference.withCString { ref in xc_session_visit(handle, id, ref, &result) }
        }
        return try copyResult(status, result)
    }
    func navigateChapter(module: String, reference: String, direction: Int32) throws -> RenderedEntry {
        var result: OpaquePointer?
        let status = module.withCString { id in
            reference.withCString { ref in xc_session_navigate_chapter(handle, id, ref, direction, &result) }
        }
        return try copyResult(status, result)
    }
    func navigate(_ direction: Int32) throws -> RenderedEntry {
        var result: OpaquePointer?
        let status = xc_session_navigate(handle, direction, &result)
        return try copyResult(status, result)
    }
}

// Shared copying helpers for synchronous and asynchronous adapters. They add
// no canon, display, navigation or history policy to either Swift interface.
enum CoreValues {
    static func bookNode(_ value: xc_book_node) -> BookNode? {
        guard value.present != 0 else { return nil }
        return BookNode(offset: value.offset, path: String(cString: value.path!),
                        title: String(cString: value.title!), hasChildren: value.has_children != 0)
    }
    static func bookTree(_ result: OpaquePointer) -> BookTree {
        func node(_ kind: xc_book_node_kind) -> BookNode? {
            var value = xc_book_node()
            precondition(xc_book_tree_get_node(result, Int32(kind.rawValue), &value) == XC_OK)
            return bookNode(value)
        }
        return BookTree(module: String(cString: xc_book_tree_module(result)!), node: node(XC_BOOK_CURRENT)!,
            parent: node(XC_BOOK_PARENT), firstChild: node(XC_BOOK_FIRST_CHILD),
            previousSibling: node(XC_BOOK_PREVIOUS_SIBLING), nextSibling: node(XC_BOOK_NEXT_SIBLING))
    }
    static func bookEntry(_ result: OpaquePointer) -> RenderedBookEntry {
        RenderedBookEntry(tree: bookTree(xc_book_entry_tree(result)!),
            html: String(cString: xc_book_entry_html(result)!), hasEntry: xc_book_entry_has_entry(result) != 0)
    }
    static func searchTextOptions(_ info: xc_search_text_info) -> CoreSearchTextOptions? {
        info.present == 0 ? nil : CoreSearchTextOptions(strongs: info.strongs != 0, morphology: info.morphology != 0, footnotes: info.footnotes != 0)
    }
    static func searchGeneration(_ info: xc_search_generation_info) -> CoreSearchGeneration {
        guard let retention = CoreSearchRetention(rawValue: info.retention) else { preconditionFailure("Unknown retention status") }
        return CoreSearchGeneration(id: info.id, retention: retention, requestedCount: info.requested_count,
            groupCount: info.group_count, failedGroups: info.failed_groups, visibleResults: info.visible_results,
            matchedElements: info.matched_elements, retainedElements: info.retained_elements,
            accountedKeyBytes: info.accounted_key_bytes, cancelled: info.cancelled != 0)
    }
    static func search(_ result: OpaquePointer) -> RenderedSearchResult {
        var info = xc_search_info()
        precondition(xc_search_get_info(result, &info) == XC_OK)
        guard let mode = CoreSearchMode(rawValue: info.mode) else { preconditionFailure("Unknown core search mode") }
        var scope = xc_search_scope_info()
        precondition(xc_search_get_scope(result, &scope) == XC_OK)
        var requestedText = xc_search_text_info(), effectiveText = xc_search_text_info()
        precondition(xc_search_requested_text_options(result, &requestedText) == XC_OK)
        precondition(xc_search_effective_text_options(result, &effectiveText) == XC_OK)
        var generation = xc_search_generation_info()
        precondition(xc_search_get_generation(result, &generation) == XC_OK)
        return RenderedSearchResult(module: String(cString: xc_search_module(result)!),
            expression: String(cString: xc_search_expression(result)!),
            effectiveExpression: String(cString: xc_search_effective_expression(result)!),
            mode: mode, engineType: info.engine_type, caseSensitive: info.case_sensitive != 0,
            limited: info.limited != 0, cancelled: info.cancelled != 0, executed: info.executed != 0, totalResults: info.total_results,
            references: (0..<info.result_count).map { String(cString: xc_search_reference(result, $0)!) },
            range: xc_search_range_module(result).map { CoreSearchRange(module: String(cString: $0),
                expression: String(cString: xc_search_range_expression(result)!)) },
            withinLastResults: xc_search_within_last_results(result) == 1,
            scope: CoreSearchScope(present: scope.present != 0, elementCount: scope.element_count),
            scopeUpdated: xc_search_scope_updated(result) == 1,
            requestedTextOptions: searchTextOptions(requestedText), effectiveTextOptions: searchTextOptions(effectiveText),
            textOptionsUpdated: xc_search_text_options_updated(result) == 1, indexFallback: xc_search_index_fallback(result) == 1,
            generation: searchGeneration(generation), generationUpdated: xc_search_generation_updated(result) == 1)
    }
    static func searchBatch(_ batch: OpaquePointer) -> RenderedSearchBatch {
        var info = xc_search_batch_info(), scope = xc_search_scope_info(), text = xc_search_text_info()
        precondition(xc_search_batch_get_info(batch, &info) == XC_OK)
        precondition(xc_search_batch_get_scope(batch, &scope) == XC_OK)
        precondition(xc_search_batch_get_text_options(batch, &text) == XC_OK)
        let groups = (0..<info.group_count).map { index in
            var group = xc_search_batch_group_info()
            precondition(xc_search_batch_get_group(batch, index, &group) == XC_OK)
            guard let status = CoreSearchGroupStatus(rawValue: group.status.rawValue) else { preconditionFailure("Unknown batch group status") }
            let copied = xc_search_batch_group_result(batch, index).map { search($0) }
            precondition((copied != nil) == (group.result_present != 0))
            return RenderedSearchGroup(requestIndex: group.request_index, status: status, result: copied)
        }
        var generation = xc_search_generation_info()
        precondition(xc_search_batch_get_generation(batch, &generation) == XC_OK)
        return RenderedSearchBatch(
            requestedModules: (0..<info.requested_count).map { String(cString: xc_search_batch_requested_module(batch, $0)!) },
            groups: groups, copiedResultCount: info.result_count, copiedReferenceBytes: info.reference_bytes,
            failureCount: info.failure_count, limited: info.limited != 0, cancelled: info.cancelled != 0,
            scopeUpdated: info.scope_updated != 0, textOptionsUpdated: info.text_options_updated != 0,
            scope: CoreSearchScope(present: scope.present != 0, elementCount: scope.element_count),
            textOptions: searchTextOptions(text), generation: searchGeneration(generation),
            generationUpdated: xc_search_batch_generation_updated(batch) == 1)
    }
    static func bookImage(_ result: OpaquePointer) -> RenderedBookImage {
        let source = xc_book_image_data_source(result)!
        let count = xc_book_image_data_size(result)
        let data = count == 0 ? Data() : Data(bytes: xc_book_image_data_bytes(result)!, count: count)
        return RenderedBookImage(module: String(cString: xc_book_image_module(source)!),
            filename: String(cString: xc_book_image_filename(source)!), mediaType: String(cString: xc_book_image_media_type(source)!), data: data)
    }
    static func bookDocument(_ result: OpaquePointer) -> RenderedBookDocument {
        let sections = (0..<xc_book_document_count(result)).map { index in
            var value = xc_book_document_section()
            precondition(xc_book_document_get_section(result, index, &value) == XC_OK)
            return RenderedBookDocumentSection(entry: bookEntry(value.entry!), depth: value.depth, rendered: value.rendered != 0)
        }
        let presentation = xc_book_document_presentation_html(result).map { html in
            var options = xc_book_document_display_options()
            precondition(xc_book_document_get_display_options(result, &options) == XC_OK &&
                (-1...1).contains(options.images) && (-1...1).contains(options.font_faces) && (0...1).contains(options.double_space))
            let numbers = xc_book_document_note_numbers(result)
            precondition((-1...1).contains(numbers))
            return RenderedBookPresentation(html: String(cString: html),
                lexical: lexicalLayout { xc_book_document_lexical_layout(result, $0) },
                options: ContentDisplayOptions(images: options.images == -1 ? nil : options.images != 0,
                    fontFaces: options.font_faces == -1 ? nil : options.font_faces != 0, doubleSpace: options.double_space != 0),
                cleanupDiagnostics: cleanupDiagnostics(count: xc_book_document_cleanup_count(result),
                    kind: { xc_book_document_cleanup_kind(result, $0) }, suffix: { xc_book_document_cleanup_suffix(result, $0) }),
                inlineImages: xc_book_document_inline_html(result).map { resourceHTML in
                    RenderedBookInlineImages(html: String(cString: resourceHTML),
                        embedded: xc_book_document_inline_image_count(result),
                        issues: (0..<xc_book_document_inline_issue_count(result)).map { index in
                            RenderedBookImageIssue(source: String(cString: xc_book_document_inline_issue_source(result, index)!),
                                message: String(cString: xc_book_document_inline_issue_message(result, index)!))
                        })
                },
                stylesheet: xc_book_document_stylesheet_css(result).map { css in
                    RenderedBookStylesheet(filename: String(cString: xc_book_document_stylesheet_filename(result)!),
                        css: String(cString: css), inlineCSS: String(cString: xc_book_document_stylesheet_inline_css(result)!))
                },
                stylesheetIssue: xc_book_document_stylesheet_issue(result).map { String(cString: $0) },
                noteNumbers: numbers == -1 ? nil : numbers != 0)
        }
        return RenderedBookDocument(tree: bookTree(xc_book_document_tree(result)!),
            displayLevel: xc_book_document_display_level(result), sections: sections,
            html: String(cString: xc_book_document_html(result)!), presentation: presentation,
            noteSource: xc_book_document_note_source_label(result).map { label in
                var offset: UInt64 = 0
                let status = xc_book_document_note_source_offset(result, &offset)
                return RenderedBookNoteSource(reportedLabel: String(cString: label), lastRenderedOffset: status == XC_OK ? offset : nil)
            })
    }
    static func withSavedBookSelection<R>(_ saved: SavedBookSelection, _ body: (UnsafePointer<xc_saved_book_selection>) -> R) throws -> R {
        guard ![saved.module, saved.key, saved.offsetText].compactMap({ $0 }).contains(where: { $0.utf8.contains(0) }) else {
            throw CoreError(status: XC_INVALID_ARGUMENT)
        }
        return withOptionalCString(saved.module) { module in
            withOptionalCString(saved.key) { key in
                withOptionalCString(saved.offsetText) { offset in
                    var value = xc_saved_book_selection(module: module, key: key, offset_text: offset)
                    return body(&value)
                }
            }
        }
    }
    static func lexicalLayout(_ load: (UnsafeMutablePointer<xc_lexical_presentation>) -> xc_status) -> RenderedLexicalPresentation {
        var value = xc_lexical_presentation()
        let status = load(&value)
        precondition(status == XC_OK && value.css != nil, "Owned snapshot must expose matching layout metadata")
        return RenderedLexicalPresentation(css: String(cString: value.css!), requested: value.requested != 0, applied: value.applied != 0)
    }
    static func cleanupDiagnostics(count: Int, kind: (Int) -> Int32, suffix: (Int) -> UnsafePointer<CChar>?) -> [RenderedCleanupDiagnostic] {
        (0..<count).map { index in
            RenderedCleanupDiagnostic(kind: .init(rawValue: kind(index))!, suffix: String(cString: suffix(index)!))
        }
    }

    static func entry(_ result: OpaquePointer) -> RenderedEntry {
        RenderedEntry(reference: String(cString: xc_entry_reference(result)!),
                      html: String(cString: xc_entry_html(result)!),
                      headingHTML: String(cString: xc_entry_heading_html(result)!),
                      cleanupDiagnostics: cleanupDiagnostics(count: xc_entry_cleanup_count(result),
                          kind: { xc_entry_cleanup_kind(result, $0) }, suffix: { xc_entry_cleanup_suffix(result, $0) }),
                      lexical: lexicalLayout { xc_entry_lexical_layout(result, $0) })
    }
    static func link(_ result: OpaquePointer) -> PassageTarget {
        PassageTarget(module: String(cString: xc_link_module(result)!),
                      reference: String(cString: xc_link_reference(result)!))
    }
    static func withOptionalCString<R>(_ value: String?, _ body: (UnsafePointer<CChar>?) -> R) -> R {
        guard let value else { return body(nil) }
        return value.withCString { body($0) }
    }
    static func withLexiconNames<R>(_ preferences: LexiconPreferences, _ body: (UnsafePointer<xc_lexicon_names>) -> R) -> R {
        withOptionalCString(preferences.greek) { greek in
            withOptionalCString(preferences.hebrew) { hebrew in
                withOptionalCString(preferences.greekMorphNT) { nt in
                    withOptionalCString(preferences.greekMorphOT) { ot in
                        withOptionalCString(preferences.hebrewMorph) { hm in
                            var names = xc_lexicon_names(greek: greek, hebrew: hebrew, greek_morph_nt: nt, greek_morph_ot: ot, hebrew_morph: hm)
                            return body(&names)
                        }
                    }
                }
            }
        }
    }
    static func lexicons(_ result: OpaquePointer) -> LexiconPreferences? {
        guard xc_lexicons_configured(result) != 0 else { return nil }
        return LexiconPreferences(greek: xc_lexicons_greek(result).map { String(cString: $0) },
            hebrew: xc_lexicons_hebrew(result).map { String(cString: $0) },
            greekMorphNT: xc_lexicons_greek_morph_nt(result).map { String(cString: $0) },
            greekMorphOT: xc_lexicons_greek_morph_ot(result).map { String(cString: $0) },
            hebrewMorph: xc_lexicons_hebrew_morph(result).map { String(cString: $0) })
    }
    static func lexicalEntry(_ result: OpaquePointer) -> RenderedLexicalEntry {
        RenderedLexicalEntry(kind: LexicalKind(rawValue: xc_lexical_entry_kind(result))!,
            module: String(cString: xc_lexical_entry_module(result)!), type: String(cString: xc_lexical_entry_type(result)!),
            value: String(cString: xc_lexical_entry_value(result)!), passage: String(cString: xc_lexical_entry_passage(result)!),
            sourceReference: String(cString: xc_lexical_entry_source_reference(result)!),
            entry: RenderedDictionaryEntry(module: String(cString: xc_lexical_entry_dictionary_module(result)!),
                key: String(cString: xc_lexical_entry_dictionary_key(result)!), html: String(cString: xc_lexical_entry_dictionary_html(result)!)))
    }
    static func dictionaryEntry(_ result: OpaquePointer) -> RenderedDictionaryEntry {
        RenderedDictionaryEntry(module: String(cString: xc_dictionary_entry_module(result)!),
            key: String(cString: xc_dictionary_entry_key(result)!), html: String(cString: xc_dictionary_entry_html(result)!))
    }
    static func referenceList(_ result: OpaquePointer) -> RenderedReferenceList {
        RenderedReferenceList(module: String(cString: xc_reference_list_module(result)!),
            context: String(cString: xc_reference_list_context(result)!), expression: String(cString: xc_reference_list_expression(result)!),
            entries: (0..<xc_reference_list_count(result)).map { index in
                RenderedEntry(reference: String(cString: xc_reference_list_entry_reference(result, index)!),
                              html: String(cString: xc_reference_list_entry_html(result, index)!))
            }, html: String(cString: xc_reference_list_html(result)!), limited: xc_reference_list_limited(result) != 0,
            skipped: xc_reference_list_skipped(result))
    }
    static func chapter(_ result: OpaquePointer) -> RenderedChapter {
        return RenderedChapter(reference: String(cString: xc_chapter_reference(result)!),
            title: String(cString: xc_chapter_title(result)!),
            verses: (0..<xc_chapter_count(result)).map { index in
                RenderedEntry(reference: String(cString: xc_chapter_verse_reference(result, index)!),
                              html: String(cString: xc_chapter_verse_html(result, index)!),
                              headingHTML: String(cString: xc_chapter_verse_heading_html(result, index)!),
                              cleanupDiagnostics: cleanupDiagnostics(count: xc_chapter_verse_cleanup_count(result, index),
                                  kind: { xc_chapter_verse_cleanup_kind(result, index, $0) }, suffix: { xc_chapter_verse_cleanup_suffix(result, index, $0) }),
                              lexical: lexicalLayout { xc_chapter_verse_lexical_layout(result, index, $0) })
            }, verseOrdinals: (0..<xc_chapter_count(result)).map { Int(xc_chapter_verse_ordinal(result, $0)) },
            selectedVerse: xc_chapter_selected_verse(result),
            selectedAnchor: String(cString: xc_chapter_selected_anchor(result)!),
            html: String(cString: xc_chapter_html(result)!),
            lexical: lexicalLayout { xc_chapter_lexical_layout(result, $0) })
    }
    static func choices(_ result: OpaquePointer) -> ReferenceChoices {
        func choices(_ kind: xc_choice_kind) -> [ReferenceChoice] {
            (0..<xc_choices_count(result, kind)).map { index in
                ReferenceChoice(label: String(cString: xc_choices_label(result, kind, index)!),
                                reference: String(cString: xc_choices_reference(result, kind, index)!))
            }
        }
        return ReferenceChoices(books: choices(XC_BOOK), chapters: choices(XC_CHAPTER), verses: choices(XC_VERSE),
            bookLabel: String(cString: xc_choices_current_label(result, XC_BOOK)!),
            chapterLabel: String(cString: xc_choices_current_label(result, XC_CHAPTER)!),
            verseLabel: String(cString: xc_choices_current_label(result, XC_VERSE)!))
    }
}
