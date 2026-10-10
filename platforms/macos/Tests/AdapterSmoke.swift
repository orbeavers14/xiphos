// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main
struct AdapterSmoke {
    static func main() throws {
        guard CommandLine.arguments.count == 2 else { fatalError("fixture root required") }
        let reader = try CoreReader(root: CommandLine.arguments[1])
        guard reader.modules.count == 1, reader.modules[0].id == "ParityTest" else { fatalError("module snapshot") }
        let first = try reader.read(module: "ParityTest", reference: "John 3:16")
        let second = try reader.read(module: "ParityTest", reference: "John 3:17")
        guard first.html.contains("café"), second.html.contains("Second fixture"),
              first.reference.contains("3:16") else { fatalError("adapter results") }
        do {
            _ = try reader.read(module: "Missing", reference: "John 3:16")
            fatalError("missing module succeeded")
        } catch is CoreError { }
        let session = try CoreSession(reader: reader)
        let other = try CoreSession(reader: reader)
        _ = try session.visit(module: "ParityTest", reference: "John 3:16")
        _ = try session.visit(module: "ParityTest", reference: "John 3:17")
        guard session.canBack, !session.canForward, other.historyCount == 0 else { fatalError("session isolation") }
        let back = try session.navigate(-1)
        guard back.html.contains("café"), session.canForward, session.historyModule == "ParityTest" else { fatalError("back") }
        let forward = try session.navigate(1)
        guard forward.html.contains("Second fixture") else { fatalError("forward") }
        let chapter = try reader.readChapter(module: "ParityTest", reference: "John 3:16")
        guard chapter.title == "John 3", chapter.verses.count == 36,
              chapter.verses[15].html.contains("café"), chapter.verses[16].html.contains("Second fixture"),
              chapter.html.contains("id=\"verse-16\"") else { fatalError("chapter snapshot") }
        let nextBook = try session.navigateChapter(module: "ParityTest", reference: "John 21:25", direction: 1)
        guard nextBook.reference == "Acts 1:1" else { fatalError("chapter navigation") }
        session.clearHistory()
        guard session.historyCount == 0, !session.canBack, !session.canForward else { fatalError("clear") }
        print("Swift adapter smoke checks passed")
    }
}
