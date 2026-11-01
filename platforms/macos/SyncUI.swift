// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
import SwiftUI

@MainActor final class NativeSyncModel: ObservableObject {
    let worker: CoreWorker
    @Published var settings = CoreSyncSettings(mode:1)
    @Published var user = ""
    @Published var phrase = ""
    @Published var acknowledged = false
    @Published var speakerIDs = ""
    @Published private(set) var available = false
    @Published private(set) var active = false
    @Published private(set) var busy = false
    @Published private(set) var message = "Disconnected. Sync shares passages, not notes or files."
    @Published private(set) var peers: [String] = []
    @Published private(set) var pending: [CoreSyncNavigation] = []
    private var applied = CoreSyncSettings()
    private var ticker: Task<Void,Never>?
    private var operation: Task<Void,Never>?
    private var sending: Task<Void,Never>?
    private var nextSend: CoreSyncNavigation?
    private var generation: UInt64 = 0
    var canSend: Bool { active && !busy && (applied.mode == 1 || applied.mode == 2) }
    var current: (() -> PassageTarget?)?
    var receive: ((CoreSyncNavigation) -> Bool)?
    var review: ((CoreSyncNavigation) -> Bool)?
    init(worker: CoreWorker) {
        self.worker = worker
        Task { self.available = await worker.syncAvailable() }
    }
    func waitUntilIdle() async { await operation?.value }
    private func cancelPolling() async {
        ticker?.cancel();sending?.cancel();nextSend = nil
        await ticker?.value;await sending?.value
        ticker = nil;sending = nil
    }
    func connect() {
        guard !busy else { return };busy = true;generation &+= 1
        let selected = settings, identity = user, sessionPhrase = phrase, consent = acknowledged
        let speakers = speakerIDs.split(whereSeparator: { $0 == "\n" || $0 == "," }).map { $0.trimmingCharacters(in:.whitespacesAndNewlines) }.filter { !$0.isEmpty }
        operation = Task {
            defer { busy = false;operation = nil }
            await cancelPolling();active = false;peers = [];pending = []
            var config = selected;config.speakers = speakers
            do {
                guard config.group.isEmpty || (config.group.count == 1 && "123456789".contains(config.group)) else { throw SessionHostError(message:"Use an empty group or one digit from 1 to 9.") }
                try await worker.configureSync(config,user:identity,phrase:sessionPhrase,acknowledged:consent)
                applied = config;active = config.mode != 0
                message = active ? "Connected. Applied mode: \(["Disabled","Personal","Speaker","Audience"][Int(config.mode)])." : "Disconnected."
                if active { startPolling() }
            } catch {
                try? await worker.configureSync(CoreSyncSettings())
                applied = CoreSyncSettings();message = "Connection failed; sync is disconnected. " + error.localizedDescription
            }
        }
    }
    func disconnect() {
        guard !busy else { return };busy = true;generation &+= 1
        operation = Task {
            defer { busy = false;operation = nil }
            await stop();message = "Disconnected."
        }
    }
    func stop() async {
        generation &+= 1;await cancelPolling()
        do { try await worker.configureSync(CoreSyncSettings()) }
        catch { message = "Disconnect failed. " + error.localizedDescription }
        active = false;applied = CoreSyncSettings();peers = [];pending = [];phrase = "";acknowledged = false
    }
    private func startPolling() {
        let token = generation
        ticker = Task { [weak self] in
            while !Task.isCancelled {
                guard let self else { return }
                await self.poll(generation:token)
                do { try await Task.sleep(nanoseconds:1_050_000_000) } catch { return }
            }
        }
    }
    func poll(generation token: UInt64? = nil) async {
        let token = token ?? generation
        do {
            let batch = try await worker.pollSync()
            guard token == generation, !Task.isCancelled else { return }
            active = batch.active
            if !active { message = "Connection stopped. Reconnect explicitly to resume.";ticker?.cancel();return }
            try await consume(batch,generation:token)
        } catch {
            guard token == generation else { return }
            active = false;ticker?.cancel();message = "Sync failed. Reconnect explicitly. " + error.localizedDescription
            try? await worker.configureSync(CoreSyncSettings())
        }
    }
    func consume(_ batch: CoreSyncBatch, generation token: UInt64? = nil) async throws {
        let token = token ?? generation
        guard token == generation,!Task.isCancelled else { return }
        for event in batch.events {
            switch event.kind {
            case 83: if !peers.contains(event.navigation.speaker),peers.count < 256 { peers.append(event.navigation.speaker) }
            case 68: peers.removeAll { $0 == event.navigation.speaker }
            case 78:
                let decision = try await worker.planSync(event.navigation)
                guard token == generation,!Task.isCancelled else { return }
                guard let navigation = decision.navigation else { continue }
                if decision.route == 1, receive?(navigation) == true { message = "Received \(navigation.module) · \(navigation.reference)" }
                else { let dropped = pending.count == 32;if dropped { pending.removeFirst() };pending.append(navigation);message = dropped ? "Older received passages were dropped; the latest 32 remain available for review.":"A received passage needs review. Missing modules and ranges are retained below." }
            default: break
            }
        }
        if batch.overflow { message = "Some sync events were dropped. Verify the current passage with the sender." }
    }

    func reviewPending(_ navigation: CoreSyncNavigation) {
        if review?(navigation) == true { message = "Opened received references for review." }
        else { message = "Open a reading window with the required module, then review this passage again." }
    }
    func broadcast(_ target: PassageTarget, received: Bool = false) {
        guard active,!busy,!received,(applied.mode == 1 || applied.mode == 2) else { return }
        let token = generation
        let navigation = CoreSyncNavigation(speaker:"local",module:target.module,reference:target.reference,group:applied.group.isEmpty ? "1":applied.group)
        nextSend = navigation
        guard sending == nil else { return }
        sending = Task {
            defer { sending = nil }
            while let navigation = nextSend,token == generation,active,!Task.isCancelled {
                nextSend = nil
                do { _ = try await worker.transmitSync(navigation,received:false) }
                catch {
                    guard token == generation else { return }
                    active = false;ticker?.cancel();nextSend = nil
                    message = "Send failed. Reconnect explicitly. " + error.localizedDescription
                    try? await worker.configureSync(CoreSyncSettings());return
                }
            }
        }
    }

    func sendCurrent() { if let target = current?() { broadcast(target) } else { message = "Open a Bible passage in a reading window first." } }
    func clearPending() { pending = [] }
}

struct NativeSyncView: View {
    @ObservedObject var model: NativeSyncModel
    var body: some View {
        VStack(alignment:.leading,spacing:12) {
            Text("BibleSync Passage Sharing").font(.title2)
            Text("Shares your current passage with running clients. Personal notes, bookmarks and files are not synchronized.").fixedSize(horizontal:false,vertical:true)
            if !model.available { Text("This build does not include the optional safe BibleSync networking SDK.").foregroundStyle(.secondary) }
            Form {
                Picker("Mode",selection:$model.settings.mode) { Text("Personal").tag(Int32(1));Text("Speaker").tag(Int32(2));Text("Audience").tag(Int32(3)) }
                Picker("Receive from",selection:$model.settings.listen) { Text("Nobody").tag(Int32(0));Text("Selected speaker IDs").tag(Int32(1));Text("Everyone in this session").tag(Int32(2)) }
                TextField("Your display name",text:$model.user)
                SecureField("Session phrase",text:$model.phrase)
                TextField("Group (1–9, optional)",text:$model.settings.group)
                if model.settings.listen == 1 { TextField("Allowed speaker IDs",text:$model.speakerIDs) }
                Toggle("Personal mode: this Mac only",isOn:$model.settings.privatePersonal)
                Toggle("Follow received passages",isOn:$model.settings.direct)
                Toggle("Allow network access and sharing my identity",isOn:$model.acknowledged)
            }.disabled(model.busy)
            Text("The session phrase and traffic are not encrypted. Changes take effect when you connect or reconnect. Startup is always disconnected.").font(.caption).fixedSize(horizontal:false,vertical:true)
            HStack {
                Button(model.active ? "Reconnect / Apply Changes":"Connect") { model.connect() }.disabled(!model.available || model.busy || !model.acknowledged || model.user.isEmpty || model.phrase.isEmpty)
                Button("Disconnect") { model.disconnect() }.disabled(model.busy || !model.active)
                Button("Send Current Passage") { model.sendCurrent() }.disabled(!model.canSend)
                if model.busy { ProgressView().controlSize(.small) }
            }
            Text(model.message).textSelection(.enabled).fixedSize(horizontal:false,vertical:true)
            Text("Speaker IDs").font(.headline)
            ScrollView { Text(model.peers.isEmpty ? "No peers observed.":model.peers.joined(separator:"\n")).textSelection(.enabled).frame(maxWidth:.infinity,alignment:.leading) }.frame(height:65)
            HStack { Text("Received passages for review").font(.headline);Spacer();Button("Clear") { model.clearPending() } }
            List(Array(model.pending.enumerated()),id:\.offset) { _,row in HStack { Text("\(row.module) · \(row.reference) · sender \(row.speaker)").textSelection(.enabled);Spacer();Button("Review") { model.reviewPending(row) } } }
        }.padding(20).frame(minWidth:640,minHeight:680).background(Color(nsColor:.windowBackgroundColor))
    }
}
@MainActor final class NativeSyncWindow {
    let model: NativeSyncModel, window: NSWindow
    init(model: NativeSyncModel) {
        self.model = model
        window = NSWindow(contentRect:NSRect(x:0,y:0,width:700,height:740),styleMask:[.titled,.closable,.miniaturizable,.resizable],backing:.buffered,defer:false)
        window.title = "BibleSync";window.minSize = NSSize(width:640,height:680);window.isReleasedWhenClosed = false;window.contentView = NSHostingView(rootView:NativeSyncView(model:model));window.center();window.makeKeyAndOrderFront(nil)
    }
}
