// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct BookAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("original book fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let a = try await worker.createTab(), b = try await worker.createTab()
        let modules = try await worker.modules()
        guard modules.filter(\.hasBookTree).map(\.id).sorted() == ["OSISBook", "ThMLBook"],
              try await worker.bookSelection(tab: a) == nil else { fatalError("core capability and initial absence") }
        let root = try await worker.bookTree(module: "OSISBook", offset: 0)
        guard root.node.offset == 0, root.parent == nil, root.firstChild?.offset == 4 else { fatalError("copied root and absent relation") }
        let page = try await worker.bookChildren(module: "OSISBook", parent: 0, limit: 1)
        guard page.parent == root.node, page.nodes.count == 1, page.nodes[0].offset == 4,
              page.nextAfter == 4 else { fatalError("ordered bounded page") }
        let rest = try await worker.bookChildren(module: "OSISBook", parent: 0, after: page.nextAfter)
        guard rest.nodes.map(\.path) == ["/Empty Branch", "/Linked"], rest.nextAfter == nil else { fatalError("copied continuation") }
        try await worker.setEngineOptions(tab: a, options: [.footnotes, .wordsInRed])
        try await worker.setEngineOptions(tab: b, options: [])
        let entry = try await worker.selectBook(tab: a, module: "OSISBook", offset: 0)
        let plain = try await worker.selectBook(tab: b, module: "OSISBook", offset: 4)
        guard entry.tree.node.offset == 4, entry.tree.parent?.offset == 0, entry.tree.firstChild?.offset == 8,
              entry.hasEntry, entry.html.contains("café"), entry.html.contains("wordsOfJesus"),
              !plain.html.contains("wordsOfJesus") else { fatalError("shared numeric reading and isolated configured rendering") }
        guard try await worker.navigateBook(tab: a, direction: .firstChild)?.tree.node.offset == 8,
              try await worker.navigateBook(tab: a, direction: .firstChild) == nil,
              try await worker.navigateBook(tab: a, direction: .parent)?.tree.node.offset == 4,
              try await worker.navigateBook(tab: a, direction: .previousSibling) == nil else { fatalError("shared directional commands and optional boundary") }
        let saved = SavedBookSelection(module: "OSISBook", key: "Part One", offsetText: "20")
        let initial = try await worker.restoreBookSelection(tab: a, saved: saved, mode: .initialWindow)
        let display = try await worker.restoreBookSelection(tab: a, saved: saved, mode: .displayKey)
        guard initial?.tree.node.offset == 20, display == entry else { fatalError("distinct core restore contexts") }
        let pathRoot = try await worker.restoreBookSelection(tab: a,
            saved: .init(module: "OSISBook", key: "/", offsetText: nil), mode: .displayKey)
        guard pathRoot?.tree.node.offset == 0 else { fatalError("explicit root path stays root") }
        _ = try await worker.selectBook(tab: a, module: "OSISBook", offset: 4)
        let absent = try await worker.restoreBookSelection(tab: a, saved: .init(), mode: .displayKey)
        guard absent == nil, try await worker.bookSelection(tab: a) == entry.tree else { fatalError("absent restore preserves source") }
        for module in ["Missing", "VerseBook", "OSISBook\0ignored"] {
            do { _ = try await worker.selectBook(tab: a, module: module, offset: 4); fatalError("invalid selection accepted") }
            catch is CoreError { }
        }
        for saved in [SavedBookSelection(module: "OSISBook", key: "Missing/path", offsetText: nil),
                      .init(module: "OSISBook", key: "Part One\0ignored", offsetText: nil),
                      .init(module: nil, key: nil, offsetText: "0\0ignored")] {
            do { _ = try await worker.restoreBookSelection(tab: a, saved: saved, mode: .displayKey); fatalError("invalid restore accepted") }
            catch is CoreError { }
        }
        do { _ = try await worker.bookChildren(module: "OSISBook", parent: 0, limit: 0); fatalError("invalid page accepted") }
        catch is CoreError { }
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent("xiphos-book-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: folder) }
        let profile = folder.appendingPathComponent("café.xml")
        let bytes = Data("<Xiphos><modules><book>OSISBook</book></modules><keys><book>Part One</book><offset>20</offset></keys></Xiphos>".utf8)
        try bytes.write(to: profile)
        let imported = try await worker.restoreBookProfile(tab: a, path: profile.path, mode: .initialWindow)
        guard imported == initial, try Data(contentsOf: profile) == bytes,
              try await worker.bookSelection(tab: b) == plain.tree else { fatalError("explicit copied file restore and tab isolation") }
        for path in [profile.path + ".missing", profile.path + "\0ignored"] {
            do { _ = try await worker.restoreBookProfile(tab: a, path: path, mode: .displayKey); fatalError("invalid file restore accepted") }
            catch is CoreError { }
        }
        guard try await worker.bookSelection(tab: a) == initial?.tree,
              try await worker.history(a).count == 0,
              try await worker.dictionaryHistory(tab: a).entries.isEmpty else { fatalError("failure/source-query/history neutrality") }
        try await worker.closeTab(a)
        do { _ = try await worker.bookSelection(tab: a); fatalError("closed tab accepted") }
        catch is CoreError { }
        try await worker.closeTab(b); await worker.shutdown()
        guard entry.tree.node.title == "Part One", page.nodes[0].path == "/Part One", initial?.html.contains("café") == true else { fatalError("owned values after shutdown") }
        print("Swift book ownership, paging, restore contexts, configured selection and file restore passed")
    }
}
