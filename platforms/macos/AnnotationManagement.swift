// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import SwiftUI
import AppKit

@MainActor final class NativeAnnotationModel: ObservableObject {
    @Published var module: String
    @Published var reference: String
    @Published var generic = false
    @Published var note = ""
    @Published var bookmarkPath = ""
    @Published var frozen = false
    @Published private(set) var path: String
    @Published private(set) var profile: CoreAnnotationProfile?
    @Published private(set) var target: CoreAnnotationTarget?
    @Published private(set) var decoration: CoreVerseDecoration?
    @Published private(set) var busy = false
    @Published private(set) var message = "Load the verse note before editing."
    private var loadedModule = "",loadedReference = "",loadedGeneric = false,savedNote = ""
    private let worker: CoreWorker
    private var tab: UInt64?
    private var originRevision: String?
    let recoveryID: String
    var onRecoveryChanged: (() -> Void)?
    var onUseInReading: ((CoreReadingSources) -> Void)?
    var onSaved: (() -> Void)?
    init(worker: CoreWorker,path: String,module: String,reference: String,recovered: CoreRecoveryDraft? = nil) {
        self.worker=worker;self.path=path;self.module=module;self.reference=reference;recoveryID=recovered?.id ?? UUID().uuidString
        if let recovered,let label=recovered.annotationLabel,recovered.target==3 {
            self.path=recovered.path;self.module=recovered.module;self.reference=recovered.reference;generic=recovered.annotationGeneric
            loadedModule=self.module;loadedReference=self.reference;loadedGeneric=generic
            target=CoreAnnotationTarget(module:self.module,reference:self.reference,label:label)
            originRevision=recovered.originRevision;savedNote=recovered.savedText;note=recovered.text
            message="Recovered unsaved note. Save checks the original file revision; copy the draft before reloading if the source changed."
        }
    }
    deinit { if let tab { let worker=worker;Task {try? await worker.closeTab(tab)} } }
    var matches: Bool { target != nil && loadedModule.utf8.elementsEqual(module.utf8) && loadedReference.utf8.elementsEqual(reference.utf8) && loadedGeneric==generic }
    var dirty: Bool { !note.utf8.elementsEqual(savedNote.utf8) }
    var duplicateCount: Int { guard let target else { return 0 };return profile?.rows.filter { $0.label?.utf8.elementsEqual(target.label.utf8)==true }.count ?? 0 }
    private func accept(_ snapshot: CoreAnnotationProfile?) {
        profile=snapshot;originRevision=snapshot?.revision;note=snapshot?.rows.first(where: { $0.label?.utf8.elementsEqual(target?.label.utf8 ?? "".utf8)==true })?.note ?? "";savedNote=note;decoration=nil;onRecoveryChanged?()
    }
    func load(path selected: String? = nil) async {
        guard !busy,!frozen else { return };busy=true;defer {busy=false}
        do {
            if tab==nil {tab=try await worker.createTab()}
            let file=selected ?? path
            let snapshot=FileManager.default.fileExists(atPath:file) ? try await worker.annotations(tab:tab!,path:file) : nil
            let canonical=try await worker.annotationTarget(tab:tab!,module:module,reference:reference,generic:generic)
            path=file;target=canonical;loadedModule=module;loadedReference=reference;loadedGeneric=generic;accept(snapshot);message="Loaded. Notes are stored as plain text."
            if snapshot?.rows.first(where:{$0.label?.utf8.elementsEqual(canonical.label.utf8)==true})?.visible == false {message += " Legacy formatting hides this record in the old application; its source is preserved."}
        } catch { message="The note could not be loaded. \(error.localizedDescription)" }
    }
    @discardableResult func save(remove: Bool = false) async -> Bool {
        guard !busy,!frozen,matches,let target else { return false };busy=true;defer {busy=false}
        do {if tab==nil {tab=try await worker.createTab()};let saved=try await worker.saveAnnotation(tab:tab!,path:path,revision:originRevision,label:target.label,note:note,remove:remove);accept(saved);onSaved?();message=remove ? "First matching note removed. Any duplicate records are retained." : "Note saved.";return true }
        catch {message="The note was not saved. The file may have changed. Reload before retrying, or copy your draft to keep it. \(error.localizedDescription)";return false}
    }
    func preview() async {
        guard !busy,matches,let tab,!bookmarkPath.isEmpty,profile != nil else {return};busy=true;defer {busy=false}
        do { decoration=try await worker.verseDecoration(tab:tab,bookmarks:bookmarkPath,annotations:path,module:module,reference:reference);message="Preview reads saved files; unsaved text is not included." }
        catch { decoration=nil;message="Saved composition could not be read. \(error.localizedDescription)" }
    }
    func recoveryDraft() -> CoreRecoveryDraft? {
        guard dirty else {return nil}
        guard let target else {return CoreRecoveryDraft(id:recoveryID,title:"Unattached verse note",originRevision:nil,savedText:"",text:note)}
        return CoreRecoveryDraft(id:recoveryID,title:target.module+" · "+target.reference,target:3,path:path,module:target.module,reference:target.reference,originRevision:originRevision,savedText:savedNote,text:note,annotationLabel:target.label,annotationGeneric:loadedGeneric)
    }
    func draftChanged() {if dirty {message="Unsaved note changes."};onRecoveryChanged?()}
    func discard() {note=savedNote;onRecoveryChanged?()}
}
private struct NativeAnnotationPane: View {
    @ObservedObject var model: NativeAnnotationModel
    private func mayReload() -> Bool {
        guard model.dirty else {return true}
        let alert=NSAlert();alert.messageText="Discard this unsaved note draft and reload?";alert.addButton(withTitle:"Discard Draft");alert.addButton(withTitle:"Cancel")
        return alert.runModal() == .alertFirstButtonReturn
    }
    private func choose(_ action: @escaping (String) -> Void) { let panel=NSOpenPanel();panel.canChooseDirectories=false;panel.allowsMultipleSelection=false;if panel.runModal() == .OK,let url=panel.url {action(url.path)} }
    var body: some View {
        VStack(alignment:.leading,spacing:10) {
            HStack {TextField("Module",text:$model.module);TextField("Verse reference",text:$model.reference);Toggle("All translations",isOn:$model.generic);Button("Load / Reload") {if mayReload() {Task {await model.load()}}}}
            HStack {Text(model.path).font(.caption).textSelection(.enabled);Spacer();Button("Open Notes File…") {if mayReload() {choose {path in Task {await model.load(path:path)}}}}}
            Text(model.target.map {$0.module+" · "+$0.reference} ?? "No note loaded").font(.headline)
            if model.duplicateCount>1 {Text("Multiple legacy records share this target. Save updates the first; later records remain and may take precedence in the legacy display.").font(.caption)}
            TextEditor(text:$model.note).font(.body).frame(minHeight:150).onChange(of:model.note) {_ in model.draftChanged()}
            HStack {Button("Show in Reading") {model.onUseInReading?(CoreReadingSources(bookmarks:model.bookmarkPath.isEmpty ? nil : model.bookmarkPath,annotations:model.path))}.disabled(model.profile == nil);Button("Save Note") {Task {await model.save()}};Button("Remove Note") {
                let alert=NSAlert();alert.messageText="Remove this saved note?";alert.informativeText="Only the first matching record is removed. Other notes and duplicate records remain.";alert.addButton(withTitle:"Remove Note");alert.addButton(withTitle:"Cancel")
                if alert.runModal() == .alertFirstButtonReturn {Task {await model.save(remove:true)}}
            }.disabled(model.profile == nil);Spacer();if model.dirty {Text("Unsaved changes").font(.caption)}}.disabled(!model.matches)
            HStack {Button("Choose Bookmark XML…") {choose {model.bookmarkPath=$0}};Text(model.bookmarkPath.isEmpty ? "No bookmark file selected" : URL(fileURLWithPath:model.bookmarkPath).lastPathComponent).font(.caption);Button("Preview Saved Composition") {Task {await model.preview()}}.disabled(!model.matches || model.bookmarkPath.isEmpty || model.profile == nil)}
            if let plan=model.decoration {
                Text("Saved composition — "+plan.reference).font(.headline)
                Text("Ordered bookmark colors: "+(plan.colors.isEmpty ? "none" : plan.colors.joined(separator:", "))).font(.caption)
                Text(plan.inner==1 ? "Personal-note highlight takes precedence." : plan.inner==2 ? "Current-verse highlight applies." : plan.inner==3 ? "Current-verse text color applies." : "No inner highlight.").font(.caption)
                PassageView(html:plan.noteHTML,anchor:nil,onScrollFailure:{_ in},onLink:{_ in}).frame(minHeight:100)
                if plan.incomplete {Text("Some source records could not be included.").foregroundStyle(.secondary)}
            }
            Text(model.message).font(.caption).textSelection(.enabled)
        }.padding().disabled(model.busy || model.frozen).frame(minWidth:760,minHeight:500)
    }
}
@MainActor final class NativeAnnotationWindow: NSObject,NSWindowDelegate {
    let model: NativeAnnotationModel,window: NSWindow
    var onClosed: (() -> Void)?
    init(worker: CoreWorker,path: String,module: String,reference: String,recovered: CoreRecoveryDraft? = nil) {
        model=NativeAnnotationModel(worker:worker,path:path,module:module,reference:reference,recovered:recovered)
        window=NSWindow(contentRect:NSRect(x:0,y:0,width:900,height:650),styleMask:[.titled,.closable,.resizable],backing:.buffered,defer:false)
        super.init();window.title="Personal Verse Notes";window.isReleasedWhenClosed=false;window.delegate=self;window.contentView=NSHostingView(rootView:NativeAnnotationPane(model:model));window.center();window.makeKeyAndOrderFront(nil);if recovered==nil {Task {await model.load()}}
    }
    func confirmClose() async -> Bool {
        guard !model.busy else {return false};guard model.dirty else {return true}
        let alert=NSAlert();alert.messageText="Save this verse note?";alert.addButton(withTitle:"Save");alert.addButton(withTitle:"Discard");alert.addButton(withTitle:"Cancel")
        switch alert.runModal() {case .alertFirstButtonReturn:return await model.save();case .alertSecondButtonReturn:model.discard();return true;default:return false}
    }
    func windowWillClose(_ notification: Notification) {onClosed?()}
    func windowShouldClose(_ sender: NSWindow) -> Bool {guard !model.busy else {return false};if !model.dirty {return true};Task {if await confirmClose() {window.performClose(nil)}};return false}
}
