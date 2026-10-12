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
        await worker.shutdown()
        guard first.commentary?.html.contains("café") == true else { fatalError("owned Swift result after shutdown") }
        print("Swift study passage checks passed")
    }
}
