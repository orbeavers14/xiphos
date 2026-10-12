// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct PassageAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("study fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let tab = try await worker.createTab()
        let first = try await worker.studyPassage(tab: tab, module: "Text", commentary: "Notes", reference: "Jn3:16")
        guard first.text.reference == "John 3:16", first.commentary?.reference == first.text.reference,
              first.commentary?.html.contains("café") == true else { fatalError("typed paired read") }
        _ = try await worker.studyPassage(tab: tab, module: "Text", commentary: "OtherNotes", reference: "John3:17")
        let back = try await worker.navigateStudyPassage(tab: tab, direction: -1)
        guard back.commentaryModule == "Notes" else { fatalError("paired history restoration") }
        let refresh = try await worker.studyPassage(tab: tab, module: "Text", commentary: "", reference: "John3:17", record: false)
        guard refresh.commentary == nil, refresh.commentaryModule.isEmpty,
              try await worker.history(tab).count == 2 else { fatalError("no-record hidden commentary") }
        let chapter = try await worker.navigateStudyChapter(tab: tab, module: "Text", commentary: "Notes", reference: "John3:17", direction: 1)
        guard chapter.text.reference == "John 4:1", chapter.commentary?.reference == chapter.text.reference else { fatalError("paired chapter step") }
        let before = try await worker.history(tab).count
        let notesChapter = try await worker.readTabCommentaryChapter(tab: tab, module: "Notes", reference: "John3:17", verseNumbers: true, highlightSelected: true)
        guard notesChapter.verses.count == 3, notesChapter.selectedAnchor == "commentary-verse-17",
              try await worker.history(tab).count == before else { fatalError("typed sparse chapter uses engine ordinal without visit") }
        guard case .passage(let target) = try await worker.followStudyLinkAt(tab: tab, currentModule: "Notes", currentReference: "John3:17",
            uri: "sword:///John3:16", navigationModule: "Text"), target.module == "Text" else { fatalError("typed pane navigation context") }
        let linked = try await worker.visitLinkedStudyPassage(tab: tab, bible: "Text", commentary: "OtherNotes",
            target: PassageTarget(module: "Notes", reference: "John3:17"))
        guard linked.module == "Text", linked.commentaryModule == "Notes" else { fatalError("typed commentary link routing") }
        do {
            _ = try await worker.studyPassage(tab: tab, module: "Text", commentary: "Notes", reference: "John3:17\0ignored")
            fatalError("embedded NUL crossed adapter boundary")
        } catch is CoreError { }
        await worker.shutdown()
        guard first.commentary?.html.contains("café") == true else { fatalError("owned Swift result after shutdown") }
        print("Swift study passage checks passed")
    }
}
