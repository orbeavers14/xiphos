// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main
struct ReaderModelSmoke {
    // Original-prose variant fixtures contain no encoded text except spacing.
    // Compare their words independently of the core's per-word layout spans.
    static func fixtureWords(_ html: String) -> String {
        html.replacingOccurrences(of: "<[^>]*>", with: "", options: .regularExpression)
            .replacingOccurrences(of: "&nbsp;", with: " ")
            .split(whereSeparator: { $0.isWhitespace }).joined(separator: " ")
    }
    @MainActor static func idle(_ model: ReaderModel) async throws {
        let deadline = Date().addingTimeInterval(10)
        while model.isBusy {
            guard Date() < deadline else { fatalError("model request timeout") }
            try await Task.sleep(nanoseconds: 1_000_000)
        }
    }
    @MainActor static func previewIdle(_ model: ReaderModel) async throws {
        let deadline = Date().addingTimeInterval(10)
        while model.isPreviewLoading {
            guard Date() < deadline else { fatalError("information preview request timeout") }
            try await Task.sleep(nanoseconds: 1_000_000)
        }
    }
    @MainActor static func bookDocuments(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        let mainHTML = model.html, mainSelection = model.selection, mainBack = model.canBack
        model.chooseBook("Document5"); try await idle(model)
        guard let document = model.bookDocument, document.displayLevel == 3, document.sections.count == 5,
              document.html.contains("Original deep section"), document.html.contains("empty-branch child"),
              model.bookEntry?.tree.node.offset == 4, model.html == mainHTML, model.selection == mainSelection,
              model.canBack == mainBack else { fatalError("native document renders configured descendants without replacing Bible") }
        let uri = "xiphos-preview://study/passagestudy.jsp?action=showNote&type=n&value=1&module=Document5&passage=%2FBranch%2FLeaf"
        model.showBookNoteLink(uri); try await idle(model)
        guard model.bookNote?.tree.node.offset == 8, model.bookNote?.content.html.contains("Original descendant note") == true,
              model.bookEntry?.tree.node.offset == 4, model.bookDocument == document else { fatalError("native descendant note retains selected document root") }
        model.browseBook(0); try await idle(model)
        let page = model.bookChildren
        model.chooseBook("TooDeep"); try await idle(model)
        guard model.bookDocument == document, model.bookChildren == page, model.bookNote?.tree.node.offset == 8,
              !model.message.isEmpty else { fatalError("failed complete-document render preserves reading, contents and note") }
        model.showBookNoteLink(uri.replacingOccurrences(of: "%2FBranch%2FLeaf", with: "%2FLinked")); try await idle(model)
        guard model.bookDocument == document, model.bookNote?.tree.node.offset == 8 else { fatalError("outside-document note preserves presentation") }
        model.navigateBook(.firstChild); try await idle(model)
        guard model.bookDocument?.tree.node.offset == 8, model.bookDocument?.sections.count == 2, model.bookNote == nil else { fatalError("native navigation publishes new complete document and clears stale note") }
        model.bookKey = "/Branch/Empty"; model.readBookKey(); try await idle(model)
        guard model.bookEntry?.hasEntry == false, model.bookDocument?.html.contains("empty-branch child") == true else { fatalError("entryless branch displays configured descendants") }
        model.bookKey = "Missing/path"; let preserved = model.bookDocument; model.readBookKey(); try await idle(model)
        guard model.bookDocument == preserved else { fatalError("failed document key keeps reading") }
        other.chooseBook("Document4"); try await idle(other)
        guard other.bookDocument?.displayLevel == 2, other.bookDocument?.sections.count == 3,
              other.bookDocument?.html.contains("Original deep section") == false,
              model.bookDocument == preserved else { fatalError("native document depth and windows remain independent") }
        model.close(); other.close(); await worker.shutdown()
        print("Native configured document, descendant note and atomic failure model checks passed")
    }
    @MainActor static func books(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        guard model.bookEntry == nil, !model.showingBook,
              model.modules.filter(\.hasBookTree).map(\.id).sorted() == ["OSISBook", "ThMLBook"] else { fatalError("book capability/initial state") }
        let mainSelection = model.selection, mainHTML = model.html, mainBack = model.canBack
        model.selection = "OSISBook"; model.selectionChanged()
        model.chooseBook("ThMLBook") // busy actions must not replace the accepted module
        try await idle(model)
        guard model.bookEntry?.tree.module == "OSISBook", model.bookEntry?.tree.node.offset == 4,
              model.bookEntry?.html.contains("café") == true, model.showingBook, model.selection == mainSelection,
              model.bookBrowser?.node.offset == 0, model.bookChildren?.nodes.count == 3,
              model.html == mainHTML, model.canBack == mainBack else { fatalError("book opens separate lower-right pane through core") }
        let noteURI = "xiphos-preview://study/passagestudy.jsp?action=showNote&type=n&value=1&module=OSISBook&passage=%2FPart+One"
        let sourceBeforeNote = model.bookEntry
        model.showBookNoteLink(noteURI); try await idle(model)
        guard model.bookNote?.tree.node.offset == 4, model.bookNote?.content.html.contains("Original book note") == true,
              model.bookEntry == sourceBeforeNote, model.html == mainHTML, model.canBack == mainBack else { fatalError("native exact-source book note preview is state neutral") }
        model.followLink("passagestudy.jsp?action=showRef&type=scripRef&value=John.3.16&module=",
            currentModule: mainSelection, currentReference: "John 3:16")
        try await idle(model)
        guard model.bookNote == nil, model.referencePreview != nil else { fatalError("verse reference preview replaces book-note sidebar") }
        model.showBookNoteLink(noteURI); try await idle(model)
        guard model.bookNote != nil, model.referencePreview == nil, model.studyNote == nil else { fatalError("book note replaces verse sidebar without changing source") }
        model.browseBook(0); try await idle(model)
        guard model.bookNote != nil else { fatalError("contents browsing preserves a valid book note") }
        model.showBookNoteLink(noteURI.replacingOccurrences(of: "%2FPart+One", with: "%2FLinked")); try await idle(model)
        guard model.bookNote?.tree.node.offset == 4, model.bookEntry == sourceBeforeNote,
              model.message.contains("displayed book") else { fatalError("mismatched note preserves preview and reading") }
        model.closeStudyNote()
        guard model.bookNote == nil else { fatalError("native close book note") }
        model.showBookNoteLink(noteURI); try await idle(model)
        model.navigateBook(.firstChild); try await idle(model)
        guard model.bookEntry?.tree.node.offset == 8, model.bookNote == nil else { fatalError("native first child clears stale book note") }
        let leaf = model.bookEntry
        model.navigateBook(.firstChild); try await idle(model)
        guard model.bookEntry == leaf else { fatalError("native boundary no-op") }
        model.navigateBook(.parent); try await idle(model)
        model.navigateBook(.nextSibling); try await idle(model)
        guard model.bookEntry?.tree.node.offset == 12, model.bookEntry?.hasEntry == false else { fatalError("entryless branch stays selectable") }
        model.browseBook(12); try await idle(model)
        guard model.bookChildren?.nodes.map(\.offset) == [16], model.bookEntry?.tree.node.offset == 12 else { fatalError("hierarchy browse is separate from reading") }
        model.chooseBookNode(model.bookChildren!.nodes[0]); try await idle(model)
        guard model.bookEntry?.tree.node.offset == 16 else { fatalError("native contents selection") }
        model.bookKey = "Part One"; model.readBookKey(); try await idle(model)
        guard model.bookEntry?.tree.node.offset == 4 else { fatalError("direct key interpretation stays in core") }
        let preserved = model.bookEntry, page = model.bookChildren
        model.bookKey = "Missing/path"; model.readBookKey(); try await idle(model)
        guard model.bookEntry == preserved, model.bookChildren == page, model.message.contains("key or path") else { fatalError("bad key preserves committed presentation") }
        model.browseBook(40000); try await idle(model)
        guard model.bookEntry == preserved, model.bookChildren == page else { fatalError("bad browse preserves tree and reading") }
        model.bookKey = "/"; model.readBookKey(); try await idle(model)
        guard model.bookEntry?.tree.node.offset == 0 else { fatalError("direct explicit path root stays root") }
        model.navigateBook(.firstChild); try await idle(model)
        model.bookKey = "unsubmitted draft"; model.browseBook(0); try await idle(model)
        guard model.bookKey == "unsubmitted draft", model.bookEntry?.tree.node.offset == 4 else { fatalError("contents browsing preserves reading draft") }
        model.closeBook(); model.chooseBook("OSISBook"); try await idle(model)
        guard model.showingBook, model.bookKey == "unsubmitted draft", model.bookEntry?.tree.node.offset == 4 else { fatalError("pane reopen keeps current source") }
        other.chooseBook("ThMLBook"); try await idle(other)
        guard other.bookEntry?.tree.module == "ThMLBook", model.bookEntry?.tree.module == "OSISBook" else { fatalError("native windows isolate book selection") }
        model.chooseBook("VerseBook"); try await idle(model)
        guard model.bookEntry?.tree.module == "OSISBook" else { fatalError("unsupported book capability not selectable") }
        let committed = model.bookEntry
        model.navigateBook(.firstChild); model.close(); other.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard model.bookEntry == committed else { fatalError("closed book pane cannot publish queued UI results") }
        await worker.shutdown()
        print("Native book model routing, hierarchy, core commands and stale presentation checks passed")
    }
    @MainActor static func content(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        guard model.contentOptions.images == nil, model.contentOptions.fontFaces == nil else { fatalError("native preserves unknown content state") }
        model.setImageContent(false); try await idle(model)
        guard model.contentOptions.images == nil else { fatalError("unknown content control stays unavailable") }
        model.reference = "John 3:21"; model.read(); try await idle(model)
        guard model.contentOptions.images == true, model.contentOptions.fontFaces == true, model.html.contains("<img ") else { fatalError("native fetches discovered content after reading") }
        let original = model.html, back = model.canBack
        model.reference = "John 4:2"
        model.setImageContent(false); model.setImageContent(true); try await idle(model)
        guard model.contentOptions.images == false, !model.html.contains("<img "), model.html.contains("OriginalBibleFont"),
              model.reference == "John 4:2", model.canBack == back else { fatalError("image control refreshes committed body and rejects busy edit") }
        model.setRespectFontFaces(false); try await idle(model)
        guard model.contentOptions.fontFaces == false, !model.html.contains("OriginalBibleFont"), model.html.contains("Original styled Bible body") else { fatalError("font control uses shared byte-preserving cleanup") }
        other.reference = "John 3:21"; other.read(); try await idle(other)
        guard other.html == original, other.contentOptions.images == true else { fatalError("content controls isolated across native windows") }
        model.chooseCommentary("ThMLNotes"); try await idle(model)
        guard model.commentaryContentOptions.images == true, model.commentaryContentOptions.fontFaces == true,
              model.commentaryEntry?.headingHTML.contains("OriginalHeadingFont") == true else { fatalError("commentary discovery exposes separate module controls") }
        model.setImageContent(false, commentary: true); try await idle(model)
        model.setRespectFontFaces(false, commentary: true); try await idle(model)
        guard model.commentaryEntry?.headingHTML.contains("<img ") == false, model.commentaryEntry?.headingHTML.contains("OriginalHeadingFont") == false,
              model.commentaryChapter?.html.contains("<img ") == false, model.reference == "John 4:2", model.canBack == back else { fatalError("commentary controls clean headings/body/chapter without submitting draft") }
        guard model.commentaryChapter?.verseOrdinals.contains(21) == true,
              model.commentaryChapter?.verseOrdinals.contains(17) == false else { fatalError("native sparse chapter copies SDK positions without deriving them") }
        model.setCommentaryChapterMode(false); try await idle(model)
        guard model.commentaryChapter == nil, model.commentaryEntry?.html.contains("OriginalBodyFont") == false else { fatalError("content preferences retained in verse mode") }
        let profile = URL(fileURLWithPath: root).appendingPathComponent("native-content.conf")
        try Data("[Text]\nCustom=keep\n[Uninstalled]\nImage Content=On\n".utf8).write(to: profile)
        model.saveReadingOptions(path: profile.path); try await idle(model)
        model.setImageContent(true); try await idle(model)
        model.setRespectFontFaces(true); try await idle(model)
        model.importReadingOptions(path: profile.path); try await idle(model)
        guard model.contentOptions.images == false, model.contentOptions.fontFaces == false, model.commentaryContentOptions.images == false,
              model.reference == "John 4:2", model.canBack == back, try String(contentsOf: profile, encoding: .utf8).contains("Custom=keep") else { fatalError("native content save/import preserves draft/history/other values") }
        let diagnosticTab = try await worker.createTab()
        try await worker.setRespectFontFaces(tab: diagnosticTab, module: "Text", enabled: false)
        let diagnosticEntry = try await worker.readTabEntry(tab: diagnosticTab, module: "Text", reference: "John 3:22")
        let diagnosticPair = try await worker.studyPassage(tab: diagnosticTab, module: "Text", commentary: "ThMLNotes", reference: "John 3:22", record: false)
        let diagnosticChapter = try await worker.readTabChapter(tab: diagnosticTab, module: "Text", reference: "John 3:22", verseNumbers: false, highlightSelected: false)
        guard diagnosticEntry.cleanupDiagnostics.first?.kind == .fontFace,
              diagnosticEntry.cleanupDiagnostics.first?.suffix.contains("unfinished") == true,
              diagnosticPair.text.cleanupDiagnostics.first?.suffix == diagnosticEntry.cleanupDiagnostics.first?.suffix,
              diagnosticChapter.verseOrdinals[21] == 22,
              diagnosticChapter.verses[21].cleanupDiagnostics.first?.suffix == diagnosticEntry.cleanupDiagnostics.first?.suffix else { fatalError("Swift copies diagnostic kinds/suffixes and SDK positions across result families") }
        try await worker.closeTab(diagnosticTab)
        let snapshot = model.html
        model.close(); model.setImageContent(true); try await idle(model)
        guard model.html == snapshot else { fatalError("closed model cannot publish content changes") }
        other.close(); await worker.shutdown()
        guard diagnosticEntry.cleanupDiagnostics.first?.suffix.contains("unfinished") == true,
              diagnosticPair.text.cleanupDiagnostics.first?.suffix == diagnosticChapter.verses[21].cleanupDiagnostics.first?.suffix else { fatalError("copied Swift metadata survives worker shutdown") }
        print("Native content control checks passed")
    }
    static func headingURI(_ html: String) -> String {
        guard let uri = html.components(separatedBy: "href=\"").dropFirst().compactMap({ $0.components(separatedBy: "\"").first })
            .first(where: { $0.contains("&value=xiphos-heading-") }) else { fatalError("core publishes scoped heading URI") }
        return uri
    }
    @MainActor static func headingNotes(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        try await idle(model)
        model.reference = "John4:1"; model.read(); try await idle(model)
        model.chooseCommentary("Notes"); try await idle(model)
        let uri = headingURI(model.commentaryEntry?.headingHTML ?? "")
        let back = model.canBack, forward = model.canForward
        model.reference = "John 4:2"
        model.followCommentaryLink(uri); try await idle(model)
        guard model.studyNote?.identifier.hasPrefix("xiphos-heading-") == true,
              model.studyNote?.html.contains("Original heading note.") == true,
              model.studyNote?.html.contains("Original body note.") == false,
              model.reference == "John 4:2", model.canBack == back, model.canForward == forward else { fatalError("native heading click retains source body, draft and navigation history") }
        model.previewStudyLink(uri); try await previewIdle(model)
        guard model.informationNote?.html.contains("Original heading note.") == true else { fatalError("native heading hover uses the same core lookup") }
        model.setNoteNumbers(false, commentary: true); try await idle(model)
        guard model.commentaryContentOptions.noteNumbers == false,
              headingURI(model.commentaryEntry?.headingHTML ?? "") == uri else { fatalError("counter/label display toggle retains scoped source identity") }
        model.read(); try await idle(model)
        model.followCommentaryLink(uri); try await idle(model)
        guard model.studyNote?.reference == "John 4:1", model.studyNote?.html.contains("Original heading note.") == true,
              model.renderedReference == "John 4:2" else { fatalError("retained heading URI does not drift to a later selected passage") }
        model.reference = "John4:7"; model.read(); try await idle(model)
        let linked = headingURI(model.commentaryEntry?.headingHTML ?? "")
        model.followCommentaryLink(linked); try await idle(model)
        guard model.studyNote?.reference == "John 4:7", model.studyNote?.html.contains("Original heading note.") == true else { fatalError("linked heading uses selected passage context") }
        model.reference = "John6:1"; model.read(); try await idle(model)
        guard model.commentaryEntry?.headingHTML.contains("xiphos-heading-") == false,
              fixtureWords(model.commentaryEntry?.headingHTML ?? "").contains("Original mixed") else { fatalError("unsupported heading capture keeps readable original rendering") }
        let retained = model.commentaryEntry?.headingHTML
        model.close(); model.followCommentaryLink(uri); try await idle(model); await worker.shutdown()
        guard model.commentaryEntry?.headingHTML == retained else { fatalError("closed heading action cannot publish") }
        print("Native heading click/hover, retained source, linked context and conservative fallback passed")
    }
    @MainActor static func noteNumbers(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        model.reference = "John4:5"; model.read(); try await idle(model)
        guard model.contentOptions.noteNumbers == true, model.html.contains("*n1"), model.html.contains("*n2") else { fatalError("native default On uses shared-core numbering") }
        let back = model.canBack, forward = model.canForward
        model.reference = "John 4:2"
        model.setNoteNumbers(false); try await idle(model)
        guard model.contentOptions.noteNumbers == false, !model.html.contains("*n1"),
              model.reference == "John 4:2", model.canBack == back, model.canForward == forward,
              other.contentOptions.noteNumbers == true else { fatalError("number control preserves draft/history and isolates windows") }
        model.setNoteNumbers(true); try await idle(model)
        model.showChapter = true; model.refreshDisplay(); try await idle(model)
        guard model.renderedAnchor == "verse-5", model.html.contains("*n3") else { fatalError("chapter refresh uses document counters and preserves selected verse") }
        model.chooseCommentary("ThMLNotes"); try await idle(model)
        model.setNoteNumbers(false, commentary: true); try await idle(model)
        guard model.commentaryContentOptions.noteNumbers == false, model.contentOptions.noteNumbers == true,
              model.commentaryChapter?.html.contains("*na") == false else { fatalError("commentary controls have independent core policy") }
        let profile = URL(fileURLWithPath: root).appendingPathComponent("native-note-controls.conf")
        model.saveReadingOptions(path: profile.path); try await idle(model)
        model.setNoteNumbers(false); try await idle(model)
        model.setNoteNumbers(true, commentary: true); try await idle(model)
        model.importReadingOptions(path: profile.path); try await idle(model)
        guard model.contentOptions.noteNumbers == true, model.commentaryContentOptions.noteNumbers == false,
              model.html.contains("*n3"), model.reference == "John 4:2", model.canBack == back, model.canForward == forward else { fatalError("number preferences round trip through shared persistence without navigation") }
        model.importReadingOptions(path: URL(fileURLWithPath: root).appendingPathComponent("missing/native-note.conf").path); try await idle(model)
        guard model.contentOptions.noteNumbers == true, model.html.contains("*n3") else { fatalError("failed import retains numbered snapshot") }
        let retained = model.html
        model.close(); model.setNoteNumbers(false); try await idle(model)
        guard model.html == retained else { fatalError("closed number action cannot publish") }
        // Copied Swift adapter states distinguish unavailable from explicit Off.
        let tab = try await worker.createTab()
        let unavailable = try await worker.contentDisplayOptions(tab: tab, module: "Text")
        guard unavailable.noteNumbers == nil else { fatalError("adapter never invents unknown consumer availability") }
        try await worker.enableModulePreferences(tab: tab)
        try await worker.setNoteNumberingEnabled(tab: tab, enabled: true)
        try await worker.setNoteNumbers(tab: tab, module: "Text", enabled: false)
        let copied = try await worker.contentDisplayOptions(tab: tab, module: "Text")
        do { try await worker.setNoteNumbers(tab: tab, module: "Text\0suffix", enabled: true); fatalError("embedded NUL must reject") } catch is CoreError {}
        try await worker.closeTab(tab)
        other.close(); await worker.shutdown()
        guard copied.noteNumbers == false, unavailable.noteNumbers == nil, retained.contains("*n3") else { fatalError("copied settings and HTML survive worker shutdown") }
        print("Native note numbering controls and adapter ownership passed")
    }
    @MainActor static func doubleSpace(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        model.reference = "John4:1"; model.read(); try await idle(model)
        guard model.contentOptions.doubleSpace == false else { fatalError("copied spacing defaults off") }
        let plain = model.html, back = model.canBack, forward = model.canForward
        model.reference = "John 4:2"
        model.setDoubleSpace(true); try await idle(model)
        guard model.contentOptions.doubleSpace == true, model.html == plain,
              model.lexicalPresentation.css.contains("2em ! important"), !model.lexicalPresentation.requested,
              model.reference == "John 4:2", model.canBack == back, model.canForward == forward,
              other.contentOptions.doubleSpace == false else { fatalError("spacing changes only CSS and preserves draft/history/window isolation") }
        model.showChapter = true; model.refreshDisplay(); try await idle(model)
        guard model.renderedAnchor == "verse-1", model.lexicalPresentation.css.contains("2em ! important") else { fatalError("chapter retains spacing CSS and committed anchor") }
        model.setReadingOption(.strongs, enabled: true); try await idle(model)
        let annotationHTML = model.html, annotationCSS = model.lexicalPresentation.css
        guard annotationCSS.contains("2.7em"), model.contentOptions.doubleSpace == true else { fatalError("core annotation spacing takes precedence") }
        model.setDoubleSpace(false); try await idle(model)
        guard model.html == annotationHTML, model.lexicalPresentation.css == annotationCSS,
              model.contentOptions.doubleSpace == false else { fatalError("spacing preference stays independent of active annotation CSS") }
        model.setDoubleSpace(true); try await idle(model)
        model.chooseCommentary("ThMLNotes"); try await idle(model)
        let commentaryHTML = model.commentaryChapter?.html
        guard model.commentaryContentOptions.doubleSpace == false else { fatalError("commentary has separate spacing default") }
        model.setDoubleSpace(true, commentary: true); try await idle(model)
        guard model.commentaryContentOptions.doubleSpace == true, model.commentaryChapter?.html == commentaryHTML,
              model.commentaryChapter?.lexical.css.contains("2em ! important") == true else { fatalError("commentary spacing changes CSS without reformatting body/headings") }
        let profile = URL(fileURLWithPath: root).appendingPathComponent("native-double-space.conf")
        model.saveReadingOptions(path: profile.path); try await idle(model)
        model.setDoubleSpace(false); try await idle(model)
        model.setDoubleSpace(false, commentary: true); try await idle(model)
        model.importReadingOptions(path: profile.path); try await idle(model)
        guard model.contentOptions.doubleSpace == true, model.commentaryContentOptions.doubleSpace == true,
              model.reference == "John 4:2", model.canBack == back, model.canForward == forward else { fatalError("native spacing profile round trip preserves draft/history") }
        model.reference = "John6:1"; model.read(); try await idle(model)
        guard model.lexicalPresentation.requested, !model.lexicalPresentation.applied,
              model.lexicalPresentation.css.contains("2em ! important"), !model.lexicalPresentation.css.contains("position: absolute") else { fatalError("native unsupported annotations retain spacing-only CSS") }
        let retained = model.html, retainedCSS = model.lexicalPresentation.css
        model.close(); model.setDoubleSpace(false); try await idle(model)
        guard model.html == retained, model.lexicalPresentation.css == retainedCSS else { fatalError("closed spacing action cannot publish") }
        other.close(); await worker.shutdown()
        print("Native spacing CSS-only refresh, precedence, fallback, profiles and isolation passed")
    }
    @MainActor static func lexicalLayout(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        model.reference = "John4:1"; model.read(); try await idle(model)
        guard model.lexicalPresentation == .plain else { fatalError("disabled annotation preference has no positioning CSS") }
        let reference = model.renderedReference, back = model.canBack
        model.reference = "John 4:2"
        model.setReadingOption(.strongs, enabled: true); try await idle(model)
        guard model.lexicalPresentation.applied, model.lexicalPresentation.css.contains("line-height: 2.7em"),
              model.html.contains("class=\"word\""), model.reference == "John 4:2",
              model.renderedReference == reference, model.canBack == back else { fatalError("native opt-in consumes paired HTML/CSS without submitting draft/history") }
        let document = PassageView.documentHTML(html: model.html, css: model.lexicalPresentation.css)
        guard document.contains(model.lexicalPresentation.css), document.contains("Content-Security-Policy"),
              document.contains("default-src 'none'") else { fatalError("native document includes core CSS and retains resource policy") }
        model.setImageContent(false); try await idle(model)
        model.setRespectFontFaces(false); try await idle(model)
        guard !model.html.contains("<img "), !model.html.contains("Original Lexical Font"), model.lexicalPresentation.applied else { fatalError("native layout refresh preserves core cleanup order") }
        model.showChapter = true; model.refreshDisplay(); try await idle(model)
        guard model.lexicalPresentation.applied, model.renderedAnchor == "verse-1", model.reference == "John 4:2" else { fatalError("native chapter uses matching CSS and committed anchor") }
        let profile = URL(fileURLWithPath: root).appendingPathComponent("native-lexical-profile.conf")
        try Data("[Text]\nStrong's Numbers=On\nImage Content=Off\nRespect Font Faces=Off\n[ThMLNotes]\nStrong's Numbers=On\n".utf8).write(to: profile)
        model.importReadingOptions(path: profile.path); try await idle(model)
        model.chooseCommentary("ThMLNotes"); try await idle(model)
        guard model.commentaryEntry?.lexical.applied == true, model.commentaryChapter?.lexical.applied == true,
              model.commentaryChapter?.lexical.css == model.lexicalPresentation.css else { fatalError("native paired commentary copies its own matching CSS") }
        other.reference = "John4:1"; other.read(); try await idle(other)
        guard other.lexicalPresentation == .plain else { fatalError("native windows retain independent annotation preferences") }
        model.reference = "John5:1"; model.read(); try await idle(model)
        guard model.lexicalPresentation.requested, !model.lexicalPresentation.applied, model.lexicalPresentation.css.isEmpty,
              model.commentaryChapter?.lexical.applied == false,
              !PassageView.documentHTML(html: model.html, css: model.lexicalPresentation.css).contains("position: absolute") else { fatalError("native unsupported documents omit annotation CSS for both panes") }
        let closedCSS = model.lexicalPresentation
        model.close(); model.setReadingOption(.strongs, enabled: false)
        try await Task.sleep(nanoseconds: 20_000_000)
        guard model.lexicalPresentation == closedCSS else { fatalError("closed native window cannot publish stale CSS") }
        other.close(); await worker.shutdown()
        print("Native lexical HTML/CSS, cleanup, chapter/pane fallback and draft/window guards passed")
    }
    @MainActor static func headings(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        model.reference = "John 4:2"
        model.chooseCommentary("Notes"); try await idle(model)
        guard model.commentaryHeadings, model.showCommentaryChapter,
              model.commentaryEntry?.headingHTML.contains("Original heading") == true,
              model.commentaryEntry?.html.contains("Original heading") == false,
              model.commentaryChapter?.verses.contains(where: { !$0.headingHTML.isEmpty }) == true else { fatalError("Swift copies owned heading snapshots") }
        let body = model.commentaryEntry?.html
        model.setCommentaryHeadings(false); model.setCommentaryHeadings(true)
        try await idle(model)
        guard !model.commentaryHeadings, model.commentaryEntry?.headingHTML.isEmpty == true,
              model.commentaryEntry?.html == body, model.reference == "John 4:2", !model.canBack,
              model.commentaryChapter?.html.contains("Original heading") == false else { fatalError("heading update preserves body/draft/history and rejects busy edit") }
        other.chooseCommentary("Notes"); try await idle(other)
        guard other.commentaryHeadings else { fatalError("heading preferences isolated by window") }
        other.chooseCommentary("ThMLNotes"); try await idle(other)
        guard other.commentaryEntry?.headingHTML.contains("Second ThML heading") == true,
              other.commentaryChapter?.verses.filter({ $0.headingHTML.contains("Original ThML heading café") }).count == 1 else { fatalError("native ThML sparse chapter keeps shared header once") }
        other.reference = "John 3:20"; other.read(); try await idle(other)
        guard let header = other.commentaryEntry?.headingHTML,
              let start = header.range(of: "href=\""), let end = header[start.upperBound...].firstIndex(of: "\"") else { fatalError("native ThML heading retains SDK reference URI") }
        let uri = String(header[start.upperBound..<end])
        let otherBody = other.commentaryEntry?.html, otherBack = other.canBack
        other.reference = "John 4:2"
        other.followCommentaryLink(uri); try await idle(other)
        guard other.referencePreview?.module == "ThMLNotes", other.referencePreview?.entries.first?.reference == "John 3:17",
              other.commentaryEntry?.html == otherBody, other.reference == "John 4:2", other.canBack == otherBack else { fatalError("native heading reference preview retains source/draft/history") }
        other.setCommentaryHeadings(false); try await idle(other)
        guard other.commentaryEntry?.headingHTML.isEmpty == true, other.commentaryEntry?.html == otherBody,
              model.commentaryHeadings == false else { fatalError("ThML heading preference preserves body and other module/window") }
        model.setCommentaryChapterMode(false); try await idle(model)
        model.setCommentaryHeadings(true); try await idle(model)
        guard model.commentaryChapter == nil, model.commentaryEntry?.headingHTML.contains("Original heading") == true,
              model.reference == "John 4:2" else { fatalError("verse headings use shared preference") }
        model.reference = "John 3:19"; model.read(); try await idle(model)
        guard model.commentaryEntry?.html.isEmpty == true,
              model.commentaryEntry?.headingHTML.contains("Original heading-only") == true else { fatalError("heading-only verse retained") }
        let profile = URL(fileURLWithPath: root).appendingPathComponent("native-headings.conf")
        try Data("[Notes]\nHeadings=Off\nCommentary by Chapter=Off\nCustom=keep\n".utf8).write(to: profile)
        model.reference = "John 4:2"
        let back = model.canBack
        model.importReadingOptions(path: profile.path); try await idle(model)
        guard !model.commentaryHeadings, model.commentaryEntry?.headingHTML.isEmpty == true,
              model.reference == "John 4:2", model.canBack == back else { fatalError("heading import preserves draft/history") }
        model.saveReadingOptions(path: profile.path); try await idle(model)
        let tab = try await worker.createTab()
        try await worker.importModulePreferences(tab: tab, path: profile.path)
        guard !(try await worker.headingDisplay(tab: tab, module: "Notes")),
              try String(contentsOf: profile, encoding: .utf8).contains("Custom=keep") else { fatalError("native heading save round trip") }
        try await worker.closeTab(tab)
        model.close(); model.setCommentaryHeadings(true); try await idle(model)
        guard !model.commentaryHeadings else { fatalError("closed window cannot publish heading changes") }
        other.close(); await worker.shutdown()
        print("Native heading model checks passed")
    }
    @MainActor static func commentary(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let model = ReaderModel(worker: worker, startupError: nil)
        let other = ReaderModel(worker: worker, startupError: nil)
        try await idle(model); try await idle(other)
        guard model.selection == "Text", model.commentaryEntry == nil else { fatalError("Bible startup remains unchanged") }
        model.reference = "John 4:2"
        let main = model.html
        model.chooseCommentary("Notes")
        model.chooseCommentary("OtherNotes") // busy selection must not race
        try await idle(model)
        guard model.reference == "John 4:2", model.html == main, model.commentarySelection == "Notes",
              model.commentaryEntry?.reference == "John 3:16", model.commentaryEntry?.html.contains("café") == true,
              !model.canBack, other.commentaryEntry == nil else { fatalError("committed passage/draft/window isolation") }
        guard model.showCommentaryChapter, model.commentaryChapter?.title == "John 3" else { fatalError("legacy regular-commentary default applies on selection") }
        model.setCommentaryChapterMode(false); try await idle(model)
        guard !model.showCommentaryChapter, model.commentaryChapter == nil, model.reference == "John 4:2", !model.canBack else { fatalError("core mode update preserves draft/history") }
        model.chooseCommentary("OtherNotes"); try await idle(model)
        guard model.showCommentaryChapter else { fatalError("other module keeps its default") }
        model.chooseCommentary("Notes"); try await idle(model)
        guard !model.showCommentaryChapter else { fatalError("returning module restores its setting") }
        model.clearHistory(); try await idle(model)
        model.reference = "John 3:16"; model.read(); try await idle(model)
        model.chooseCommentary("OtherNotes"); try await idle(model)
        model.reference = "John 3:17"; model.read(); try await idle(model)
        guard model.commentaryEntry?.reference == "John 3:17", model.commentaryEntry?.html.contains("OtherNotes second") == true else { fatalError("paired passage visit") }
        model.navigate(-1); try await idle(model)
        guard model.commentarySelection == "Notes", model.commentaryEntry?.reference == "John 3:16", model.canForward else { fatalError("paired Back restores modules") }
        model.navigate(1); try await idle(model)
        guard model.commentarySelection == "OtherNotes", model.commentaryEntry?.reference == "John 3:17" else { fatalError("paired Forward restores modules") }
        model.reference = "John 4:2"
        let notes = model.commentaryEntry?.html
        model.chooseCommentary("Missing"); try await idle(model)
        guard model.commentarySelection == "OtherNotes", model.commentaryEntry?.html == notes,
              model.reference == "John 4:2", model.canBack, !model.canForward else { fatalError("failed commentary preserves panes/draft/history") }
        let profile = URL(fileURLWithPath: root).appendingPathComponent("commentary-model-modops.conf")
        try Data("[OtherNotes]\nCommentary by Chapter=Off\nCustom=keep\n[Notes]\nCommentary by Chapter=On\n".utf8).write(to: profile)
        let historyBack = model.canBack, historyForward = model.canForward
        model.importReadingOptions(path: profile.path); try await idle(model)
        guard !model.showCommentaryChapter, model.commentaryChapter == nil, model.reference == "John 4:2",
              model.canBack == historyBack, model.canForward == historyForward else { fatalError("combined import refreshes commentary mode without submitting draft") }
        model.saveReadingOptions(path: profile.path); try await idle(model)
        let reloaded = try await worker.createTab()
        try await worker.importModulePreferences(tab: reloaded, path: profile.path)
        guard !(try await worker.commentaryChapterMode(tab: reloaded, module: "OtherNotes")),
              try await worker.commentaryChapterMode(tab: reloaded, module: "Notes"),
              try String(contentsOf: profile, encoding: .utf8).contains("Custom=keep") else { fatalError("native mode save round trip preserves other values") }
        try await worker.closeTab(reloaded)
        model.showChapter = true; model.refreshDisplay(); try await idle(model)
        guard model.renderedReference == "John 3", model.commentaryEntry?.reference == "John 3:17",
              model.reference == "John 4:2" else { fatalError("Bible chapter mode retains synchronized selected commentary verse") }
        model.setCommentaryChapterMode(true); try await idle(model)
        guard model.commentaryChapter?.title == "John 3", model.commentaryChapter?.selectedAnchor == "commentary-verse-17",
              model.reference == "John 4:2", !other.showCommentaryChapter else { fatalError("independent commentary chapter mode preserves draft/window state") }
        model.verseNumbers = false; model.highlightSelected = false; model.refreshDisplay(); try await idle(model)
        guard model.commentaryChapter?.html.contains("<table") == false, model.commentaryChapter?.html.contains("xc-current") == false else { fatalError("commentary display flags use shared presentation") }
        model.showChapter = false; model.refreshDisplay(); try await idle(model)
        guard model.renderedReference == "John 3:17", model.commentaryChapter?.title == "John 3", model.reference == "John 4:2" else { fatalError("Bible and commentary chapter modes independent") }
        model.chooseCommentary(""); try await idle(model)
        guard model.commentaryEntry == nil, model.commentaryChapter == nil, model.commentarySelection.isEmpty, model.reference == "John 4:2" else { fatalError("close commentary preserves Bible draft") }
        model.selection = "Notes"; model.selectionChanged(); try await idle(model)
        guard model.selection == "Text", model.commentarySelection == "Notes" else { fatalError("sidebar commentary opens correct pane") }
        model.dictionarySelection = "Dictionary"; model.dictionaryKey = "ALPHA"; model.lookupDictionary(); try await idle(model)
        guard model.dictionaryEntry?.html.contains("dictionary pane fixture") == true,
              model.commentarySelection == "Notes", model.commentaryEntry?.reference == "John 3:17" else { fatalError("dictionary lookup preserves synchronized commentary") }
        model.navigateChapter(1); try await idle(model)
        guard model.reference == "John 4:1", model.commentaryEntry?.reference == "John 4:1" else { fatalError("paired model chapter navigation") }
        model.chooseReference("John 3:16"); try await idle(model)
        guard model.commentaryEntry?.reference == "John 3:16" else { fatalError("selector synchronizes commentary") }
        model.followCommentaryLink("sword:///John3:17"); try await idle(model)
        guard model.selection == "Text", model.reference == "John 3:17", model.commentarySelection == "Notes",
              model.commentaryChapter?.selectedAnchor == "commentary-verse-17" else { fatalError("module-less commentary section link navigates Bible pair") }
        model.followLink("sword://OtherNotes/John3:16"); try await idle(model)
        guard model.selection == "Text", model.commentarySelection == "OtherNotes", model.reference == "John 3:16" else { fatalError("explicit commentary link retains Bible") }
        model.followCommentaryLink("sword:///John3:17"); try await idle(model)
        model.followCommentaryLink("passagestudy.jsp?action=showNote&type=n&value=1&passage=John3:17"); try await idle(model)
        guard model.studyNote?.module == "OtherNotes", model.studyNote?.html.contains("Original pane note") == true,
              model.selection == "Text", model.commentaryEntry?.reference == "John 3:17" else { fatalError("commentary note keeps source module and synchronized panes") }
        let last = model.commentaryEntry?.html
        model.chooseCommentary("OtherNotes"); model.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard model.commentaryEntry?.html == last else { fatalError("closed model cannot publish commentary") }
        other.close(); await worker.shutdown()
        print("Native commentary model checks passed")
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
        guard fixtureWords(first.html).contains("Original primary reading"), !fixtureWords(first.html).contains("Original secondary reading") else { fatalError("legacy primary reading default") }
        let variantFlags: EngineOptionFlags = [.variantsPrimary, .variantsSecondary, .variantsAll]
        let unrelatedFlags = first.engineOptions?.subtracting(variantFlags)
        let variantBack = first.canBack, variantForward = first.canForward
        first.reference = "John 4:2"
        for choice in [VariantReading.secondary, .all, .primary] {
            first.selectVariant(choice)
            guard first.isBusy else { fatalError("nonblocking variant selection") }
            try await idle(first)
            guard fixtureWords(first.html).contains("Original primary reading") == (choice != .secondary),
                  fixtureWords(first.html).contains("Original secondary reading") == (choice != .primary),
                  first.engineOptions?.subtracting(variantFlags) == unrelatedFlags,
                  first.reference == "John 4:2", first.canBack == variantBack, first.canForward == variantForward,
                  second.html == primaryHTML, first.renderedAnchor == "verse-22" else { fatalError("native variant chapter/output/options/history/window isolation") }
        }
        first.showChapter = false; first.refreshDisplay(); try await idle(first)
        first.selectVariant(.secondary); try await idle(first)
        guard !fixtureWords(first.html).contains("Original primary reading"), fixtureWords(first.html).contains("Original secondary reading") else { fatalError("variant single-entry output") }
        first.saveReadingOptions(path: saveURL.path); try await idle(first)
        second.importReadingOptions(path: saveURL.path); try await idle(second)
        guard second.engineOptions?.contains(.variantsSecondary) == true, fixtureWords(second.html).contains("Original secondary reading"),
              !fixtureWords(second.html).contains("Original primary reading") else { fatalError("native variant save/reload") }
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
        let beforeHoverMessage = first.message
        first.previewStudyLink("xiphos.url?action=showStrongs&type=Greek&value=caf%C3%A9")
        guard !first.isBusy else { fatalError("hover blocks reading actions") }
        try await previewIdle(first)
        guard first.informationPreview?.value == "café", first.informationPreview?.entry.module == "DictionaryOne",
              first.informationPreview?.sourceReference == "John 3:16", first.dictionaryEntry == nil, first.dictionaryHistory == nil,
              first.reference == "John 4:2", first.html == priorHTML, first.canBack == back, first.canForward == forward,
              second.informationPreview == nil else { fatalError("raw information hover preserves Bible/draft/dictionary/tab state") }
        first.previewStudyLink("xiphos.url?action=showMorph&type=Greek&value=alpha")
        first.previewStudyLink("xiphos.url?action=showStrongs&type=Greek&value=caf%C3%A9")
        try await previewIdle(first)
        guard first.informationPreview?.value == "café", first.informationPreview?.entry.module == "DictionaryOne" else { fatalError("latest hover owns publication") }
        first.previewStudyLink("https://example.invalid/xiphos.url?action=showStrongs&type=Greek&value=alpha")
        try await previewIdle(first)
        guard first.informationPreview?.value == "café", first.message == beforeHoverMessage else { fatalError("failed hover retains preview/status") }
        first.followLink("xiphos.url?action=showStrongs&type=Greek&value=alpha")
        try await idle(first)
        guard let lexical = first.lexicalPreview, lexical.kind == .strongs, lexical.entry.module == "DictionaryOne",
              lexical.entry.html.contains("café"), lexical.sourceReference == "John 3:16",
              first.reference == "John 4:2", first.html == priorHTML, first.renderedReference == priorRendered,
              first.canBack == back, first.canForward == forward, second.lexicalPreview == nil else { fatalError("native lexical source/pane/draft/history/isolation") }
        first.followLink("xiphos.url?action=showMorph&type=Greek&value=alpha")
        try await idle(first)
        guard first.lexicalPreview?.kind == .morphology, first.lexicalPreview?.entry.module == "MorphGreek" else { fatalError("native morphology preference") }
        guard first.dictionaryHistory?.entries.count == 2, first.dictionaryHistory?.canBack == true,
              first.dictionaryHistory?.canForward == false, second.dictionaryHistory == nil else { fatalError("native recorded lexical history/window isolation") }
        first.navigateDictionaryHistory(-1); try await idle(first)
        guard first.dictionaryEntry?.module == "DictionaryOne", first.dictionaryHistory?.cursor == 0,
              first.dictionaryHistory?.canForward == true, first.reference == "John 4:2", first.html == priorHTML,
              first.canBack == back, first.canForward == forward else { fatalError("native dictionary Back preserves Bible") }
        first.followLink("xiphos.url?action=showStrongs&type=Greek&value=alpha"); try await idle(first)
        guard first.dictionaryHistory?.entries.count == 2, first.dictionaryHistory?.canForward == true else { fatalError("duplicate lexical click keeps forward history") }
        first.navigateDictionaryHistory(1); try await idle(first)
        guard first.dictionaryEntry?.module == "MorphGreek", first.dictionaryHistory?.cursor == 1 else { fatalError("native dictionary Forward") }
        first.followLink("https://example.invalid/xiphos.url?action=showStrongs&type=Greek&value=alpha")
        try await idle(first)
        guard first.dictionaryEntry?.module == "MorphGreek", !first.message.isEmpty else { fatalError("failed lookup retained dictionary") }
        first.closeLexicalPreview()
        guard first.lexicalPreview == nil, first.html == priorHTML, first.reference == "John 4:2" else { fatalError("close dictionary preserved reading") }
        first.selection = "GenericDictionary"; first.dictionaryKey = ""; first.selectionChanged()
        guard first.selection == "OriginalBible", first.dictionarySelection == "GenericDictionary", first.dictionaryEntry == nil else { fatalError("dictionary module selection kept Bible selection") }
        first.dictionaryKey = "café"; first.lookupDictionary(); try await idle(first)
        guard first.dictionaryEntry?.module == "GenericDictionary", first.dictionaryEntry?.html.contains("Unicode") == true,
              first.dictionarySource?.module == "OriginalBible", first.dictionarySource?.reference == "John 3:16",
              first.lexicalPreview == nil, first.reference == "John 4:2", first.html == priorHTML,
              first.canBack == back, first.canForward == forward else { fatalError("direct dictionary source/draft/history") }
        first.dictionaryKey = ""; first.lookupDictionary()
        guard !first.isBusy, first.dictionaryEntry?.module == "GenericDictionary", first.message.contains("Enter") else { fatalError("empty direct lookup retained result") }
        first.dictionaryKey = "alpha\0ignored"; first.lookupDictionary(); try await idle(first)
        guard first.dictionaryEntry?.html.contains("Unicode") == true, !first.message.isEmpty else { fatalError("invalid direct lookup retained result") }
        first.dictionaryKey = "ALPHA"; first.dictionarySelection = "DictionaryTwo"; first.lookupDictionary(); try await idle(first)
        guard first.dictionaryEntry?.module == "DictionaryTwo", first.dictionaryEntry?.html.contains("DictionaryTwo") == true else { fatalError("explicit direct dictionary switch") }
        first.dictionaryKey = "unfinished key"
        first.navigateDictionary(1); try await idle(first)
        guard first.dictionaryEntry?.html.contains("Unicode") == true, first.dictionaryKey == first.dictionaryEntry?.key,
              first.dictionarySource?.reference == "John 3:16", first.reference == "John 4:2", first.html == priorHTML,
              first.canBack == back, first.canForward == forward else { fatalError("native key navigation uses committed key and preserves Bible") }
        first.navigateDictionary(-1); try await idle(first)
        guard first.dictionaryEntry?.html.contains("DictionaryTwo") == true else { fatalError("native previous dictionary key") }
        first.navigateDictionary(0)
        guard !first.isBusy else { fatalError("invalid native direction queued") }
        first.dictionarySelection = "DictionaryOne"; first.navigateDictionary(1)
        guard !first.isBusy, first.dictionaryEntry?.module == "DictionaryTwo" else { fatalError("uncommitted dictionary switch blocked navigation") }
        guard first.dictionaryHistory?.entries.count == 2 else { fatalError("direct lookup/key navigation changed recorded history") }
        let displayedDictionary = first.dictionaryEntry?.html
        first.clearDictionaryHistory(); try await idle(first)
        guard first.dictionaryHistory?.entries.isEmpty == true, first.dictionaryEntry?.html == displayedDictionary,
              first.html == priorHTML, first.reference == "John 4:2", first.canBack == back, first.canForward == forward else { fatalError("dictionary history clear preserved pane/Bible") }
        first.dictionarySelection = "DictionaryTwo"
        guard let nearby = first.dictionaryChoices, nearby.module == "DictionaryTwo", nearby.keys.count == 9,
              nearby.key == first.dictionaryEntry?.key else { fatalError("native committed nearby dictionary choices") }
        first.chooseDictionaryKey("not a copied choice")
        guard !first.isBusy else { fatalError("invalid nearby choice queued") }
        first.chooseDictionaryKey(nearby.keys[0]); try await idle(first)
        guard first.dictionaryEntry?.key == nearby.keys[0], first.dictionaryChoices?.key == first.dictionaryEntry?.key,
              first.dictionaryHistory?.entries.isEmpty == true, first.html == priorHTML, first.reference == "John 4:2" else { fatalError("native nearby selection preserves histories/Bible/draft") }
        first.followLink("xiphos.url?action=showStrongs&type=Greek&value=caf%C3%A9"); try await idle(first)
        guard first.lexicalPreview?.value == "café", first.dictionaryEntry?.key == "CAFÉ",
              first.dictionaryEntry?.html.contains("Unicode") == true, first.dictionaryHistory?.entries.last?.key == "CAFÉ",
              first.reference == "John 4:2", first.html == priorHTML, first.canBack == back, first.canForward == forward else { fatalError("native Unicode click key/history/raw metadata and Bible preservation") }
        first.closeLexicalPreview()
        guard first.dictionarySelection == nil, first.dictionaryEntry == nil, first.dictionaryChoices == nil else { fatalError("close direct dictionary") }
        guard first.informationPreview?.value == "café" else { fatalError("dictionary close retains information preview") }
        first.previewStudyLink("xiphos.url?action=showMorph&type=Greek&value=alpha")
        first.close(); try await Task.sleep(nanoseconds: 50_000_000)
        guard first.informationPreview == nil, !first.isPreviewLoading else { fatalError("closed window drops late hover") }
        second.applyLexiconPreferences(changed); second.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard second.lexiconPreferences == defaults else { fatalError("closed model does not publish preferences") }
        first.close(); await worker.shutdown()
        print("Native lexicon preference model checks passed")
    }
    @MainActor static func profiles(root: String, profileRoot: String) async throws {
        let directory = URL(fileURLWithPath: profileRoot, isDirectory: true)
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let source = directory.appendingPathComponent("settings-café.xml")
        let saved = directory.appendingPathComponent("saved.xml")
        let invalid = directory.appendingPathComponent("malformed.xml")
        let busy = directory.appendingPathComponent("busy-save.xml")
        let closedSave = directory.appendingPathComponent("closed-save.xml")
        for file in [busy, closedSave] where FileManager.default.fileExists(atPath: file.path) { try FileManager.default.removeItem(at: file) }
        let original = "<Xiphos><!--keep--><lexicons><greek>DictionaryOne</greek><hebrew>UnavailableHebrew</hebrew><greek_morph_nt>MorphGreek</greek_morph_nt><greek_morph_ot>UnavailableOT</greek_morph_ot><hebrew_morph>MorphHebrew</hebrew_morph></lexicons><locale><special>tr_TR.UTF-8</special></locale><other>unchanged</other></Xiphos>"
        try Data(original.utf8).write(to: source)
        try Data("<Xiphos>".utf8).write(to: invalid)
        try Data(original.utf8).write(to: saved)
        let worker = try CoreWorker(root: root)
        let first = ReaderModel(worker: worker, startupError: nil), second = ReaderModel(worker: worker, startupError: nil)
        try await idle(first); try await idle(second)
        let defaults = second.lexiconPreferences
        first.selection = "OriginalBible"; first.reference = "John 3:16"; first.read(); try await idle(first)
        first.setReadingOption(.strongs, enabled: true); try await idle(first)
        first.dictionarySelection = "DictionaryOne"; first.dictionaryKey = "03056"; first.lookupDictionary(); try await idle(first)
        first.reference = "John 4:2"
        let html = first.html, committed = first.renderedReference, engine = first.engineOptions
        let dictionary = first.dictionaryHistory, entry = first.dictionaryEntry?.html
        let back = first.canBack, forward = first.canForward
        first.importLexiconProfile(path: source.path)
        guard first.isBusy else { fatalError("profile import starts nonblocking busy state") }
        first.saveLexiconProfile(path: busy.path) // ignore actions while import is active
        try await idle(first)
        guard first.lexiconPreferences?.greek == "DictionaryOne", first.lexiconPreferences?.hebrew == "UnavailableHebrew",
              first.lexiconPreferences?.greekMorphNT == "MorphGreek", first.lexiconPreferences?.greekMorphOT == "UnavailableOT",
              first.lexiconPreferences?.hebrewMorph == "MorphHebrew", second.lexiconPreferences == defaults,
              first.message == "Lexicon choices imported for this window.", !FileManager.default.fileExists(atPath: busy.path) else { fatalError("model profile import/isolation/busy action") }
        guard first.reference == "John 4:2", first.html == html, first.renderedReference == committed,
              first.engineOptions == engine, first.dictionaryHistory == dictionary, first.dictionaryEntry?.html == entry,
              first.canBack == back, first.canForward == forward else { fatalError("profile import preserves drafts/content/options/histories") }
        let imported = first.lexiconPreferences
        first.importLexiconProfile(path: invalid.path); try await idle(first)
        guard first.lexiconPreferences == imported, first.message.contains("Unable to import lexicon choices"),
              first.reference == "John 4:2", first.html == html, first.dictionaryHistory == dictionary else { fatalError("friendly import error preserves prior state") }
        first.saveLexiconProfile(path: saved.path); try await idle(first)
        guard first.message == "Lexicon choices saved.", try Data(contentsOf: source) == Data(original.utf8),
              try String(contentsOf: saved, encoding: .utf8).contains("<other>unchanged</other>") else { fatalError("model save source/unrelated preservation") }
        second.importLexiconProfile(path: saved.path); try await idle(second)
        guard second.lexiconPreferences == imported else { fatalError("model save/import round trip") }
        let malformedBytes = try Data(contentsOf: invalid)
        first.saveLexiconProfile(path: invalid.path); try await idle(first)
        guard first.message.contains("Unable to save lexicon choices"), try Data(contentsOf: invalid) == malformedBytes,
              first.lexiconPreferences == imported, first.dictionaryHistory == dictionary else { fatalError("friendly save error preserves state/file") }
        first.previewStudyLink("xiphos.url?action=showStrongs&type=Greek&value=03056"); try await previewIdle(first)
        guard first.informationPreview?.entry.module == "DictionaryOne" else { fatalError("profile drives shared lexical hover") }
        let replacement = original.replacingOccurrences(of: "<greek>DictionaryOne</greek>", with: "<greek>DictionaryTwo</greek>")
        try Data(replacement.utf8).write(to: source)
        first.previewStudyLink("xiphos.url?action=showStrongs&type=Greek&value=alpha")
        first.importLexiconProfile(path: source.path); try await idle(first); try await previewIdle(first)
        guard first.lexiconPreferences?.greek == "DictionaryTwo", first.informationPreview?.value == "03056",
              first.reference == "John 4:2", first.html == html else { fatalError("profile import invalidates late hover publication") }
        first.previewStudyLink("xiphos.url?action=showStrongs&type=Greek&value=alpha"); try await previewIdle(first)
        guard first.informationPreview?.entry.module == "DictionaryTwo" else { fatalError("import retains installed names regardless of picker role metadata") }
        let beforeClose = second.lexiconPreferences, beforeMessage = second.message
        second.importLexiconProfile(path: source.path); second.close()
        second.saveLexiconProfile(path: closedSave.path)
        try await Task.sleep(nanoseconds: 50_000_000)
        guard second.lexiconPreferences == beforeClose, second.message == beforeMessage,
              !FileManager.default.fileExists(atPath: closedSave.path) else { fatalError("closed model drops late profile publication and new save") }
        first.close(); await worker.shutdown()
        print("Native profile controls/state/isolation/failure checks passed")
    }
    @MainActor static func studyHovers(root: String) async throws {
        let worker = try CoreWorker(root: root)
        let first = ReaderModel(worker: worker, startupError: nil), second = ReaderModel(worker: worker, startupError: nil)
        try await idle(first); try await idle(second)
        first.selection = "OptionOther"; first.reference = "John 3:23"; first.read(); try await idle(first)
        first.reference = "John 4:2"
        let html = first.html, rendered = first.renderedReference, options = first.engineOptions
        let back = first.canBack, forward = first.canForward, status = first.message
        let body = "passagestudy.jsp?action=showNote&type=n&value=1&module=OptionOther&passage=John3%3A19"
        let xref = "passagestudy.jsp?action=showNote&type=x&value=1&module=OptionOther&passage=John3%3A20"
        guard first.crossReferencesInVerseList, second.crossReferencesInVerseList else { fatalError("native preview preference comes from core default") }
        first.previewStudyLink(body)
        guard !first.isBusy else { fatalError("body hover blocks reading controls") }
        try await previewIdle(first)
        guard first.informationNote?.reference == "John 3:19", first.informationNote?.html.contains("Original other note") == true,
              first.informationPreview == nil, first.studyNote == nil, second.informationNote == nil,
              first.html == html, first.renderedReference == rendered, first.reference == "John 4:2",
              first.engineOptions == options, first.canBack == back, first.canForward == forward, first.message == status else { fatalError("native body information destination/source/state/isolation") }
        let retained = first.informationNote?.html
        first.previewStudyLink("passagestudy.jsp?action=showRef&type=scripRef&value=16-19&module=OptionOther"); try await previewIdle(first)
        first.previewStudyLink("https://example.invalid/passagestudy.jsp"); try await previewIdle(first)
        guard first.informationNote?.html == retained, first.referencePreview == nil, first.message == status else { fatalError("no-presentation/invalid hover retains preview/status") }
        first.previewStudyLink(xref); try await previewIdle(first)
        guard first.referencePreview?.entries.count == 5, first.referencePreview?.context == "John 3:23",
              first.informationNote?.html == retained, first.studyNote == nil, first.reference == "John 4:2",
              first.html == html, first.canBack == back, first.canForward == forward, second.referencePreview == nil else { fatalError("xref sidebar hover retains last information and committed source") }
        first.closeStudyNote()
        first.setCrossReferencesInVerseList(false)
        first.setCrossReferencesInVerseList(true) // busy toggle is ignored
        try await idle(first)
        guard !first.crossReferencesInVerseList, second.crossReferencesInVerseList,
              first.reference == "John 4:2", first.html == html else { fatalError("per-window core preference/busy/pane isolation") }
        first.previewStudyLink(xref); try await previewIdle(first)
        guard first.informationNote?.type == "crossReference", first.informationNote?.html.contains("Back to John 3:23") == true,
              first.referencePreview == nil, first.reference == "John 4:2", first.html == html else { fatalError("xref information-link preference destination") }
        let links = first.informationNote?.html
        first.previewStudyLink(body); first.previewStudyLink(xref); try await previewIdle(first)
        guard first.informationNote?.html == links else { fatalError("latest note hover owns publication") }
        first.followLink("sword:///John%203:23", currentModule: first.informationNote?.module)
        try await idle(first)
        guard first.reference == "John 3:23", first.selection == "OptionOther" else { fatalError("information return links use note module") }
        first.reference = "John 4:2"
        let newHTML = first.html, newRendered = first.renderedReference
        first.previewStudyLink(body)
        first.setCrossReferencesInVerseList(true); try await idle(first); try await previewIdle(first)
        guard first.informationNote?.html == links, first.crossReferencesInVerseList,
              first.html == newHTML, first.renderedReference == newRendered, first.reference == "John 4:2" else { fatalError("preference action invalidates pending hover publication") }
        // An older information preview must not become the main hover context.
        first.previewStudyLink(body); try await previewIdle(first)
        guard first.informationNote?.reference == "John 3:19" else { fatalError("older information source fixture") }
        first.reference = "John 3:16"; first.read(); try await idle(first)
        first.reference = "John 4:2"
        let currentHTML = first.html, currentHistoryBack = first.canBack, currentHistoryForward = first.canForward
        guard first.informationNote?.reference == "John 3:19" else { fatalError("reading retains previous information") }
        first.previewStudyLink("passagestudy.jsp?action=showNote&type=n&value=1")
        try await previewIdle(first)
        guard first.informationNote?.reference == "John 3:16", first.reference == "John 4:2",
              first.html == currentHTML, first.canBack == currentHistoryBack, first.canForward == currentHistoryForward else { fatalError("main hover fallback uses live committed context rather than old preview/draft") }
        first.followLink("passagestudy.jsp?action=showRef&type=scripRef&value=19-20&module=OptionOther"); try await idle(first)
        let nestedList = first.referencePreview?.html
        first.previewStudyLink(body); try await previewIdle(first)
        guard first.informationNote?.reference == "John 3:19", first.referencePreview?.html == nestedList,
              first.reference == "John 4:2", first.html == currentHTML else { fatalError("nested note hover preserves existing reference list and main state") }
        second.previewStudyLink(body); second.close()
        try await Task.sleep(nanoseconds: 50_000_000)
        guard second.informationNote == nil, !second.isPreviewLoading else { fatalError("closed window drops body result") }
        first.close(); await worker.shutdown()
        print("Native main note/reference hover destinations and guarded preference checks passed")
    }
    @MainActor static func main() async throws {
        let selectedLocale = initializePreviewLocale()
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--lexical-layout" {
            try await lexicalLayout(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--hover" {
            try await studyHovers(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 4, CommandLine.arguments[1] == "--profile" {
            try await profiles(root: CommandLine.arguments[2], profileRoot: CommandLine.arguments[3]); return
        }
        if CommandLine.arguments.count == 3, ["--locale", "--invalid-locale"].contains(CommandLine.arguments[1]) {
            let invalidEnvironment = CommandLine.arguments[1] == "--invalid-locale"
            guard invalidEnvironment ? selectedLocale == nil : selectedLocale?.hasPrefix("tr_TR") == true else { fatalError("environment locale startup selection") }
            let expectedKey = invalidEnvironment ? "I" : "İ"
            let worker = try CoreWorker(root: CommandLine.arguments[2])
            let tab = try await worker.createTab()
            try await worker.setLexiconPreferences(tab: tab, preferences: LexiconPreferences(greek: "DictionaryOne"))
            let uri = "xiphos.url?action=showStrongs&type=Greek&value=i"
            let clicked = try await worker.visitLexical(tab: tab, module: "OriginalBible", reference: "John3:16", uri: uri)
            let recorded = try await worker.dictionaryHistory(tab: tab)
            guard clicked.value == "i", recorded.entries.last?.key == expectedKey else { fatalError("host locale reaches shared GLib click casing") }
            _ = try await worker.visitLexical(tab: tab, module: "OriginalBible", reference: "John3:16", uri: uri, origin: .dialog)
            _ = try await worker.readLexical(tab: tab, module: "OriginalBible", reference: "John3:16", uri: uri)
            guard try await worker.dictionaryHistory(tab: tab) == recorded,
                  try await worker.history(tab).count == 0 else { fatalError("locale dialog/hover preserves dictionary and Bible histories") }
            try await worker.closeTab(tab); await worker.shutdown()
            guard recorded.entries.last?.key == expectedKey else { fatalError("locale snapshot lifetime") }
            print("Native environment locale startup/click checks passed")
            return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--lexicons" {
            try await lexicons(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--content" {
            try await content(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--headings" {
            try await headings(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--commentary" {
            try await commentary(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--heading-notes" {
            try await headingNotes(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--note-numbers" {
            try await noteNumbers(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--double-space" {
            try await doubleSpace(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--options" {
            try await readingOptions(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--book-documents" {
            try await bookDocuments(root: CommandLine.arguments[2]); return
        }
        if CommandLine.arguments.count == 3, CommandLine.arguments[1] == "--books" {
            try await books(root: CommandLine.arguments[2]); return
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
