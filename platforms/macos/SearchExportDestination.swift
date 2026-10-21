// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import Foundation

enum SearchExportDestinationError: LocalizedError {
    case incompletePayload, clipboard, save
    var errorDescription: String? {
        switch self {
        case .incompletePayload: "No complete search export is available to save or copy."
        case .clipboard: "Unable to copy search results. Try again."
        case .save: "Unable to save search results. Choose a writable file location."
        }
    }
}
enum SearchExportFormat: String, CaseIterable, Identifiable, Sendable {
    case plainText = "Plain text", html = "HTML"
    var id: Self { self }
}
// A copied core document and its native destination format. No rendering,
// resource resolution or markup conversion belongs in this adapter.
enum SearchExportPayload: Sendable {
    case text(RenderedSearchTextExport), html(RenderedSearchHTMLExport)
    var format: SearchExportFormat { switch self { case .text: .plainText; case .html: .html } }
    var status: CoreSearchExportStatus { switch self { case .text(let value): value.status; case .html(let value): value.status } }
    var selection: CoreSearchExportSelection { switch self { case .text(let value): value.selection; case .html(let value): value.selection } }
    var contents: String { switch self { case .text(let value): value.text; case .html(let value): value.html } }
}
// Destination handling only. The shared core has already resolved selection,
// rendered scripture, assembled the document and checked aggregate output caps.
enum SearchExportDestination {
    @MainActor static func copy(_ payload: RenderedSearchTextExport, to pasteboard: NSPasteboard = .general) throws {
        try copy(.text(payload), to: pasteboard)
    }
    @MainActor static func copy(_ payload: SearchExportPayload, to pasteboard: NSPasteboard = .general) throws {
        guard payload.status == .success else { throw SearchExportDestinationError.incompletePayload }
        let item = NSPasteboardItem()
        guard item.setString(payload.contents, forType: payload.format == .html ? .html : .string) else { throw SearchExportDestinationError.clipboard }
        pasteboard.clearContents()
        guard pasteboard.writeObjects([item]) else { throw SearchExportDestinationError.clipboard }
    }
    static func save(_ payload: RenderedSearchTextExport, to url: URL) async throws {
        try await save(.text(payload), to: url)
    }
    static func save(_ payload: SearchExportPayload, to url: URL) async throws {
        guard payload.status == .success else { throw SearchExportDestinationError.incompletePayload }
        let data = Data(payload.contents.utf8)
        do {
            try await Task.detached(priority: .utility) { try data.write(to: url, options: .atomic) }.value
        } catch { throw SearchExportDestinationError.save }
    }
}
