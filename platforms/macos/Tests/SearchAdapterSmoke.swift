// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct SearchAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("Original search fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let tab = try await worker.createTab(), other = try await worker.createTab()
        _ = try await worker.visit(tab: tab, module: "Text", reference: "John1:2")
        let book = try await worker.selectBookDocument(tab: tab, module: "Book", offset: 4)
        let prefix = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", maxResults: 1)
        guard prefix.module == "Text", prefix.expression == "Original", prefix.mode == .phrase,
              prefix.engineType == -1, !prefix.caseSensitive, prefix.limited,
              prefix.totalResults == 3, prefix.references.count == 1,
              try await worker.history(tab).count == 1, try await worker.history(other).count == 0,
              try await worker.bookDocumentSelection(tab: tab) == book else { fatalError("Swift owns bounded search context without changing reading/history") }
        let sensitive = try await worker.searchModule(tab: tab, module: "Text", expression: "Alpha", caseSensitive: true)
        let insensitive = try await worker.searchModule(tab: tab, module: "Text", expression: "Alpha")
        guard sensitive.references.count == 1, insensitive.references.count == 2 else { fatalError("Search case behavior matches checked SDK") }
        do { _ = try await worker.searchModule(tab: tab, module: "Dictionary", expression: "Original"); fatalError("Unsupported family accepted") } catch is CoreError { }
        do { _ = try await worker.searchModule(tab: tab, module: "Text", expression: "bad\0query"); fatalError("NUL query accepted") } catch is CoreError { }
        do { _ = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", maxResults: 0); fatalError("Zero output bound accepted") } catch is CoreError { }
        let commentary = try await worker.searchModule(tab: tab, module: "Commentary", expression: "Original")
        guard commentary.references.count == 3, try await worker.bookDocumentSelection(tab: tab) == book,
              try await worker.dictionaryHistory(tab: tab).entries.isEmpty else { fatalError("Search queue recovers after failures and remains state-neutral") }
        try await worker.closeTab(tab); try await worker.closeTab(other); await worker.shutdown()
        guard prefix.references.count == 1, prefix.module == "Text", commentary.references.count == 3 else { fatalError("Swift search copies outlive worker destruction") }
        print("Swift owned search snapshots, modes/case, bounds, failure recovery and reading neutrality passed")
    }
}
