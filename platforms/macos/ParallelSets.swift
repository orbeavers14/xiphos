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
    let apply: () -> Void
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
                Button("Read Selected",action:apply).disabled(model.selected.isEmpty)
                Button("Make Default") { Task { await model.edit(action:3) } }.disabled(model.selected.isEmpty)
                Button("Delete Set") {
                    let alert=NSAlert();alert.messageText="Delete this saved parallel set?";alert.informativeText="This removes the saved set, not its installed modules.";alert.addButton(withTitle:"Delete Set");alert.addButton(withTitle:"Cancel")
                    if alert.runModal() == .alertFirstButtonReturn { Task { await model.edit(action:2) } }
                }.disabled(model.selected.isEmpty)
            }.disabled(!model.loaded)
            Text("Read Selected opens this set in the active reading window. Make Default selects the set opened by Parallel Reading.").font(.caption).foregroundStyle(.secondary)
            Text(model.message).font(.caption).textSelection(.enabled)
        }.padding().disabled(model.busy).task { await model.reload() }
    }
}
@MainActor final class NativeParallelSetsWindow: NSObject, NSWindowDelegate {
    let model: NativeParallelSetsModel
    let window: NSWindow
    init(worker: CoreWorker,path: String,apply: @escaping (String,String) -> Void) {
        model=NativeParallelSetsModel(worker:worker,path:path)
        window=NSWindow(contentRect:NSRect(x:0,y:0,width:960,height:520),styleMask:[.titled,.closable,.resizable],backing:.buffered,defer:false)
        super.init();window.minSize=NSSize(width:900,height:480);window.title="Saved Parallel Reading Sets";window.isReleasedWhenClosed=false;window.delegate=self
        window.contentView=NSHostingView(rootView:NativeParallelSetsPane(model:model,apply:{ [weak self] in guard let self,!self.model.selected.isEmpty else { return };apply(self.model.path,self.model.selected) }));window.center();window.makeKeyAndOrderFront(nil)
    }
    func windowShouldClose(_ sender: NSWindow) -> Bool { !model.busy }
}

// Layout adapts the already aligned shared matrix; it performs no verse matching.
struct NativeParallelChapterPane: View {
    @ObservedObject var model: ReaderModel
    let chapter: CoreParallelChapter
    var onSearchWord: ((ReadingWordSelection, String?, Bool) -> Void)? = nil
    @State private var verseOffset: Int?
    @State private var selectedWord: ReadingWordSelection?

    private var verseIndex: Int {
        min(max(0, verseOffset ?? (chapter.selectedVerse - 1)), max(0, chapter.rows.count - 1))
    }
    func stackedVerse(at index: Int) -> String {
        guard chapter.rows.indices.contains(index) else { return "" }
        let row = chapter.rows[index]
        var result = "<h2>" + escape(row.reference) + "</h2>"
        for (column, cell) in zip(chapter.columns, row.cells) {
            result += "<article><h3>" + escape(column.requested) + "</h3>"
            if let status = unavailable(column, cell) {
                result += "<p>" + escape(status) + "</p>"
            } else {
                if cell.reference != row.reference && cell.additional.isEmpty {
                    result += "<p class=\"xc-parallel-reference\">" + escape(cell.reference) + "</p>"
                }
                result += "<div dir=\"" + (column.rtl ? "rtl" : "ltr") + "\" data-xc-module=\"" +
                    escape(column.module) + "\" data-xc-reference=\"" + escape(cell.reference) + "\">"
                result += cellContents(cell, module: column.module) + "</div>"
            }
            result += "</article>"
        }
        return result
    }
    private func unavailable(_ column: CoreParallelColumn, _ cell: CoreParallelCell) -> String? {
        if !column.ready { return NSLocalizedString("Module unavailable", comment: "Parallel comparison column") }
        if cell.reference.isEmpty || (cell.html.isEmpty && cell.additional.allSatisfy { $0.html.isEmpty }) {
            return NSLocalizedString("Verse unavailable", comment: "Parallel comparison cell")
        }
        return nil
    }
    private func cellContents(_ cell: CoreParallelCell, module: String) -> String {
        func body(_ html: String, headings: [String]) -> String {
            headings.map { "<div class=\"xc-parallel-heading\">" + $0 + "</div>" }.joined() + html
        }
        guard !cell.additional.isEmpty else { return body(cell.html, headings: cell.headings) }
        let verses = [CoreParallelVerse(reference: cell.reference, html: cell.html, headings: cell.headings)] + cell.additional
        return verses.map { verse in
            "<section data-xc-module=\"" + escape(module) + "\" data-xc-reference=\"" +
                escape(verse.reference) + "\"><h4>" + escape(verse.reference) + "</h4>" +
                (verse.html.isEmpty ? "<p>Verse unavailable</p>" : body(verse.html, headings: verse.headings)) + "</section>"
        }.joined()
    }
    private func escape(_ value: String) -> String { value.replacingOccurrences(of:"&",with:"&amp;").replacingOccurrences(of:"<",with:"&lt;").replacingOccurrences(of:">",with:"&gt;").replacingOccurrences(of:"\"",with:"&quot;") }
    var table: String {
        var result="<table class=\"xc-parallel\" aria-label=\""+escape(NSLocalizedString("Parallel reading",comment:"Accessible parallel chapter table"))+"\"><thead><tr><th scope=\"col\">"+escape(NSLocalizedString("Reference",comment:"Parallel table column"))+"</th>"
        for column in chapter.columns { result += "<th scope=\"col\">"+escape(column.requested)+(column.ready ? "" : " (unavailable)")+"</th>" }
        result += "</tr></thead><tbody>"
        for row in chapter.rows {
            result += "<tr><th scope=\"row\">"+escape(row.reference)+"</th>"
            for (index,cell) in row.cells.enumerated() {
                let column=chapter.columns[index]
                result += "<td dir=\""+(column.rtl ? "rtl" : "ltr")+"\" data-xc-module=\""+escape(column.module)+"\" data-xc-reference=\""+escape(cell.reference)+"\">"
                if let status = unavailable(column, cell) {
                    result += "<span>" + escape(status) + "</span>"
                } else {
                    if cell.reference != row.reference && cell.additional.isEmpty {
                        result += "<div class=\"xc-parallel-reference\">" + escape(cell.reference) + "</div>"
                    }
                    result += cellContents(cell, module: column.module)
                }
                result += "</td>"
            }
            result += "</tr>"
        }
        return result+"</tbody></table>"
    }
    var body: some View {
        VStack(alignment:.leading,spacing:8) {
            VStack(alignment:.leading,spacing:6) {
                Text("Parallel Reading — "+chapter.title).font(.headline).lineLimit(2)
                Toggle("Stacked verse", isOn: $model.stackedParallel)
                if model.stackedParallel {
                    HStack {
                        Button("Previous verse") { verseOffset = verseIndex - 1 }.disabled(verseIndex == 0)
                        Button("Next verse") { verseOffset = verseIndex + 1 }.disabled(verseIndex + 1 >= chapter.rows.count)
                    }
                }
                HStack { Button("Previous chapter") { model.navigateParallel(-1) };Button("Next chapter") { model.navigateParallel(1) };Spacer(minLength:0);Button("Close") { model.closeParallel() } }
            }.padding(8)
            if !model.parallelSourceNotice.isEmpty {Text(model.parallelSourceNotice).font(.caption).foregroundStyle(.secondary)}
            PassageView(html:model.stackedParallel ? stackedVerse(at: verseIndex) : table,css:".xc-parallel {border-collapse:collapse;min-width:100%;} .xc-parallel td,.xc-parallel th {vertical-align:top;padding:8px;border:1px solid GrayText;min-width:160px;} .xc-parallel thead th {position:sticky;top:0;background:Canvas;} .xc-parallel-heading {font-weight:bold;}",anchor:nil,onScrollFailure:{model.message=$0},onLink:{model.followLink($0)},onContextLink:{ uri,module,reference in model.followLink(uri,currentModule:module,currentReference:reference) },
                onSelection: { text, module, reference in
                    selectedWord = ReadingWordSelection(text: text, module: module, reference: reference)
                })
            if let word = selectedWord, let onSearchWord {
                ReadingWordSearchMenu(model: model, word: word, search: onSearchWord).id(word).padding(.horizontal, 8)
            }
        }.frame(minWidth:230,minHeight:220).disabled(model.isBusy)
            .onChange(of: chapter.reference) { verseOffset = nil; selectedWord = nil }
            .onChange(of: model.stackedParallel) { selectedWord = nil }
            .onChange(of: verseIndex) { selectedWord = nil }
    }
}
