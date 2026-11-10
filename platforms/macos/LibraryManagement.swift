// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
import SwiftUI
import UniformTypeIdentifiers
import XiphosCore

@MainActor final class LibraryManagementModel: ObservableObject {
    let worker: CoreWorker
    let path: String
    let workspace: String
    let allowsChanges: Bool
    @Published private(set) var profile: CoreRepositoryProfile?
    @Published private(set) var installed: [CoreLibraryChoice] = []
    @Published private(set) var remote: [CoreLibraryChoice] = []
    @Published private(set) var local: [CoreLibraryChoice] = []
    @Published private(set) var offer: CoreLocalInstallOffer?
    @Published private(set) var maintenance: CoreMaintenanceOffer?
    @Published private(set) var isBusy = false
    @Published private(set) var transferring = false
    @Published private(set) var progress: CoreTransferProgress?
    @Published var pane = 0
    @Published private(set) var legacy: [CoreLegacyRepositoryCandidate] = []
    @Published var legacyID: Int?
    @Published private(set) var legacyPath = ""
    @Published var message = ""
    @Published var sourceID: String?
    @Published var remoteID: String?
    @Published var installedID: String?
    @Published var localID: String?
    @Published var localPath = ""
    @Published var name = ""
    @Published var host = ""
    @Published var catalog = "/"
    @Published var packages = "/"
    @Published var port = "443"
    @Published var enabled = true
    @Published var cipherKey = ""
    private var editingID: String?
    private var operation: Task<Void,Never>?
    let organizationPath: String?
    @Published private(set) var organization = CoreLibraryOrganization.empty
    @Published var favorite = false
    @Published var hidden = false
    @Published var organizationGroup = ""
    var onOrganizationChanged: ((CoreLibraryOrganization) -> Void)?
    var onModulesChanged: (() -> Void)?
    init(worker: CoreWorker,path: String,workspace: String,allowsChanges: Bool = true,organizationPath: String? = nil) { self.organizationPath=organizationPath;self.worker = worker;self.path = path;self.workspace = workspace;self.allowsChanges = allowsChanges }
    var sourceIndex: Int? { profile?.rows.firstIndex { $0.id == sourceID } }
    var hasUnsavedSourceChanges: Bool {
        guard let index = sourceIndex, let row = profile?.rows[index] else { return true }
        return editingID != row.id || name != row.name || host != row.endpoint.host || catalog != row.endpoint.catalog || packages != row.endpoint.packages || UInt32(port) != row.endpoint.port || enabled != row.enabled
    }
    func selectLocal(_ id: String?) { localID = id;offer = nil }
    func selectInstalled(_ id: String?) {
        installedID=id;maintenance=nil;cipherKey=""
        let selected=installed.first { $0.id == id };let value=selected.flatMap { organization.preference(provider:$0.provider,item:$0.module) }
        favorite=value?.favorite ?? false;hidden=value?.hidden ?? false;organizationGroup=value?.group ?? ""
    }
    func saveOrganization() {
        guard let path=organizationPath,let selected=installed.first(where: { $0.id == installedID }) else { return }
        let preference=CoreLibraryPreference(identity:CoreLibraryIdentity(provider:selected.provider,item:selected.module),favorite:favorite,hidden:hidden,group:organizationGroup)
        run(failureHint:"Library preferences could not be saved. Reload before retrying.") {
            let value=try await self.worker.setLibraryOrganization(path:path,revision:self.organization.revision,preference:preference)
            self.organization=value;self.onOrganizationChanged?(value);self.message="Library preferences saved. Hiding affects the module list; installed files are unchanged."
        }
    }
    var selectedRemote: CoreLibraryChoice? { remote.first { $0.id == remoteID } }
    func run(transferring: Bool = false,failureHint: String = "Operation did not complete. Reload the library or source and try again.",_ action: @escaping @MainActor () async throws -> Void) {
        guard !isBusy else { return };isBusy = true;self.transferring = transferring;message = "Working…";progress = nil
        operation = Task {
            defer { self.isBusy = false;self.transferring = false;self.operation = nil }
            do { try await action() }
            catch { self.message = (error as? CoreError)?.status == XC_CANCELLED ? "Operation cancelled before applying changes." : failureHint + " " + error.localizedDescription }
        }
    }
    func waitUntilIdle() async { await operation?.value }
    func cancel() { if transferring { operation?.cancel();message = "Cancellation requested. Waiting for the current operation to finish safely." } }
    func reload() {
        run { self.installed = try await self.worker.libraryInventory();if let path=self.organizationPath { self.organization=try await self.worker.libraryOrganization(path:path);self.onOrganizationChanged?(self.organization);self.selectInstalled(self.installedID) };self.profile = try await self.worker.readRepositories(path:self.path);self.remote = [];self.offer = nil;self.maintenance = nil;self.message = "Library and sources loaded.";if let id = self.sourceID, self.profile?.rows.contains(where: { $0.id == id }) != true { self.sourceID = nil } }
    }
    func selectSource(_ id: String?) {
        guard !isBusy else { return };sourceID = id;remoteID = nil;remote = [];offer = nil
        guard let index = sourceIndex, let row = profile?.rows[index] else { return }
        editingID = row.id;name = row.name;host = row.endpoint.host;catalog = row.endpoint.catalog;packages = row.endpoint.packages;port = String(row.endpoint.port);enabled = row.enabled
        run { guard let profile = self.profile else { return };self.remote = try await self.worker.repositoryChoices(profile:profile,index:index);self.message = row.cached ? "Cached catalog. Refresh before installing if the library changed." : "Refresh this source to browse its catalog." }
    }
    func selectRemote(_ id: String?) {
        guard !isBusy else { return };remoteID = id
        guard let id, let profile, let index = sourceIndex else { return }
        run { self.remote = try await self.worker.repositoryChoices(profile:profile,index:index,planning:id);self.message = "Review module details and the installation choice." }
    }
    func newSource() { guard !isBusy else { return };sourceID = nil;editingID = nil;name = "";host = "";catalog = "/";packages = "/";port = "443";enabled = true;remote = [];remoteID = nil }
    func saveSource() {
        guard let profile else { return }
        let id = editingID ?? UUID().uuidString.lowercased()
        guard let number = UInt32(port), number > 0, number <= 65535 else { message = "Enter a port between 1 and 65535.";return }
        let endpoint = CoreRepositoryEndpoint(id:id,host:host,catalog:catalog,packages:packages,port:number)
        run { self.profile = try await self.worker.editRepository(path:self.path,profile:profile,endpoint:endpoint,name:self.name,enabled:self.enabled);self.editingID = id;self.sourceID = id;self.remoteID = nil;self.remote = try await self.worker.repositoryChoices(profile:self.profile!,index:self.sourceIndex!);self.message = "Source saved." }
    }
    func previewLegacy(_ path: String) {
        run { self.legacy = try await self.worker.previewLegacyRepositories(path:path);self.legacyPath = path;self.legacyID = nil;self.message = "Read-only preview. Select a supported HTTPS source to review its fields." }
    }
    func reviewLegacy() {
        guard !isBusy,let row = legacy.first(where: { $0.id == legacyID }),row.disposition == 0 else { return }
        newSource();name = row.name;host = row.host;catalog = row.catalog;packages = "";port = "443";enabled = false;pane = 0
        message = "Enter the verified HTTPS package folder, then Save Source. The source starts disabled; the legacy file is unchanged."
    }
    func addCrossWire() {
        guard let profile, !profile.rows.contains(where: { $0.id == "crosswire" }) else { return }
        run { self.profile = try await self.worker.editRepository(path:self.path,profile:profile,endpoint:CoreRepositoryEndpoint(id:"crosswire",host:"www.crosswire.org",catalog:"/ftpmirror/pub/sword/raw",packages:"/ftpmirror/pub/sword/packages/rawzip"),name:"CrossWire");self.message = "CrossWire source added. Refresh to connect and browse." }
    }
    func removeSource() { guard let profile, let sourceID else { return };run { self.profile = try await self.worker.editRepository(path:self.path,profile:profile,remove:sourceID);self.sourceID = nil;self.editingID = nil;self.remote = [];self.message = "Source removed." } }
    func moveSource(_ delta: Int) { guard let profile, let sourceID, let index = sourceIndex, profile.rows.indices.contains(index + delta) else { return };run { self.profile = try await self.worker.editRepository(path:self.path,profile:profile,move:(sourceID,index + delta));self.message = "Source order saved." } }
    private func transferProgress(_ value: CoreTransferProgress) { progress = value }
    func refreshSource() {
        guard allowsChanges, let profile, let index = sourceIndex, profile.rows[index].enabled, !hasUnsavedSourceChanges else { return }
        run(transferring:true) {
            try FileManager.default.createDirectory(atPath:self.workspace,withIntermediateDirectories:true)
            self.profile = try await self.worker.refreshRepository(path:self.path,profile:profile,index:index,workspace:self.workspace,progress:{ [weak self = self] value in await self?.transferProgress(value) })
            self.remote = try await self.worker.repositoryChoices(profile:self.profile!,index:index,planning:self.remoteID);self.message = "Catalog refreshed and saved for offline browsing."
        }
    }
    func installRemote(replace: Bool) {
        guard allowsChanges, let profile, let index = sourceIndex, let item = selectedRemote, !hasUnsavedSourceChanges else { return }
        run(transferring:true) {
            try FileManager.default.createDirectory(atPath:self.workspace,withIntermediateDirectories:true)
            let outcome = try await self.worker.installRemote(path:self.path,profile:profile,index:index,module:item.module,replace:replace,workspace:self.workspace,progress:{ [weak self = self] value in await self?.transferProgress(value) })
            if let updated = outcome.profile { self.profile = updated }
            self.onModulesChanged?();self.remoteID = nil;self.remote = []
            if let fresh = try? await self.worker.libraryInventory() { self.installed = fresh }
            if let updated = outcome.profile { self.remote = (try? await self.worker.repositoryChoices(profile:updated,index:index)) ?? [] }
            self.message = outcome.message
        }
    }
    func loadLocal(_ path: String) { localPath = path;local = [];localID = nil;offer = nil;run { self.local = try await self.worker.libraryInventory(source:path);self.message = "Choose a module and review installation." } }
    func prepareLocal() { guard allowsChanges, let item = local.first(where: { $0.id == localID }) else { return };run { self.offer = try await self.worker.prepareLocalInstall(source:self.localPath,module:item.module);self.message = "Review the module and version before installation." } }
    func installLocal() { guard allowsChanges, let offer else { return };run(transferring:true) { try await self.worker.installLocal(offer,replace:offer.decision == 1 || offer.decision == 2,progress:{ [weak self] value in await self?.transferProgress(value) });self.offer = nil;if let fresh = try? await self.worker.libraryInventory() { self.installed = fresh };self.message = "Local module installed.";self.onModulesChanged?() } }
    func archiveSelectedModule() {
        guard let item = installed.first(where: { $0.id == installedID }) else { return }
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.zip]
        panel.nameFieldStringValue = item.module + ".zip"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            self.run(failureHint: "Choose a new ZIP outside the module library.") {
                try await self.worker.archiveModule(item.module, destination: url.path)
                self.message = "Module archived. Unzip the archive and use local installation to restore it."
            }
        }
    }
    func inspectModule() { guard allowsChanges, let item = installed.first(where: { $0.id == installedID }) else { return };cipherKey = "";maintenance = nil;run { self.maintenance = try await self.worker.maintenanceOffer(module:item.module);self.message = "Module details loaded." } }
    func maintain(_ action: Int32) { guard allowsChanges, let maintenance else { return };let key = action == 1 ? cipherKey : "";cipherKey = "";run(transferring:true,failureHint:action == 0 ? "Removal was refused. Close reading and editing windows using this module, including their history, then reload and retry." : "Maintenance did not complete. The engine may not support this operation. Reload the module list before retrying.") { try await self.worker.maintainModule(maintenance,action:action,key:key,progress:{ [weak self] value in await self?.transferProgress(value) });self.maintenance = nil;if let fresh = try? await self.worker.libraryInventory() { self.installed = fresh };self.message = action == 1 ? "Cipher configuration saved. Check the module to confirm the key is correct." : "Module maintenance completed.";self.onModulesChanged?() } }
    var phaseLabel: String {
        switch progress?.phase {case 1: "Downloading";case 2: "Extracting";case 3: "Verifying";case 4: "Applying changes";case 5: "Complete";case 6: "Cancelled";default: "Preparing"}
    }
}

struct LibraryManagementPane: View {
    @ObservedObject var model: LibraryManagementModel
    private func confirm(_ title: String,_ detail: String) -> Bool { let alert = NSAlert();alert.messageText = title;alert.informativeText = detail;alert.addButton(withTitle:"Continue");alert.addButton(withTitle:"Cancel");return alert.runModal() == .alertFirstButtonReturn }
    var body: some View {
        VStack(alignment:.leading,spacing:12) {
            HStack { Text("Module Library").font(.title2);Spacer();Button("Reload") { model.reload() }.disabled(model.isBusy) }
            if !model.allowsChanges { Text("To install or maintain modules, choose a module library folder from File and restart.").font(.callout) }
            TabView(selection:$model.pane) {
                sources.tabItem { Text("Sources & Downloads") }.tag(0)
                local.tabItem { Text("Install from Folder") }.tag(1)
                installed.tabItem { Text("Installed Modules") }.tag(2)
                legacy.tabItem { Text("Import Legacy Sources") }.tag(3)
            }.disabled(model.isBusy)
            HStack {
                if model.isBusy { ProgressView().controlSize(.small);Text(model.phaseLabel) }
                if let progress = model.progress, progress.total > 0 { ProgressView(value:Double(progress.downloaded),total:Double(progress.total)).frame(maxWidth:220) }
                Spacer();if model.transferring { Button("Cancel Operation") { model.cancel() }.disabled((model.progress?.phase ?? 0) >= 4) }
            }
            Text(model.message).font(.callout).textSelection(.enabled).frame(maxWidth:.infinity,alignment:.leading)
        }.padding(18).frame(minWidth:920,minHeight:690).background(Color(nsColor:.windowBackgroundColor))
    }
    private var sources: some View {
        HStack(alignment:.top,spacing:16) {
            VStack {
                List(selection:Binding(get:{ model.sourceID },set:{ model.selectSource($0) })) {
                    ForEach(model.profile?.rows ?? []) { row in Text(row.name + (row.enabled ? "" : " (disabled)")).tag(row.id) }
                }.frame(width:210)
                HStack { Button("New") { model.newSource() };Button("Remove") { model.removeSource() }.disabled(model.sourceID == nil) }
                HStack { Button("Move Up") { model.moveSource(-1) };Button("Move Down") { model.moveSource(1) } }.disabled(model.sourceID == nil)
                Button("Add CrossWire") { model.addCrossWire() }.disabled(model.profile?.rows.contains(where: { $0.id == "crosswire" }) == true)
            }
            VStack(alignment:.leading,spacing:8) {
                Grid(alignment:.leading) {
                    GridRow { Text("Name");TextField("Repository name",text:$model.name) }
                    GridRow { Text("HTTPS server");TextField("example.org",text:$model.host) }
                    GridRow { Text("Port");TextField("443",text:$model.port) }
                    GridRow { Text("Catalog folder");TextField("/path/to/raw",text:$model.catalog) }
                    GridRow { Text("Package folder");TextField("/path/to/rawzip",text:$model.packages) }
                }
                HStack { Toggle("Enabled",isOn:$model.enabled);Spacer();Button("Save Source") { model.saveSource() }.disabled(model.profile == nil || model.name.isEmpty || model.host.isEmpty || model.packages.isEmpty);Button("Refresh Catalog…") { model.refreshSource() }.disabled(!model.allowsChanges || model.sourceIndex == nil || model.profile?.rows[model.sourceIndex ?? 0].enabled != true || model.hasUnsavedSourceChanges) }
                Text("Refresh connects to the selected server. Saved catalogs are available offline.").font(.caption)
                List(selection:Binding(get:{ model.remoteID },set:{ model.selectRemote($0) })) { ForEach(model.remote) { item in HStack { VStack(alignment:.leading) { Text(item.title.isEmpty ? item.module : item.title);Text([item.module,item.version,item.language].filter { !$0.isEmpty }.joined(separator:" · ")).font(.caption).foregroundStyle(.secondary) };Spacer();Text(item.decision == 2 ? "Installed" : item.decision == 1 ? "Replacement" : "") }.tag(item.id) } }
                if let selected = model.selectedRemote, !selected.license.isEmpty || !selected.attribution.isEmpty {
                    DisclosureGroup("Module details and license") { ScrollView { Text(selected.attribution + "\n" + selected.license).font(.caption).textSelection(.enabled).frame(maxWidth:.infinity,alignment:.leading) }.frame(maxHeight:90) }
                }
                Button(model.selectedRemote.map { [1,2].contains($0.decision) } == true ? "Download & Replace…" : "Download & Install…") {
                    guard let item = model.selectedRemote else { return }
                    if confirm("Install \(item.module)?","Version: \(item.version.isEmpty ? "unspecified" : item.version). This downloads the package and changes the installed module library.") { model.installRemote(replace:item.decision != 0) }
                }.disabled(!model.allowsChanges || model.hasUnsavedSourceChanges || model.sourceIndex.flatMap { model.profile?.rows[$0].enabled } != true || (model.selectedRemote.map { ![0,1,2].contains($0.decision) } ?? true))
            }
        }.padding(.top,10)
    }
    private var legacy: some View {
        VStack(alignment:.leading,spacing:10) {
            HStack { Text(model.legacyPath.isEmpty ? "Choose the old InstallMgr.conf file." : model.legacyPath).lineLimit(2);Spacer();Button("Choose File…") { let panel = NSOpenPanel();panel.canChooseDirectories = false;panel.allowsMultipleSelection = false;panel.showsHiddenFiles = true;panel.title = "Preview Legacy Module Sources";if panel.runModal() == .OK,let url = panel.url { model.previewLegacy(url.path) } } }
            Text("This preview does not change the old file or contact servers. Credentials are excluded. HTTP/FTP sources are not automatically converted to HTTPS.").font(.callout)
            List(selection:$model.legacyID) { ForEach(model.legacy) { row in Text("Line \(row.line): \(row.summary)").tag(row.id) } }
            Button("Review Selected HTTPS Source") { model.reviewLegacy() }.disabled(model.legacy.first(where: { $0.id == model.legacyID })?.disposition != 0)
            Text("Review copies fields into a new disabled source. You must provide the verified package folder and save explicitly.").font(.caption)
        }.padding(.top,10)
    }
    private var local: some View {
        VStack(alignment:.leading) {
            HStack { Text(model.localPath.isEmpty ? "Choose a SWORD module folder containing mods.d." : model.localPath).lineLimit(2);Spacer();Button("Choose Folder…") { let panel = NSOpenPanel();panel.canChooseDirectories = true;panel.canChooseFiles = false;panel.allowsMultipleSelection = false;if panel.runModal() == .OK, let url = panel.url { model.loadLocal(url.path) } } }
            List(selection:Binding(get:{ model.localID },set:{ model.selectLocal($0) })) { ForEach(model.local) { item in Text(item.module + (item.version.isEmpty ? "" : " · " + item.version) + " — " + item.title).tag(item.id) } }
            HStack { Button("Review Installation") { model.prepareLocal() }.disabled(!model.allowsChanges || model.localID == nil)
                if let offer = model.offer { Text("\(offer.module): source \(offer.version.isEmpty ? "version not supplied" : offer.version), installed \(offer.decision == 0 ? "not installed" : offer.installedVersion.isEmpty ? "version not supplied" : offer.installedVersion)");Spacer();Button(offer.decision == 0 ? "Install" : "Replace Installed Module…") { if offer.decision == 0 || confirm("Replace \(offer.module)?","The selected source will replace the installed module.") { model.installLocal() } }.disabled(![0,1,2].contains(offer.decision)) }
            }
        }.padding(.top,10)
    }
    private var installed: some View {
        VStack(alignment:.leading) {
            List(selection:Binding(get:{ model.installedID },set:{ model.selectInstalled($0) })) { ForEach(model.installed) { item in Text(item.module + (item.version.isEmpty ? "" : " · " + item.version) + " — " + item.title).tag(item.id) } }
            if model.organizationPath != nil,model.installedID != nil {
                HStack { Toggle("Favorite",isOn:$model.favorite);Toggle("Hide in module list",isOn:$model.hidden);TextField("Group (optional)",text:$model.organizationGroup);Button("Save Organization") { model.saveOrganization() } }.disabled(model.isBusy)
            }
            Button("Archive Selected Module…") { model.archiveSelectedModule() }.disabled(model.installedID == nil)
            Button("Manage Selected Module") { model.inspectModule() }.disabled(!model.allowsChanges || model.installedID == nil)
            if let offer = model.maintenance {
                Text(offer.module).font(.headline)
                HStack {
                    Button("Remove Module…") { if confirm("Remove \(offer.module)?","Close reading and editing windows using this module first. Its installed data will be removed.") { model.maintain(0) } }
                    Button("Build Search Index") { model.maintain(2) }
                    Button("Delete Search Index…") { if confirm("Delete the search index for \(offer.module)?","The module can still be read and searched without an index.") { model.maintain(3) } }.disabled(!offer.indexed)
                }
                if offer.cipher { HStack { SecureField("Module cipher key",text:$model.cipherKey);Button("Save Key…") { if confirm("Save cipher configuration for \(offer.module)?","The key is stored in this module’s SWORD configuration. Existing search indexes will be cleared.") { model.maintain(1) } } } }
            }
        }.padding(.top,10)
    }
}
@MainActor final class NativeLibraryWindow: NSObject,NSWindowDelegate {
    let model: LibraryManagementModel
    let window: NSWindow
    init(worker: CoreWorker,path: String,workspace: String,show: Bool = true,allowsChanges: Bool = true,organizationPath: String? = nil) {
        model = LibraryManagementModel(worker:worker,path:path,workspace:workspace,allowsChanges:allowsChanges,organizationPath:organizationPath)
        window = NSWindow(contentRect:NSRect(x:0,y:0,width:1040,height:760),styleMask:[.titled,.closable,.miniaturizable,.resizable],backing:.buffered,defer:false)
        super.init();window.title = "Module Library";window.minSize = NSSize(width:920,height:690);window.isReleasedWhenClosed = false;window.delegate = self;window.contentView = NSHostingView(rootView:LibraryManagementPane(model:model));window.center();if show { window.makeKeyAndOrderFront(nil) } else { window.orderOut(nil) };model.reload()
    }
    func windowShouldClose(_ sender: NSWindow) -> Bool { if model.isBusy { window.orderOut(nil);return false };return true }
}
