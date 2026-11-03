// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
@main struct RecoverySmoke {
    @MainActor static func check(_ value: Bool,_ message: String) { if !value { fatalError(message) } }
    @MainActor static func main() async throws {
        _ = NSApplication.shared;NSApplication.shared.setActivationPolicy(.prohibited)
        guard CommandLine.arguments.count >= 3 else { fatalError("Fixture paths required") }
        let fixture = URL(fileURLWithPath:CommandLine.arguments[2]);check(fixture.lastPathComponent == "native-recovery-fixture","Named fixture only")
        if CommandLine.arguments.count == 4 {
            let worker = try CoreWorker(root:fixture.appendingPathComponent("library").path)
            let file = try await worker.readRecovery(path:fixture.appendingPathComponent("fresh.recovery").path)
            check(file.drafts.first?.text == "Latest Ω unsaved draft" && file.session.windows.count == 1,"Fresh process source/session round trip")
            let owner = NativeSessionController(worker:worker,path:fixture.appendingPathComponent("normal-session.xml").path,showsWindows:false)
            try await owner.start(recovered:file.session);check(owner.records.count == 1,"Recovery reading windows reconstructed")
            let draft = file.drafts[0];let model = NativeEditorModel(worker:worker,location:.file(draft.path),recovered:draft);await model.start()
            check(model.ready && model.dirty && model.source == draft.text && model.selection == NSRange(location:7,length:1),"Fresh process dirty editor and UTF-8 selection reconstructed")
            do { try await model.save();fatalError("Recovered stale origin overwrote newer file") } catch {}
            check(try String(contentsOfFile:draft.path,encoding:.utf8) == "External source update","Fresh process protects changed source")
            try await model.save(to:fixture.appendingPathComponent("recovered.spt").path);check(!model.dirty,"Recovered Save As establishes saved baseline")
            let noteFile=try await worker.readRecovery(path:fixture.appendingPathComponent("fresh-note.recovery").path)
            let noteDraft=noteFile.drafts[0]
            check(noteDraft.target==3 && noteDraft.annotationLabel==" John.3.16" && noteDraft.annotationGeneric,"Fresh process distinguishes annotation identity and generic control")
            let note=NativeAnnotationModel(worker:worker,path:noteDraft.path,module:noteDraft.module,reference:noteDraft.reference,recovered:noteDraft)
            check(note.matches && note.dirty && note.note=="Recovered Ω 🕊 verse note" && note.recoveryID==noteDraft.id,"Recovered note restores exact source and stable identity")
            check(note.selection==NSRange(location:10,length:4) && noteDraft.selectionStart==10 && noteDraft.selectionEnd==17,"Fresh-process Unicode note selection restores UTF-8 offsets as UTF-16")
            var malformed=noteDraft;malformed.selectionStart=11;malformed.selectionEnd=12
            let invalidSelection=NativeAnnotationModel(worker:worker,path:noteDraft.path,module:noteDraft.module,reference:noteDraft.reference,recovered:malformed)
            check(invalidSelection.selection==NSRange(location:0,length:0) && invalidSelection.note==noteDraft.text,"Mid-codepoint selection cannot corrupt recovered text")
            check(await note.save()==false && note.note==noteDraft.text,"Recovered stale annotation cannot replace newer source or lose draft")
            let preserved=try String(contentsOfFile:noteDraft.path,encoding:.utf8)
            check(preserved.contains("External annotation metadata"),"Fresh-process source metadata remains unchanged")
            await note.load();note.note="Recovered note after explicit reload";check(await note.save(),"Reload permits a deliberate checked retry")
            model.close();for record in owner.records { record.model.close() };await worker.shutdown();return
        }
        try? FileManager.default.removeItem(at:fixture);try FileManager.default.createDirectory(at:fixture,withIntermediateDirectories:true)
        try FileManager.default.copyItem(at:URL(fileURLWithPath:CommandLine.arguments[1]),to:fixture.appendingPathComponent("library"))
        let managed = try await NativeLibrarySetup.open(profile:fixture.appendingPathComponent("first-run",isDirectory:true),explicit:nil,stored:nil,isolated:true)
        check(try await managed.worker.modules().isEmpty && managed.managedRoot != nil,"Fresh profiles create an explicit empty module library")
        let initialInstall = try await managed.worker.prepareLocalInstall(source:fixture.appendingPathComponent("library").path,module:"Text")
        check(initialInstall.decision == 0,"Fresh managed library accepts a first installation")
        try await managed.worker.installLocal(initialInstall,replace:false)
        check(try await managed.worker.modules().contains(where: { $0.id == "Text" }),"First-run library can install and discover modules")
        await managed.worker.shutdown()
        let worker = try CoreWorker(root:fixture.appendingPathComponent("library").path)
        let tab = try await worker.createTab();_ = try await worker.visit(tab:tab,module:"Text",reference:"John3:16")
        let session = try await worker.captureNativeSession(windows:[CoreWindowCapture(tab:tab,key:"reading-original")],activeKey:"reading-original")
        let file = fixture.appendingPathComponent("study.spt").path;try "Original saved source".write(toFile:file,atomically:true,encoding:.utf8)
        let editor = NativeEditorModel(worker:worker,location:.file(file));await editor.start();editor.setMode(false);editor.accept(text:"Initial Ω draft");editor.selection = NSRange(location:0,length:7)
        var latest = [editor.recoveryDraft()!]
        let path = fixture.appendingPathComponent("autosave.recovery").path
        let autosave = NativeRecoveryController(worker:worker,path:path,interval:100_000_000);check(try await autosave.load() == nil,"Missing recovery starts without writing")
        autosave.capture = { session };autosave.drafts = { latest };await autosave.flush()
        let first = try await worker.readRecovery(path:path);check(first.drafts[0].savedText == "Original saved source" && first.drafts[0].text == "Initial Ω draft","Autosave retains exact saved/current source")
        await autosave.flush();check(!FileManager.default.fileExists(atPath:path + ".previous"),"Unchanged snapshot does not rotate backup")
        // Exercise the actual periodic trigger, without calling request/flush.
        autosave.start();editor.accept(text:"Timer-saved source");latest = [editor.recoveryDraft()!]
        let deadline = Date().addingTimeInterval(5)
        while true { let value = try await worker.readRecovery(path:path);if value.drafts.first?.text == "Timer-saved source" { break };check(Date() < deadline,"Periodic autosave did not run");try await Task.sleep(nanoseconds:100_000_000) }
        await autosave.stop();autosave.start()
        editor.accept(text:"Intermediate source");latest = [editor.recoveryDraft()!];autosave.request()
        editor.accept(text:"Latest Ω unsaved draft");editor.selection = NSRange(location:7,length:1);latest = [editor.recoveryDraft()!];await autosave.flush()
        let final = try await worker.readRecovery(path:path);check(final.drafts[0].text == "Latest Ω unsaved draft","Coalesced save keeps newest text")
        let notePath=fixture.appendingPathComponent("notes.xml").path
        let note=NativeAnnotationModel(worker:worker,path:notePath,module:"Text",reference:"John3:16");note.generic=true
        await note.load();note.note="Original saved verse note";check(await note.save(),"Create note before autosave")
        note.note="Recovered Ω 🕊 verse note";note.selection=NSRange(location:10,length:4);note.draftChanged()
        let noteAutosave=NativeRecoveryController(worker:worker,path:fixture.appendingPathComponent("note-autosave.recovery").path,interval:100_000_000)
        noteAutosave.capture={session};noteAutosave.drafts={note.recoveryDraft().map {[$0]} ?? []};note.onRecoveryChanged={[weak noteAutosave] in noteAutosave?.request()};noteAutosave.start()
        let noteDeadline=Date().addingTimeInterval(5)
        while true {if let stored=try? await worker.readRecovery(path:noteAutosave.path),stored.drafts.first?.text==note.note {check(stored.drafts[0].annotationLabel==" John.3.16" && stored.drafts[0].annotationGeneric,"Annotation autosave preserves generic canonical identity");break};check(Date()<noteDeadline,"Periodic annotation autosave did not run");try await Task.sleep(nanoseconds:100_000_000)}
        let textView=NSTextView(frame:.zero);textView.string=note.note
        textView.setSelectedRange(NSRange(location:14,length:0))
        let coordinator=NativeAnnotationText.Coordinator(note)
        coordinator.textViewDidChangeSelection(Notification(name:NSTextView.didChangeSelectionNotification,object:textView))
        check(note.selection==NSRange(location:14,length:0),"Actual AppKit selection delegate updates note model")
        let selectionDeadline=Date().addingTimeInterval(5)
        while true {let stored=try await worker.readRecovery(path:noteAutosave.path);if stored.drafts[0].selectionStart==17 && stored.drafts[0].selectionEnd==17 {break};check(Date()<selectionDeadline,"Selection-only autosave did not run");try await Task.sleep(nanoseconds:100_000_000)}
        note.selection=NSRange(location:10,length:4)
        await noteAutosave.stop()
        _ = try await worker.saveRecovery(path:fixture.appendingPathComponent("fresh-note.recovery").path,revision:nil,session:session,drafts:[note.recoveryDraft()!])
        let externalNotes=try String(contentsOfFile:notePath,encoding:.utf8)+"\n<!--External annotation metadata-->"
        try externalNotes.write(toFile:notePath,atomically:true,encoding:.utf8)
        let unattached=NativeAnnotationModel(worker:worker,path:notePath,module:"Missing",reference:"John3:16")
        await unattached.load();unattached.note="Unattached source draft";check(unattached.recoveryDraft()?.target==0,"Unvalidated note intent recovers only as an untitled document")
        // Core origin revision remains stale after an external source edit.
        try "External source update".write(toFile:file,atomically:true,encoding:.utf8)
        _ = try await worker.saveRecovery(path:fixture.appendingPathComponent("fresh.recovery").path,revision:nil,session:session,drafts:latest)
        try Data("Unknown future recovery file".utf8).write(to:URL(fileURLWithPath:path))
        latest[0].text = "Another unsaved source";await autosave.flush();check(autosave.error != nil,"External recovery changes pause autosave")
        check(try String(contentsOfFile:path,encoding:.utf8) == "Unknown future recovery file","Unknown recovery preserved")
        await autosave.stop()
        let fallback = NativeRecoveryController(worker:worker,path:path);let recovered = try await fallback.load();check(recovered?.previous == true && fallback.path != path,"Corrupt-primary fallback uses a new safe destination")
        fallback.capture = { session };fallback.drafts = { latest };await fallback.flush();check(FileManager.default.fileExists(atPath:fallback.path),"Fallback autosave writes independently")
        try await fallback.clear();let empty = try await worker.readRecovery(path:fallback.path);let previous = try await worker.readRecovery(path:fallback.path + ".previous");check(empty.drafts.isEmpty && empty.session.windows.isEmpty && previous.drafts.isEmpty,"Clean exit clears both generations")
        editor.close();try await worker.closeTab(tab);await worker.shutdown()
        print("Native recovery scheduling, conflict and backup checks passed")
    }
}
