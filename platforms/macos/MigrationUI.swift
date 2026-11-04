// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import SwiftUI

@MainActor final class NativeMigrationModel: ObservableObject {
    let worker: CoreWorker
    @Published var assetFolder = ""
    @Published private(set) var assetPreview: CoreLegacyAssets?
    @Published private(set) var copiedAssetFolder: String?
    @Published var settings = ""
    @Published var tabs = ""
    @Published var options = ""
    @Published var package = ""
    @Published var mappingText = ""
    @Published var useOnRestart = false
    @Published private(set) var preview: CoreMigrationPreview?
    @Published private(set) var busy = false
    @Published private(set) var message = "Select the legacy files to preview. Original files are never changed."
    private var selection: [String] = []
    private var operation: Task<Void,Never>?
    var currentSessionPath: (() -> String?)?
    var onUseSession: ((String) async throws -> Void)?
    init(worker: CoreWorker) { self.worker = worker }
    var needsPreview: Bool { preview == nil || selection != [settings,tabs,options,package,mappingText] }
    private func mappings() throws -> [(String,String)] {
        try mappingText.split(separator:"\n").map { line in
            let pair = line.split(separator:"=",maxSplits:1,omittingEmptySubsequences:false)
            guard pair.count == 2 else { throw SessionHostError(message:"Use one old module ID = installed module ID per line.") }
            let from = pair[0].trimmingCharacters(in:.whitespaces), to = pair[1].trimmingCharacters(in:.whitespaces)
            guard !from.isEmpty,!to.isEmpty else { throw SessionHostError(message:"Both module IDs are required.") };return (from,to)
        }
    }
    func run(_ action: @escaping @MainActor () async throws -> Void) {
        guard !busy else { return };busy = true
        operation = Task {
            defer { busy = false;operation = nil }
            do { try await action() } catch { message = error is CoreError ? "Operation did not complete. Preview again if a source changed, and choose a new output filename or folder. " + error.localizedDescription:error.localizedDescription }
        }
    }
    func waitUntilIdle() async { await operation?.value }
    func prepare() {
        run {
            self.preview = nil
            let selected = [self.settings,self.tabs,self.options,self.package,self.mappingText]
            self.preview = try await self.worker.previewMigration(settings:self.settings,tabs:self.tabs,options:self.options,package:self.package,mappings:self.mappings())
            self.selection = selected
            self.message = self.preview?.ready == true ? "Conversion ready. Save a backup package, then export a new native session.":"Resolve the listed issues and preview again. You can still save a package containing the original files."
        }
    }
    private func protectsCurrentSession(_ path: String) -> Bool {
        guard let current = currentSessionPath?() else { return false }
        let destination = URL(fileURLWithPath:path).standardizedFileURL.resolvingSymlinksInPath()
        let active = URL(fileURLWithPath:current).standardizedFileURL.resolvingSymlinksInPath()
        guard destination == active else { return false }
        message = "Choose a different output name. This is the current reading-session destination."
        return true
    }
    func savePackage(path: String) {
        guard !needsPreview,let preview,!protectsCurrentSession(path) else { return }
        run { try await self.worker.saveMigration(preview,path:path);self.message = "Migration package saved with exact copies of the selected originals." }
    }
    func exportSession(path: String) {
        guard !needsPreview,let preview,preview.ready,!protectsCurrentSession(path) else { return }
        let use = useOnRestart
        run {
            try await self.worker.exportMigration(preview,path:path)
            self.message = "Native session exported. Current reading windows are unchanged."
            if use,let action = self.onUseSession {
                do { try await action(path);self.message = "Native session exported. Restart the app to use it; current work has been saved separately." }
                catch { self.message = "Native session was exported, but startup selection was not changed. " + error.localizedDescription }
            }
        }
    }
    var needsAssetReview: Bool { assetPreview?.source != URL(fileURLWithPath:assetFolder).standardizedFileURL.resolvingSymlinksInPath().path }
    func reviewAssets() {
        run { self.assetPreview = nil;self.copiedAssetFolder = nil;self.assetPreview = try await self.worker.inspectLegacyAssets(self.assetFolder);self.message = "Folder reviewed. Copy into a new folder to preserve all files, hidden files and empty directories." }
    }
    func copyAssets(to destination: String) {
        guard !needsAssetReview,let preview = assetPreview,!protectsCurrentSession(destination) else {return}
        run { let copied = try await self.worker.copyLegacyAssets(preview,to:destination);self.copiedAssetFolder = copied.source;self.message = "All reviewed assets copied and verified. Originals are unchanged. Open bookmarks or study pads from the new folder when ready." }
    }
    func restoreOriginals(folder: String) {
        guard !needsPreview,let preview,!protectsCurrentSession(folder) else { return }
        run { try await self.worker.restoreMigrationOriginals(preview,folder:folder);self.message = "Exact originals restored into the new folder. Existing legacy files were not changed." }
    }
}
struct NativeMigrationView: View {
    @ObservedObject var model: NativeMigrationModel
    private func choose(_ kind: Int) {
        let panel = NSOpenPanel();panel.canChooseDirectories = false;panel.allowsMultipleSelection = false;panel.showsHiddenFiles = true
        guard panel.runModal() == .OK,let path = panel.url?.path else { return }
        switch kind {
        case 0: model.settings = path;model.package = ""
        case 1: model.tabs = path;model.package = ""
        case 2: model.options = path;model.package = ""
        default: model.package = path
        }
    }
    private func destination(_ title: String,_ name: String,_ action: (String) -> Void) {
        let panel = NSSavePanel();panel.title = title;panel.nameFieldStringValue = name;panel.canCreateDirectories = true
        if panel.runModal() == .OK,let path = panel.url?.path { action(path) }
    }
    var body: some View {
        VStack(alignment:.leading,spacing:12) {
            Text("Import Legacy Settings and Session").font(.title2)
            Text("Select settings.xml and optional saved tabs/modops.conf. You can also reopen a migration package. Supported settings become a native session; other settings remain in the exact original backup.").fixedSize(horizontal:false,vertical:true)
            DisclosureGroup("Copy a complete legacy asset folder") {
                Text("Select the legacy Xiphos folder, or an external study-pad/module folder separately. Every regular file is retained, including bookmarks, notes, templates and hidden files. Select linked targets separately; symbolic links are refused. Copying does not activate modules or change saved absolute paths.").font(.caption).fixedSize(horizontal:false,vertical:true)
                HStack {
                    TextField("Legacy asset folder",text:$model.assetFolder)
                    Button("Choose Folder…") {let panel = NSOpenPanel();panel.canChooseDirectories = true;panel.canChooseFiles = false;panel.showsHiddenFiles = true;if panel.runModal() == .OK,let path = panel.url?.path {model.assetFolder = path}}
                    Button("Review Folder") {model.reviewAssets()}.disabled(model.assetFolder.isEmpty)
                }.disabled(model.busy)
                if let assets = model.assetPreview {
                    Text("\(assets.entries.filter { !$0.directory }.count) files · \(assets.entries.filter { $0.directory }.count) folders · \(ByteCountFormatter.string(fromByteCount:Int64(assets.bytes),countStyle:.file))")
                    ScrollView {VStack(alignment:.leading) {ForEach(assets.entries,id:\.path) {entry in Text(entry.path + (entry.directory ? "/":"")).font(.caption).textSelection(.enabled)}}.frame(maxWidth:.infinity,alignment:.leading)}.frame(maxHeight:100)
                    Button("Copy Verified Assets…") {destination("Copy into a New Folder","Migrated Legacy Assets") {model.copyAssets(to:$0)}}.disabled(model.busy || model.needsAssetReview)
                    if model.needsAssetReview {Text("Folder selection changed. Review again.").font(.caption)}
                }
            }
            Grid(alignment:.leading) {
                GridRow { Text("Settings");TextField("settings.xml",text:$model.settings);Button("Choose…") { choose(0) } }
                GridRow { Text("Saved tabs");TextField("Optional tabs file",text:$model.tabs);Button("Choose…") { choose(1) } }
                GridRow { Text("Module options");TextField("Optional modops.conf",text:$model.options);Button("Choose…") { choose(2) } }
                GridRow { Text("Saved package");TextField("Optional migration package",text:$model.package);Button("Open…") { choose(3) } }
            }.disabled(model.busy)
            DisclosureGroup("Map missing modules to installed module IDs") {
                Text("One mapping per line, for example: OldModule = InstalledModule. Modules are never substituted automatically.").font(.caption)
                TextEditor(text:$model.mappingText).font(.system(.body,design:.monospaced)).frame(height:70).disabled(model.busy)
            }
            HStack { Button("Preview Migration") { model.prepare() }.disabled(model.busy || (model.settings.isEmpty && model.package.isEmpty));if model.busy { ProgressView().controlSize(.small) } }
            if let preview = model.preview {
                Text("\(preview.sources) original files · \(preview.tabs) converted windows · \(preview.mappedCount) global preferences mapped · \(preview.settings.count - preview.mappedCount) additional entries backed up").font(.callout)
                if model.needsPreview { Text("Selections changed. Preview again before saving.").foregroundStyle(.secondary) }
                if !preview.issues.isEmpty { ScrollView { Text(preview.issues.joined(separator:"\n")).textSelection(.enabled).frame(maxWidth:.infinity,alignment:.leading) }.frame(maxHeight:100) }
                List(Array(preview.settings.enumerated()),id:\.offset) { _,row in Text(row).textSelection(.enabled) }
                Toggle("Use exported session on next launch",isOn:$model.useOnRestart).disabled(model.busy)
                HStack {
                    Button("Save Backup Package…") { destination("Save Migration Package","legacy-migration.xiphos-migration",model.savePackage) }
                    Button("Export Native Session…") { destination("Export Migrated Session","migrated-session.xml",model.exportSession) }.disabled(!preview.ready)
                    Button("Restore Original Copies…") { destination("Restore into a New Folder","Restored Legacy Settings",model.restoreOriginals) }
                }.disabled(model.busy || model.needsPreview)
            }
            Text(model.message).textSelection(.enabled).fixedSize(horizontal:false,vertical:true)
        }.padding(20).frame(minWidth:780,maxWidth:.infinity,minHeight:680,maxHeight:.infinity,alignment:.topLeading).background(Color(nsColor:.windowBackgroundColor))
    }
}
@MainActor final class NativeMigrationWindow {
    let model: NativeMigrationModel, window: NSWindow
    init(model: NativeMigrationModel) {
        self.model = model
        window = NSWindow(contentRect:NSRect(x:0,y:0,width:920,height:780),styleMask:[.titled,.closable,.miniaturizable,.resizable],backing:.buffered,defer:false)
        window.title = "Legacy Migration";window.minSize = NSSize(width:780,height:680);window.isReleasedWhenClosed = false;window.contentView = NSHostingView(rootView:NativeMigrationView(model:model));window.center();window.makeKeyAndOrderFront(nil)
    }
}
