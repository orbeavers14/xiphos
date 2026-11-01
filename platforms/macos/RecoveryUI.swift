// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import Combine

// Native scheduling only. Schema, revisions, backups and source conflicts remain
// in the shared core. One in-flight save prevents older autosaves racing newer ones.
@MainActor final class NativeRecoveryController: ObservableObject {
    private let worker: CoreWorker
    private(set) var path: String
    private var revision: String?
    private var lastXML: String?
    private var lastDrafts: [CoreRecoveryDraft]?
    private var ticker: Task<Void,Never>?
    private let interval: UInt64
    private var task: Task<Void,Never>?
    private var pending = false, stopped = false
    @Published private(set) var error: String?
    var onPathChanged: ((String) -> Void)?
    var onError: ((String) -> Void)?
    var capture: (() async throws -> CoreNativeSessionFile?)?
    var drafts: (() -> [CoreRecoveryDraft]?)?
    init(worker: CoreWorker, path: String, interval: UInt64 = 5_000_000_000) { self.worker = worker; self.path = path; self.interval = interval }
    func newDestination() {
        path += ".recovered-" + UUID().uuidString
        revision = nil; lastXML = nil; lastDrafts = nil
        onPathChanged?(path)
    }
    func load() async throws -> CoreRecoveryFile? {
        guard FileManager.default.fileExists(atPath:path) || FileManager.default.fileExists(atPath:path + ".previous") else { return nil }
        do {
            let saved = try await worker.readRecovery(path:path)
            if saved.previous { newDestination() }
            else { revision = saved.revision; lastXML = saved.session.xml; lastDrafts = saved.drafts }
            return saved
        } catch { newDestination(); throw error }
    }
    func start() {
        stopped = false
        ticker?.cancel()
        let interval = self.interval
        ticker = Task { [weak self] in
            while !Task.isCancelled {
                do { try await Task.sleep(nanoseconds:interval) } catch { return }
                guard let self, !self.stopped else { return };self.request()
            }
        }
        request()
    }
    func request() {
        guard !stopped, error == nil else { return }
        pending = true; guard task == nil else { return }
        task = Task { [weak self] in
            guard let self else { return }
            defer { self.task = nil }
            try? await Task.sleep(nanoseconds:300_000_000)
            while self.pending && !self.stopped {
                self.pending = false
                do {
                    guard let drafts = self.drafts?(), let session = try await self.capture?(), !self.stopped else { continue }
                    if self.lastXML == session.xml && self.lastDrafts == drafts { continue }
                    try FileManager.default.createDirectory(at:URL(fileURLWithPath:self.path).deletingLastPathComponent(),withIntermediateDirectories:true)
                    let saved = try await self.worker.saveRecovery(path:self.path,revision:self.revision,session:session,drafts:drafts)
                    self.revision = saved.revision; self.lastXML = saved.session.xml; self.lastDrafts = saved.drafts
                } catch {
                    self.error = error.localizedDescription; self.pending = false; self.onError?(error.localizedDescription)
                }
            }
        }
    }
    func flush() async { request(); await task?.value }
    func stop() async { stopped = true; pending = false; ticker?.cancel(); ticker = nil; await task?.value }
    func clear() async throws {
        await stop()
        let empty = try await worker.captureNativeSession(windows:[],activeKey:nil)
        try FileManager.default.createDirectory(at:URL(fileURLWithPath:path).deletingLastPathComponent(),withIntermediateDirectories:true)
        // Rotate twice so both valid generations record the deliberate clean exit.
        let first = try await worker.saveRecovery(path:path,revision:revision,session:empty,drafts:[])
        revision = first.revision
        let second = try await worker.saveRecovery(path:path,revision:revision,session:empty,drafts:[])
        revision = second.revision; lastXML = second.session.xml; lastDrafts = []
    }
}
