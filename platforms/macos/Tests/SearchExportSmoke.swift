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
    @MainActor static func html(root: String) async throws {
        let library = URL(fileURLWithPath: root).appendingPathComponent("library %20 café").path
        let worker = try CoreWorker(root: library)
        let tab = try await worker.createTab(), modules = try await worker.modules()
        let model = SearchModel(); model.configure(worker: worker, tab: tab, modules: modules, preferred: "OSISNotes")
        model.expression = "PNG"; model.search(); try await idle(model)
        guard model.canExportSelected, model.canExportAll, let source = model.result else { fatalError("PNG search exports available") }
        model.expression = "Unsubmitted draft"; model.exportScripture = true; model.exportFormat = .html; model.exportImages = true
        var selected: SearchExportPayload?
        model.exportResults(allMatches: false) { selected = $0 }
        model.exportFormat = .plainText; model.exportImages = false
        try await idle(model)
        guard case .html(let value) = selected, value.status == .success, value.rowCount == 1,
              value.selection == .visible, value.images.count == 1, value.images.originalBytes == 75,
              value.html.contains("Search results: PNG"), !value.html.contains("Unsubmitted"),
              value.html.contains("data:image/png;base64,"), value.generation == source.generation else { fatalError("HTML action captures format/options and committed query before editable drafts change") }
        model.exportFormat = .html; model.exportImages = true
        var all: SearchExportPayload?
        model.exportResults(allMatches: true) { all = $0 }; try await idle(model)
        guard case .html(let complete) = all, complete.status == .success, complete.selection == .allRetained,
              complete.rowCount == 1, complete.images.count == 1, model.result == source,
              try await worker.history(tab).count == 0 else { fatalError("All HTML consumes shared selection without navigation") }
        let board = NSPasteboard(name: NSPasteboard.Name("XiphosHTMLExportTest-" + UUID().uuidString))
        defer { board.releaseGlobally() }
        try SearchExportDestination.copy(.html(value), to: board)
        guard board.data(forType: .html) == Data(value.html.utf8), board.string(forType: .string) == nil else { fatalError("Native clipboard gets exact HTML representation without frontend text conversion") }
        let directory = FileManager.default.temporaryDirectory.appendingPathComponent("XiphosHTMLExportTest-" + UUID().uuidString)
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: directory) }
        let file = directory.appendingPathComponent("search.html")
        try Data("Original prior file".utf8).write(to: file)
        try await SearchExportDestination.save(.html(complete), to: file)
        guard try Data(contentsOf: file) == Data(complete.html.utf8) else { fatalError("Atomic HTML destination preserves complete core document bytes") }
        let failed = try await worker.exportSearchHTML(tab: tab, generation: source.generation.id,
            rows: [CoreSearchVisibleRow(group: 0, row: 0)], maxBytes: 1)
        do { try SearchExportDestination.copy(.html(failed), to: board); fatalError("Failed HTML cannot copy") } catch is SearchExportDestinationError {}
        do { try await SearchExportDestination.save(.html(failed), to: file); fatalError("Failed HTML cannot save") } catch is SearchExportDestinationError {}
        guard board.data(forType: .html) == Data(value.html.utf8), try Data(contentsOf: file) == Data(complete.html.utf8) else { fatalError("Rejected HTML leaves existing destinations untouched") }
        do { try await SearchExportDestination.save(.html(complete), to: directory); fatalError("Directory is not an HTML destination file") } catch is SearchExportDestinationError {}
        model.exportImages = false
        var unsupported = false
        model.exportResults(allMatches: false) { _ in unsupported = true }; try await idle(model)
        guard !unsupported, model.message.contains("not enabled or supported"), model.result == source else { fatalError("Resource failure is actionable and cannot open a destination") }
        model.exportImages = true
        var stopped = false
        model.exportResults(allMatches: false) { _ in stopped = true }; model.stop(); try await idle(model)
        guard !stopped, model.message == "Export stopped." else { fatalError("Stopped HTML action suppresses destination even on late success") }
        _ = try await worker.searchModule(tab: tab, module: "OSISNotes", expression: "Original")
        var stale = false
        model.exportResults(allMatches: false) { _ in stale = true }; try await idle(model)
        guard !stale, model.message.contains("Search results changed"), model.result == source else { fatalError("Stale displayed HTML cannot reach clipboard or save picker") }
        model.expression = "PNG"; model.search(); try await idle(model)
        var closed = false
        model.exportResults(allMatches: false) { _ in closed = true }; model.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard !closed, !model.isBusy else { fatalError("Closed window suppresses HTML destination") }
        try await worker.closeTab(tab); await worker.shutdown()
        guard value.html.contains("data:image/png;base64,"), complete.images.pixels == 64 else { fatalError("Copied destination payload survives worker teardown") }
        print("Native HTML format/options, exact isolated pasteboard/file, typed failures and stale/stop/close suppression passed")
    }
    @MainActor static func main() async throws {
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--html" {
            try await html(root: CommandLine.arguments[2]); return
        }
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
