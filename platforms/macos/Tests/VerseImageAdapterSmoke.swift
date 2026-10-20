// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct VerseImageAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("original image fixture required") }
        let root = URL(fileURLWithPath: CommandLine.arguments[1]).appendingPathComponent("library %20 café").path
        let expected = Data(Array("Original".utf8) + [0] + Array("image".utf8) + [255])
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab()
        var held: RenderedVerseImage?
        for module in ["ThMLText", "ThMLNotes", "OSISText", "OSISNotes"] {
            let source = module.hasPrefix("OSIS") ? "file:" + root + "/data/" + module + "/caf%C3%A9.png" : "caf%C3%A9.png"
            let image = try await worker.readVerseInlineImage(module: module, source: source, maxBytes: expected.count)
            guard image.module == module, image.source == source, image.mediaType == "image/png", image.data == expected else {
                fatalError("shared module/source policy and copied binary values")
            }
            held = image
            do { _ = try await worker.readVerseInlineImage(module: module, source: source, maxBytes: expected.count - 1); fatalError("short image cap accepted") }
            catch is CoreError { }
        }
        let empty = try await worker.readVerseInlineImage(module: "ThMLText", source: "empty.png")
        guard empty.data.isEmpty, empty.mediaType == "image/png" else { fatalError("empty image remains owned") }
        for (module, source, cap) in [("", "original.png", 1024), ("ThMLText", "", 1024),
            ("ThMLText", "original.png", 0), ("ThMLText", "original.png", -1),
            ("ThMLText", "original.png", 16 * 1024 * 1024 + 1),
            ("ThMLText\0ignored", "original.png", 1024), ("ThMLText", "original.png\0ignored", 1024),
            (String(repeating: "m", count: 4097), "original.png", 1024),
            ("ThMLText", String(repeating: "x", count: 32769), 1024),
            ("ThMLText", "../OSISText/original.png", 1024), ("Missing", "original.png", 1024),
            ("ThMLText", "https://example.invalid/original.png", 1024)] {
            do { _ = try await worker.readVerseInlineImage(module: module, source: source, maxBytes: cap); fatalError("invalid image request accepted") }
            catch is CoreError { }
        }
        let accepted = Task { try await worker.readVerseInlineImage(module: "ThMLText", source: "a+b.png") }
        accepted.cancel()
        guard try await accepted.value.data == expected else { fatalError("source read drains to authoritative copied outcome after observer cancellation") }
        guard try await worker.history(tab).count == 0, try await worker.dictionaryHistory(tab: tab).entries.isEmpty,
              try await worker.bookDocumentSelection(tab: tab) == nil else { fatalError("image reads preserve logical state") }
        try await worker.closeTab(tab)
        await worker.shutdown()
        guard held?.data == expected, held?.source.contains("caf%C3%A9.png") == true else { fatalError("copied image survives all C handles and worker") }
        do { _ = try await worker.readVerseInlineImage(module: "ThMLText", source: "original.png"); fatalError("closed worker accepted read") }
        catch is CoreError { }
        print("Swift6 copied verse image values, signed/NUL guards, shared path errors and drained lifetime passed")
    }
}
