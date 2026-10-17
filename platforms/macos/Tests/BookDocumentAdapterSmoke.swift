// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct BookDocumentAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("original document fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let a = try await worker.createTab(), b = try await worker.createTab()
        guard try await worker.bookDocumentSelection(tab: a) == nil else { fatalError("initial document absence") }
        try await worker.setEngineOptions(tab: a, options: [.footnotes, .wordsInRed])
        try await worker.setEngineOptions(tab: b, options: [])
        let document = try await worker.selectBookDocument(tab: a, module: "Document5", offset: 0)
        let plain = try await worker.selectBookDocument(tab: b, module: "Document5", offset: 4)
        guard document.tree.node.offset == 4, document.displayLevel == 3, document.sections.count == 5,
              document.sections.map(\.depth) == [1, 2, 3, 2, 3],
              document.sections.map(\.entry.tree.node.path) == ["/Branch", "/Branch/Leaf", "/Branch/Leaf/Deep", "/Branch/Empty", "/Branch/Empty/Twig"],
              document.sections[3].rendered == false, document.html.contains("Original deep section"),
              document.html.contains("<br/><hr/>"), document.html.contains("wordsOfJesus"), !plain.html.contains("wordsOfJesus"),
              try await worker.bookDocumentSelection(tab: a) == document else { fatalError("owned complete document, depth, empty branch and configured publication") }
        _ = try await worker.readBookDocument(tab: a, module: "Document5", offset: 0)
        guard try await worker.bookDocumentSelection(tab: a) == document else { fatalError("read-only document remains neutral") }
        let uri = "xiphos-preview://study/passagestudy.jsp?action=showNote&type=n&value=1&module=Document5&passage=%2FBranch%2FLeaf"
        let note = try await worker.bookDocumentNoteLink(tab: a, uri: uri)
        guard note.tree.node.offset == 8, note.content.reference == "/Branch/Leaf", note.content.html.contains("Original descendant note"),
              try await worker.bookDocumentSelection(tab: a) == document else { fatalError("note source resolves through committed document without moving root") }
        for bad in [uri + "\0ignored", uri.replacingOccurrences(of: "%2FBranch%2FLeaf", with: "%2FLinked"), "https://example.invalid"] {
            do { _ = try await worker.bookDocumentNoteLink(tab: a, uri: bad); fatalError("invalid document note accepted") }
            catch is CoreError { }
        }
        for module in ["TooDeep", "Missing", "Document5\0ignored"] {
            do { _ = try await worker.selectBookDocument(tab: a, module: module, offset: 4); fatalError("invalid/oversized document accepted") }
            catch is CoreError { }
        }
        guard try await worker.bookDocumentSelection(tab: a) == document,
              try await worker.bookDocumentSelection(tab: b) == plain else { fatalError("failure and tab isolation") }
        let child = try await worker.navigateBookDocument(tab: a, direction: .firstChild)
        guard child?.tree.node.offset == 8, child?.sections.count == 2,
              try await worker.navigateBookDocument(tab: a, direction: .previousSibling) == nil else { fatalError("shared document navigation and absent relation") }
        let saved = SavedBookSelection(module: "Document5", key: "/Branch", offsetText: "8")
        guard try await worker.restoreBookDocumentSelection(tab: a, saved: saved, mode: .displayKey) == document,
              try await worker.restoreBookDocumentSelection(tab: a, saved: saved, mode: .initialWindow) == child,
              try await worker.restoreBookDocumentSelection(tab: a, saved: .init(), mode: .displayKey) == nil else { fatalError("explicit restore modes and no-op absence") }
        do { _ = try await worker.restoreBookDocumentSelection(tab: a, saved: .init(module: "Document5", key: "/Branch\0ignored", offsetText: nil), mode: .displayKey); fatalError("NUL document restore accepted") }
        catch is CoreError { }
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent("xiphos-document-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: folder) }
        let profile = folder.appendingPathComponent("café.xml")
        let bytes = Data("<Xiphos><modules><book>Document5</book></modules><keys><book>/Branch</book><offset>8</offset></keys></Xiphos>".utf8)
        try bytes.write(to: profile)
        guard try await worker.restoreBookDocumentProfile(tab: a, path: profile.path, mode: .displayKey) == document,
              try Data(contentsOf: profile) == bytes else { fatalError("copied explicit profile document restore") }
        guard try await worker.history(a).count == 0, try await worker.dictionaryHistory(tab: a).entries.isEmpty else { fatalError("document commands do not create unrelated history") }
        try await worker.closeTab(a)
        do { _ = try await worker.bookDocumentSelection(tab: a); fatalError("closed document tab accepted") }
        catch is CoreError { }
        try await worker.closeTab(b); await worker.shutdown()
        guard document.html.contains("café"), document.sections[4].entry.html.contains("empty-branch child"), note.content.html.contains("Original descendant note") else { fatalError("owned document sections/notes survive shutdown") }
        print("Swift complete book document, atomic selection, restore and descendant-note adapter passed")
    }
}
