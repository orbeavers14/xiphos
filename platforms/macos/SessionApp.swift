// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
import SwiftUI

@MainActor final class NativeSessionController: NSObject, NSWindowDelegate {
    struct Record {
        let key: String
        let model: ReaderModel
        var capture: CoreWindowCapture
        var window: NSWindow?
        var frame: SessionWindowFrame?
    }
    let worker: CoreWorker
    private(set) var path: String
    private(set) var records: [Record] = []
    private(set) var activeKey: String?
    private(set) var restoreError: String?
    private var operation = false
    private var quitting = false
    private var approvedClose = Set<String>()
    var canOpenWindow: Bool { !operation && !quitting }
    private let showsWindows: Bool
    private let offscreenWindows: Bool
    init(worker: CoreWorker, path: String, showsWindows: Bool = true, offscreenWindows: Bool = false) {
        self.worker = worker; self.path = path; self.showsWindows = showsWindows; self.offscreenWindows = offscreenWindows
    }
    static func defaultPath() throws -> String {
        if let selected = ProcessInfo.processInfo.environment["XIPHOS_SESSION_PATH"], !selected.isEmpty { return selected }
        if let selected = UserDefaults.standard.string(forKey: "NativeSessionPath"), !selected.isEmpty { return selected }
        let support = try FileManager.default.url(for: .applicationSupportDirectory, in: .userDomainMask,
            appropriateFor: nil, create: false)
        return support.appendingPathComponent("org.xiphos.native-preview", isDirectory: true)
            .appendingPathComponent("session.xml").path
    }
    func start() async throws {
        guard records.isEmpty, !operation else { return }
        operation = true
        defer { operation = false }
        guard FileManager.default.fileExists(atPath: path) else { addWindow(); return }
        var stagedIDs: [UInt64] = []
        do {
            let saved = try await worker.readNativeSession(path: path)
            // Decode every host payload BEFORE the core's atomic replacement.
            let hosts = try saved.windows.map { try SessionHostState.decode($0.hostState) }
            let restored = try await worker.restoreNativeSession(saved)
            stagedIDs = restored.windows.map(\.tab)
            var staged: [Record] = []
            for (index, row) in restored.windows.enumerated() {
                guard index < saved.windows.count, saved.windows[index].key == row.key else {
                    throw SessionHostError(message: "Saved window identities do not match.")
                }
                let model = ReaderModel(worker: worker, startupError: nil, autostart: false)
                try await model.restoreWindow(saved.windows[index], tab: row.tab, host: hosts[index])
                var capture = row.capture; capture.hostState = saved.windows[index].hostState
                staged.append(Record(key: row.key, model: model, capture: capture, frame: hosts[index]?.frame))
            }
            records = staged; activeKey = saved.activeKey
            let desired = activeKey
            for index in records.indices { showWindow(at: index) }
            activeKey = desired
            if let desired, let window = records.first(where: { $0.key == desired })?.window { window.makeKeyAndOrderFront(nil) }
        } catch {
            for tab in stagedIDs { try? await worker.closeTab(tab) }
            restoreError = "The saved session could not be restored. It has not been changed. Use Save Session As to keep a new session separately."
            throw error
        }
    }
    @discardableResult func addWindow() -> ReaderModel {
        let key = UUID().uuidString
        let model = ReaderModel(worker: worker, startupError: nil)
        records.append(Record(key: key, model: model, capture: CoreWindowCapture(tab: 0, key: key)))
        activeKey = key; showWindow(at: records.count - 1); return model
    }
    private func showWindow(at index: Int) {
        guard showsWindows || offscreenWindows else { return }
        let record = records[index]
        let window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 1100, height: 760),
            styleMask: [.titled,.closable,.miniaturizable,.resizable], backing: .buffered, defer: false)
        window.title = "Xiphos Native Preview"; window.minSize = NSSize(width: 700, height: 450)
        window.isReleasedWhenClosed = false; window.delegate = self
        window.identifier = NSUserInterfaceItemIdentifier(record.key)
        window.contentView = NSHostingView(rootView: ReaderWindow(model: record.model))
        if let frame = record.frame, frame.valid {
            window.setFrame(window.constrainFrameRect(frame.rect, to: NSScreen.main), display: false)
        } else { window.center() }
        records[index].window = window
        if showsWindows { window.makeKeyAndOrderFront(nil) } else { window.orderOut(nil) }
    }
    private func ready() async throws {
        let deadline = Date().addingTimeInterval(30)
        while records.contains(where: { $0.model.isBusy || $0.model.search.isBusy }) {
            guard Date() < deadline else { throw SessionHostError(message: "Reading is still busy. Please try saving again when it finishes.") }
            try await Task.sleep(nanoseconds: 10_000_000)
        }
    }
    func save(to destination: String? = nil, excluding key: String? = nil) async throws {
        let deadline = Date().addingTimeInterval(30)
        while operation {
            guard Date() < deadline else { throw SessionHostError(message: "The previous session operation is still running.") }
            try await Task.sleep(nanoseconds: 10_000_000)
        }
        operation = true
        defer { operation = false }
        if restoreError != nil && destination == nil { throw SessionHostError(message: restoreError!) }
        if records.isEmpty {
            // Automatic quit preserves the last window, while explicit Save As
            // may deliberately write a zero-window session.
            guard let destination else { return }
            try FileManager.default.createDirectory(at: URL(fileURLWithPath: destination).deletingLastPathComponent(), withIntermediateDirectories: true)
            try await worker.saveNativeSession(path: destination, windows: [], activeKey: nil)
            path = destination; restoreError = nil; return
        }
        try await ready()
        for record in records { record.model.isBusy = true }
        defer { for record in records { record.model.isBusy = false } }
        let remaining = records.filter { $0.key != key }
        // Closing the last window retains it for the next launch.
        let selected = remaining.isEmpty ? records : remaining
        var captures: [CoreWindowCapture] = []
        for record in selected {
            guard let tab = record.model.sessionTab else { throw SessionHostError(message: "A reading window could not be saved.") }
            var capture = record.capture; capture.tab = tab
            capture.hostState = try SessionHostState(record.model, frame: record.window?.frame ?? record.frame?.rect).encoded(preserving: record.capture.hostState)
            captures.append(capture)
        }
        let active = selected.first(where: { $0.key == activeKey })?.key ?? selected.first?.key
        let target = destination ?? path
        try FileManager.default.createDirectory(at: URL(fileURLWithPath: target).deletingLastPathComponent(), withIntermediateDirectories: true)
        try await worker.saveNativeSession(path: target, windows: captures, activeKey: active)
        if destination != nil { path = target; restoreError = nil }
    }
    func prepareToQuit() async throws {
        try await save()
        quitting = true
    }
    func closeWindow(key: String) async throws {
        guard records.contains(where: { $0.key == key }) else { return }
        try await save(excluding: key)
        guard let current = records.firstIndex(where: { $0.key == key }) else { return }
        if let window = records[current].window {
            approvedClose.insert(key); window.performClose(nil)
        } else { removeWindow(key: key) }
    }
    private func removeWindow(key: String) {
        guard let at = records.firstIndex(where: { $0.key == key }) else { return }
        let removed = records.remove(at: at); removed.model.close()
        if activeKey == key { activeKey = records.first?.key }
    }
    func windowShouldClose(_ sender: NSWindow) -> Bool {
        if quitting { return true }
        guard let key = sender.identifier?.rawValue else { return true }
        if approvedClose.remove(key) != nil { return true }
        Task {
            do { try await closeWindow(key: key) }
            catch { showError(error.localizedDescription) }
        }
        return false
    }
    func windowWillClose(_ notification: Notification) {
        guard let key = (notification.object as? NSWindow)?.identifier?.rawValue else { return }
        removeWindow(key: key)
    }
    func windowDidBecomeMain(_ notification: Notification) {
        activeKey = (notification.object as? NSWindow)?.identifier?.rawValue ?? activeKey
    }
    func showError(_ message: String) {
        let alert = NSAlert(); alert.messageText = "Session could not be saved"; alert.informativeText = message
        alert.addButton(withTitle: "OK"); alert.runModal()
    }
}

@MainActor final class SessionAppDelegate: NSObject, NSApplicationDelegate {
    private var controller: NativeSessionController?
    private var terminating = false
    func applicationDidFinishLaunching(_ notification: Notification) {
        _ = initializePreviewLocale()
        Task {
            do {
                let worker = try CoreWorker(root: ProcessInfo.processInfo.environment["XIPHOS_MODULE_ROOT"])
                let owner = NativeSessionController(worker: worker, path: try NativeSessionController.defaultPath())
                controller = owner
                do { try await owner.start() }
                catch {
                    owner.addWindow()
                    let alert = NSAlert(); alert.messageText = "Saved session could not be restored"
                    alert.informativeText = owner.restoreError ?? error.localizedDescription
                    alert.addButton(withTitle: "OK"); alert.runModal()
                }
                NSApplication.shared.activate(ignoringOtherApps: true)
            } catch {
                let alert = NSAlert(); alert.messageText = "Unable to start the reading engine"
                alert.informativeText = error.localizedDescription; alert.runModal()
            }
        }
    }
    func openNewWindow() { guard !terminating, let controller, controller.canOpenWindow else { return }; controller.addWindow() }
    func saveSession() {
        guard !terminating, let controller else { return }
        Task { do { try await controller.save() } catch { controller.showError(error.localizedDescription) } }
    }
    func saveSessionAs() {
        guard !terminating, let controller else { return }
        let panel = NSSavePanel(); panel.title = "Save Reading Session"; panel.nameFieldStringValue = "session.xml"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            Task { do { try await controller.save(to: url.path); UserDefaults.standard.set(url.path, forKey: "NativeSessionPath") } catch { controller.showError(error.localizedDescription) } }
        }
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { false }
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        guard !terminating else { return .terminateLater }
        guard let controller else { return .terminateNow }
        terminating = true
        Task {
            do { try await controller.prepareToQuit(); sender.reply(toApplicationShouldTerminate: true) }
            catch {
                let alert = NSAlert(); alert.messageText = "Quit without saving this session?"
                alert.informativeText = error.localizedDescription
                alert.addButton(withTitle: "Cancel"); alert.addButton(withTitle: "Quit Without Saving")
                let quit = alert.runModal() == .alertSecondButtonReturn
                terminating = quit; sender.reply(toApplicationShouldTerminate: quit)
            }
        }
        return .terminateLater
    }
}
