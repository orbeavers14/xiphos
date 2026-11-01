// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
@main struct SyncSmoke {
    @MainActor static func check(_ value: Bool,_ message: String) { if !value { fatalError(message) } }
    @MainActor static func idle(_ reader: ReaderModel) async throws {
        let deadline = Date().addingTimeInterval(15)
        while reader.isBusy { check(Date() < deadline,"Reader deadline");try await Task.sleep(nanoseconds:10_000_000) }
    }
    @MainActor static func main() async throws {
        _ = NSApplication.shared;NSApplication.shared.setActivationPolicy(.prohibited)
        let worker = try CoreWorker(root:CommandLine.arguments[1])
        let model = NativeSyncModel(worker:worker)
        check(!model.active && !model.acknowledged,"Startup is disconnected without consent")
        model.user = "Original Ω identity";model.phrase = "Original session"
        model.connect();await model.waitUntilIdle()
        check(!model.active && model.message.contains("failed"),"Missing consent fails without opening transport")
        check(try await !worker.pollSync().active,"Failed native configuration leaves transport disconnected")
        let reader = ReaderModel(worker:worker,startupError:nil);try await idle(reader)
        var callbacks: [Bool] = []
        reader.onPassageCommitted = { _,received in callbacks.append(received) }
        model.receive = { navigation in reader.receiveSync(navigation) }
        model.review = { navigation in reader.reviewSync(navigation) }
        let nav = CoreSyncNavigation(speaker:"Original-speaker",module:"Text",reference:"John.3.17")
        try await worker.configureSync(CoreSyncSettings(mode:1,listen:2),network:false)
        let planned = try await worker.planSync(nav)
        check(planned.route == 1 && planned.navigation?.module == "Text","Owned Swift planning resolves installed modules")
        try await model.consume(CoreSyncBatch(active:true,overflow:false,events:[CoreSyncEvent(kind:78,navigation:nav)]))
        try await idle(reader)
        check(reader.syncTarget?.reference.contains("17") == true && callbacks == [true],"Incoming direct navigation uses existing reader with no-retransmit origin")
        reader.reference = "John 3:18";reader.read();try await idle(reader)
        check(callbacks == [true,false],"Local committed navigation retains broadcast origin")
        let range = CoreSyncNavigation(speaker:"Original-speaker",module:"Text",reference:"John.3.16-John.3.18")
        try await model.consume(CoreSyncBatch(active:true,overflow:false,events:[CoreSyncEvent(kind:78,navigation:range)]))
        check(model.pending.count == 1,"Ranges stay reviewable instead of implicit navigation")
        model.reviewPending(range);try await idle(reader)
        check(reader.referencePreview != nil && callbacks == [true,false],"Range review uses shared reference-list service without broadcast")
        var selected = CoreSyncSettings(mode:3,listen:1);selected.speakers = ["Different-speaker"]
        try await worker.configureSync(selected,network:false)
        check(try await worker.planSync(nav).route == 0,"Selected-speaker filtering crosses the owned Swift bridge")
        let missing = CoreSyncNavigation(speaker:"Original-speaker",module:"Missing",reference:"John.3.16")
        try await worker.configureSync(CoreSyncSettings(mode:3,listen:2),network:false)
        check(try await worker.planSync(missing).route == 2,"Missing module is retained for review")
        do { _ = try await worker.planSync(CoreSyncNavigation(speaker:"bad\0speaker",module:"Text",reference:"John3:16"));fatalError("NUL accepted") } catch {}
        let window = NativeSyncWindow(model:model);window.window.orderOut(nil)
        check(window.window.contentView != nil,"Native controls construct")
        await model.stop()
        check(!model.active && model.phrase.isEmpty && !model.acknowledged,"Disconnect clears phrase and consent")
        check(try await worker.planSync(nav).route == 0,"Disconnect clears shared receive policy")
        reader.close();await worker.shutdown()
        print("Native sync bridge, policy routing, origin suppression, review and disconnected controls passed (no network)")
    }
}
