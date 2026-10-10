// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main
struct ReaderModelSmoke {
    @MainActor static func idle(_ model: ReaderModel) async throws {
        let deadline = Date().addingTimeInterval(10)
        while model.isBusy {
            guard Date() < deadline else { fatalError("model request timeout") }
            try await Task.sleep(nanoseconds: 1_000_000)
        }
    }
    @MainActor static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("fixture root required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let model = ReaderModel(worker: worker, startupError: nil)
        guard model.isBusy, model.modules.isEmpty else { fatalError("nonblocking startup state") }
        try await idle(model)
        guard model.reference == "John 3:16", model.html.contains("café"), !model.canBack else { fatalError("model initial load") }
        model.reference = "John 3:17"
        model.read()
        guard model.isBusy else { fatalError("action starts loading immediately") }
        model.chooseReference("John 3:18") // second action must not race the first
        try await idle(model)
        guard model.reference == "John 3:17", model.html.contains("Second fixture"), model.canBack else { fatalError("serialized model actions") }
        model.navigate(-1)
        try await idle(model)
        guard model.reference == "John 3:16", !model.canBack, model.canForward else { fatalError("no extra busy action in history") }
        model.reference = "John 3:18" // unsubmitted draft survives display changes
        model.showChapter = true
        model.refreshDisplay()
        try await idle(model)
        guard model.reference == "John 3:18", model.renderedReference == "John 3", model.renderedAnchor == "verse-16" else { fatalError("display refresh preserves draft and committed passage") }
        model.chooseReference("John 3:17")
        try await idle(model)
        guard model.reference == "John 3:17", model.renderedAnchor == "verse-17" else { fatalError("selector updates chapter selection") }
        let committed = model.html
        model.followLink("https://example.invalid")
        try await idle(model)
        guard model.html == committed, model.reference == "John 3:17", !model.message.isEmpty else { fatalError("unsupported link preserves pane") }
        model.clearHistory()
        try await idle(model)
        guard !model.canBack, !model.canForward, model.html == committed else { fatalError("clear keeps passage") }
        model.read()
        model.close() // invalidate an action before its result may publish
        model.close() // idempotent lifecycle
        try await Task.sleep(nanoseconds: 50_000_000)
        guard model.html == committed else { fatalError("closed model result publication") }
        let closedAtStartup = ReaderModel(worker: worker, startupError: nil)
        closedAtStartup.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard closedAtStartup.modules.isEmpty, closedAtStartup.html.isEmpty else { fatalError("closed startup remains unpublished") }
        let unavailable = ReaderModel(worker: nil, startupError: "Fixture startup error")
        guard !unavailable.isBusy, unavailable.message == "Fixture startup error" else { fatalError("worker setup error") }
        await worker.shutdown()
        print("Async reader model checks passed")
    }
}
