// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore

struct InstalledModule: Identifiable {
    let id: String
    let description: String
    let type: String
    let language: String
}
struct RenderedEntry {
    let reference: String
    let html: String
}
struct RenderedChapter {
    let reference: String
    let title: String
    let verses: [RenderedEntry]
    let selectedVerse: Int
    let selectedAnchor: String
    let html: String
}
struct CoreError: LocalizedError {
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
    func readChapter(module: String, reference: String, verseNumbers: Bool = true, highlightSelected: Bool = true) throws -> RenderedChapter {
        var result: OpaquePointer?
        let flags = (verseNumbers ? UInt32(XC_CHAPTER_VERSE_NUMBERS) : 0) |
                    (highlightSelected ? UInt32(XC_CHAPTER_HIGHLIGHT_SELECTED) : 0)
        let status = module.withCString { id in
            reference.withCString { ref in xc_read_chapter_with_options(handle, id, ref, flags, &result) }
        }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_chapter_release(result) }
        return RenderedChapter(reference: String(cString: xc_chapter_reference(result)!),
            title: String(cString: xc_chapter_title(result)!),
            verses: (0..<xc_chapter_count(result)).map { index in
                RenderedEntry(reference: String(cString: xc_chapter_verse_reference(result, index)!),
                              html: String(cString: xc_chapter_verse_html(result, index)!))
            }, selectedVerse: xc_chapter_selected_verse(result),
            selectedAnchor: String(cString: xc_chapter_selected_anchor(result)!),
            html: String(cString: xc_chapter_html(result)!))
    }
    func read(module: String, reference: String) throws -> RenderedEntry {
        var result: OpaquePointer?
        let status = module.withCString { id in
            reference.withCString { key in xc_read(handle, id, key, &result) }
        }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_entry_release(result) }
        return RenderedEntry(reference: String(cString: xc_entry_reference(result)!),
                             html: String(cString: xc_entry_html(result)!))
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
        return RenderedEntry(reference: String(cString: xc_entry_reference(result)!),
                             html: String(cString: xc_entry_html(result)!))
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
