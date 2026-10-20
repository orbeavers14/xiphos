// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
@main struct SearchExportSmoke {
    @MainActor static func idle(_ model: SearchModel) async throws {
        let deadline = Date().addingTimeInterval(10)
        while model.isBusy {
            guard Date() < deadline else { fatalError("Native export deadline") }
            try await Task.sleep(nanoseconds: 1_000_000)
        }
    }
    @MainActor static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("Original search fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let tab = try await worker.createTab(), modules = try await worker.modules()
        let model = SearchModel();model.configure(worker: worker, tab: tab, modules: modules, preferred: "Text")
        model.expression = "Original";model.search();try await idle(model)
        model.select(1);try await idle(model)
        guard model.canExportSelected, model.canExportAll, let source = model.result else { fatalError("Displayed result exposes native export actions") }
        model.expression = "Unsubmitted changed draft";model.exportScripture = true
        var selectedValue: RenderedSearchTextExport?
        model.exportText(allMatches: false) { selectedValue = $0 };try await idle(model)
        guard let selected = selectedValue, selected.status == .success, selected.selection == .visible, selected.rowCount == 1,
              selected.text.hasPrefix("Search results: Original\n\n"), selected.text.contains("John 1:2"),
              !selected.text.contains("John 1:1"), !selected.text.contains("Unsubmitted"), selected.includesScripture,
              selected.generation == source.generation, model.selection == 1 else { fatalError("Native export captures displayed identity and selection instead of drafts") }
        var allValue: RenderedSearchTextExport?
        model.exportText(allMatches: true) { allValue = $0 };try await idle(model)
        guard let all = allValue, all.selection == .allRetained, all.rowCount == 3, all.text.contains("John 1:1"),
              try await worker.history(tab).count == 0, model.result == source else { fatalError("Native all action uses shared service without navigation or search mutation") }
        let board = NSPasteboard(name: NSPasteboard.Name("XiphosOriginalExportTest-" + UUID().uuidString))
        defer { board.releaseGlobally() }
        try SearchExportDestination.copy(selected, to: board)
        guard board.string(forType: .string) == selected.text else { fatalError("Pasteboard gets exact owned payload") }
        let directory = FileManager.default.temporaryDirectory.appendingPathComponent("XiphosOriginalExportTest-" + UUID().uuidString)
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: directory) }
        let file = directory.appendingPathComponent("search.txt")
        try Data("Original previous file".utf8).write(to: file)
        try await SearchExportDestination.save(all, to: file)
        guard try Data(contentsOf: file) == Data(all.text.utf8) else { fatalError("Background atomic replacement writes exact UTF-8") }
        let failed = try await worker.exportSearchVisibleText(tab: tab, generation: source.generation.id,
            rows: [CoreSearchVisibleRow(group: 0, row: 0)], maxBytes: 1)
        do { try SearchExportDestination.copy(failed, to: board);fatalError("Expected failed payload refusal") } catch is SearchExportDestinationError {}
        do { try await SearchExportDestination.save(failed, to: file);fatalError("Expected failed payload refusal") } catch is SearchExportDestinationError {}
        guard board.string(forType: .string) == selected.text, try Data(contentsOf: file) == Data(all.text.utf8) else { fatalError("Failed payload cannot touch clipboard or file") }
        var stopped = false
        model.exportText(allMatches: false) { _ in stopped = true };model.stop();try await idle(model)
        guard !stopped, model.message == "Export stopped." else { fatalError("Stop suppresses destination even if core completion wins") }
        _ = try await worker.searchModule(tab: tab, module: "Text", expression: "Alpha")
        var stale = false
        model.exportText(allMatches: false) { _ in stale = true };try await idle(model)
        guard !stale, model.message.contains("Search results changed"), model.result == source else { fatalError("Stale displayed results cannot reach a destination") }
        model.expression = "Original";model.search();try await idle(model)
        var closed = false
        model.exportText(allMatches: false) { _ in closed = true };model.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard !closed, !model.isBusy else { fatalError("Closed window cannot publish destination") }
        try await worker.closeTab(tab);await worker.shutdown()
        guard selected.text.contains("Original"), all.rowCount == 3 else { fatalError("Copied snapshots outlive worker") }
        print("Native selected/all export, original identity, isolated clipboard, atomic file and stale/stop/close passed")
    }
}
