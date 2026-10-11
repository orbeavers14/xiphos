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
        first.close(); second.close(); await worker.shutdown()
        print("Native reading option model checks passed")
    }
    @MainActor static func main() async throws {
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
