// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import Darwin
import XiphosCore
struct EditorRun: Sendable {
    var text: String, flags: UInt32 = 0, block: Int32 = 0
    var foreground = "", background = "", link = ""
}
struct EditorRichDocument: Sendable { let runs: [EditorRun]; let text: String; let html: String }
struct EditorSource: Sendable { let text: String; let revision: String }
struct EditorModuleSource: Sendable {
    let module: String, reference: String, driver: String, sourceType: String, text: String, revision: String
}
enum EditorRichCodec {
    private static func copy(_ result: OpaquePointer) -> EditorRichDocument {
        let runs = (0..<xc_rich_document_run_count(result)).map { index in
            var run = xc_rich_run(); precondition(xc_rich_document_run(result, index, &run) == XC_OK)
            return EditorRun(text: String(cString: run.text!), flags: run.flags, block: run.block,
                foreground: String(cString: run.foreground!), background: String(cString: run.background!), link: String(cString: run.link!))
        }
        return EditorRichDocument(runs: runs, text: String(cString: xc_rich_document_text(result)!), html: String(cString: xc_rich_document_html(result)!))
    }
    static func parse(_ html: String) throws -> EditorRichDocument {
        guard !html.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var result: OpaquePointer?; let status = html.withCString { xc_rich_document_parse($0, &result) }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_rich_document_release(result) }; return copy(result)
    }
    private static func owned(_ runs: [EditorRun]) throws -> OpaquePointer {
        var strings: [UnsafeMutablePointer<CChar>] = []; defer { strings.forEach { free($0) } }
        var rows: [xc_rich_run] = []
        for r in runs where !r.text.isEmpty {
            guard [r.text,r.foreground,r.background,r.link].allSatisfy({ !$0.utf8.contains(0) }) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
            let copied = [r.text,r.foreground,r.background,r.link].compactMap { strdup($0) }
            strings.append(contentsOf: copied); guard copied.count == 4 else { throw CoreError(status: XC_ENGINE_ERROR) }
            rows.append(xc_rich_run(text: UnsafePointer(copied[0]), flags: r.flags, block: r.block,
                foreground: UnsafePointer(copied[1]), background: UnsafePointer(copied[2]), link: UnsafePointer(copied[3])))
        }
        var result: OpaquePointer?
        let status = rows.withUnsafeBufferPointer { xc_rich_document_create($0.baseAddress, $0.count, &result) }
        guard status == XC_OK, let result else { throw CoreError(status: status) }; return result
    }
    static func make(_ runs: [EditorRun]) throws -> EditorRichDocument {
        let result = try owned(runs); defer { xc_rich_document_release(result) }; return copy(result)
    }
    static func format(_ document: EditorRichDocument, selection: NSRange, action: Int32, value: Int32 = -1, text: String = "") throws -> EditorRichDocument {
        guard !text.utf8.contains(0), let range = Range(selection, in: document.text) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        let start = document.text[..<range.lowerBound].utf8.count, length = document.text[range].utf8.count
        let original = try owned(document.runs); defer { xc_rich_document_release(original) }
        var result: OpaquePointer?
        let status = text.withCString { xc_rich_document_format(original, start, length, action, value, $0, &result) }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_rich_document_release(result) }; return copy(result)
    }
}
final class EditorSourceBuffer {
    private let handle: OpaquePointer
    init(_ source: String) throws {
        guard !source.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }
        var result: OpaquePointer?; let status = source.withCString { xc_editor_buffer_create($0, &result) }
        guard status == XC_OK, let result else { throw CoreError(status: status) }; handle = result
    }
    deinit { xc_editor_buffer_release(handle) }
    var text: String { String(cString: xc_editor_buffer_text(handle)!) }
    var dirty: Bool { xc_editor_buffer_dirty(handle) != 0 }
    var canUndo: Bool { xc_editor_buffer_can_undo(handle) != 0 }
    var canRedo: Bool { xc_editor_buffer_can_redo(handle) != 0 }
    func replace(_ text: String) throws { guard !text.utf8.contains(0) else { throw CoreError(status: XC_INVALID_ARGUMENT) }; let status = text.withCString { xc_editor_buffer_replace(handle, $0) }; guard status == XC_OK else { throw CoreError(status: status) } }
    func undo() { _ = xc_editor_buffer_undo(handle) }; func redo() { _ = xc_editor_buffer_redo(handle) }
    func saved() { _ = xc_editor_buffer_mark_saved(handle) }
}
