// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore
@main struct LexicalAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("original dictionary fixture required") }
        guard LexiconOrigin.main.rawValue == Int32(XIPHOS_LEXICON_MAIN.rawValue),
              LexiconOrigin.dialog.rawValue == Int32(XIPHOS_LEXICON_DIALOG.rawValue),
              LexicalKind.strongs.rawValue == Int32(XIPHOS_LEXICON_STRONGS.rawValue),
              LexicalKind.morphology.rawValue == Int32(XIPHOS_LEXICON_MORPHOLOGY.rawValue) else { fatalError("lexical ABI constants") }
        let synchronousModules = try { let reader = try CoreReader(root: CommandLine.arguments[1]); return reader.modules }()
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let modules = try await worker.modules()
        for module in modules {
            guard let synchronous = synchronousModules.first(where: { $0.id == module.id }),
                  synchronous.isDictionary == module.isDictionary, synchronous.lexiconRoles == module.lexiconRoles else { fatalError("synchronous/worker catalog metadata") }
        }
        let roles: [String: LexiconRoles] = ["DictionaryOne": .greekDefinition, "DictionaryTwo": .hebrewDefinition,
            "MorphGreek": .greekMorphology, "MorphHebrew": .hebrewMorphology, "GenericDictionary": [], "CaseFeature": []]
        for (id, role) in roles {
            guard let module = modules.first(where: { $0.id == id }), module.isDictionary, module.lexiconRoles == role else { fatalError("copied dictionary role metadata") }
        }
        guard let devotional = modules.first(where: { $0.id == "OriginalDevotional" }), !devotional.isDictionary, devotional.lexiconRoles.isEmpty,
              let bible = modules.first(where: { $0.id == "OriginalBible" }), !bible.isDictionary, bible.lexiconRoles.isEmpty else { fatalError("non-dictionary feature filtering") }
        let first = try await worker.createTab(), second = try await worker.createTab()
        guard try await worker.lexiconPreferences(tab: first) == nil else { fatalError("lexicons should start unconfigured") }
        try await worker.enableLexiconPreferences(tab: first)
        guard let defaults = try await worker.lexiconPreferences(tab: first),
              defaults.greek == "StrongsGreek", defaults.hebrew == "StrongsHebrew",
              defaults.greekMorphNT == "Robinson", defaults.greekMorphOT == "Packard", defaults.hebrewMorph == "OSHM" else { fatalError("shared default names") }
        var preferences = LexiconPreferences(greek: "DictionaryOne", hebrew: "", greekMorphNT: "DictionaryOne", greekMorphOT: "DictionaryTwo", hebrewMorph: nil)
        try await worker.setLexiconPreferences(tab: first, preferences: preferences)
        let copied = try await worker.lexiconPreferences(tab: first)
        guard copied == preferences, copied?.hebrew == "", copied?.hebrewMorph == nil else { fatalError("copied nullable preferences") }
        preferences.greek = "DictionaryTwo"
        try await worker.setLexiconPreferences(tab: second, preferences: preferences)
        let strongs = "xiphos.url?action=showStrongs&type=Greek&value=alpha"
        let firstEntry = try await worker.readLexical(tab: first, module: "OriginalBible", reference: "John3:16", uri: strongs)
        guard case .lexical(let contextual) = try await worker.resolveStudyLinkAt(tab: first, currentModule: "OriginalBible", currentReference: "John3:16", uri: strongs),
              contextual.entry.module == firstEntry.entry.module, contextual.entry.html == firstEntry.entry.html,
              contextual.sourceReference == "John3:16" else { fatalError("contextual lexical adapter") }
        do {
            _ = try await worker.resolveStudyLinkAt(tab: first, currentModule: "OriginalBible", currentReference: "John3:16", uri: strongs + "\0ignored")
            fatalError("contextual embedded NUL accepted")
        } catch is CoreError { }
        let secondEntry = try await worker.readLexical(tab: second, module: "", reference: "", uri: strongs)
        guard firstEntry.kind == .strongs, firstEntry.entry.module == "DictionaryOne", firstEntry.entry.html.contains("café"),
              firstEntry.module == "OriginalBible", firstEntry.sourceReference == "John3:16", firstEntry.type == "Greek", firstEntry.value == "alpha",
              secondEntry.entry.module == "DictionaryTwo" else { fatalError("copied lookup metadata/tab isolation") }
        let morph = try await worker.readLexical(tab: first, module: "Missing", reference: "Gen1:1",
            uri: "passagestudy.jsp?action=showMorph&type=Greek&value=alpha&module=OriginalBible&passage=John3:16", origin: .dialog)
        guard morph.kind == .morphology, morph.entry.module == "DictionaryTwo", morph.sourceReference == "Gen1:1", morph.passage == "John3:16" else { fatalError("source testament/query metadata") }
        preferences.greek = "Broken\0Name"
        do { try await worker.setLexiconPreferences(tab: first, preferences: preferences); fatalError("NUL preference accepted") }
        catch is CoreError { }
        guard try await worker.lexiconPreferences(tab: first) == copied else { fatalError("failed setting changed state") }
        for (module, reference, uri) in [("Missing", "John3:16", "xiphos.url?action=showMorph&type=Greek&value=alpha"),
                                       ("OriginalBible", "\0", strongs),
                                       ("OriginalBible", "John3:16", "https://example/xiphos.url?action=showStrongs&value=1")] {
            do { _ = try await worker.readLexical(tab: first, module: module, reference: reference, uri: uri); fatalError("invalid lookup accepted") }
            catch is CoreError { }
        }
        guard try await worker.history(first).count == 0 else { fatalError("lookup changed Bible history") }
        try await worker.closeTab(first)
        do { _ = try await worker.readLexical(tab: first, module: "", reference: "", uri: strongs); fatalError("closed tab accepted") }
        catch is CoreError { }
        try await worker.closeTab(second); await worker.shutdown()
        guard copied?.greek == "DictionaryOne", firstEntry.entry.html.contains("café"), morph.entry.module == "DictionaryTwo" else { fatalError("copied values after shutdown") }
        print("Swift lexical adapter checks passed")
    }
}
