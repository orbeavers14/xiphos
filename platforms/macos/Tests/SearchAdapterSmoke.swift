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
        let plain = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", mode: .multiword)
        let indexed = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", mode: .indexed)
        guard plain.engineType == -2, !plain.indexFallback, indexed.engineType == -2, indexed.indexFallback,
              indexed.mode == .indexed, indexed.references == plain.references, indexed.effectiveExpression == "Original" else { fatalError("Explicit advanced dialects retain requested mode and report missing-index fallback without AND rewriting") }
        try await worker.closeTab(tab); try await worker.closeTab(other); await worker.shutdown()
        guard owned[0].references == ["John 1:1", "John 1:4"], owned[2].expression == "Alpha" else { fatalError("Attribute copies survive shutdown") }
        print("Swift copied lemma/morph/footnote execution, scopes, bounds and shutdown passed")
    }
    static func textOptions(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab(), other = try await worker.createTab()
        let wanted = CoreSearchTextOptions(strongs: true, morphology: false, footnotes: true)
        let changed = CoreSearchTextOptions(strongs: false, morphology: true, footnotes: false)
        let seed = try await worker.searchModule(tab: tab, module: "Text", expression: "G140", mode: .lemma, textOptions: wanted)
        guard seed.requestedTextOptions == wanted, seed.effectiveTextOptions == wanted, seed.textOptionsUpdated,
              try await worker.searchTextOptions(tab: tab) == wanted, try await worker.searchTextOptions(tab: other) == nil else { fatalError("Swift copies prepared search-only context and isolates tabs") }
        let phrase = try await worker.searchModule(tab: tab, module: "Text", expression: "G140", mode: .phrase, textOptions: changed)
        guard phrase.requestedTextOptions == changed, phrase.effectiveTextOptions == wanted, !phrase.textOptionsUpdated else { fatalError("Phrase keeps previous search context in core") }
        let bounded = try await worker.searchModule(tab: tab, module: "Text", expression: "N-ASF", mode: .morphology,
            range: CoreSearchRange(module: "Text", expression: "John1:2-John1:3"), textOptions: changed)
        let last = try await worker.searchModule(tab: tab, module: "Commentary", expression: "Original", mode: .footnote, withinLastResults: true)
        guard bounded.references == ["John 1:3"], last.references == ["John 1:3"],
              last.requestedTextOptions == nil, last.effectiveTextOptions == changed else { fatalError("Composable descriptor unifies scopes and retained annotation settings") }
        let before = try await worker.lastSearchScope(tab: tab)
        guard try await worker.clearSearchTextOptions(tab: tab) == nil, try await worker.lastSearchScope(tab: tab) == before else { fatalError("Reset search text options preserves result scope") }
        let ignored = try await worker.searchModule(tab: other, module: "Text", expression: "Alpha", mode: .phrase, textOptions: wanted)
        guard ignored.effectiveTextOptions == nil, !ignored.textOptionsUpdated, try await worker.searchTextOptions(tab: other) == nil else { fatalError("Unconfigured phrase does not invent an all-Off context") }
        _ = try await worker.searchModule(tab: tab, module: "Text", expression: "G140", mode: .lemma, textOptions: wanted)
        do {
            _ = try await worker.searchModule(tab: tab, module: "Missing", expression: "Original", mode: .words, textOptions: changed)
            fatalError("Expected missing module")
        } catch is CoreError {}
        guard try await worker.searchTextOptions(tab: tab) == wanted else { fatalError("Failed query cannot overwrite committed context") }
        let stopped = Task { try await worker.searchModule(tab: tab, module: "Text", expression: "Original", mode: .words,
                                                          textOptions: changed, returnCancelledResults: true) }
        stopped.cancel();let returned = try await stopped.value
        guard try await worker.searchTextOptions(tab: tab) == (returned.textOptionsUpdated ? changed : wanted) else { fatalError("Cancelled/late completed query exposes authoritative context publication") }
        try await worker.closeTab(tab); try await worker.closeTab(other);await worker.shutdown()
        guard seed.requestedTextOptions == wanted, phrase.effectiveTextOptions == wanted else { fatalError("Copied text metadata survives shutdown") }
        print("Swift composable scopes, requested/effective annotation context, phrase retention and lifetime passed")
    }
    static func batch(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab(), other = try await worker.createTab()
        let flags = CoreSearchTextOptions(strongs: true, morphology: false, footnotes: true)
        let log = ProgressLog()
        let result = try await worker.searchModules(tab: tab, modules: ["Text", "Missing", "Commentary", "Text"],
            expression: "Original", mode: .multiword, maxResults: 1, textOptions: flags,
            onProgress: { await log.append($0) })
        guard result.requestedModules == ["Text", "Missing", "Commentary", "Text"], result.groups.map(\.requestIndex) == [0, 1, 2, 3],
              result.groups.map(\.status) == [.success, .moduleNotFound, .success, .success],
              result.groups[0].result?.references == ["John 1:1"], result.groups[1].result == nil,
              result.groups[2].result?.references == [], result.groups[3].result?.executed == true,
              result.copiedResultCount == 1, result.copiedReferenceBytes == 8, result.failureCount == 1,
              result.limited, result.scope.elementCount == 4, result.scopeUpdated, result.textOptions == flags,
              result.textOptionsUpdated, try await worker.searchTextOptions(tab: tab) == flags,
              !(try await worker.lastSearchScope(tab: other)).present else { fatalError("Swift copies ordered bounded batch groups and complete isolated state") }
        let events = await log.snapshot()
        guard events.last?.phase == .complete, events.last?.percent == 100,
              zip(events, events.dropFirst()).allSatisfy({ $0.percent <= $1.percent && $0 != $1 }) else { fatalError("Batch progress is changed-only and monotonic across modules") }
        _ = try await worker.searchModule(tab: tab, module: "Text", expression: "N-ASF", mode: .morphology)
        let narrowed = try await worker.searchModules(tab: tab, modules: ["Text", "Commentary"], expression: "G140", mode: .lemma,
                                                       maxResults: 1, withinLastResults: true)
        guard narrowed.scope.elementCount == 1, narrowed.groups[0].result?.references == ["John 1:1"],
              narrowed.groups[1].result?.references == [], narrowed.groups[1].result?.executed == true else { fatalError("Batch last scope chains in core even after copied prefix fills") }
        let bounded = try await worker.searchModules(tab: other, modules: ["Commentary", "Text"], expression: "Original",
            range: CoreSearchRange(module: "Text", expression: "John1:2-John1:3"))
        guard bounded.groups.allSatisfy({ $0.result?.references == ["John 1:2", "John 1:3"] }),
              try await worker.history(tab).count == 0 else { fatalError("Shared batch range remains history neutral") }
        for modules in [[], Array(repeating: "Text", count: 33), ["Text\0bad"]] {
            do { _ = try await worker.searchModules(tab: tab, modules: modules, expression: "Original"); fatalError("Expected invalid modules") }
            catch is CoreError {}
        }
        do { _ = try await worker.searchModules(tab: tab, modules: ["Text"], expression: "Original", maxResults: 0); fatalError("Expected invalid limit") }
        catch is CoreError {}
        let defaultStop = Task { try await worker.searchModules(tab: tab, modules: ["Text", "Commentary"], expression: "Original") }
        defaultStop.cancel()
        do { _ = try await defaultStop.value; fatalError("Default cancellation must drain then throw") } catch is CancellationError {}
        let stop = Task { try await worker.searchModules(tab: tab, modules: Array(repeating: "Text", count: 32), expression: "Original", returnCancelledResults: true) }
        stop.cancel();let stopped = try await stop.value
        guard stopped.groups.count <= 32, try await worker.lastSearchScope(tab: tab) == stopped.scope else { fatalError("Interactive batch stop returns authoritative committed prefix state") }
        guard try await worker.searchModules(tab: tab, modules: ["Text"], expression: "Original").scope.elementCount == 4 else { fatalError("Fresh batch recovers after stop") }
        try await worker.closeTab(tab);try await worker.closeTab(other);await worker.shutdown()
        guard result.groups[0].result?.expression == "Original", narrowed.groups[0].result?.references == ["John 1:1"] else { fatalError("All batch copies outlive C batch and worker") }
        print("Swift ordered batch copies, per-module failures, budgets, scopes, progress, cancellation and lifetime passed")
    }
    static func main() async throws {
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--batch" {
            try await batch(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--text-options" {
            try await textOptions(root: CommandLine.arguments[2]); return
        }
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
