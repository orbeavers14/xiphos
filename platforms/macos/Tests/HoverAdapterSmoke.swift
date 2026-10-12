// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct HoverAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 3 else { fatalError("note/dictionary roots required") }
        let body = "passagestudy.jsp?action=showNote&type=n&value=1&module=OptionOther&passage=John3%3A19"
        let xref = "passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John3%3A20"
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let first = try await worker.createTab(), second = try await worker.createTab()
        try await worker.enableModulePreferences(tab: first); try await worker.enableModulePreferences(tab: second)
        _ = try await worker.visit(tab: first, module: "OptionOther", reference: "John3:23")
        guard try await worker.crossReferencePreviewList(tab: first), try await worker.crossReferencePreviewList(tab: second) else { fatalError("copied legacy preview defaults") }
        guard case .note(let copied) = try await worker.hoverStudyLinkAt(tab: first, currentModule: "OptionOther", currentReference: "John3:23", uri: body),
              copied.reference == "John 3:19", copied.html.contains("Original other note") else { fatalError("typed body hover result") }
        guard case .references(let references) = try await worker.hoverStudyLinkAt(tab: first, currentModule: "OptionOther", currentReference: "John3:23", uri: xref),
              references.context == "John 3:23", references.entries.count == 5, references.entries[0].reference == references.entries[4].reference else { fatalError("typed ordered cross-reference hover") }
        try await worker.setCrossReferencePreviewList(tab: first, enabled: false)
        guard !(try await worker.crossReferencePreviewList(tab: first)), try await worker.crossReferencePreviewList(tab: second) else { fatalError("typed preview preference isolation") }
        guard case .note(let links) = try await worker.hoverStudyLinkAt(tab: first, currentModule: "OptionOther", currentReference: "John3:23", uri: xref),
              links.type == "crossReference", links.html.contains("Back to John 3:23"), !links.html.contains("Original other fixture") else { fatalError("typed link-only information destination") }
        for uri in ["sword:///John3:16", "passagestudy.jsp?action=showRef&type=scripRef&value=16-19&module=OptionOther"] {
            guard case .noPresentation = try await worker.hoverStudyLinkAt(tab: first, currentModule: "OptionOther", currentReference: "John3:23", uri: uri) else { fatalError("typed no-presentation marker") }
        }
        for (context, uri) in [("", body), ("John3:23\0ignored", body), ("John3:23", "https://example.invalid/passagestudy.jsp"), ("John3:23", body + "\0ignored")] {
            do { _ = try await worker.hoverStudyLinkAt(tab: first, currentModule: "OptionOther", currentReference: context, uri: uri); fatalError("invalid hover accepted") }
            catch is CoreError { }
        }
        guard try await worker.history(first).count == 1, try await worker.dictionaryHistory(tab: first).entries.isEmpty else { fatalError("typed hover/preferences changed histories") }
        try await worker.closeTab(first)
        do { _ = try await worker.hoverStudyLinkAt(tab: first, currentModule: "OptionOther", currentReference: "John3:23", uri: body); fatalError("closed hover accepted") }
        catch is CoreError { }
        try await worker.closeTab(second); await worker.shutdown()
        guard copied.html.contains("Original other note"), references.entries.count == 5, links.html.contains("Back to John 3:23") else { fatalError("copied hover snapshots after shutdown") }
        let dictionaries = try CoreWorker(root: CommandLine.arguments[2]), tab = try await dictionaries.createTab()
        try await dictionaries.setLexiconPreferences(tab: tab, preferences: LexiconPreferences(greek: "DictionaryOne"))
        guard case .lexical(let lexical) = try await dictionaries.hoverStudyLinkAt(tab: tab, currentModule: "OriginalBible", currentReference: "John3:16", uri: "xiphos.url?action=showStrongs&type=Greek&value=caf%C3%A9"),
              lexical.value == "café", lexical.entry.module == "DictionaryOne", try await dictionaries.dictionaryHistory(tab: tab).entries.isEmpty else { fatalError("typed raw lexical hover") }
        try await dictionaries.closeTab(tab); await dictionaries.shutdown()
        print("Swift main-hover destination, preference, ownership and failure checks passed")
    }
}
