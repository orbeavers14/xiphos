// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct DictionaryAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("dictionary fixture root required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let first = try await worker.createTab(), second = try await worker.createTab()
        try await worker.setModuleEngineOptions(tab: first, module: "DictionaryOne", options: [.wordsInRed])
        try await worker.setModuleEngineOptions(tab: second, module: "DictionaryOne", options: [])
        let red = try await worker.readDictionary(tab: first, module: "DictionaryOne", key: "G03056")
        let plain = try await worker.readDictionary(tab: second, module: "DictionaryOne", key: "G03056")
        guard red.module == "DictionaryOne", !red.key.isEmpty, red.html.contains("wordsOfJesus"),
              !plain.html.contains("wordsOfJesus"), plain.html.contains("Original keyed word") else { fatalError("typed dictionary settings/isolation") }
        let unicode = try await worker.readDictionary(tab: first, module: "DictionaryOne", key: "ALPHA")
        let other = try await worker.readDictionary(tab: first, module: "DictionaryTwo", key: "alpha")
        guard unicode.html.contains("café"), other.module == "DictionaryTwo", other.html.contains("DictionaryTwo") else { fatalError("dictionary Unicode/module context") }
        for (module, key) in [("Missing", "alpha"), ("OriginalBible", "John3:16"), ("DictionaryOne", ""), ("DictionaryOne", "alpha\0ignored"), ("DictionaryOne\0ignored", "alpha")] {
            do { _ = try await worker.readDictionary(tab: first, module: module, key: key); fatalError("invalid dictionary accepted") }
            catch is CoreError { }
        }
        let next = try await worker.navigateDictionary(tab: first, module: "DictionaryOne", key: "ALPHA", direction: 1)
        let previous = try await worker.navigateDictionary(tab: first, module: "DictionaryOne", key: next.key, direction: -1)
        guard next.html.contains("Unicode"), previous.key == unicode.key, previous.html == unicode.html else { fatalError("typed dictionary key navigation") }
        for (key, direction) in [("ALPHA", Int32(0)), ("ALPHA\0ignored", Int32(1))] {
            do { _ = try await worker.navigateDictionary(tab: first, module: "DictionaryOne", key: key, direction: direction); fatalError("invalid dictionary navigation accepted") }
            catch is CoreError { }
        }
        guard try await worker.dictionaryHistory(tab: first).entries.isEmpty else { fatalError("preview/key step recorded dictionary history") }
        _ = try await worker.visitDictionary(tab: first, module: "DictionaryOne", key: "alpha")
        _ = try await worker.visitDictionary(tab: first, module: "DictionaryTwo", key: "café")
        let dictionaryBack = try await worker.navigateDictionaryHistory(tab: first, direction: -1)
        let copiedHistory = try await worker.dictionaryHistory(tab: first)
        guard dictionaryBack.module == "DictionaryOne", copiedHistory.cursor == 0, copiedHistory.canForward, !copiedHistory.canBack,
              copiedHistory.entries == [DictionaryLocation(module: "DictionaryOne", key: "alpha"), DictionaryLocation(module: "DictionaryTwo", key: "café")],
              try await worker.dictionaryHistory(tab: second).entries.isEmpty else { fatalError("Swift copied dictionary history/navigation/isolation") }
        do { _ = try await worker.visitDictionary(tab: first, module: "DictionaryOne", key: "alpha\0ignored"); fatalError("recorded NUL key accepted") }
        catch is CoreError { }
        guard try await worker.dictionaryHistory(tab: first) == copiedHistory else { fatalError("failed recorded lookup changed history") }
        _ = try await worker.navigateDictionaryHistory(tab: first, direction: 1)
        try await worker.clearDictionaryHistory(tab: first)
        guard try await worker.dictionaryHistory(tab: first).entries.isEmpty else { fatalError("Swift dictionary history clear") }
        let history = try await worker.history(first)
        guard history.count == 0 else { fatalError("dictionary preview changed history") }
        try await worker.closeTab(first)
        do { _ = try await worker.dictionaryHistory(tab: first); fatalError("closed history tab accepted") }
        catch is CoreError { }
        do { _ = try await worker.readDictionary(tab: first, module: "DictionaryOne", key: "alpha"); fatalError("closed dictionary tab accepted") }
        catch is CoreError { }
        try await worker.closeTab(second); await worker.shutdown()
        guard copiedHistory.entries[1].key == "café", unicode.html.contains("café"), red.html.contains("wordsOfJesus") else { fatalError("copied dictionary values after shutdown") }
        print("Swift dictionary adapter checks passed")
    }
}
