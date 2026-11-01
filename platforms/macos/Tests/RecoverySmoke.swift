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
