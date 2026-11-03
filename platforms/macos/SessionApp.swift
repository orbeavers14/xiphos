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
    var onPassageCommitted: ((PassageTarget,Bool) -> Void)?
    let worker: CoreWorker
    let organizationPath: String?
    private var libraryOrganization = CoreLibraryOrganization.empty
    private var organizationNotice = ""
    func applyLibraryOrganization(_ value: CoreLibraryOrganization) {
        libraryOrganization=value;organizationNotice="";records.forEach { $0.model.applyLibraryOrganization(value) }
    }
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
    init(worker: CoreWorker, path: String, showsWindows: Bool = true, offscreenWindows: Bool = false,organizationPath: String? = nil) {
        self.organizationPath=organizationPath;self.worker = worker; self.path = path; self.showsWindows = showsWindows; self.offscreenWindows = offscreenWindows
    }
    static func profileFolder() throws -> URL {
        if let selected = ProcessInfo.processInfo.environment["XIPHOS_PROFILE_DIRECTORY"], !selected.isEmpty { return URL(fileURLWithPath:selected,isDirectory:true) }
        // Isolate developer/test session overrides from the normal user profile.
        if let selected = ProcessInfo.processInfo.environment["XIPHOS_SESSION_PATH"], !selected.isEmpty { return URL(fileURLWithPath:selected).deletingLastPathComponent().appendingPathComponent("native-profile",isDirectory:true) }
        return try FileManager.default.url(for:.applicationSupportDirectory,in:.userDomainMask,appropriateFor:nil,create:false).appendingPathComponent("org.xiphos.native-preview",isDirectory:true)
    }
    static func sessionPreferenceKey() -> String {
        if let selected = ProcessInfo.processInfo.environment["XIPHOS_PROFILE_DIRECTORY"], !selected.isEmpty { return "NativeSessionPath:" + selected }
        if let selected = ProcessInfo.processInfo.environment["XIPHOS_SESSION_PATH"], !selected.isEmpty { return "NativeSessionPath:" + selected }
        return "NativeSessionPath"
    }
    static func defaultPath() throws -> String {
        if let selected = ProcessInfo.processInfo.environment["XIPHOS_SESSION_PATH"], !selected.isEmpty { return selected }
        if let selected = UserDefaults.standard.string(forKey:sessionPreferenceKey()), !selected.isEmpty { return selected }
        return try profileFolder().appendingPathComponent("session.xml").path
    }
    func start(recovered: CoreNativeSessionFile? = nil) async throws {
        guard records.isEmpty, !operation else { return }
        operation = true
        defer { operation = false }
        if let path=organizationPath { do { libraryOrganization=try await worker.libraryOrganization(path:path) } catch { organizationNotice="Library preferences could not be loaded. Open Module Library and reload." } }
        guard recovered != nil || FileManager.default.fileExists(atPath: path) else { addWindow(); return }
        var stagedIDs: [UInt64] = []
        do {
            let saved: CoreNativeSessionFile
            if let recovered { saved = recovered } else { saved = try await worker.readNativeSession(path:path) }
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
                model.applyLibraryOrganization(libraryOrganization,notice:organizationNotice)
                model.onPassageCommitted = { [weak self] target,received in self?.onPassageCommitted?(target,received) }
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
        model.applyLibraryOrganization(libraryOrganization,notice:organizationNotice)
        model.onPassageCommitted = { [weak self] target,received in self?.onPassageCommitted?(target,received) }
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
    func captureRecoverySession() async throws -> CoreNativeSessionFile? {
        guard !operation, !quitting, !records.contains(where: { $0.model.isBusy || $0.model.search.isBusy }) else { return nil }
        operation = true; defer { operation = false }
        var captures: [CoreWindowCapture] = []
        for record in records {
            guard let tab = record.model.sessionTab else { return nil }
            var capture = record.capture; capture.tab = tab
            capture.hostState = try SessionHostState(record.model,frame:record.window?.frame ?? record.frame?.rect).encoded(preserving:record.capture.hostState)
            captures.append(capture)
        }
        return try await worker.captureNativeSession(windows:captures,activeKey:activeKey)
    }
    func cancelQuit() { quitting = false }
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
    func refreshAfterModuleEdit() {
        Task { do { try await ready(); for record in records { record.model.refreshAfterModuleEdit() } } catch { showError(error.localizedDescription) } }
    }
    func showError(_ message: String) {
        let alert = NSAlert(); alert.messageText = "Session could not be saved"; alert.informativeText = message
        alert.addButton(withTitle: "OK"); alert.runModal()
    }
}

@MainActor final class SessionAppDelegate: NSObject, NSApplicationDelegate {
    private var controller: NativeSessionController?
    private var terminating = false
    private var editors: [NativeEditorWindow] = []
    private var sectionCreator: NativeBookSectionWindow?
    private var recovery: NativeRecoveryController?
    private var migration: NativeMigrationWindow?
    private var sync: NativeSyncModel?
    private var syncWindow: NativeSyncWindow?
    private var library: NativeLibraryWindow?
    private var bookmarks: NativeBookmarkWindow?
    private var managedRoot: String?
    func applicationDidFinishLaunching(_ notification: Notification) {
        _ = initializePreviewLocale()
        Task {
            do {
                let folder = try NativeSessionController.profileFolder()
                try FileManager.default.createDirectory(at:folder,withIntermediateDirectories:true)
                let context = try await NativeLibrarySetup.open(profile:folder,explicit:ProcessInfo.processInfo.environment["XIPHOS_MODULE_ROOT"],stored:UserDefaults.standard.string(forKey:NativeLibrarySetup.preferenceKey(profile:folder)),isolated:ProcessInfo.processInfo.environment["XIPHOS_PROFILE_DIRECTORY"].map { !$0.isEmpty } ?? false)
                let worker = context.worker;managedRoot = context.managedRoot
                let owner = NativeSessionController(worker: worker, path: try NativeSessionController.defaultPath(),organizationPath:folder.appendingPathComponent("library-organization.xml").path)
                controller = owner
                let sync = NativeSyncModel(worker:worker);self.sync = sync
                sync.current = { [weak owner] in owner?.records.first(where: { $0.key == owner?.activeKey })?.model.syncTarget }
                sync.receive = { [weak self,weak owner] navigation in guard self?.terminating == false else { return false };return owner?.records.first(where: { $0.key == owner?.activeKey })?.model.receiveSync(navigation) ?? false }
                sync.review = { [weak self,weak owner] navigation in guard self?.terminating == false else { return false };return owner?.records.first(where: { $0.key == owner?.activeKey })?.model.reviewSync(navigation) ?? false }
                owner.onPassageCommitted = { [weak self,weak sync] target,received in guard self?.terminating == false else { return };sync?.broadcast(target,received:received) }
                let key = "NativeRecoveryPath:" + folder.path
                let recovery = NativeRecoveryController(worker:worker,path:UserDefaults.standard.string(forKey:key) ?? folder.appendingPathComponent("recovery.snapshot").path)
                self.recovery = recovery
                recovery.onPathChanged = { UserDefaults.standard.set($0,forKey:key) }
                var saved: CoreRecoveryFile?
                do { saved = try await recovery.load() }
                catch { let alert = NSAlert(); alert.messageText = "Recovery copy could not be opened"; alert.informativeText = "Existing recovery files are unchanged. A new recovery file will be used. Save any recovered source separately."; alert.runModal() }
                if let candidate = saved, candidate.drafts.isEmpty && candidate.session.windows.isEmpty { saved = nil }
                if let candidate = saved, !candidate.drafts.isEmpty || !candidate.session.windows.isEmpty {
                    let alert = NSAlert(); alert.messageText = "Recover the previous session?"
                    alert.informativeText = "Restore \(candidate.session.windows.count) reading windows and \(candidate.drafts.count) study documents. Original documents and modules will not be overwritten." + (candidate.previous ? " This is the previous valid recovery copy." : "")
                    alert.addButton(withTitle:"Recover"); alert.addButton(withTitle:"Discard Recovery")
                    if alert.runModal() != .alertFirstButtonReturn { try await recovery.clear(); saved = nil }
                }
                do { try await owner.start(recovered:saved?.session) }
                catch {
                    owner.addWindow()
                    let alert = NSAlert(); alert.messageText = "Saved session could not be restored"
                    alert.informativeText = owner.restoreError ?? error.localizedDescription
                    alert.addButton(withTitle: "OK"); alert.runModal(); recovery.newDestination()
                }
                if let saved { for draft in saved.drafts { self.openRecoveredEditor(draft) } }
                recovery.capture = { [weak owner] in try await owner?.captureRecoverySession() }
                recovery.drafts = { [weak self = self] in
                    guard let self, !self.editors.contains(where: { $0.model.isBusy }) else { return nil }
                    return self.editors.compactMap { $0.model.recoveryDraft() }
                }
                recovery.onError = { message in let alert = NSAlert(); alert.messageText = "Autosave paused"; alert.informativeText = "Save documents normally or use Save As. Your original documents and modules are unchanged. " + message; alert.runModal() }
                recovery.start()
                NSApplication.shared.activate(ignoringOtherApps: true)
            } catch {
                let alert = NSAlert(); alert.messageText = "Unable to start the reading engine"
                alert.informativeText = error.localizedDescription; alert.runModal()
            }
        }
    }
    private func openEditor(_ location: NativeEditorLocation, recovered: CoreRecoveryDraft? = nil) {
        guard !terminating, let controller, controller.canOpenWindow else { return }
        let editor = NativeEditorWindow(worker:controller.worker,location:location,recovered:recovered) { [weak controller] _ in controller?.refreshAfterModuleEdit() }
        editor.model.onRecoveryChanged = { [weak self] in self?.recovery?.request() }
        editor.onClosed = { [weak self, weak editor] in self?.editors.removeAll { $0 === editor }; self?.recovery?.request() }
        editors.append(editor)
    }
    private func openRecoveredEditor(_ draft: CoreRecoveryDraft) {
        let location: NativeEditorLocation = draft.target == 1 ? .file(draft.path) : draft.target == 2 ? .module(draft.module,draft.reference) : .newDocument
        openEditor(location,recovered:draft)
    }
    func chooseModuleLibrary(create: Bool = false) {
        guard !terminating else { return }
        let selected: URL?
        if create {
            let panel = NSSavePanel();panel.title = "New Module Library";panel.nameFieldStringValue = "Modules";panel.canCreateDirectories = true
            selected = panel.runModal() == .OK ? panel.url : nil
        } else {
            let panel = NSOpenPanel();panel.title = "Choose Module Library";panel.canChooseDirectories = true;panel.canChooseFiles = false;panel.allowsMultipleSelection = false;panel.showsHiddenFiles = true
            selected = panel.runModal() == .OK ? panel.url : nil
        }
        guard let selected else { return }
        do {
            if create { try NativeLibrarySetup.create(at:selected) }
            else { var directory: ObjCBool = false;guard FileManager.default.fileExists(atPath:selected.appendingPathComponent("mods.d").path,isDirectory:&directory),directory.boolValue else { throw SessionHostError(message:"Select a module library folder containing mods.d, or use New Module Library.") } }
            let profile = try NativeSessionController.profileFolder()
            UserDefaults.standard.set(selected.path,forKey:NativeLibrarySetup.preferenceKey(profile:profile))
            let alert = NSAlert();alert.messageText = "Restart to use this module library";alert.informativeText = selected.path + "\nThe current library stays active until the app restarts.";alert.runModal()
        } catch { let alert = NSAlert();alert.messageText = "Module library could not be selected";alert.informativeText = error.localizedDescription;alert.runModal() }
    }
    func openLegacyMigration() {
        guard !terminating,let controller else { return }
        if let migration { migration.window.makeKeyAndOrderFront(nil);return }
        let model = NativeMigrationModel(worker:controller.worker)
        model.currentSessionPath = { [weak controller] in controller?.path }
        model.onUseSession = { [weak controller] path in
            guard let controller,controller.canOpenWindow else { throw SessionHostError(message:"The reading session is busy.") }
            guard ProcessInfo.processInfo.environment["XIPHOS_SESSION_PATH"] == nil else { throw SessionHostError(message:"A development session-path override is active. Select the exported session explicitly after removing that override.") }
            try await controller.save()
            UserDefaults.standard.set(path,forKey:NativeSessionController.sessionPreferenceKey())
        }
        migration = NativeMigrationWindow(model:model)
    }
    func openSync() {
        guard !terminating,let sync else { return }
        if let syncWindow { syncWindow.window.makeKeyAndOrderFront(nil) } else { syncWindow = NativeSyncWindow(model:sync) }
    }
    func openBookmarks() {
        guard !terminating,let controller else { return }
        if let bookmarks { bookmarks.window.makeKeyAndOrderFront(nil) }
        else { bookmarks=NativeBookmarkWindow(worker:controller.worker) }
    }
    func openModuleLibrary() {
        guard !terminating, let controller else { return }
        if let library { library.window.makeKeyAndOrderFront(nil);library.model.reload();return }
        let workspace = FileManager.default.temporaryDirectory.appendingPathComponent("org.xiphos.module-transfer",isDirectory:true).path
        let opened = NativeLibraryWindow(worker:controller.worker,path:(try? NativeSessionController.profileFolder().appendingPathComponent("repositories.profile").path) ?? controller.path + ".repositories",workspace:workspace,allowsChanges:managedRoot != nil,organizationPath:controller.organizationPath)
        opened.model.onOrganizationChanged = { [weak controller] value in controller?.applyLibraryOrganization(value) }
        opened.model.onModulesChanged = { [weak controller] in controller?.records.forEach { $0.model.refreshModuleLibrary() } }
        library = opened
    }
    func newBookSection() {
        guard !terminating, let controller, controller.canOpenWindow else { return }
        guard managedRoot != nil else { controller.showError("Choose a module library folder and restart before creating personal book sections."); return }
        if let sectionCreator { sectionCreator.window.makeKeyAndOrderFront(nil); return }
        sectionCreator = NativeBookSectionWindow(worker:controller.worker) { [weak self,weak controller] result in
            self?.sectionCreator = nil
            controller?.refreshAfterModuleEdit()
            self?.openEditor(.module(result.module,result.reference))
        }
    }
    func newEditorDocument() { openEditor(.newDocument) }
    func openEditorDocument() {
        guard !terminating else { return }; let panel = NSOpenPanel(); panel.canChooseDirectories = false; panel.allowsMultipleSelection = false; panel.title = "Open Study Document"
        if panel.runModal() == .OK, let url = panel.url { openEditor(.file(url.path)) }
    }
    func editCurrentModule() {
        guard managedRoot != nil else { let alert = NSAlert();alert.messageText = "Choose a module library folder first";alert.informativeText = "Use File → Choose Module Library Folder, then restart to edit module entries.";alert.runModal();return }
        guard let controller, let model = controller.records.first(where: { $0.key == controller.activeKey })?.model,
              let target = model.moduleEditingTarget else {
            let alert = NSAlert(); alert.messageText = "Select personal commentary or a general book first"; alert.informativeText = "Open the entry you want to edit in a reading window."; alert.runModal(); return
        }
        openEditor(.module(target.0,target.1))
    }
    func openNewWindow() { guard !terminating, let controller, controller.canOpenWindow else { return }; controller.addWindow() }
    func saveSession() {
        guard !terminating, let controller else { return }
        if let editor = editors.first(where: { $0.window.isKeyWindow }) { editor.save(); return }
        Task { do { try await controller.save() } catch { controller.showError(error.localizedDescription) } }
    }
    func saveSessionAs() {
        guard !terminating, let controller else { return }
        if let editor = editors.first(where: { $0.window.isKeyWindow }) { editor.save(asNew:true); return }
        let panel = NSSavePanel(); panel.title = "Save Reading Session"; panel.nameFieldStringValue = "session.xml"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            Task { do { try await controller.save(to: url.path); UserDefaults.standard.set(url.path, forKey: NativeSessionController.sessionPreferenceKey()) } catch { controller.showError(error.localizedDescription) } }
        }
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { false }
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        guard !terminating else { return .terminateLater }
        if migration?.model.busy == true { let alert = NSAlert();alert.messageText = "A migration operation is still running";alert.informativeText = "Wait for it to finish, then quit.";alert.runModal();return .terminateCancel }
        if sync?.busy == true { let alert = NSAlert();alert.messageText = "A sync connection change is still running";alert.informativeText = "Wait for it to finish, then quit.";alert.runModal();return .terminateCancel }
        if library?.model.isBusy == true { let alert = NSAlert();alert.messageText = "A module operation is still running";alert.informativeText = "Wait for it to finish, or stop the transfer in Module Library, then quit.";alert.runModal();return .terminateCancel }
        if sectionCreator?.model.busy == true { let alert = NSAlert(); alert.messageText = "A section operation is still running"; alert.informativeText = "Wait for it to finish, then quit."; alert.runModal(); return .terminateCancel }
        guard let controller else { return .terminateNow }
        terminating = true
        Task {
            do {
                for editor in editors {
                    guard await editor.confirmClose() else { for item in editors { item.model.frozen = false }; terminating = false; sender.reply(toApplicationShouldTerminate:false); return }
                    editor.model.frozen = true
                }
                try await controller.prepareToQuit(); try await recovery?.clear(); await sync?.stop(); sender.reply(toApplicationShouldTerminate: true) }
            catch {
                let alert = NSAlert(); alert.messageText = "Quit without saving this session?"
                alert.informativeText = error.localizedDescription
                alert.addButton(withTitle: "Cancel"); alert.addButton(withTitle: "Quit Without Saving")
                let quit = alert.runModal() == .alertSecondButtonReturn
                if quit { await sync?.stop() };terminating = quit; if !quit { controller.cancelQuit();recovery?.start();for item in editors { item.model.frozen = false } }; sender.reply(toApplicationShouldTerminate: quit)
            }
        }
        return .terminateLater
    }
}
