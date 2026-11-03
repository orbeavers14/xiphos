// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import SwiftUI
import AppKit

@MainActor final class BookmarkManagementModel: ObservableObject {
    let worker: CoreWorker
    private var tab: UInt64?
    @Published private(set) var path = ""
    @Published private(set) var profile: CoreBookmarkProfile?
    @Published private(set) var preview: CoreAndBiblePreview?
    @Published private(set) var source = ""
    @Published private(set) var busy = false
    @Published var folder = "Imported from AndBible"
    @Published var message = "Choose an existing bookmark file or a new destination."
    init(worker: CoreWorker) { self.worker=worker }
    private func ownedTab() async throws -> UInt64 {
        if let tab { return tab };let created=try await worker.createTab();tab=created;return created
    }
    func select(_ selected: String, new: Bool) async {
        guard !busy else { return };busy=true;defer { busy=false }
        do {
            let snapshot: CoreBookmarkProfile?
            if new { snapshot=nil } else { snapshot=try await worker.readBookmarkProfile(tab:ownedTab(),path:selected) }
            path=selected;profile=snapshot;message=new ? "New file will be created when you import." : "Bookmark file loaded. Unknown XML is preserved."
        } catch { message="Unable to open bookmark file: \(error.localizedDescription)" }
    }
    func review(_ selected: String) async {
        guard !busy else { return };busy=true;defer { busy=false }
        do { let value=try await worker.previewAndBible(path:selected);preview=value;source=selected;message="Backup reviewed. Import adds a new folder and preserves the original backup." }
        catch { preview=nil;source="";message="Unable to review backup: \(error.localizedDescription)" }
    }
    func importReviewed() async {
        guard !busy,!path.isEmpty,!folder.isEmpty,let preview else { return };busy=true;defer { busy=false }
        do {
            profile=try await worker.appendBookmarkImport(path:path,revision:profile?.revision,preview:preview,folder:folder)
            self.preview=nil;source="";message="Import saved."
        } catch { message="Import was not saved. The file may have changed or contain these bookmarks already. Reload the destination and review before retrying. \(error.localizedDescription)" }
    }
    func export(to destination: String) async {
        guard !busy,let profile else { return };busy=true;defer { busy=false }
        do {
            let expected: String?
            if FileManager.default.fileExists(atPath:destination) { expected=try await worker.readBookmarkProfile(tab:ownedTab(),path:destination).revision } else { expected=nil }
            try await worker.exportBookmarkProfile(source:path,revision:profile.revision,destination:destination,destinationRevision:expected)
            message="Exact bookmark XML exported."
        } catch { message="Export was not saved: \(error.localizedDescription)" }
    }
    func close() async { if let tab { self.tab=nil;try? await worker.closeTab(tab) } }
}

struct BookmarkManagementView: View {
    @ObservedObject var model: BookmarkManagementModel
    private func open(_ title: String, action: @escaping (String) async -> Void) {
        let panel=NSOpenPanel();panel.title=title;panel.canChooseDirectories=false;panel.allowsMultipleSelection=false
        guard panel.runModal() == .OK,let url=panel.url else { return };Task { await action(url.path) }
    }
    private func save(_ title: String, action: @escaping (String) async -> Void) {
        let panel=NSSavePanel();panel.title=title;panel.nameFieldStringValue="bookmarks.xml"
        guard panel.runModal() == .OK,let url=panel.url else { return };Task { await action(url.path) }
    }
    var body: some View {
        VStack(alignment:.leading,spacing:12) {
            HStack {
                Button("Open Bookmarks…") { open("Open bookmark XML") { await model.select($0,new:false) } }
                Button("New Bookmark File…") { save("Choose a new bookmark file") { await model.select($0,new:true) } }
                Button("Reload") { Task { await model.select(model.path,new:false) } }.disabled(model.path.isEmpty)
                if model.busy { ProgressView().controlSize(.small) }
            }
            Text(model.path.isEmpty ? "No destination selected" : model.path).font(.caption).textSelection(.enabled)
            HStack {
                Button("Review AndBible Backup…") { open("Choose a closed standalone AndBible SQLite backup",action:model.review) }
                Button("Export Bookmark XML…") { save("Export exact bookmark XML",action:model.export) }.disabled(model.profile == nil)
            }
            if let preview=model.preview {
                Text(URL(fileURLWithPath:model.source).lastPathComponent).bold()
                Text("Bookmarks: \(preview.imported) · Skipped: \(preview.skipped) · Unknown versifications: \(preview.unknownCanon) · Invalid ranges: \(preview.invalidOrdinal) · Invalid identities: \(preview.invalidIdentity)")
                Text("Unsupported versifications, invalid ranges and invalid identities are skipped. The original backup is preserved. Duplicate identities and changed destinations stop the import.").font(.caption)
                HStack { TextField("New folder",text:$model.folder);Button("Import Reviewed Backup") { Task { await model.importReviewed() } }.disabled(model.path.isEmpty || model.folder.isEmpty) }
            }
            List(model.profile?.rows ?? []) { row in
                VStack(alignment:.leading) {
                    Text((row.folder ? "Folder: " : "")+row.title)
                    if !row.folder { Text([row.fields[2],row.fields[3],row.fields[7]].compactMap { $0 }.joined(separator:" · ")).font(.caption) }
                }.padding(.leading,CGFloat(min(row.depth,16))*16)
            }
            Text(model.message).textSelection(.enabled).fixedSize(horizontal:false,vertical:true)
        }.padding(20).disabled(model.busy).frame(minWidth:760,minHeight:560)
    }
}
@MainActor final class NativeBookmarkWindow {
    let model: BookmarkManagementModel,window: NSWindow
    init(worker: CoreWorker) {
        model=BookmarkManagementModel(worker:worker)
        window=NSWindow(contentRect:NSRect(x:0,y:0,width:900,height:650),styleMask:[.titled,.closable,.miniaturizable,.resizable],backing:.buffered,defer:false)
        window.title="Bookmarks and Imports";window.isReleasedWhenClosed=false;window.contentView=NSHostingView(rootView:BookmarkManagementView(model:model));window.center();window.makeKeyAndOrderFront(nil)
    }
}
