// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct ProfileAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 3 else { fatalError("dictionary/disposable profile roots required") }
        let root = URL(fileURLWithPath: CommandLine.arguments[2], isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: true)
        let source = root.appendingPathComponent("profile-café.xml"), saved = root.appendingPathComponent("saved.xml")
        let malformed = root.appendingPathComponent("malformed.xml")
        let original = "<Xiphos><!--retain--><lexicons><greek>DictionaryOne</greek><hebrew>Unavailable &amp; Hebrew</hebrew><greek_morph_nt>MorphGreek</greek_morph_nt></lexicons><locale><special>tr_TR.UTF-8</special></locale><other>retained</other></Xiphos>"
        try Data(original.utf8).write(to: source)
        try Data("<Xiphos>".utf8).write(to: malformed)
        if FileManager.default.fileExists(atPath: saved.path) { try FileManager.default.removeItem(at: saved) }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let first = try await worker.createTab(), second = try await worker.createTab()
        try await worker.enableLexiconPreferences(tab: second)
        let otherChoices = try await worker.lexiconPreferences(tab: second)
        _ = try await worker.visit(tab: first, module: "OriginalBible", reference: "John3:16")
        _ = try await worker.visitDictionary(tab: first, module: "DictionaryOne", key: "alpha")
        try await worker.setModuleEngineOptions(tab: first, module: "OriginalBible", options: [.strongs])
        let imported = try await worker.importLexiconProfile(tab: first, path: source.path)
        guard imported.greek == "DictionaryOne", imported.hebrew == "Unavailable & Hebrew",
              imported.greekMorphNT == "MorphGreek", imported.greekMorphOT == "Packard", imported.hebrewMorph == "OSHM",
              try await worker.lexiconPreferences(tab: second) == otherChoices else { fatalError("typed profile import/defaults/window isolation") }
        guard try await worker.moduleEngineOptions(tab: first, module: "OriginalBible") == [.strongs] else { fatalError("profile import changed reading options") }
        let history = try await worker.history(first), dictionary = try await worker.dictionaryHistory(tab: first)
        let lookup = try await worker.readLexical(tab: first, module: "OriginalBible", reference: "John3:16", uri: "xiphos.url?action=showStrongs&type=Greek&value=03056")
        guard lookup.entry.module == "DictionaryOne", lookup.entry.html.contains("Original word definition") else { fatalError("typed imported choice drives shared lookup") }
        do { _ = try await worker.importLexiconProfile(tab: first, path: malformed.path); fatalError("malformed profile accepted") }
        catch is CoreError { }
        guard try await worker.lexiconPreferences(tab: first) == imported else { fatalError("failed typed import changed preferences") }
        try await worker.saveLexiconProfile(tab: first, path: saved.path)
        let savedXML = try String(contentsOf: saved, encoding: .utf8)
        guard savedXML.contains("Unavailable &amp; Hebrew"), try Data(contentsOf: source) == Data(original.utf8) else { fatalError("typed save escaping/source preservation") }
        guard try await worker.importLexiconProfile(tab: second, path: saved.path) == imported else { fatalError("typed save/import round trip") }
        for path in ["", "ignored\0path", root.appendingPathComponent("missing.xml").path] {
            do { _ = try await worker.importLexiconProfile(tab: first, path: path); fatalError("invalid import path accepted") }
            catch is CoreError { }
        }
        for path in ["", "ignored\0path", root.path] {
            do { try await worker.saveLexiconProfile(tab: first, path: path); fatalError("invalid save path accepted") }
            catch is CoreError { }
        }
        guard try await worker.history(first).count == history.count,
              try await worker.dictionaryHistory(tab: first) == dictionary else { fatalError("typed profile commands changed histories") }
        try await worker.closeTab(first)
        do { _ = try await worker.importLexiconProfile(tab: first, path: source.path); fatalError("closed import tab accepted") }
        catch is CoreError { }
        try await worker.closeTab(second); await worker.shutdown()
        guard imported.greek == "DictionaryOne", imported.hebrew == "Unavailable & Hebrew", lookup.entry.module == "DictionaryOne" else { fatalError("copied typed results after shutdown") }
        print("Swift profile import/save, isolation, ownership and failure checks passed")
    }
}
