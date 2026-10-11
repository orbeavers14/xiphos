// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore
@main
struct OptionAdapterSmoke {
    @MainActor static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("option fixture root required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let modules = try await worker.modules()
        let declared: EngineOptionFlags = [.wordsInRed, .strongs, .morphology, .footnotes]
        guard modules.count == 1, modules[0].options.declared == declared,
              !modules[0].options.hasHeadings else { fatalError("copied module option metadata") }
        guard VariantReading.primary.rawValue == Int32(XIPHOS_VARIANT_PRIMARY.rawValue),
              VariantReading.secondary.rawValue == Int32(XIPHOS_VARIANT_SECONDARY.rawValue),
              VariantReading.all.rawValue == Int32(XIPHOS_VARIANT_ALL.rawValue) else { fatalError("Swift variant ABI values") }
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
        let unconfigured = try await worker.moduleEngineOptions(tab: tab, module: "OptionTest")
        guard unconfigured == nil else { fatalError("SDK state distinguished from explicit preferences") }
        try await worker.enableModulePreferences(tab: tab)
        let defaults = try await worker.moduleEngineOptions(tab: tab, module: "OptionTest")
        guard let defaults, defaults.contains(.footnotes), defaults.contains(.wordsInRed),
              defaults.contains(.variantsPrimary), !defaults.contains(.strongs) else { fatalError("shared legacy defaults") }
        try await worker.setModuleEngineOptions(tab: tab, module: "OptionTest", options: [.strongs])
        let changed = try await worker.moduleEngineOptions(tab: tab, module: "OptionTest")
        guard changed == [.strongs] else { fatalError("module options round trip") }
        let changedEntry = try await worker.readTabEntry(tab: tab, module: "OptionTest", reference: "John 3:16")
        guard changedEntry.html.contains("3056"), !changedEntry.html.contains("wordsOfJesus") else { fatalError("module preferences render") }
        try await worker.clearModuleEngineOptions(tab: tab, module: "OptionTest")
        let cleared = try await worker.moduleEngineOptions(tab: tab, module: "OptionTest")
        guard cleared == defaults else { fatalError("module clear restores shared defaults") }
        let unchangedHistory = try await worker.history(tab)
        guard unchangedHistory.count == 1 else { fatalError("module settings changed history") }
        let preferenceURL = URL(fileURLWithPath: CommandLine.arguments[1]).appendingPathComponent("adapter-modops.conf")
        let preferenceBytes = Data("; Original adapter fixture\n[OptionTest]\nStrong's Numbers=On\nWords of Christ in Red=Off\nFootnotes=Off\nCustomValue=café\n".utf8)
        try preferenceBytes.write(to: preferenceURL)
        try await worker.importModulePreferences(tab: tab, path: preferenceURL.path)
        let imported = try await worker.moduleEngineOptions(tab: tab, module: "OptionTest")
        guard imported?.contains(.strongs) == true, imported?.contains(.wordsInRed) == false,
              imported?.contains(.footnotes) == false else { fatalError("Swift explicit preference import") }
        do {
            try await worker.importModulePreferences(tab: tab, path: preferenceURL.path + ".missing")
            fatalError("missing import accepted")
        } catch is CoreError { }
        let preservedOptions = try await worker.moduleEngineOptions(tab: tab, module: "OptionTest")
        let importHistory = try await worker.history(tab)
        guard preservedOptions == imported, importHistory.count == 1,
              try Data(contentsOf: preferenceURL) == preferenceBytes else { fatalError("import failure/source/history preservation") }
        let saveURL = URL(fileURLWithPath: CommandLine.arguments[1]).appendingPathComponent("adapter-saved-modops.conf")
        try await worker.saveModulePreferences(tab: tab, path: saveURL.path)
        let restoredTab = try await worker.createTab()
        try await worker.importModulePreferences(tab: restoredTab, path: saveURL.path)
        let savedOptions = try await worker.moduleEngineOptions(tab: restoredTab, module: "OptionTest")
        guard savedOptions == imported else { fatalError("Swift explicit save/reload") }
        _ = try await worker.readTabChapter(tab: tab, module: "OptionTest", reference: "John 3:16", verseNumbers: true, highlightSelected: true)
        let note = try await worker.studyNote(tab: tab, module: "OptionTest", reference: "John 3:16", identifier: "1")
        guard note.reference == "John 3:16", note.identifier == "1", note.type == "study",
              note.label == "a", note.html.contains("Original note."), note.referenceList.isEmpty else { fatalError("Swift copied study-note metadata/body") }
        do {
            _ = try await worker.studyNote(tab: tab, module: "OptionTest", reference: "John 3:17", identifier: "1")
            fatalError("Swift stale note accepted")
        } catch is CoreError { }
        let linkedNote = try await worker.studyNoteLink(tab: tab, currentModule: "OptionTest",
            uri: "xiphos-preview://study/passagestudy.jsp?action=showNote&type=n&value=1&module=OptionTest&passage=John+3%3A16")
        guard linkedNote.html == note.html, linkedNote.reference == note.reference else { fatalError("Swift core note link dispatch") }
        do {
            _ = try await worker.studyNoteLink(tab: tab, currentModule: "OptionTest", uri: "https://example.invalid/passagestudy.jsp")
            fatalError("Swift external note link accepted")
        } catch is CoreError { }
        let references = try await worker.readReferenceList(tab: tab, module: "OptionTest", expression: "16-17;16", context: "John.3.16")
        guard references.module == "OptionTest", references.context == "John 3:16", references.expression == "16-17;16",
              references.entries.count == 3, references.entries[0].reference == "John 3:16",
              references.entries[2].reference == "John 3:16", references.entries[0].html.contains("3056"),
              references.html.contains("reference-3"), !references.limited else { fatalError("Swift reference-list metadata/options/order") }
        let emptyReferences = try await worker.readReferenceList(tab: tab, module: "OptionTest", expression: "", context: "John3:16")
        guard emptyReferences.entries.isEmpty, emptyReferences.html.isEmpty else { fatalError("Swift empty reference list") }
        do {
            _ = try await worker.readReferenceList(tab: tab, module: "OptionTest", expression: "16", context: "")
            fatalError("Swift missing reference-list context accepted")
        } catch is CoreError { }
        let contextual = try await worker.resolveStudyLinkAt(tab: tab, currentModule: "OptionTest", currentReference: "John3:16",
            uri: "passagestudy.jsp?action=showRef&type=scripRef&value=16-17%3B16&module=")
        guard case .references(let contextualReferences) = contextual, contextualReferences.entries.count == 3,
              contextualReferences.context == "John 3:16", contextualReferences.entries[0].html.contains("3056") else { fatalError("Swift contextual reference result") }
        let contextualNote = try await worker.resolveStudyLinkAt(tab: tab, currentModule: "OptionTest", currentReference: "John3:17",
            uri: "passagestudy.jsp?action=showNote&type=n&value=1&passage=John3:16")
        guard case .note(let dispatchedNote) = contextualNote, dispatchedNote.html == note.html else { fatalError("Swift contextual note result") }
        let contextualPassage = try await worker.resolveStudyLinkAt(tab: tab, currentModule: "OptionTest", currentReference: "John3:16", uri: "sword:///John3:17")
        guard case .passage(let dispatchedPassage) = contextualPassage, dispatchedPassage.module == "OptionTest", dispatchedPassage.reference == "John3:17" else { fatalError("Swift contextual passage result") }
        do {
            _ = try await worker.resolveStudyLinkAt(tab: tab, currentModule: "OptionTest", currentReference: "", uri: "sword:///John3:17")
            fatalError("Swift missing contextual reference accepted")
        } catch is CoreError { }
        do {
            try await worker.selectModuleVariant(tab: tab, module: "OptionTest", reading: .all)
            fatalError("Swift undeclared variant accepted")
        } catch is CoreError { }
        let noteHistory = try await worker.history(tab)
        guard noteHistory.count == 1 else { fatalError("Swift note preview changed history") }
        try await worker.closeTab(restoredTab)
        try await worker.closeTab(tab)
        await worker.shutdown()
        guard contextualReferences.entries[0].html.contains("3056"), references.entries[0].html.contains("3056"), note.html.contains("Original note."), modules[0].options.declared == declared, restored.html == original.html else { fatalError("copied metadata/results lifetime") }
        print("Swift tab option/metadata adapter checks passed")
    }
}
