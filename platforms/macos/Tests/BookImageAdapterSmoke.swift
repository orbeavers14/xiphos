// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct BookImageAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("original image resource fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let tab = try await worker.createTab()
        let selected = try await worker.selectBookDocument(tab: tab, module: "StyledBook", offset: 4)
        let uri = "xiphos-preview://study/passagestudy.jsp?action=showImage&value=file%3Acaf%C3%A9.png&module=OtherBook"
        let result = try await worker.followBookLink(tab: tab, uri: uri)
        guard case .image(let image) = result, image.module == "StyledBook", image.filename == "café.png",
              image.mediaType == "image/png", image.data.prefix(8) == Data([137,80,78,71,13,10,26,10]),
              try await worker.bookDocumentSelection(tab: tab) == selected else { fatalError("typed core image dispatch, copied bytes and reading neutrality") }
        for bad in [uri + "\0ignored", "passagestudy.jsp?action=showImage&value=..%2Foutside.svg", "https://example.invalid"] {
            do { _ = try await worker.followBookLink(tab: tab, uri: bad); fatalError("invalid image link accepted") }
            catch is CoreError { }
        }
        guard try await worker.history(tab).count == 0, try await worker.dictionaryHistory(tab: tab).entries.isEmpty,
              try await worker.bookDocumentSelection(tab: tab) == selected else { fatalError("failed image links retain reading and history") }
        try await worker.closeTab(tab); await worker.shutdown()
        guard image.data.prefix(8) == Data([137,80,78,71,13,10,26,10]), image.filename == "café.png" else { fatalError("image data/source copies survive shutdown") }
        print("Swift core-owned book link dispatch, image byte ownership and state neutrality passed")
    }
}
