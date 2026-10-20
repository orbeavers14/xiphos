// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import Foundation

enum SearchExportDestinationError: LocalizedError {
    case incompletePayload, clipboard, save
    var errorDescription: String? {
        switch self {
        case .incompletePayload: "No complete search text is available to save or copy."
        case .clipboard: "Unable to copy search text. Try again."
        case .save: "Unable to save search text. Choose a writable file location."
        }
    }
}
// Destination handling only. The shared core has already resolved selection,
// rendered scripture, assembled the document and checked aggregate output caps.
enum SearchExportDestination {
    @MainActor static func copy(_ payload: RenderedSearchTextExport, to pasteboard: NSPasteboard = .general) throws {
        guard payload.status == .success else { throw SearchExportDestinationError.incompletePayload }
        let item = NSPasteboardItem()
        guard item.setString(payload.text, forType: .string) else { throw SearchExportDestinationError.clipboard }
        pasteboard.clearContents()
        guard pasteboard.writeObjects([item]) else { throw SearchExportDestinationError.clipboard }
    }
    static func save(_ payload: RenderedSearchTextExport, to url: URL) async throws {
        guard payload.status == .success else { throw SearchExportDestinationError.incompletePayload }
        let data = Data(payload.text.utf8)
        do {
            try await Task.detached(priority: .utility) { try data.write(to: url, options: .atomic) }.value
        } catch { throw SearchExportDestinationError.save }
    }
}
