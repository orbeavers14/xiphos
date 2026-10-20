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
    static func generations(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab(), other = try await worker.createTab()
        guard try await worker.searchGeneration(tab: tab) == .empty,
              try await worker.searchRetentionLimits(tab: tab) == CoreSearchRetentionLimits() else { fatalError("Empty generation and bounded defaults") }
        let wanted = CoreSearchTextOptions(strongs: true, morphology: false, footnotes: true)
        let batch = try await worker.searchModules(tab: tab, modules: ["Text", "Missing", "Commentary", "Text"],
            expression: "Original", mode: .multiword, maxResults: 1, textOptions: wanted)
        let generation = batch.generation
        guard batch.generationUpdated, generation.id != 0, generation.retention == .complete,
              generation.groupCount == 4, generation.failedGroups == 1, generation.visibleResults == 1,
              generation.retainedElements == 12, batch.groups[3].result?.generation == generation,
              try await worker.searchGeneration(tab: other) == .empty else { fatalError("Swift preserves shared batch identity and isolated capacity metadata") }
        let full = try await worker.readSearchGeneration(tab: tab, id: generation.id, group: 2)
        let visible = try await worker.readSearchGeneration(tab: tab, id: generation.id, group: 2, source: .visible)
        guard full.status == .success, full.result?.references == ["John 1:1", "John 1:2", "John 1:3", "John 1:4"],
              full.query?.module == "Commentary", full.query?.textOptions == wanted,
              full.result?.generation == generation, visible.result?.references == [], visible.result?.limited == true,
              try await worker.history(tab).count == 0 else { fatalError("Complete read recovers omitted group without navigation") }
        let failed = try await worker.readSearchGeneration(tab: tab, id: generation.id, group: 1)
        let invalid = try await worker.readSearchGeneration(tab: tab, id: generation.id, group: 4)
        guard failed.status == .failedGroup, failed.failure == .moduleNotFound, failed.result == nil,
              failed.query?.module == "Missing", invalid.status == .invalidGroup, invalid.query == nil else { fatalError("Typed unavailable outcomes preserve original queries") }
        _ = try await worker.clearSearchScope(tab: tab)
        _ = try await worker.clearSearchTextOptions(tab: tab)
        let low = CoreSearchRetentionLimits(maxElements: 1, maxKeyBytes: 1024)
        guard try await worker.setSearchRetentionLimits(tab: tab, limits: low) == low,
              try await worker.searchGeneration(tab: tab) == generation else { fatalError("Reset/policy does not discard existing generation") }
        let next = try await worker.searchModule(tab: tab, module: "Text", expression: "Original")
        let overflow = try await worker.readSearchGeneration(tab: tab, id: next.generation.id, group: 0)
        let fallback = try await worker.readSearchGeneration(tab: tab, id: next.generation.id, group: 0, source: .visible, maxResults: 1)
        let stale = try await worker.readSearchGeneration(tab: tab, id: generation.id, group: 0)
        guard next.generationUpdated, next.generation.retention == .capacityExceeded,
              overflow.status == .capacityExceeded, overflow.query?.module == "Text", overflow.result == nil,
              fallback.result?.references == ["John 1:1"], fallback.result?.limited == true,
              stale.status == .stale, stale.query == nil, stale.generation == next.generation else { fatalError("No stale or unavailable read silently exports latest/full results") }
        _ = try await worker.setSearchRetentionLimits(tab: tab, limits: CoreSearchRetentionLimits())
        let bounded = try await worker.searchModule(tab: tab, module: "Text", expression: "Original", mode: .multiword,
            caseSensitive: true, range: CoreSearchRange(module: "Text", expression: "John1:2-John1:3"), textOptions: wanted)
        let copied = try await worker.readSearchGeneration(tab: tab, id: bounded.generation.id, group: 0, maxReferenceBytes: 8)
        guard copied.query?.range?.expression == "John1:2-John1:3", copied.query?.caseSensitive == true,
              copied.query?.mode == .multiword, copied.result?.references == ["John 1:2"], copied.result?.limited == true else { fatalError("Raw query and bounded copied references survive transport") }
        for bad in [-1, 0, 10001] {
            do { _ = try await worker.readSearchGeneration(tab: tab, id: bounded.generation.id, group: 0, maxResults: bad); fatalError("Expected invalid bound") }
            catch is CoreError {}
        }
        do { _ = try await worker.setSearchRetentionLimits(tab: tab, limits: CoreSearchRetentionLimits(maxElements: -1)); fatalError("Expected invalid policy") }
        catch is CoreError {}
        try await worker.closeTab(tab);try await worker.closeTab(other);await worker.shutdown()
        guard full.result?.references.count == 4, copied.query?.textOptions == wanted else { fatalError("Swift strings/metadata outlive request, tab and worker") }
        print("Swift shared generation identity, capacity, stale reads, original queries and copied lifetime passed")
    }
    static func textExport(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab()
        let search = try await worker.searchModule(tab: tab, module: "Text", expression: "Original")
        let id = search.generation.id
        let rows = [CoreSearchVisibleRow(group: 0, row: 2), CoreSearchVisibleRow(group: 0, row: 0), CoreSearchVisibleRow(group: 0, row: 2)]
        let log = ProgressLog()
        let copied = try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: rows, title: "Selected café", includeScripture: true,
            onProgress: { await log.append($0) })
        guard copied.status == .success, copied.selection == .visible, copied.rowCount == 3, copied.includesScripture, copied.generation == search.generation,
              copied.text.hasPrefix("Selected café\n\n"), copied.text.contains("John 1:3 Text"), copied.text.contains("Original"),
              (await log.snapshot()).last?.phase == .complete, try await worker.history(tab).count == 0 else { fatalError("Swift copies complete selected text and shared progress without navigating") }
        let exact = try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: rows, title: "Selected café", includeScripture: true, maxBytes: copied.text.utf8.count)
        let limited = try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: rows, title: "Selected café", includeScripture: true, maxBytes: copied.text.utf8.count - 1)
        guard exact.text == copied.text, limited.status == .outputLimit, limited.text.isEmpty, limited.rowCount == 0 else { fatalError("UTF-8 byte budgets produce all-or-nothing payloads") }
        let invalid = try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: [CoreSearchVisibleRow(group: 0, row: 99)])
        let stale = try await worker.exportSearchVisibleText(tab: tab, generation: id + 1, rows: rows)
        guard invalid.status == .invalidSelection, invalid.failedRow == 0, invalid.text.isEmpty,
              stale.status == .stale, stale.generation.id == id, stale.text.isEmpty else { fatalError("Typed failures never silently export a substitute") }
        for bad in [[], [CoreSearchVisibleRow(group: -1, row: 0)], [CoreSearchVisibleRow(group: 0, row: -1)]] {
            do { _ = try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: bad); fatalError("Expected invalid selection shape") }
            catch is CoreError {}
        }
        do { _ = try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: rows, title: "bad\0tail"); fatalError("Expected NUL rejection") }
        catch is CoreError {}
        let cancel = Task { try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: rows) };cancel.cancel()
        do { _ = try await cancel.value; fatalError("Default cancellation drains then throws") } catch is CancellationError {}
        let stop = Task { try await worker.exportSearchVisibleText(tab: tab, generation: id, rows: rows, returnCancelledResults: true) };stop.cancel()
        let stopped = try await stop.value
        guard (stopped.status == .cancelled && stopped.text.isEmpty) || stopped.status == .success else { fatalError("Interactive stop returns authoritative completion race") }
        try await worker.closeTab(tab);await worker.shutdown()
        guard copied.text.hasPrefix("Selected café"), copied.generation.id == id else { fatalError("Copied Swift export outlives worker") }
        print("Swift selected text export, exact UTF-8 caps, typed failures, cancellation and lifetime passed")
    }
    static func allTextExport(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab()
        let search = try await worker.searchModules(tab: tab, modules: ["Text", "Commentary", "Text"], expression: "Original", maxResults: 1)
        let id = search.generation.id
        let all = try await worker.exportSearchAllText(tab: tab, generation: id, includeScripture: true)
        guard all.status == .success, all.selection == .allRetained, all.rowCount == 12,
              all.generation.visibleResults == 1, all.text.contains("John 1:4"), all.includesScripture else { fatalError("Swift complete export recovers every retained group") }
        let exact = try await worker.exportSearchAllText(tab: tab, generation: id, includeScripture: true, maxBytes: all.text.utf8.count, maxRows: 12)
        let clipped = try await worker.exportSearchAllText(tab: tab, generation: id, maxRows: 11)
        guard exact.text == all.text, clipped.status == .outputLimit, clipped.text.isEmpty else { fatalError("Complete export rejects clipped group/document") }
        let failed = try await worker.searchModules(tab: tab, modules: ["Text", "Missing"], expression: "Original", maxResults: 1)
        let refused = try await worker.exportSearchAllText(tab: tab, generation: failed.generation.id)
        let accepted = try await worker.exportSearchAllText(tab: tab, generation: failed.generation.id, allowPartialSource: true)
        guard refused.status == .sourceIncomplete, refused.text.isEmpty, accepted.status == .success,
              accepted.rowCount == 4, accepted.generation.failedGroups == 1 else { fatalError("Explicit source acceptance remains a shared policy with copied failure metadata") }
        _ = try await worker.setSearchRetentionLimits(tab: tab, limits: CoreSearchRetentionLimits(maxElements: 1, maxKeyBytes: 1024))
        let overflow = try await worker.searchModule(tab: tab, module: "Text", expression: "Original")
        let unavailable = try await worker.exportSearchAllText(tab: tab, generation: overflow.generation.id)
        let stale = try await worker.exportSearchAllText(tab: tab, generation: id)
        guard unavailable.status == .capacityExceeded, unavailable.selection == .allRetained, unavailable.text.isEmpty,
              stale.status == .stale, stale.generation == overflow.generation else { fatalError("Complete export never substitutes visible or newer keys") }
        for rows in [-1, 0, 10001] {
            do { _ = try await worker.exportSearchAllText(tab: tab, generation: id, maxRows: rows); fatalError("Expected invalid row cap") } catch is CoreError {}
        }
        let stop = Task { try await worker.exportSearchAllText(tab: tab, generation: overflow.generation.id) };stop.cancel()
        do { _ = try await stop.value; fatalError("Cancelled complete export drains then throws") } catch is CancellationError {}
        try await worker.closeTab(tab);await worker.shutdown()
        guard all.rowCount == 12, all.text.contains("Original"), accepted.generation.failedGroups == 1 else { fatalError("Complete copied values survive shutdown") }
        print("Swift complete retained export, strict caps, explicit partial-source choice and lifetime passed")
    }
    static func htmlExport(root: String, richRoot: String) async throws {
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab()
        let search = try await worker.searchModules(tab: tab, modules: ["Text", "Commentary"], expression: "Original", maxResults: 1)
        let id = search.generation.id, title = "Original <query> café"
        let log = ProgressLog()
        let all = try await worker.exportSearchHTML(tab: tab, generation: id, selection: .allRetained, title: title,
            includeScripture: true, onProgress: { await log.append($0) })
        guard all.status == .success, all.selection == .allRetained, all.rowCount == 8, all.issue == .none,
              all.html.contains("Original &lt;query&gt; café"), all.generation.visibleResults == 1,
              (await log.snapshot()).last?.phase == .complete else { fatalError("Swift copies complete shared HTML and source/progress metadata") }
        let exact = try await worker.exportSearchHTML(tab: tab, generation: id, selection: .allRetained, title: title,
            includeScripture: true, maxBytes: all.html.utf8.count, maxRows: 8)
        let limited = try await worker.exportSearchHTML(tab: tab, generation: id, selection: .allRetained, title: title,
            includeScripture: true, maxBytes: all.html.utf8.count - 1)
        guard exact.html == all.html, limited.status == .outputLimit, limited.html.isEmpty, limited.rowCount == 0 else { fatalError("HTML exact UTF-8 byte cap and no partial document survive transport") }
        let selected = try await worker.exportSearchHTML(tab: tab, generation: id,
            rows: [CoreSearchVisibleRow(group: 0, row: 0), CoreSearchVisibleRow(group: 0, row: 0)])
        let stale = try await worker.exportSearchHTML(tab: tab, generation: id + 1, selection: .allRetained)
        guard selected.rowCount == 2, selected.selection == .visible, stale.status == .stale, stale.generation.id == id,
              stale.html.isEmpty, try await worker.history(tab).count == 0 else { fatalError("Swift preserves duplicate selection, stale identity and history neutrality") }
        for bad in [[], [CoreSearchVisibleRow(group: -1, row: 0)], [CoreSearchVisibleRow(group: 0, row: -1)]] {
            do { _ = try await worker.exportSearchHTML(tab: tab, generation: id, rows: bad); fatalError("Expected invalid HTML selection") } catch is CoreError {}
        }
        do { _ = try await worker.exportSearchHTML(tab: tab, generation: id, selection: .allRetained, title: "bad\0tail"); fatalError("Expected NUL rejection") } catch is CoreError {}
        let failed = try await worker.searchModules(tab: tab, modules: ["Text", "Missing"], expression: "Original")
        let refused = try await worker.exportSearchHTML(tab: tab, generation: failed.generation.id, selection: .allRetained)
        let partial = try await worker.exportSearchHTML(tab: tab, generation: failed.generation.id, selection: .allRetained, allowPartialSource: true)
        guard refused.status == .sourceIncomplete, refused.html.isEmpty, partial.status == .success,
              partial.generation.failedGroups == 1, partial.html.contains("only returned matches are included") else { fatalError("Partial-source choice and artifact labelling stay in shared core") }
        let stop = Task { try await worker.exportSearchHTML(tab: tab, generation: failed.generation.id, selection: .allRetained, allowPartialSource: true) };stop.cancel()
        do { _ = try await stop.value; fatalError("Cancelled HTML observer must drain then throw") } catch is CancellationError {}
        try await worker.closeTab(tab);await worker.shutdown()
        let richWorker = try CoreWorker(root: richRoot), richTab = try await richWorker.createTab()
        let rich = try await richWorker.searchModule(tab: richTab, module: "Text", expression: "Original")
        guard let ordinal = rich.references.firstIndex(of: "John 4:1") else { fatalError("Original rich source required") }
        let unsupported = try await richWorker.exportSearchHTML(tab: richTab, generation: rich.generation.id,
            rows: [CoreSearchVisibleRow(group: 0, row: ordinal)], includeScripture: true)
        guard unsupported.status == .unsupportedHTML, unsupported.issue == .resources, unsupported.failedRow == 0,
              unsupported.html.isEmpty, unsupported.rowCount == 0 else { fatalError("Typed unsupported assets cannot become successful Swift HTML") }
        try await richWorker.closeTab(richTab);await richWorker.shutdown()
        guard all.rowCount == 8, all.html.contains("Original"), partial.generation.failedGroups == 1 else { fatalError("Copied HTML values outlive observers and both workers") }
        print("Swift copied HTML, strict bytes, duplicate/stale selection, partial/unsupported outcomes and lifetime passed")
    }
    static func main() async throws {
        if CommandLine.arguments.count == 4, CommandLine.arguments[1] == "--html" {
            try await htmlExport(root: CommandLine.arguments[2], richRoot: CommandLine.arguments[3]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--all-export" {
            try await allTextExport(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--export" {
            try await textExport(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--generation" {
            try await generations(root: CommandLine.arguments[2]); return
        }
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
