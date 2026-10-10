// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main
struct AsyncAdapterSmoke {
    @MainActor static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("fixture root required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        var heartbeats = 0
        let heartbeat = Task { @MainActor in
            for _ in 0..<100 { heartbeats += 1; await Task.yield() }
        }
        let modules = try await worker.modules()
        await heartbeat.value
        guard modules.count == 1, modules[0].id == "ParityTest", heartbeats == 100 else { fatalError("async discovery/main actor progress") }
        let tab = try await worker.createTab()
        let other = try await worker.createTab()
        let first = try await worker.visit(tab: tab, module: "ParityTest", reference: "John 3:16")
        let second = try await worker.visit(tab: tab, module: "ParityTest", reference: "John 3:17")
        guard first.html.contains("café"), second.html.contains("Second fixture") else { fatalError("async values") }
        let back = try await worker.navigate(tab: tab, direction: -1)
        let history = try await worker.history(tab)
        let untouched = try await worker.history(other)
        guard back.reference == "John 3:16", history.canForward, history.count == 2,
              history.module == "ParityTest", untouched.count == 0 else { fatalError("async history isolation") }
        do {
            _ = try await worker.visit(tab: tab, module: "Missing", reference: "John 3:16")
            fatalError("missing module succeeded")
        } catch is CoreError { }
        let forward = try await worker.navigate(tab: tab, direction: 1)
        guard forward.reference == "John 3:17" else { fatalError("queue after error") }
        let next = try await worker.navigateChapter(tab: tab, module: "ParityTest", reference: "John 21:25", direction: 1)
        guard next.reference == "Acts 1:1" else { fatalError("async chapter navigation") }
        let choices = try await worker.referenceChoices(module: "ParityTest", reference: "John 3:16")
        let link = try await worker.resolveLink("sword:///John+3%3A17", currentModule: "ParityTest")
        guard choices.books.count == 27, choices.verses.count == 36, link.reference == "John 3:17" else { fatalError("async selectors/link") }
        try await withThrowingTaskGroup(of: RenderedChapter.self) { group in
            for index in 0..<20 {
                group.addTask {
                    try await worker.readChapter(module: "ParityTest", reference: "John 3:\(index % 2 == 0 ? 16 : 17)",
                                                 verseNumbers: false, highlightSelected: true)
                }
            }
            var count = 0
            for try await chapter in group {
                guard chapter.verses.count == 36, !chapter.html.contains("<sup>"),
                      chapter.selectedVerse == 16 || chapter.selectedVerse == 17 else { fatalError("concurrent async snapshots") }
                count += 1
            }
            guard count == 20 else { fatalError("missing async response") }
        }
        // Cancellation cannot strand an accepted tab creation without its ID.
        let creation = Task { try await worker.createTab() }
        creation.cancel()
        let canceledTab = try await creation.value
        try await worker.closeTab(canceledTab)
        try await worker.closeTab(other)
        do { _ = try await worker.history(other); fatalError("closed tab succeeded") }
        catch is CoreError { }
        try await worker.clearHistory(tab)
        let cleared = try await worker.history(tab)
        guard cleared.count == 0, !cleared.canBack, !cleared.canForward else { fatalError("async clear") }
        let chapter = try await worker.readChapter(module: "ParityTest", reference: "John 3:16", verseNumbers: true, highlightSelected: true)
        try await worker.closeTab(tab)
        await worker.shutdown()
        await worker.shutdown() // idempotent explicit shutdown
        guard first.html.contains("café"), chapter.selectedAnchor == "verse-16" else { fatalError("copied values after shutdown") }
        do { _ = try await worker.modules(); fatalError("closed worker succeeded") }
        catch is CoreError { }
        let broken = try CoreWorker(root: CommandLine.arguments[1] + "/not-installed")
        do { _ = try await broken.modules(); fatalError("invalid root succeeded") }
        catch is CoreError { }
        await broken.shutdown()
        print("Async Swift adapter checks passed")
    }
}
