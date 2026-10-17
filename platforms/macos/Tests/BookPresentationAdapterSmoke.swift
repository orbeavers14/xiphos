// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct BookPresentationAdapterSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("original book presentation fixture required") }
        let worker = try CoreWorker(root: CommandLine.arguments[1])
        let a = try await worker.createTab(), b = try await worker.createTab()
        try await worker.setEngineOptions(tab: a, options: [.strongs])
        try await worker.setLexicalLayoutEnabled(tab: a, enabled: true)
        guard try await worker.refreshBookDocument(tab: a) == nil else { fatalError("unselected refresh no-op") }
        let initial = try await worker.selectBookDocument(tab: a, module: "StyledBook", offset: 4)
        guard initial.presentation?.options.images == true, initial.presentation?.options.fontFaces == true,
              initial.lexical.requested, initial.lexical.applied, initial.displayHTML.contains("class=\"word\""),
              initial.html.contains("<img ") else { fatalError("copied raw/presented document and scoped discovery") }
        try await worker.setImageContent(tab: a, module: "StyledBook", enabled: false)
        try await worker.setRespectFontFaces(tab: a, module: "StyledBook", enabled: false)
        try await worker.setDoubleSpace(tab: a, module: "StyledBook", enabled: true)
        let hidden = try await worker.refreshBookDocument(tab: a)
        guard let hidden, hidden.html == initial.html, hidden.tree == initial.tree,
              hidden.presentation?.options == .init(images: false, fontFaces: false, doubleSpace: true),
              hidden.displayHTML.contains("<img ") == false, hidden.displayHTML.contains("Original Book Font") == false,
              hidden.lexical.css.contains("line-height: 2.7em"), hidden.presentation?.cleanupDiagnostics.isEmpty == true else { fatalError("matching owned CSS/cleanup/settings and current-source refresh") }
        let raw = try await worker.readBookDocument(tab: a, module: "StyledBook", offset: 4)
        guard raw.presentation == nil, raw.displayHTML == raw.html, raw.lexical == .plain else { fatalError("raw document has no inferred presentation") }
        let fallback = try await worker.restoreBookDocumentSelection(tab: a,
            saved: .init(module: "StyledBook", key: "/Fallback", offsetText: nil), mode: .displayKey)
        guard fallback?.lexical.requested == true, fallback?.lexical.applied == false,
              fallback?.lexical.css.contains("line-height: 2em") == true,
              fallback?.displayHTML.contains("ordinary note") == true else { fatalError("whole-document fallback retains spacing CSS") }
        let malformed = try await worker.restoreBookDocumentSelection(tab: a,
            saved: .init(module: "StyledBook", key: "/Malformed", offsetText: nil), mode: .displayKey)
        guard malformed?.presentation?.cleanupDiagnostics.count == 1,
              malformed?.presentation?.cleanupDiagnostics.first?.kind == .fontFace,
              malformed?.presentation?.cleanupDiagnostics.first?.suffix.contains("unfinished") == true else { fatalError("copied cleanup diagnostics") }
        _ = try await worker.restoreBookDocumentSelection(tab: a,
            saved: .init(module: "StyledBook", key: "/", offsetText: nil), mode: .displayKey)
        guard try await worker.refreshBookDocument(tab: a)?.tree.node.offset == 0 else { fatalError("root intent remains in core on refresh") }
        try await worker.setEngineOptions(tab: b, options: [.strongs])
        let other = try await worker.selectBookDocument(tab: b, module: "StyledBook", offset: 4)
        guard other.presentation?.options.images == true, other.presentation?.options.fontFaces == true,
              other.presentation?.options.doubleSpace == false, other.lexical.requested == false else { fatalError("presentation/consumer isolation") }
        guard try await worker.history(a).count == 0, try await worker.dictionaryHistory(tab: a).entries.isEmpty else { fatalError("refresh has no unrelated history") }
        try await worker.closeTab(a); try await worker.closeTab(b); await worker.shutdown()
        guard hidden.lexical.css.contains("line-height: 2.7em"), hidden.displayHTML.contains("café"),
              malformed?.presentation?.cleanupDiagnostics.first?.suffix.contains("unfinished") == true else { fatalError("presentation copies outlive worker") }
        print("Swift owned book presentation/CSS, cleanup, discovery, diagnostics and root-preserving refresh passed")
    }
}
