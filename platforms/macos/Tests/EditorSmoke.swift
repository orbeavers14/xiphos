// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
import SwiftUI
@main struct EditorSmoke {
    @MainActor static func idle(_ model: NativeEditorModel) async throws {
        let deadline = Date().addingTimeInterval(20)
        while model.isBusy { guard Date() < deadline else { fatalError("Editor timeout") }; try await Task.sleep(nanoseconds:1_000_000) }
    }
    @MainActor static func main() async throws {
        guard CommandLine.arguments.count == 3 else { fatalError("Module and disposable editor paths required") }
        _ = NSApplication.shared; NSApplication.shared.setActivationPolicy(.prohibited)
        let sourceRoot = CommandLine.arguments[1], folder = URL(fileURLWithPath:CommandLine.arguments[2])
        guard folder.lastPathComponent == "native-editor-fixture" else { fatalError("Disposable editor folder only") }
        try? FileManager.default.removeItem(at:folder); try FileManager.default.createDirectory(at:folder,withIntermediateDirectories:true)
        let root=folder.appendingPathComponent("module-library").path
        try FileManager.default.copyItem(atPath:sourceRoot,toPath:root)
        let legacy=try EditorRichCodec.parse("<font color='#abc'><b>Original Ω</b></font>")
        guard legacy.text=="Original Ω",legacy.runs.first?.foreground=="#aabbcc",legacy.runs.first?.flags==1 else {fatalError("Shared C/Swift legacy color parsing preserves formatting")}
        let inlineFonts=try EditorRichCodec.parse("<span style='font-weight:700;font-style:italic'>Original Ω</span><span style='font-weight:400;font-style:normal'> plain</span>")
        guard inlineFonts.text=="Original Ω plain",inlineFonts.runs.count==2,inlineFonts.runs[0].flags==3,inlineFonts.runs[1].flags==0 else {fatalError("Shared C/Swift inline font traits and resets preserve original text")}
        let worker = try CoreWorker(root:root)
        let model = NativeEditorModel(worker:worker,location:.newDocument); await model.start()
        guard model.ready && model.richMode else { fatalError("New rich editor not ready") }
        model.accept(runs:[EditorRun(text:"Original Ω editor\nSecond line")]); model.selection = NSRange(location:0,length:8)
        model.format(0); model.format(1); model.format(5,text:"#ff0000"); model.format(7,text:"sword://Text/John3:16")
        guard model.dirty, model.source.contains("<b><i>Original</i></b>"), model.source.contains("sword://Text/John3:16") else { fatalError("Shared rich toolbar semantics") }
        let formatted = model.source; model.undo(); guard model.source != formatted else { fatalError("Rich undo") }; model.redo(); guard model.source == formatted else { fatalError("Rich redo") }
        let path = folder.appendingPathComponent("original.spt").path; try await model.save(to:path)
        guard !model.dirty, try String(contentsOfFile:path,encoding:.utf8) == formatted else { fatalError("Rich file save") }
        model.undo(); guard model.dirty else { fatalError("Undo after save must be dirty") }; model.redo(); guard !model.dirty else { fatalError("Saved baseline restored by redo") }
        let reopened = NativeEditorModel(worker:worker,location:.file(path)); await reopened.start()
        guard reopened.richMode, reopened.rich?.text == "Original Ω editor\nSecond line", reopened.source == formatted else { fatalError("Rich reopen lost source/styles") }
        reopened.accept(text:"A pending source edit")
        try "External original prose".write(toFile:path,atomically:true,encoding:.utf8)
        do { try await reopened.save(); fatalError("Native stale save accepted") } catch {}
        guard reopened.dirty, try String(contentsOfFile:path,encoding:.utf8) == "External original prose" else { fatalError("Native stale save overwrote file") }
        let unknown = folder.appendingPathComponent("unknown.spt").path
        let preserved = "<table><tr><td>Original unsupported prose %20.</td></tr></table>"
        try preserved.write(toFile:unknown,atomically:true,encoding:.utf8)
        let source = NativeEditorModel(worker:worker,location:.file(unknown)); await source.start(); source.setMode(true)
        guard source.ready, !source.richMode, source.source == preserved, !source.dirty else { fatalError("Unsupported markup was silently converted") }
        try await source.save(); guard try String(contentsOfFile:unknown,encoding:.utf8) == preserved else { fatalError("Source fallback lost markup") }
        let personal = NativeEditorModel(worker:worker,location:.module("Personal","John3:16")); await personal.start()
        guard personal.ready, personal.permitsRich else { fatalError("Personal module editor not ready: \(personal.message)") }
        personal.accept(runs:[EditorRun(text:"Original native personal note.")]); personal.selection = NSRange(location:0,length:8); personal.format(0)
        var saved = 0; personal.onModuleSaved = { _ in saved += 1 }; try await personal.save()
        let second = NativeEditorModel(worker:worker,location:.module("Personal","John3:16")); await second.start()
        guard saved == 1, second.source == personal.source, second.source.contains("<b>Original</b>") else { fatalError("Native rich SWORD save/reopen") }
        personal.accept(runs:[EditorRun(text:"Newer original personal content")]); try await personal.save()
        second.accept(text:"Stale module source")
        do { try await second.save(); fatalError("Native stale module overwrite") } catch {}
        try await personal.save(erase:true)
        let cleared = try await worker.openModuleEditor(tab:try await worker.createTab(),module:"Personal",reference:"John3:16")
        guard cleared.text.isEmpty else { fatalError("Personal clear") }
        let annotationsPath=folder.appendingPathComponent("annotations.xml").path
        let annotations=NativeAnnotationModel(worker:worker,path:annotationsPath,module:"Text",reference:"John3:16")
        await annotations.load();guard annotations.matches,annotations.profile==nil else {fatalError("Native note canonical/create target")}
        annotations.note="Original & <note> Ω";guard await annotations.save(),!annotations.dirty else {fatalError("Native checked note save: \(annotations.message)")}
        let originalRevision=annotations.profile!.revision
        let annotationExternal=try String(contentsOfFile:annotationsPath,encoding:.utf8)+"\n<!--External note metadata-->"
        try annotationExternal.write(toFile:annotationsPath,atomically:true,encoding:.utf8)
        annotations.note="Pending native note";guard await annotations.save()==false,annotations.dirty,annotations.profile!.revision==originalRevision,try String(contentsOfFile:annotationsPath,encoding:.utf8)==annotationExternal else {fatalError("Native stale note overwrote external data or lost draft")}
        await annotations.load();annotations.note="Retried note";guard await annotations.save() else {fatalError("Native note reload/retry")}
        let bookmarkPath=folder.appendingPathComponent("composition-bookmarks.xml").path
        try "<SwordBookmarks/>".write(toFile:bookmarkPath,atomically:true,encoding:.utf8)
        annotations.bookmarkPath=bookmarkPath;await annotations.preview()
        guard annotations.decoration?.noteHTML=="Retried note",annotations.decoration?.inner==1 else {fatalError("Native saved composition uses shared escaped notes and highlight precedence: \(annotations.message)")}
        annotations.reference="John3:17";guard !annotations.matches,await annotations.save()==false else {fatalError("Changed native note target accepted without loading")}
        annotations.reference="John3:16";guard await annotations.save(remove:true),annotations.note.isEmpty else {fatalError("Native checked note removal")}
        let setsPath=folder.appendingPathComponent("parallel-sets.xml").path
        let sets=NativeParallelSetsModel(worker:worker,path:setsPath);await sets.reload()
        guard sets.loaded,sets.profile.revision == nil else { fatalError("Explicit empty saved-set profile") }
        sets.name="Study Set";sets.orderedModules=["Text","TextAlt","Text"];await sets.edit(action:4)
        guard sets.profile.sets.count==1,sets.profile.sets[0].modules==sets.orderedModules else { fatalError("Native ordered duplicate set creation: \(sets.message)") }
        sets.newSet();sets.name="Study_Set";sets.orderedModules=["Notes"];await sets.edit(action:4)
        guard sets.profile.sets.count==1 else { fatalError("Native name codec collision overwrote saved set") }
        await sets.reload();sets.select("Study_Set");sets.move(2,by:-1);await sets.edit(action:0)
        guard sets.profile.sets[0].modules==["Text","Text","TextAlt"] else { fatalError("Native saved-set ordering") }
        let external=try String(contentsOfFile:setsPath,encoding:.utf8)+"\n"
        try external.write(toFile:setsPath,atomically:true,encoding:.utf8)
        sets.name="Renamed";await sets.edit(action:1)
        guard try String(contentsOfFile:setsPath,encoding:.utf8)==external,sets.profile.sets[0].key=="Study_Set" else { fatalError("Native stale set edit overwrote external data") }
        await sets.reload();sets.name="Renamed";await sets.edit(action:1);await sets.edit(action:3)
        guard sets.profile.current=="Renamed" else { fatalError("Native rename/default persistence") }
        let reloadedSets=NativeParallelSetsModel(worker:worker,path:setsPath);await reloadedSets.reload()
        guard reloadedSets.profile.current=="Renamed",reloadedSets.profile.sets[0].modules==["Text","Text","TextAlt"] else { fatalError("Fresh native set model reload") }
        sets.newSet();sets.name="café";sets.orderedModules=["Text"];await sets.edit(action:4)
        sets.newSet();sets.name="cafe\u{301}";sets.orderedModules=["Notes"];await sets.edit(action:4)
        guard sets.profile.sets.count==3 else { fatalError("Exact Unicode set names were merged") }
        sets.select("café");guard sets.orderedModules==["Text"] else { fatalError("Composed set selection") }
        sets.select("cafe\u{301}");guard sets.orderedModules==["Notes"] else { fatalError("Decomposed set selection") }
        await sets.edit(action:2);guard sets.selected.isEmpty,sets.profile.sets.count==2 else { fatalError("Native selected set deletion") }
        sets.select("Missing");guard sets.selected.isEmpty,sets.orderedModules.isEmpty else { fatalError("Missing selected identity remained an upsert target") }
        let creator = NativeBookSectionModel(worker:worker); await creator.load()
        guard creator.modules.contains(where: { $0.id == "CaptureBook" }) else { fatalError("Section creation catalog") }
        creator.module = "CaptureBook"; creator.path = "/Journals/Native Ω"
        guard let created = await creator.create(), created.text.isEmpty, created.reference == creator.path else { fatalError("Native empty-section creation: \(creator.message)") }
        guard await creator.create() == nil else { fatalError("Native creation overwrote an existing section") }
        let journal = NativeEditorModel(worker:worker,location:.module(created.module,created.reference)); await journal.start()
        guard journal.ready, journal.source.isEmpty, journal.richMode else { fatalError("Created section did not open in normal rich editor") }
        journal.accept(runs:[EditorRun(text:"Original journal draft Ω")]); journal.selection = NSRange(location:0,length:8); journal.format(0)
        guard let draft = journal.recoveryDraft(), draft.target == 2, draft.module == created.module, draft.reference == created.reference, draft.savedText.isEmpty else { fatalError("Created section draft missed ordinary module recovery") }
        journal.close()
        let recoveredJournal = NativeEditorModel(worker:worker,location:.module(created.module,created.reference),recovered:draft); await recoveredJournal.start()
        guard recoveredJournal.ready, recoveredJournal.dirty, recoveredJournal.source == draft.text else { fatalError("New journal draft restoration") }
        try await recoveredJournal.save()
        let reopenedJournal = NativeEditorModel(worker:worker,location:.module(created.module,created.reference)); await reopenedJournal.start()
        guard reopenedJournal.source == draft.text, !reopenedJournal.dirty else { fatalError("Recovered journal save/reopen") }
        creator.path = "/Prayers/"; guard await creator.create() == nil else { fatalError("Native malformed creation accepted") }
        recoveredJournal.close(); reopenedJournal.close()
        let book = NativeEditorModel(worker:worker,location:.module("CaptureBook","/Original")); await book.start()
        guard book.ready && book.permitsRich else { fatalError("Book rich editor readiness: \(book.message)") }
        book.accept(runs:[EditorRun(text:"Original rich book contents")]); book.selection = NSRange(location:0,length:27); book.format(4,value:2); try await book.save()
        guard book.source.contains("<h2>"), !book.dirty else { fatalError("Book heading save") }
        // Save As exports a module draft to a file without changing the module.
        let exportPath = folder.appendingPathComponent("export.spt").path
        book.accept(text:"Unsaved exported source"); try await book.save(to:exportPath)
        guard book.moduleSource == nil, try String(contentsOfFile:exportPath,encoding:.utf8) == "Unsaved exported source" else { fatalError("Module Save As did not export independently") }
        // Real offscreen window close: Cancel retains dirty text, Save finishes
        // the shared IO before the close delegate removes the editor.
        let window = NativeEditorWindow(worker:worker,location:.newDocument,show:false,moduleSaved:{ _ in })
        try await idle(window.model); window.model.accept(runs:[EditorRun(text:"Original close-save document")])
        window.model.selection = NSRange(location:0,length:8); window.model.format(0)
        window.window.contentView?.layoutSubtreeIfNeeded()
        try await Task.sleep(nanoseconds:50_000_000)
        func textView(_ view: NSView) -> NSTextView? {
            if let text = view as? NSTextView { return text }
            for child in view.subviews { if let found = textView(child) { return found } }
            return nil
        }
        guard let native = window.window.contentView.flatMap(textView),
              let font = native.textStorage?.attribute(.font,at:0,effectiveRange:nil) as? NSFont,
              NSFontManager.shared.traits(of:font).contains(.boldFontMask) else { fatalError("Native bold presentation missing") }
        native.setSelectedRange(NSRange(location:0,length:8))
        native.insertText("Changed",replacementRange:native.selectedRange())
        guard window.model.rich?.text.hasPrefix("Changed close-save document") == true else { fatalError("Native typing did not update the shared source model") }
        window.model.undo()
        try await Task.sleep(nanoseconds:20_000_000)
        native.setSelectedRange(NSRange(location:0,length:8))
        native.setMarkedText("原",selectedRange:NSRange(location:1,length:0),replacementRange:native.selectedRange())
        try await Task.sleep(nanoseconds:20_000_000)
        guard native.hasMarkedText() else { fatalError("Rich presentation interrupted input-method composition") }
        native.insertText("原创",replacementRange:native.markedRange())
        try await Task.sleep(nanoseconds:20_000_000)
        guard !native.hasMarkedText(), window.model.rich?.text.hasPrefix("原创 close-save document") == true else { fatalError("Input-method commit lost text") }
        window.model.accept(runs:[EditorRun(text:"Original Heading\n",flags:1,block:2),EditorRun(text:"First original item\nSecond original item",block:9)])
        window.window.contentView?.layoutSubtreeIfNeeded()
        try await Task.sleep(nanoseconds:20_000_000)
        if let view = window.window.contentView, let bitmap = view.bitmapImageRepForCachingDisplay(in:view.bounds) {
            view.cacheDisplay(in:view.bounds,to:bitmap)
            if let png = bitmap.representation(using:.png,properties:[:]) { try png.write(to:folder.appendingPathComponent("editor-preview.png")) }
        }
        var closed = false; window.onClosed = { closed = true }; window.closeDecision = { .cancel }
        window.window.performClose(nil); try await Task.sleep(nanoseconds:30_000_000)
        guard !closed && window.model.dirty else { fatalError("Cancel close lost dirty editor") }
        let closePath = folder.appendingPathComponent("closed.spt").path
        window.closeDecision = { .save }; window.saveDestination = { closePath }; window.window.performClose(nil)
        let deadline = Date().addingTimeInterval(10)
        while !closed { guard Date() < deadline else { fatalError("Editor close save timeout") }; try await Task.sleep(nanoseconds:1_000_000) }
        guard try String(contentsOfFile:closePath,encoding:.utf8) == window.model.source else { fatalError("Editor closed before save") }
        for item in [model,reopened,source,personal,second,book] { item.close() }
        await worker.shutdown()
        print("Native rich formatting, undo, source fallback, file/SWORD saves, stale rejection and dirty close passed")
    }
}
