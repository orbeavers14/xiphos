// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct ProfileAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 3 else { fatalError("dictionary/disposable profile roots required") }
        let root = URL(fileURLWithPath: CommandLine.arguments[2], isDirectory: true)
        try FileManager.default.createDirectory(at: root, withIntermediateDirectories: true)
        let source = root.appendingPathComponent("profile-café.xml"), saved = root.appendingPathComponent("saved.xml")
        let malformed = root.appendingPathComponent("malformed.xml")
        let original = "<Xiphos><!--retain--><lexicons><greek>DictionaryOne</greek><hebrew>Unavailable &amp; Hebrew</hebrew><greek_morph_nt>MorphGreek</greek_morph_nt></lexicons><locale><special>tr_TR.UTF-8</special></locale><other>retained</other><modlists><modlist label='Original' list='DictionaryOne,, MissingModule,DictionaryOne,'/><modlist label='Original' list='Shadowed'/><modlist label='' list=''/><modlist list='Unlabelled'/><modlist label='Absent'/></modlists><ranges><range label='café α' list=' John1:1-2 '/></ranges></Xiphos>"
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
        let searchLists = try await worker.readSearchLists(tab: first, path: source.path)
        guard searchLists.moduleLists.count == 5, searchLists.ranges.count == 1,
              searchLists.moduleLists[0].effectiveIndex == 0, searchLists.moduleLists[1].effectiveIndex == 0,
              searchLists.moduleLists[1].value == "Shadowed", searchLists.moduleLists[2].label == "",
              searchLists.moduleLists[2].value == "", searchLists.moduleLists[3].label == nil,
              searchLists.moduleLists[4].value == nil, searchLists.ranges[0].label == "café α",
              searchLists.ranges[0].value == " John1:1-2 " else { fatalError("copied named lists and shared duplicate-label selection") }
        let selectedValue = searchLists.moduleLists[searchLists.moduleLists[1].effectiveIndex].value!
        guard try await worker.parseSearchModuleList(tab: first, text: selectedValue) == ["DictionaryOne", " MissingModule", "DictionaryOne"],
              try await worker.parseSearchModuleList(tab: first, text: ",,,").isEmpty else { fatalError("shared typed module splitting") }
        for text in ["Text\0Hidden", String(repeating: "x", count: 4097), Array(repeating: "Text", count: 33).joined(separator: ",")] {
            do { _ = try await worker.parseSearchModuleList(tab: first, text: text); fatalError("invalid module list accepted") }
            catch is CoreError { }
        }
        do { _ = try await worker.readSearchLists(tab: first, path: malformed.path); fatalError("malformed named-list profile accepted") }
        catch is CoreError { }
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
        let namedFile = root.appendingPathComponent("named-save.xml")
        try Data(original.utf8).write(to: namedFile)
        let edited = try await worker.saveSearchList(tab: first, path: namedFile.path,
            edit: SavedSearchListEdit(kind: .modules, label: "Original", value: "DictionaryOne, MissingModule,DictionaryOne"))
        guard edited.moduleLists[0].value == "DictionaryOne, MissingModule,DictionaryOne",
              edited.moduleLists[1].value == "Shadowed", edited.moduleLists[1].effectiveIndex == 0,
              try await worker.readSearchLists(tab: second, path: namedFile.path) == edited,
              try Data(contentsOf: source) == Data(original.utf8) else { fatalError("typed save returns the verified shared snapshot and preserves source/duplicate semantics") }
        let ranged = try await worker.saveSearchList(tab: first, path: namedFile.path,
            edit: SavedSearchListEdit(kind: .ranges, label: "café α", value: " John1:2-3 "))
        guard ranged.ranges[0].value == " John1:2-3 ",
              try await worker.lexiconPreferences(tab: first) == imported,
              try await worker.moduleEngineOptions(tab: first, module: "OriginalBible") == [.strongs],
              try await worker.history(first).count == history.count else { fatalError("saved-list edit does not apply settings, history or normalize ranges") }
        let beforeNamed = try Data(contentsOf: namedFile)
        for edit in [SavedSearchListEdit(label: "NUL\0label", value: "Text"), SavedSearchListEdit(label: "Bad", value: "NUL\0value"), SavedSearchListEdit(label: "Bad", value: "\u{01}")] {
            do { _ = try await worker.saveSearchList(tab: first, path: namedFile.path, edit: edit); fatalError("invalid edit accepted") }
            catch is CoreError { }
        }
        for path in ["", "bad\0path", root.path] {
            do { _ = try await worker.saveSearchList(tab: first, path: path, edit: SavedSearchListEdit()); fatalError("invalid save path accepted") }
            catch is CoreError { }
        }
        guard try Data(contentsOf: namedFile) == beforeNamed else { fatalError("failed typed edits preserve destination") }
        let saveTask = Task { try await worker.saveSearchList(tab: first, path: namedFile.path,
            edit: SavedSearchListEdit(kind: .ranges, label: "Accepted", value: "John1:4")) }
        saveTask.cancel(); let drainedSave = try await saveTask.value
        guard drainedSave.ranges.last?.value == "John1:4" else { fatalError("observer cancellation drains accepted save rather than pretending to undo it") }
        try await worker.closeTab(first)
        do { _ = try await worker.importLexiconProfile(tab: first, path: source.path); fatalError("closed import tab accepted") }
        catch is CoreError { }
        do { _ = try await worker.readSearchLists(tab: first, path: source.path); fatalError("closed named-list tab accepted") }
        catch is CoreError { }
        do { _ = try await worker.parseSearchModuleList(tab: first, text: "DictionaryOne"); fatalError("closed module-list tab accepted") }
        catch is CoreError { }
        do { _ = try await worker.saveSearchList(tab: first, path: namedFile.path, edit: SavedSearchListEdit()); fatalError("closed save tab accepted") }
        catch is CoreError { }
        try await worker.closeTab(second); await worker.shutdown()
        guard edited.moduleLists[0].value == "DictionaryOne, MissingModule,DictionaryOne", drainedSave.ranges.last?.value == "John1:4" else { fatalError("copied staged snapshots survive shutdown") }
        guard searchLists.ranges[0].label == "café α", searchLists.moduleLists[1].effectiveIndex == 0 else { fatalError("named-list values after shutdown") }
        guard imported.greek == "DictionaryOne", imported.hebrew == "Unavailable & Hebrew", lookup.entry.module == "DictionaryOne" else { fatalError("copied typed results after shutdown") }
        print("Swift profile import/save, isolation, ownership and failure checks passed")
    }
}
