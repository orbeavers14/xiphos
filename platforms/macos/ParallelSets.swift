// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
import SwiftUI
import Combine

@MainActor final class NativeParallelSetsModel: ObservableObject {
    @Published private(set) var profile = CoreParallelProfile.empty
    @Published private(set) var modules: [InstalledModule] = []
    @Published private(set) var busy = false
    @Published private(set) var loaded = false
    @Published private(set) var message = ""
    @Published var selected = ""
    @Published var name = ""
    @Published var orderedModules: [String] = []
    @Published var moduleToAdd = ""
    let path: String
    private let worker: CoreWorker
    private var tab: UInt64?
    init(worker: CoreWorker,path: String) { self.worker=worker;self.path=path }
    deinit { if let tab { let worker=worker;Task { try? await worker.closeTab(tab) } } }
    func reload() async {
        guard !busy else { return };busy=true;defer { busy=false }
        do {
            if tab == nil { tab = try await worker.createTab() }
            modules = try await worker.modules().filter { $0.hasVerseSearch }
            moduleToAdd = modules.first?.id ?? ""
            profile = FileManager.default.fileExists(atPath:path) ? try await worker.parallelProfile(tab:tab!,path:path) : .empty
            loaded=true;message="";select(selected)
        } catch { loaded=false;message="Sets could not be loaded. Reload before editing. \(error.localizedDescription)" }
    }
    func select(_ key: String) {
        selected=key
        if let row=profile.sets.first(where: { $0.key.utf8.elementsEqual(key.utf8) }) { name=row.name;orderedModules=row.modules }
        else { selected="";name="";orderedModules=[] }
    }
    func newSet() { selected="";name="";orderedModules=[];message="" }
    func addModule() { guard !moduleToAdd.isEmpty,orderedModules.count<32 else { return };orderedModules.append(moduleToAdd) }
    func move(_ index: Int,by delta: Int) { let target=index+delta;guard orderedModules.indices.contains(index),orderedModules.indices.contains(target) else { return };orderedModules.swapAt(index,target) }
    func removeModule(_ index: Int) { guard orderedModules.indices.contains(index) else { return };orderedModules.remove(at:index) }
    func edit(action: Int32) async {
        guard loaded,!busy,let tab else { return };busy=true;defer { busy=false }
        do {
            let create=action==4, rename=action==1, save=action==0
            let key = create ? try await worker.parallelSetKey(name:name) : selected
            guard create || (!key.isEmpty && profile.sets.contains(where: { $0.key.utf8.elementsEqual(key.utf8) })) else { return }
            let replacement = rename ? try await worker.parallelSetKey(name:name) : nil
            profile=try await worker.editParallelProfile(tab:tab,path:path,revision:profile.revision,action:action,key:key,replacement:replacement,modules:(save || create) ? orderedModules : [])
            selected=action==2 ? "" : (replacement ?? key);select(selected);message="Saved."
        } catch { message="The change was not saved. The name may already exist or the file may have changed. Reload before retrying. \(error.localizedDescription)" }
    }
}
private struct NativeParallelSetsPane: View {
    @ObservedObject var model: NativeParallelSetsModel
    var body: some View {
        VStack(alignment:.leading,spacing:10) {
            Text("Saved Parallel Reading Sets").font(.headline)
            Text("The first module controls chapter references. Module order and repeated selections are preserved.").font(.caption)
            HStack {
                Picker("Saved set",selection:Binding(get:{model.profile.sets.firstIndex { $0.key.utf8.elementsEqual(model.selected.utf8) } ?? -1},set:{ model.select(model.profile.sets.indices.contains($0) ? model.profile.sets[$0].key : "") })) {
                    Text("New set").tag(-1)
                    ForEach(Array(model.profile.sets.enumerated()),id:\.offset) { index,row in Text(row.name + (model.profile.current?.utf8.elementsEqual(row.key.utf8) == true ? " (default)" : "")).tag(index) }
                }
                Button("New") { model.newSet() };Button("Reload") { Task { await model.reload() } }
            }
            TextField("Set name",text:$model.name)
            ScrollView {
                VStack(alignment:.leading) {
                    ForEach(Array(model.orderedModules.enumerated()),id:\.offset) { index,module in
                        HStack { Text("\(index+1). \(module.isEmpty ? "Empty saved slot" : module)");Spacer();Button("Up") { model.move(index,by:-1) }.disabled(index==0);Button("Down") { model.move(index,by:1) }.disabled(index+1==model.orderedModules.count);Button("Remove") { model.removeModule(index) } }
                    }
                }
            }.frame(minHeight:150)
            HStack { Picker("Module",selection:$model.moduleToAdd) { ForEach(model.modules) { Text($0.id).tag($0.id) } };Button("Add") { model.addModule() }.disabled(model.moduleToAdd.isEmpty || model.orderedModules.count>=32) }
            HStack {
                Button("Create New Set") { Task { await model.edit(action:4) } }.disabled(model.name.isEmpty || model.orderedModules.isEmpty)
                Button("Save Selected") { Task { await model.edit(action:0) } }.disabled(model.selected.isEmpty)
                Button("Rename Selected") { Task { await model.edit(action:1) } }.disabled(model.selected.isEmpty || model.name.isEmpty)
                Button("Make Default") { Task { await model.edit(action:3) } }.disabled(model.selected.isEmpty)
                Button("Delete Set") {
                    let alert=NSAlert();alert.messageText="Delete this saved parallel set?";alert.informativeText="This removes the saved set, not its installed modules.";alert.addButton(withTitle:"Delete Set");alert.addButton(withTitle:"Cancel")
                    if alert.runModal() == .alertFirstButtonReturn { Task { await model.edit(action:2) } }
                }.disabled(model.selected.isEmpty)
            }.disabled(!model.loaded)
            Text("These controls manage saved sets. Existing reading windows keep their current content.").font(.caption).foregroundStyle(.secondary)
            Text(model.message).font(.caption).textSelection(.enabled)
        }.padding().disabled(model.busy).task { await model.reload() }
    }
}
@MainActor final class NativeParallelSetsWindow: NSObject, NSWindowDelegate {
    let model: NativeParallelSetsModel
    let window: NSWindow
    init(worker: CoreWorker,path: String) {
        model=NativeParallelSetsModel(worker:worker,path:path)
        window=NSWindow(contentRect:NSRect(x:0,y:0,width:790,height:520),styleMask:[.titled,.closable,.resizable],backing:.buffered,defer:false)
        super.init();window.title="Saved Parallel Reading Sets";window.isReleasedWhenClosed=false;window.delegate=self
        window.contentView=NSHostingView(rootView:NativeParallelSetsPane(model:model));window.center();window.makeKeyAndOrderFront(nil)
    }
    func windowShouldClose(_ sender: NSWindow) -> Bool { !model.busy }
}
