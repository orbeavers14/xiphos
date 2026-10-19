// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
actor ProgressLog {
    var items: [CoreSearchProgress] = []
    func append(_ progress: CoreSearchProgress) { items.append(progress) }
    func snapshot() -> [CoreSearchProgress] { items }
}
@main struct SearchAdapterSmoke {
    static func attributes(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab(), other = try await worker.createTab()
        let cases: [(CoreSearchMode, String, [String])] = [
            (.lemma, "G140", ["John 1:1", "John 1:4"]),
            (.morphology, "N-ASF", ["John 1:1", "John 1:3"]),
            (.footnote, "Alpha", ["John 1:1", "John 1:2"])]
        var owned: [RenderedSearchResult] = []
        for (mode, query, expected) in cases {
            let result = try await worker.searchModule(tab: tab, module: "Text", expression: query, mode: mode)
            guard result.mode == mode, result.engineType == -3, result.references == expected,
                  result.scopeUpdated, result.scope.elementCount == 2 else { fatalError("Swift attribute mode/results match shared exact identifiers and copied scope") }
            owned.append(result)
        }
        let prefix = try await worker.searchModule(tab: tab, module: "Text", expression: "N-ASF", mode: .morphology, maxResults: 1)
        let narrowed = try await worker.searchModule(tab: tab, module: "Commentary", expression: "Original", mode: .footnote, withinLastResults: true)
        guard prefix.references.count == 1, prefix.scope.elementCount == 2,
              narrowed.references == ["John 1:1", "John 1:3"], !((try await worker.lastSearchScope(tab: other)).present) else { fatalError("Attribute modes share complete cross-module scopes and isolate tabs") }
        let bounded = try await worker.searchModule(tab: tab, module: "Text", expression: "Alpha", mode: .footnote,
                                                   range: CoreSearchRange(module: "Text", expression: "John1:2-John1:3"))
        guard bounded.references == ["John 1:2"], try await worker.history(tab).count == 0 else { fatalError("Attribute bounds do not navigate") }
        try await worker.closeTab(tab); try await worker.closeTab(other); await worker.shutdown()
        guard owned[0].references == ["John 1:1", "John 1:4"], owned[2].expression == "Alpha" else { fatalError("Attribute copies survive shutdown") }
        print("Swift copied lemma/morph/footnote execution, scopes, bounds and shutdown passed")
    }
    static func main() async throws {
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--attributes" {
            try await attributes(root: CommandLine.arguments[2]); return
        }
        guard CommandLine.arguments.count == 2 else { fatalError("Original search fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let modules = try await worker.modules()
        guard modules.filter(\.hasVerseSearch).map(\.id).sorted() == ["Commentary", "Text"] else { fatalError("Swift copies core search capabilities") }
        let tab = try await worker.createTab(), other = try await worker.createTab()
        _ = try await worker.visit(tab: tab, module: "Text", reference: "John1:2")
        let book = try await worker.selectBookDocument(tab: tab, module: "Book", offset: 4)
        let progressLog = ProgressLog()
        let prefix = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", maxResults: 1,
                                                   onProgress: { await progressLog.append($0) })
        let progress = await progressLog.snapshot()
        guard progress.last?.phase == .complete, progress.last?.percent == 100,
              zip(progress, progress.dropFirst()).allSatisfy({ $0.percent <= $1.percent && $0 != $1 }) else { fatalError("Swift progress copies are monotonic, changed-only and terminal") }
        guard prefix.module == "Text", prefix.expression == "Original", prefix.mode == .phrase,
              prefix.engineType == -1, !prefix.caseSensitive, prefix.limited,
              prefix.totalResults == 3, prefix.references.count == 1, !prefix.cancelled, prefix.executed,
              try await worker.history(tab).count == 1, try await worker.history(other).count == 0,
              try await worker.bookDocumentSelection(tab: tab) == book else { fatalError("Swift owns bounded search context without changing reading/history") }
        let scope = CoreSearchRange(module: "Text", expression: "John1:2-John1:3")
        let bounded = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", range: scope)
        guard bounded.references.count == 2, bounded.totalResults == 2, bounded.range == scope,
              prefix.range == nil else { fatalError("Swift preserves explicit SDK range source and keeps unscoped requests distinct") }
        guard prefix.scope == CoreSearchScope(present: true, elementCount: 3), prefix.scopeUpdated,
              !prefix.withinLastResults, bounded.scope.elementCount == 2 else { fatalError("Swift owns complete scope metadata independently of displayed prefix") }
        let narrowed = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", withinLastResults: true)
        guard narrowed.withinLastResults, narrowed.references.count == 2, narrowed.scopeUpdated,
              try await worker.lastSearchScope(tab: tab).elementCount == 2 else { fatalError("Last scope uses complete bounded source keys") }
        let otherEmpty = try await worker.searchModule(tab: other, module: "Text", expression: "Original", withinLastResults: true)
        guard !otherEmpty.executed, !otherEmpty.scope.present, !otherEmpty.scopeUpdated,
              try await worker.clearSearchScope(tab: tab) == CoreSearchScope(present: false, elementCount: 0) else { fatalError("Tab isolation and explicit clear stay in the shared core") }
        let cleared = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", withinLastResults: true)
        guard !cleared.executed, cleared.references.isEmpty else { fatalError("Cleared scope never becomes whole-module search") }
        do {
            _ = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", range: scope, withinLastResults: true)
            fatalError("Expected mutually exclusive scope rejection")
        } catch is CoreError {}
        let sensitive = try await worker.searchModule(tab: tab, module: "Text", expression: "Alpha", caseSensitive: true)
        let insensitive = try await worker.searchModule(tab: tab, module: "Text", expression: "Alpha")
        guard sensitive.references.count == 1, insensitive.references.count == 2 else { fatalError("Search case behavior matches checked SDK") }
        do { _ = try await worker.searchModule(tab: tab, module: "Dictionary", expression: "Original"); fatalError("Unsupported family accepted") } catch is CoreError { }
        do { _ = try await worker.searchModule(tab: tab, module: "Text", expression: "bad\0query"); fatalError("NUL query accepted") } catch is CoreError { }
        do { _ = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", maxResults: 0); fatalError("Zero output bound accepted") } catch is CoreError { }
        let cancelled = Task { try await worker.searchModule(tab: tab, module: "Text", expression: "Original") }
        cancelled.cancel()
        do { _ = try await cancelled.value; fatalError("Cancelled Swift search published a result") } catch is CancellationError { }
        let cancelledProgress = ProgressLog()
        let returned = Task {
            try await worker.searchModule(tab: tab, module: "Text", expression: "Original", returnCancelledResults: true,
                                          onProgress: { await cancelledProgress.append($0) })
        }
        returned.cancel()
        let stopped = try await returned.value
        let stoppedProgress = await cancelledProgress.snapshot()
        guard stoppedProgress.last?.phase == (stopped.cancelled ? .cancelled : .complete),
              stopped.scope == (try await worker.lastSearchScope(tab: tab)),
              stopped.executed || !stopped.scopeUpdated else { fatalError("Interactive stop drains and returns authoritative execution/scope disposition; late completion may win") }
        let commentary = try await worker.searchModule(tab: tab, module: "Commentary", expression: "Original")
        guard commentary.references.count == 3, try await worker.bookDocumentSelection(tab: tab) == book,
              try await worker.dictionaryHistory(tab: tab).entries.isEmpty else { fatalError("Search queue recovers after failures and remains state-neutral") }
        try await worker.closeTab(tab); try await worker.closeTab(other); await worker.shutdown()
        guard prefix.references.count == 1, prefix.module == "Text", commentary.references.count == 3, bounded.range == scope else { fatalError("Swift search copies outlive worker destruction") }
        print("Swift owned search snapshots, modes/case, bounds, failure recovery and reading neutrality passed")
    }
}
