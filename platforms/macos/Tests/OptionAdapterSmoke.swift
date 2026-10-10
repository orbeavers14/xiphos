// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main
struct OptionAdapterSmoke {
    @MainActor static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("option fixture root required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let modules = try await worker.modules()
        let declared: EngineOptionFlags = [.wordsInRed, .strongs, .morphology, .footnotes]
        guard modules.count == 1, modules[0].options.declared == declared,
              !modules[0].options.hasHeadings else { fatalError("copied module option metadata") }
        let tab = try await worker.createTab(), other = try await worker.createTab()
        let original = try await worker.visit(tab: tab, module: "OptionTest", reference: "John 3:16")
        try await worker.setEngineOptions(tab: other, options: [])
        for bits in 0..<16 {
            var settings: EngineOptionFlags = []
            if bits & 1 != 0 { settings.insert(.wordsInRed) }
            if bits & 2 != 0 { settings.insert(.strongs) }
            if bits & 4 != 0 { settings.insert(.morphology) }
            if bits & 8 != 0 { settings.insert(.footnotes) }
            try await worker.setEngineOptions(tab: tab, options: settings)
            let entry = try await worker.readTabEntry(tab: tab, module: "OptionTest", reference: "John 3:16")
            let chapter = try await worker.readTabChapter(tab: tab, module: "OptionTest", reference: "John 3:16", verseNumbers: true, highlightSelected: true)
            guard chapter.verses[15].html == entry.html,
                  entry.html.contains("3056") == settings.contains(.strongs),
                  entry.html.contains("wordsOfJesus") == settings.contains(.wordsInRed) else { fatalError("typed options/verse/chapter output") }
            let untouched = try await worker.readTabEntry(tab: other, module: "OptionTest", reference: "John 3:16")
            guard !untouched.html.contains("3056"), !untouched.html.contains("wordsOfJesus") else { fatalError("Swift tab option isolation") }
        }
        _ = try await worker.readTabEntry(tab: tab, module: "OptionTest", reference: "John 3:17")
        let history = try await worker.history(tab)
        guard history.count == 1, !history.canBack, !history.canForward else { fatalError("refresh changed history") }
        do {
            try await worker.setEngineOptions(tab: tab, options: EngineOptionFlags(rawValue: 1 << 30))
            fatalError("unknown option bits accepted")
        } catch is CoreError { }
        try await worker.clearEngineOptions(tab: tab)
        let restored = try await worker.readTabEntry(tab: tab, module: "OptionTest", reference: "John 3:16")
        guard restored.html == original.html else { fatalError("clear restored defaults") }
        try await worker.closeTab(other)
        do { _ = try await worker.readTabEntry(tab: other, module: "OptionTest", reference: "John 3:16"); fatalError("closed tab refresh") }
        catch is CoreError { }
        try await worker.closeTab(tab)
        await worker.shutdown()
        guard modules[0].options.declared == declared, restored.html == original.html else { fatalError("copied metadata/results lifetime") }
        print("Swift tab option/metadata adapter checks passed")
    }
}
