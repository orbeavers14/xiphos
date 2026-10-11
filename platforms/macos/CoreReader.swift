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
struct InstalledModule: Identifiable, Sendable {
    let id: String
    let description: String
    let type: String
    let language: String
    let options: ModuleOptionInfo
    init(id: String, description: String, type: String, language: String, options: ModuleOptionInfo = .unknown) {
        self.id = id; self.description = description; self.type = type; self.language = language; self.options = options
    }
}
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
struct RenderedEntry: Sendable {
    let reference: String
    let html: String
}
struct RenderedChapter: Sendable {
    let reference: String
    let title: String
    let verses: [RenderedEntry]
    let selectedVerse: Int
    let selectedAnchor: String
    let html: String
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
                language: String(cString: xc_module_language(opened, index)!))
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
    static func entry(_ result: OpaquePointer) -> RenderedEntry {
        RenderedEntry(reference: String(cString: xc_entry_reference(result)!),
                      html: String(cString: xc_entry_html(result)!))
    }
    static func link(_ result: OpaquePointer) -> PassageTarget {
        PassageTarget(module: String(cString: xc_link_module(result)!),
                      reference: String(cString: xc_link_reference(result)!))
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
                              html: String(cString: xc_chapter_verse_html(result, index)!))
            }, selectedVerse: xc_chapter_selected_verse(result),
            selectedAnchor: String(cString: xc_chapter_selected_anchor(result)!),
            html: String(cString: xc_chapter_html(result)!))
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
