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
    let hasBookTree: Bool
    init(id: String, description: String, type: String, language: String, options: ModuleOptionInfo = .unknown, isDictionary: Bool = false, isRegularCommentary: Bool = false, lexiconRoles: LexiconRoles = [], hasBookTree: Bool = false) {
        self.id = id; self.description = description; self.type = type; self.language = language; self.options = options
        self.isDictionary = isDictionary; self.isRegularCommentary = isRegularCommentary; self.lexiconRoles = lexiconRoles; self.hasBookTree = hasBookTree
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
struct RenderedBookPresentation: Sendable, Equatable {
    let html: String
    let lexical: RenderedLexicalPresentation
    let options: ContentDisplayOptions
    let cleanupDiagnostics: [RenderedCleanupDiagnostic]
}
struct RenderedBookDocument: Sendable, Equatable {
    let tree: BookTree
    let displayLevel: UInt32
    let sections: [RenderedBookDocumentSection]
    let html: String
    let presentation: RenderedBookPresentation?
    var displayHTML: String { presentation?.html ?? html }
    var lexical: RenderedLexicalPresentation { presentation?.lexical ?? .plain }
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
                hasBookTree: xc_module_has_book_tree(opened, index) != 0)
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
            return RenderedBookPresentation(html: String(cString: html),
                lexical: lexicalLayout { xc_book_document_lexical_layout(result, $0) },
                options: ContentDisplayOptions(images: options.images == -1 ? nil : options.images != 0,
                    fontFaces: options.font_faces == -1 ? nil : options.font_faces != 0, doubleSpace: options.double_space != 0),
                cleanupDiagnostics: cleanupDiagnostics(count: xc_book_document_cleanup_count(result),
                    kind: { xc_book_document_cleanup_kind(result, $0) }, suffix: { xc_book_document_cleanup_suffix(result, $0) }))
        }
        return RenderedBookDocument(tree: bookTree(xc_book_document_tree(result)!),
            displayLevel: xc_book_document_display_level(result), sections: sections,
            html: String(cString: xc_book_document_html(result)!), presentation: presentation)
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
