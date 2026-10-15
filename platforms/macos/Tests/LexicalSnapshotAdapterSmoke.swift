// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct LexicalSnapshotAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("private lexical fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let first = try await worker.createTab(), other = try await worker.createTab()
        try await worker.setModuleEngineOptions(tab: first, module: "Text", options: [.strongs])
        try await worker.setModuleEngineOptions(tab: first, module: "ThMLNotes", options: [.strongs])
        try await worker.setModuleEngineOptions(tab: other, module: "Text", options: [.strongs])
        let plain = try await worker.readTabEntry(tab: first, module: "Text", reference: "John4:1")
        guard plain.lexical == .plain else { fatalError("existing adapter consumer stays plain until opt-in") }
        try await worker.setLexicalLayoutEnabled(tab: first, enabled: true)
        try await worker.setImageContent(tab: first, module: "Text", enabled: false)
        try await worker.setRespectFontFaces(tab: first, module: "Text", enabled: false)
        let held = try await worker.readTabEntry(tab: first, module: "Text", reference: "John4:1")
        guard held.lexical.requested, held.lexical.applied, held.lexical.css.contains("line-height: 2.7em"),
              held.html.contains("class=\"word\""), !held.html.contains("<img "),
              !held.html.contains("Original Lexical Font") else { fatalError("Swift copies matching HTML/CSS after core cleanup") }
        let chapter = try await worker.readTabChapter(tab: first, module: "Text", reference: "John4:2", verseNumbers: true, highlightSelected: true)
        guard chapter.lexical == held.lexical, chapter.verses[0].lexical == held.lexical,
              chapter.verseOrdinals[1] == 2, chapter.selectedAnchor == "verse-2" else { fatalError("Swift copies document and verse layout metadata") }
        let pair = try await worker.studyPassage(tab: first, module: "Text", commentary: "ThMLNotes", reference: "John4:1", record: false)
        guard pair.text.lexical == held.lexical, pair.commentary?.lexical == held.lexical,
              pair.commentary?.headingHTML.contains("class=\"word\"") == true else { fatalError("Swift paired body/header CSS copy") }
        let notes = try await worker.readTabCommentaryChapter(tab: first, module: "ThMLNotes", reference: "John4:2", verseNumbers: true, highlightSelected: true)
        guard notes.lexical == held.lexical, notes.verseOrdinals == [1, 2],
              notes.verses[0].lexical == notes.lexical else { fatalError("Swift sparse chapter CSS/ordinal copy") }
        let fallback = try await worker.readTabChapter(tab: first, module: "Text", reference: "John5:1", verseNumbers: true, highlightSelected: true)
        guard fallback.lexical.requested, !fallback.lexical.applied, fallback.lexical.css.isEmpty,
              !fallback.verses[0].lexical.applied else { fatalError("Swift retains explicit whole-document fallback") }
        let untouched = try await worker.readTabEntry(tab: other, module: "Text", reference: "John4:1")
        guard untouched.lexical == .plain, try await worker.history(first).count == 0 else { fatalError("typed layout is tab-local and history-neutral") }
        try await worker.setLexicalLayoutEnabled(tab: first, enabled: false)
        guard try await worker.readTabEntry(tab: first, module: "Text", reference: "John4:1").lexical == .plain else { fatalError("disabled adapter read has no stale CSS") }
        try await worker.closeTab(first)
        do {
            try await worker.setLexicalLayoutEnabled(tab: first, enabled: true)
            fatalError("closed tab accepted layout capability")
        } catch is CoreError { }
        await worker.shutdown()
        guard held.lexical.applied, chapter.lexical.css == held.lexical.css,
              pair.commentary?.headingHTML.contains("Original") == true,
              notes.lexical.css == held.lexical.css else { fatalError("Swift-owned CSS and HTML survive handle/worker release") }
        print("Swift lexical snapshot ownership, CSS/state, fallback and tab guards passed")
    }
}
