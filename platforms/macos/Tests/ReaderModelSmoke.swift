// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main
struct ReaderModelSmoke {
    @MainActor static func idle(_ model: ReaderModel) async throws {
        let deadline = Date().addingTimeInterval(10)
        while model.isBusy {
            guard Date() < deadline else { fatalError("model request timeout") }
            try await Task.sleep(nanoseconds: 1_000_000)
        }
    }
    @MainActor static func readingOptions(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let first = ReaderModel(worker: worker, startupError: nil)
        let second = ReaderModel(worker: worker, startupError: nil)
        try await idle(first); try await idle(second)
        first.selection = "OptionTest"; first.reference = "John 3:16"; first.read()
        second.selection = "OptionTest"; second.reference = "John 3:16"; second.read()
        try await idle(first); try await idle(second)
        // Legacy history deduplicates references across modules and retains
        // the first module. Start this restoration scenario with a fresh visit.
        first.clearHistory(); try await idle(first)
        first.read(); try await idle(first)
        guard let initial = first.engineOptions, initial.contains(.wordsInRed), initial.contains(.footnotes),
              initial.contains(.variantsPrimary), !initial.contains(.strongs) else { fatalError("model uses shared defaults") }
        first.reference = "John 4:2" // unsubmitted input must survive option refresh
        first.setReadingOption(.strongs, enabled: true)
        first.setReadingOption(.wordsInRed, enabled: false) // busy action is ignored
        try await idle(first)
        guard first.reference == "John 4:2", first.renderedReference == "John 3:16",
              first.html.contains("3056"), first.html.contains("wordsOfJesus"),
              first.engineOptions == initial.union(.strongs), !first.canBack,
              !second.html.contains("3056"), second.engineOptions == initial else { fatalError("option refresh/draft/history/window isolation") }
        first.showChapter = true; first.refreshDisplay(); try await idle(first)
        guard first.html.contains("3056"), first.renderedAnchor == "verse-16", first.reference == "John 4:2" else { fatalError("chapter uses tab preferences") }
        first.setReadingOption(.strongs, enabled: false); try await idle(first)
        guard !first.html.contains("3056"), first.reference == "John 4:2", !first.canBack else { fatalError("chapter options refresh without visit") }
        first.setReadingOption(.strongs, enabled: true); try await idle(first)
        first.selection = "OptionOther"; first.reference = "John 3:17"; first.read(); try await idle(first)
        guard first.engineOptions == initial, first.canBack else { fatalError("other module retains defaults") }
        first.setReadingOption(.wordsInRed, enabled: false); try await idle(first)
        guard first.engineOptions == initial.subtracting(.wordsInRed), !first.html.contains("wordsOfJesus") else { fatalError("other module option state") }
        first.navigate(-1); try await idle(first)
        guard first.selection == "OptionTest", first.engineOptions == initial.union(.strongs), first.html.contains("3056"), first.canForward else { fatalError("Back restores first module preferences") }
        first.navigate(1); try await idle(first)
        guard first.selection == "OptionOther", first.engineOptions == initial.subtracting(.wordsInRed) else { fatalError("Forward restores other module preferences") }
        let unchanged = first.engineOptions
        first.setReadingOption(.transliteration, enabled: true)
        guard !first.isBusy, first.engineOptions == unchanged else { fatalError("unsupported control rejected") }
        first.setReadingOption(.footnotes, enabled: false); try await idle(first)
        guard first.engineOptions == unchanged?.subtracting(.footnotes) else { fatalError("footnotes control") }
        first.setReadingOption(.morphology, enabled: true); try await idle(first)
        guard first.engineOptions?.contains(.morphology) == true else { fatalError("morphology control") }
        let importURL = URL(fileURLWithPath: root).appendingPathComponent("model-modops.conf")
        let importBytes = Data("; Original model fixture\n[OptionOther]\nStrong's Numbers=On\nWords of Christ in Red=Off\nFootnotes=Off\nMorphological Tags=Off\n[OptionTest]\nStrong's Numbers=Off\nWords of Christ in Red=On\n".utf8)
        try importBytes.write(to: importURL)
        first.reference = "John 4:2"
        let historyBack = first.canBack, historyForward = first.canForward
        first.importReadingOptions(path: importURL.path); try await idle(first)
        guard first.reference == "John 4:2", first.renderedAnchor == "verse-17",
              first.engineOptions?.contains(.strongs) == true,
              first.engineOptions?.contains(.wordsInRed) == false,
              first.engineOptions?.contains(.footnotes) == false,
              first.engineOptions?.contains(.morphology) == false,
              first.canBack == historyBack, first.canForward == historyForward,
              second.engineOptions == initial else { fatalError("model import preserves draft/history/window isolation") }
        let importedHTML = first.html, importedOptions = first.engineOptions
        first.importReadingOptions(path: importURL.path + ".missing"); try await idle(first)
        guard first.html == importedHTML, first.engineOptions == importedOptions,
              first.reference == "John 4:2", !first.message.isEmpty,
              try Data(contentsOf: importURL) == importBytes else { fatalError("model import failure preserves settings/source") }
        first.navigate(-1); try await idle(first)
        guard first.selection == "OptionTest", first.engineOptions?.contains(.strongs) == false,
              first.engineOptions?.contains(.wordsInRed) == true,
              !first.html.contains("3056") else { fatalError("imported other module preference restoration") }
        first.setReadingOption(.strongs, enabled: true); try await idle(first)
        first.reference = "John 4:2"
        let saveURL = URL(fileURLWithPath: root).appendingPathComponent("model-saved-modops.conf")
        let beforeSaveHTML = first.html, beforeSaveDraft = first.reference
        let beforeSaveBack = first.canBack, beforeSaveForward = first.canForward
        first.saveReadingOptions(path: saveURL.path); try await idle(first)
        guard first.html == beforeSaveHTML, first.reference == beforeSaveDraft,
              first.canBack == beforeSaveBack, first.canForward == beforeSaveForward,
              first.message == "Reading options saved." else { fatalError("model save preserves pane/input/history") }
        second.importReadingOptions(path: saveURL.path); try await idle(second)
        guard second.engineOptions == first.engineOptions else { fatalError("model save/reload in second window") }
        let savedBytes = try Data(contentsOf: saveURL)
        first.saveReadingOptions(path: root); try await idle(first)
        guard first.html == beforeSaveHTML, first.reference == beforeSaveDraft,
              first.canBack == beforeSaveBack, first.canForward == beforeSaveForward,
              first.message.contains("Unable to save"), try Data(contentsOf: saveURL) == savedBytes else { fatalError("model failed save preserves state/file") }
        // Module declarations gate the new controls; an undeclared option must
        // neither queue a refresh nor mutate the current module's settings.
        let undeclaredOptions = first.engineOptions
        first.setReadingOption(.greekAccents, enabled: false)
        first.setReadingOption(.hebrewVowels, enabled: false)
        first.setReadingOption(.hebrewCantillation, enabled: false)
        first.selectVariant(.all)
        first.setReadingOption(.lemmas, enabled: true)
        first.setReadingOption(.transliteratedForms, enabled: true)
        first.setReadingOption(.glosses, enabled: false)
        first.setReadingOption(.morphemeSegmentation, enabled: true)
        first.setReadingOption([.strongs, .footnotes], enabled: false)
        guard !first.isBusy, first.engineOptions == undeclaredOptions else { fatalError("undeclared/multiple control flags rejected") }
        first.showChapter = false
        first.selection = "OptionOther"; first.reference = "John 3:18"; first.read(); try await idle(first)
        first.setReadingOption(.footnotes, enabled: true); try await idle(first)
        second.selection = "OptionOther"; second.reference = "John 3:18"; second.read(); try await idle(second)
        second.setReadingOption(.footnotes, enabled: true); try await idle(second)
        let accentedReferenceHTML = first.html
        guard accentedReferenceHTML.contains("λόγος"), accentedReferenceHTML.contains("*x") else { fatalError("language/reference fixture output") }
        let beforeLanguageBack = first.canBack, beforeLanguageForward = first.canForward
        first.reference = "John 4:2"
        first.setReadingOption(.greekAccents, enabled: false); try await idle(first)
        guard first.html.contains("λογος"), !first.html.contains("λόγος"), first.html.contains("*x"),
              second.html == accentedReferenceHTML else { fatalError("Greek accents change output with window isolation") }
        first.setReadingOption(.crossReferences, enabled: false); try await idle(first)
        guard !first.html.contains("*x"), first.reference == "John 4:2",
              first.canBack == beforeLanguageBack, first.canForward == beforeLanguageForward else { fatalError("cross-reference refresh preserves draft/history") }
        first.showChapter = true; first.refreshDisplay(); try await idle(first)
        guard first.html.contains("λογος"), !first.html.contains("*x"), first.renderedAnchor == "verse-18" else { fatalError("language/reference chapter options") }
        first.setReadingOption(.greekAccents, enabled: true); try await idle(first)
        first.setReadingOption(.crossReferences, enabled: true); try await idle(first)
        guard first.html.contains("λόγος"), first.html.contains("*x"), first.reference == "John 4:2",
              second.html == accentedReferenceHTML else { fatalError("language/reference controls restore chapter output") }
        first.saveReadingOptions(path: saveURL.path); try await idle(first)
        second.importReadingOptions(path: saveURL.path); try await idle(second)
        guard second.engineOptions == first.engineOptions else { fatalError("language/reference save reload") }
        first.showChapter = false
        first.selection = "OptionOther"; first.reference = "John 3:21"; first.read(); try await idle(first)
        second.selection = "OptionOther"; second.reference = "John 3:21"; second.read(); try await idle(second)
        let pointedHebrew = second.html
        guard first.html.contains("שָׁל֑וֹם"), first.moduleOptions.declared.contains([.hebrewVowels, .hebrewCantillation]) else { fatalError("declared Hebrew filters enabled by legacy defaults") }
        let hebrewBack = first.canBack, hebrewForward = first.canForward
        first.reference = "John 4:2"
        first.setReadingOption(.hebrewCantillation, enabled: false); try await idle(first)
        guard !first.html.contains("֑"), first.html.contains("שָׁלוֹם"), second.html == pointedHebrew else { fatalError("cantillation refresh with separate window settings") }
        first.setReadingOption(.hebrewVowels, enabled: false); try await idle(first)
        guard first.html.contains("שׁלום"), first.reference == "John 4:2", first.canBack == hebrewBack,
              first.canForward == hebrewForward else { fatalError("Hebrew refresh preserves draft and history") }
        first.showChapter = true; first.refreshDisplay(); try await idle(first)
        guard first.html.contains("שׁלום"), first.renderedAnchor == "verse-21" else { fatalError("Hebrew options apply in chapter view") }
        first.saveReadingOptions(path: saveURL.path); try await idle(first)
        second.importReadingOptions(path: saveURL.path); try await idle(second)
        guard second.engineOptions?.contains(.hebrewVowels) == false, second.engineOptions?.contains(.hebrewCantillation) == false else { fatalError("Hebrew settings save/reload") }
        first.setReadingOption(.hebrewVowels, enabled: true); try await idle(first)
        first.setReadingOption(.hebrewCantillation, enabled: true); try await idle(first)
        guard first.html.contains("שָׁל֑וֹם"), first.reference == "John 4:2" else { fatalError("Hebrew controls restore original output") }
        first.reference = "John 3:22"; first.read(); try await idle(first)
        second.reference = "John 3:22"; second.read(); try await idle(second)
        let primaryHTML = second.html
        guard first.html.contains("Original primary reading"), !first.html.contains("Original secondary reading") else { fatalError("legacy primary reading default") }
        let variantFlags: EngineOptionFlags = [.variantsPrimary, .variantsSecondary, .variantsAll]
        let unrelatedFlags = first.engineOptions?.subtracting(variantFlags)
        let variantBack = first.canBack, variantForward = first.canForward
        first.reference = "John 4:2"
        for choice in [VariantReading.secondary, .all, .primary] {
            first.selectVariant(choice)
            guard first.isBusy else { fatalError("nonblocking variant selection") }
            try await idle(first)
            guard first.html.contains("Original primary reading") == (choice != .secondary),
                  first.html.contains("Original secondary reading") == (choice != .primary),
                  first.engineOptions?.subtracting(variantFlags) == unrelatedFlags,
                  first.reference == "John 4:2", first.canBack == variantBack, first.canForward == variantForward,
                  second.html == primaryHTML, first.renderedAnchor == "verse-22" else { fatalError("native variant chapter/output/options/history/window isolation") }
        }
        first.showChapter = false; first.refreshDisplay(); try await idle(first)
        first.selectVariant(.secondary); try await idle(first)
        guard !first.html.contains("Original primary reading"), first.html.contains("Original secondary reading") else { fatalError("variant single-entry output") }
        first.saveReadingOptions(path: saveURL.path); try await idle(first)
        second.importReadingOptions(path: saveURL.path); try await idle(second)
        guard second.engineOptions?.contains(.variantsSecondary) == true, second.html.contains("Original secondary reading"),
              !second.html.contains("Original primary reading") else { fatalError("native variant save/reload") }
        first.showChapter = true; first.refreshDisplay(); try await idle(first)
        first.showChapter = false; first.reference = "John 3:23"; first.read(); try await idle(first)
        second.reference = "John 3:23"; second.read(); try await idle(second)
        let annotationFlags: [EngineOptionFlags] = [.lemmas, .transliteratedForms, .glosses, .morphemeSegmentation]
        for flag in annotationFlags {
            first.setReadingOption(flag, enabled: false); try await idle(first)
            second.setReadingOption(flag, enabled: false); try await idle(second)
        }
        let plainAnnotations = first.html, secondAnnotations = second.html
        let annotationBack = first.canBack, annotationForward = first.canForward
        first.reference = "John 4:2"
        for flag in annotationFlags {
            first.setReadingOption(flag, enabled: true); try await idle(first)
            guard first.html != plainAnnotations, second.html == secondAnnotations,
                  first.reference == "John 4:2", first.canBack == annotationBack, first.canForward == annotationForward else { fatalError("annotation option refresh/isolation/history") }
            first.setReadingOption(flag, enabled: false); try await idle(first)
            guard first.html == plainAnnotations else { fatalError("annotation control restores engine output") }
        }
        for flag in annotationFlags { first.setReadingOption(flag, enabled: true); try await idle(first) }
        guard first.html.contains("fixtureLemma"), first.html.contains("fixtureXlit"), first.html.contains("fixtureGloss"),
              first.html.contains("<span class=\"morph\">first</span>") else { fatalError("all annotation controls affect real output") }
        first.showChapter = true; first.refreshDisplay(); try await idle(first)
        guard first.renderedAnchor == "verse-23", first.html.contains("fixtureGloss"), first.reference == "John 4:2" else { fatalError("annotation chapter refresh") }
        first.saveReadingOptions(path: saveURL.path); try await idle(first)
        second.importReadingOptions(path: saveURL.path); try await idle(second)
        guard second.html.contains("fixtureLemma"), second.html.contains("fixtureXlit"), second.html.contains("fixtureGloss"),
              second.engineOptions == first.engineOptions else { fatalError("annotation save/reload") }
        let noteURI = "xiphos-preview://study/passagestudy.jsp?action=showNote&type=n&value=1&module=OptionOther&passage=John+3%3A19"
        let beforeNoteHTML = first.html, beforeNoteDraft = first.reference
        let beforeNoteBack = first.canBack, beforeNoteForward = first.canForward
        first.followLink(noteURI); try await idle(first)
        guard let preview = first.studyNote, preview.module == "OptionOther", preview.reference == "John 3:19",
              preview.html.contains("Original other note"), preview.html.contains("3056"),
              first.html == beforeNoteHTML, first.reference == beforeNoteDraft,
              first.canBack == beforeNoteBack, first.canForward == beforeNoteForward,
              second.studyNote == nil else { fatalError("note preview uses linked verse/settings and preserves pane/draft/history/window isolation") }
        first.followLink("https://example.invalid/passagestudy.jsp"); try await idle(first)
        guard first.studyNote?.html == preview.html, first.html == beforeNoteHTML, first.reference == beforeNoteDraft,
              first.canBack == beforeNoteBack, first.canForward == beforeNoteForward else { fatalError("failed note link preserves preview/pane/history") }
        first.followLink("xiphos-preview://study/passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John+3%3A18")
        try await idle(first)
        guard first.studyNote?.type == "crossReference", first.studyNote?.referenceList.isEmpty == false,
              first.html == beforeNoteHTML, first.reference == beforeNoteDraft else { fatalError("cross-reference body preview metadata/context") }
        first.showReferencedPassages()
        guard first.isBusy else { fatalError("reference preview starts without blocking") }
        try await idle(first)
        guard let singlePreview = first.referencePreview, singlePreview.module == "OptionOther",
              singlePreview.entries.count == 1, singlePreview.entries[0].reference == "John 3:16",
              first.html == beforeNoteHTML, first.reference == beforeNoteDraft,
              first.canBack == beforeNoteBack, first.canForward == beforeNoteForward,
              second.referencePreview == nil else { fatalError("reference preview preserves pane/draft/history/window isolation") }
        first.showStudyNoteBody()
        guard first.referencePreview == nil, first.studyNote?.type == "crossReference" else { fatalError("return from references preserves note") }
        first.followLink("xiphos-preview://study/passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John+3%3A20")
        try await idle(first)
        first.showReferencedPassages(); try await idle(first)
        guard let rangePreview = first.referencePreview, rangePreview.entries.count == 5,
              rangePreview.entries[0].reference == "John 3:16", rangePreview.entries[3].reference == "John 3:19",
              rangePreview.entries[4].reference == "John 3:16", rangePreview.entries[0].html.contains("3056"),
              rangePreview.entries[2].html.contains("λόγος"), !rangePreview.limited,
              first.html == beforeNoteHTML, first.reference == beforeNoteDraft else { fatalError("range/duplicate reference preview uses shared options and context") }
        first.followLink("https://example.invalid"); try await idle(first)
        guard first.referencePreview?.html == rangePreview.html, first.studyNote?.reference == "John 3:20" else { fatalError("failed preview link preserves reference list") }
        first.showStudyNoteBody()
        first.followLink("xiphos-preview://study/passagestudy.jsp?action=showRef&type=scripRef&value=John.3.16-John.3.19&module=",
                         currentModule: first.studyNote?.module, currentReference: first.studyNote?.reference)
        try await idle(first)
        guard first.referencePreview?.entries.count == 4, first.referencePreview?.context == "John 3:20",
              first.html == beforeNoteHTML, first.reference == beforeNoteDraft,
              first.canBack == beforeNoteBack, first.canForward == beforeNoteForward else { fatalError("ranged note link opens list without navigation") }
        first.showStudyNoteBody()
        first.followLink("passagestudy.jsp?action=showRef&type=scripRef&value=16-19%3B16&module=",
                         currentModule: first.studyNote?.module, currentReference: first.studyNote?.reference)
        try await idle(first)
        guard first.referencePreview?.entries.count == 5, first.referencePreview?.entries[4].reference == "John 3:16",
              first.referencePreview?.context == "John 3:20", first.reference == beforeNoteDraft else { fatalError("relative list uses note source instead of draft") }
        first.followLink("passagestudy.jsp?action=showRef&type=scripRef&value=John.3.16&module=",
                         currentModule: first.referencePreview?.module, currentReference: first.referencePreview?.context)
        try await idle(first)
        guard first.referencePreview?.entries.count == 1, first.studyNote?.reference == "John 3:20",
              first.html == beforeNoteHTML, first.reference == beforeNoteDraft,
              first.canBack == beforeNoteBack, first.canForward == beforeNoteForward else { fatalError("single note reference is a sidebar preview") }
        first.followLink("sword:///John+3%3A16", currentModule: first.referencePreview?.module, currentReference: first.referencePreview?.context)
        try await idle(first)
        guard first.referencePreview == nil, first.studyNote == nil, first.selection == "OptionOther", first.reference == "John 3:16",
              first.renderedAnchor == "verse-16", !first.canBack, first.canForward else { fatalError("reference heading visits source module with shared history") }
        // The original history deduplicates this already visited reference,
        // returning to its first position while retaining forward entries.
        first.followLink(noteURI); try await idle(first)
        let beforeCloseNoteHTML = first.html
        first.closeStudyNote()
        guard first.referencePreview == nil, first.studyNote == nil, first.html == beforeCloseNoteHTML else { fatalError("close note preserves reader") }
        first.reference = "John 4:2"
        first.followLink("passagestudy.jsp?action=showRef&type=scripRef&value=16-17&module=")
        try await idle(first)
        guard first.studyNote == nil, first.referencePreview?.entries.count == 2,
              first.referencePreview?.context == "John 3:16", first.reference == "John 4:2",
              first.html == beforeCloseNoteHTML else { fatalError("standalone preview uses committed passage context") }
        first.closeStudyNote()
        guard first.referencePreview == nil else { fatalError("standalone list closes") }
        first.followLink("xiphos-preview://study/passagestudy.jsp?action=showNote&type=n&value=1&module=OptionTest&passage=John+3%3A16")
        try await idle(first)
        guard first.studyNote?.module == "OptionTest", first.selection == "OptionOther" else { fatalError("note records source module independent of main pane") }
        first.followLink("sword:///John+3%3A16", currentModule: first.studyNote?.module); try await idle(first)
        guard first.selection == "OptionTest", first.studyNote == nil else { fatalError("links in note use its source module fallback") }
        first.followLink(noteURI); try await idle(first)
        first.followLink("sword:///John+3%3A19", currentModule: first.studyNote?.module); try await idle(first)
        guard first.studyNote == nil, first.reference == "John 3:19", first.renderedAnchor == "verse-19" else { fatalError("passage link dispatch clears stale preview and visits passage") }
        first.followLink(noteURI)
        first.close(); try await Task.sleep(nanoseconds: 50_000_000)
        guard first.studyNote == nil else { fatalError("closed model does not publish note result") }
        second.followLink("xiphos-preview://study/passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John+3%3A20")
        try await idle(second)
        guard second.referencePreview == nil else { fatalError("second window has no prior reference preview") }
        second.showReferencedPassages(); second.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard second.referencePreview == nil else { fatalError("closed model does not publish reference preview") }
        first.close(); second.close(); await worker.shutdown()
        print("Native reading option model checks passed")
    }
    @MainActor static func lexicons(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let first = ReaderModel(worker: worker, startupError: nil), second = ReaderModel(worker: worker, startupError: nil)
        try await idle(first); try await idle(second)
        guard let defaults = first.lexiconPreferences, second.lexiconPreferences == defaults,
              defaults.greek == "StrongsGreek", defaults.greekMorphNT == "Robinson" else { fatalError("native shared defaults") }
        let priorHTML = first.html, priorRendered = first.renderedReference
        let back = first.canBack, forward = first.canForward
        first.reference = "John 4:2"
        let changed = LexiconPreferences(greek: "DictionaryOne", hebrew: "DictionaryTwo", greekMorphNT: "MorphGreek", greekMorphOT: "MorphGreek", hebrewMorph: "MorphHebrew")
        first.applyLexiconPreferences(changed)
        second.applyLexiconPreferences(defaults)
        try await idle(first); try await idle(second)
        guard first.lexiconPreferences == changed, second.lexiconPreferences == defaults,
              first.reference == "John 4:2", first.html == priorHTML, first.renderedReference == priorRendered,
              first.canBack == back, first.canForward == forward else { fatalError("native preference isolation/draft/pane/history") }
        var wrong = changed; wrong.greek = "DictionaryTwo"
        first.applyLexiconPreferences(wrong)
        guard !first.isBusy, first.lexiconPreferences == changed, first.message.contains("corresponding") else { fatalError("wrong role not accepted") }
        wrong = changed; wrong.greek = "Missing"
        first.applyLexiconPreferences(wrong)
        guard !first.isBusy, first.lexiconPreferences == changed else { fatalError("unavailable changed choice not accepted") }
        first.applyLexiconPreferences(LexiconPreferences(greek: nil, hebrew: nil, greekMorphNT: nil, greekMorphOT: nil, hebrewMorph: nil))
        try await idle(first)
        guard first.lexiconPreferences == changed else { fatalError("legacy placeholder preserves current names") }
        first.applyLexiconPreferences(changed)
        first.applyLexiconPreferences(defaults) // busy action must be ignored
        try await idle(first)
        guard first.lexiconPreferences == changed, first.html == priorHTML, first.reference == "John 4:2" else { fatalError("busy preference action ignored") }
        second.applyLexiconPreferences(changed); second.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard second.lexiconPreferences == defaults else { fatalError("closed model does not publish preferences") }
        first.close(); await worker.shutdown()
        print("Native lexicon preference model checks passed")
    }
    @MainActor static func main() async throws {
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--lexicons" {
            try await lexicons(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--options" {
            try await readingOptions(root: CommandLine.arguments[2]); return
        }
        guard CommandLine.arguments.count == 2 else { fatalError("fixture root required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let model = ReaderModel(worker: worker, startupError: nil)
        guard model.isBusy, model.modules.isEmpty else { fatalError("nonblocking startup state") }
        try await idle(model)
        guard model.reference == "John 3:16", model.html.contains("café"), !model.canBack else { fatalError("model initial load") }
        model.reference = "John 3:17"
        model.read()
        guard model.isBusy else { fatalError("action starts loading immediately") }
        model.chooseReference("John 3:18") // second action must not race the first
        try await idle(model)
        guard model.reference == "John 3:17", model.html.contains("Second fixture"), model.canBack else { fatalError("serialized model actions") }
        model.navigate(-1)
        try await idle(model)
        guard model.reference == "John 3:16", !model.canBack, model.canForward else { fatalError("no extra busy action in history") }
        model.reference = "John 3:18" // unsubmitted draft survives display changes
        model.showChapter = true
        model.refreshDisplay()
        try await idle(model)
        guard model.reference == "John 3:18", model.renderedReference == "John 3", model.renderedAnchor == "verse-16" else { fatalError("display refresh preserves draft and committed passage") }
        model.chooseReference("John 3:17")
        try await idle(model)
        guard model.reference == "John 3:17", model.renderedAnchor == "verse-17" else { fatalError("selector updates chapter selection") }
        let committed = model.html
        model.followLink("https://example.invalid")
        try await idle(model)
        guard model.html == committed, model.reference == "John 3:17", !model.message.isEmpty else { fatalError("unsupported link preserves pane") }
        model.clearHistory()
        try await idle(model)
        guard !model.canBack, !model.canForward, model.html == committed else { fatalError("clear keeps passage") }
        model.read()
        model.close() // invalidate an action before its result may publish
        model.close() // idempotent lifecycle
        try await Task.sleep(nanoseconds: 50_000_000)
        guard model.html == committed else { fatalError("closed model result publication") }
        let closedAtStartup = ReaderModel(worker: worker, startupError: nil)
        closedAtStartup.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard closedAtStartup.modules.isEmpty, closedAtStartup.html.isEmpty else { fatalError("closed startup remains unpublished") }
        let unavailable = ReaderModel(worker: nil, startupError: "Fixture startup error")
        guard !unavailable.isBusy, unavailable.message == "Fixture startup error" else { fatalError("worker setup error") }
        await worker.shutdown()
        print("Async reader model checks passed")
    }
}
