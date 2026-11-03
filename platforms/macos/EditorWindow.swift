// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
import SwiftUI
import Combine

enum NativeEditorLocation { case newDocument, file(String), module(String,String) }
@MainActor final class NativeEditorModel: ObservableObject {
    @Published private(set) var source = ""
    @Published private(set) var message = "Loading…"
    @Published private(set) var rich: EditorRichDocument?
    @Published private(set) var richMode = true
    @Published private(set) var dirty = false
    @Published private(set) var canUndo = false
    @Published private(set) var canRedo = false
    @Published private(set) var ready = false
    @Published private(set) var isBusy = true
    @Published var frozen = false
    @Published var selection = NSRange(location: 0, length: 0)
    @Published private(set) var presentation: UInt64 = 0
    @Published var linkInput = ""
    private(set) var location: NativeEditorLocation
    private(set) var moduleSource: EditorModuleSource?
    private var revision: String?
    private var buffer: EditorSourceBuffer?
    private let worker: CoreWorker
    private var tab: UInt64?
    private var closed = false
    let recoveryID: String
    private let recovered: CoreRecoveryDraft?
    var onRecoveryChanged: (() -> Void)?
    var onModuleSaved: ((String) -> Void)?
    init(worker: CoreWorker, location: NativeEditorLocation, recovered: CoreRecoveryDraft? = nil) { self.worker = worker; self.location = location; self.recovered = recovered; recoveryID = recovered?.id ?? UUID().uuidString }
    deinit { if let tab { let worker = worker; Task { try? await worker.closeTab(tab) } } }
    var permitsRich: Bool { moduleSource.map { ["thml","html","xhtml"].contains($0.sourceType.lowercased()) } ?? true }
    var title: String { switch location { case .newDocument: "Untitled Study Document"; case .file(let path): URL(fileURLWithPath: path).lastPathComponent; case .module(let module,let ref): "\(module) — \(ref)" } }
    func start() async {
        guard tab == nil, !closed else { return }
        defer { isBusy = false }
        do {
            tab = try await worker.createTab()
            if let recovered {
                revision = recovered.originRevision
                buffer = try EditorSourceBuffer(recovered.savedText); try buffer?.replace(recovered.text)
                if case .module(let module,let reference) = location {
                    moduleSource = try? await worker.openModuleEditor(tab:tab!,module:module,reference:reference)
                    if moduleSource == nil { moduleSource = EditorModuleSource(module:module,reference:reference,driver:"",sourceType:"Plain",text:"",revision:revision ?? "") }
                }
                guard !closed else { return }
                ready = true; publish(preferRich:false)
                let utf8 = source.utf8
                if recovered.selectionStart <= utf8.count, recovered.selectionEnd <= utf8.count,
                   let lower = String.Index(utf8.index(utf8.startIndex,offsetBy:recovered.selectionStart),within:source),
                   let upper = String.Index(utf8.index(utf8.startIndex,offsetBy:recovered.selectionEnd),within:source) { selection = NSRange(lower..<upper,in:source) }
                message = "Recovered unsaved source. Save checks the original revision; use Save As if the source changed or is unavailable."
                return
            }
            let text: String
            switch location {
            case .newDocument: text = ""
            case .file(let path): let value = try await worker.openEditorFile(tab: tab!, path: path); text = value.text; revision = value.revision
            case .module(let module,let reference):
                let value = try await worker.openModuleEditor(tab: tab!, module: module, reference: reference)
                moduleSource = value; revision = value.revision; location = .module(value.module,value.reference); text = value.text
            }
            guard !closed else { return }
            buffer = try EditorSourceBuffer(text); ready = true; publish(preferRich: permitsRich)
        } catch { message = "Unable to open this document or editable module entry. \(error.localizedDescription)" }
    }
    private func publish(preferRich: Bool) {
        source = buffer?.text ?? ""; dirty = buffer?.dirty ?? false; canUndo = buffer?.canUndo ?? false; canRedo = buffer?.canRedo ?? false
        if preferRich, permitsRich, let parsed = try? EditorRichCodec.parse(source) { rich = parsed; richMode = true; message = "" }
        else { rich = nil; richMode = false; message = permitsRich ? "This markup is preserved in Source. Rich mode supports text, links, colors, headings and lists." : "This module stores plain text." }
        presentation &+= 1; onRecoveryChanged?()
    }
    func setMode(_ richMode: Bool) { guard ready, !isBusy, !frozen else { return }; publish(preferRich: richMode) }
    func accept(text: String) {
        guard ready, !isBusy, !frozen else { return }
        do { try buffer?.replace(text); source = text; dirty = buffer?.dirty ?? false; canUndo = buffer?.canUndo ?? false; canRedo = buffer?.canRedo ?? false; message = ""; onRecoveryChanged?() }
        catch { message = "The edit exceeds the supported text limits."; presentation &+= 1 }
    }
    func accept(runs: [EditorRun]) {
        guard richMode, ready, !isBusy, !frozen else { return }
        do {
            let value = try EditorRichCodec.make(runs); try buffer?.replace(value.html)
            presentation &+= 1; source = value.html; rich = value; dirty = buffer?.dirty ?? false; canUndo = buffer?.canUndo ?? false; canRedo = buffer?.canRedo ?? false; message = ""; onRecoveryChanged?()
        } catch { message = "That rich-text edit could not be applied. The previous content is retained."; presentation &+= 1 }
    }
    func format(_ action: Int32, value: Int32 = -1, text: String = "") {
        guard richMode, let rich, !isBusy, !frozen else { return }
        do { let next = try EditorRichCodec.format(rich, selection: selection, action: action, value: value, text: text); try buffer?.replace(next.html); publish(preferRich: true) }
        catch { message = "Choose a valid selection, color or link. The content has not changed." }
    }
    func undo() { guard !isBusy, !frozen else { return }; buffer?.undo(); publish(preferRich: richMode) }
    func redo() { guard !isBusy, !frozen else { return }; buffer?.redo(); publish(preferRich: richMode) }
    func save(to selected: String? = nil, erase: Bool = false) async throws {
        guard ready, !isBusy, !frozen, let tab else { throw SessionHostError(message: "The editor is not ready to save.") }
        isBusy = true; message = "Saving…"; defer { isBusy = false; if message == "Saving…" { message = "Save did not complete. Reopen if the source has changed." } }
        if selected == nil, case .module(let module, let reference) = location {
            guard let revision else { throw SessionHostError(message: "Reopen the module entry before saving.") }
            let value = try await worker.saveModuleEditor(tab: tab, module: module, reference: reference, revision: revision, text: erase ? "" : source, erase: erase)
            moduleSource = value; self.revision = value.revision
            if erase { try buffer?.replace("") }; buffer?.saved(); publish(preferRich: richMode)
            onModuleSaved?(module)
        } else {
            let original: String? = if case .file(let path) = location { path } else { nil }
            guard let path = selected ?? original else { throw SessionHostError(message: "Choose a file for this document.") }
            var expected = path == original ? revision : nil
            if path != original && FileManager.default.fileExists(atPath: path) { expected = try await worker.openEditorFile(tab: tab, path: path).revision }
            let value = try await worker.saveEditorFile(tab: tab, path: path, revision: expected, text: source)
            revision = value.revision; location = .file(path); moduleSource = nil; try? await worker.releaseModuleEditor(tab:tab); buffer?.saved(); dirty = buffer?.dirty ?? false; message = "Saved."; onRecoveryChanged?()
        }
    }
    func recoveryDraft() -> CoreRecoveryDraft? {
        guard ready, let buffer else { return recovered }
        var d = CoreRecoveryDraft(id:recoveryID,title:title,originRevision:revision,savedText:buffer.savedText,text:buffer.text)
        switch location {
        case .newDocument: d.target = 0; d.originRevision = nil
        case .file(let path): d.target = 1; d.path = path
        case .module(let module,let reference): d.target = 2; d.module = module; d.reference = reference
        }
        if !richMode, let range = Range(selection,in:source) { d.selectionStart = source[..<range.lowerBound].utf8.count; d.selectionEnd = source[..<range.upperBound].utf8.count }
        return d
    }
    func close() { guard !closed else { return }; closed = true; if let tab { self.tab = nil; let worker = worker; Task { try? await worker.closeTab(tab) } } }
}
private extension NSAttributedString.Key {
    static let editorFlags = NSAttributedString.Key("XiphosEditorFlags")
    static let editorBlock = NSAttributedString.Key("XiphosEditorBlock")
    static let editorForeground = NSAttributedString.Key("XiphosEditorForeground")
    static let editorBackground = NSAttributedString.Key("XiphosEditorBackground")
    static let editorLink = NSAttributedString.Key("XiphosEditorLink")
}
@MainActor private enum EditorAttributes {
    static func color(_ hex: String) -> NSColor? { guard hex.count == 7, let value = Int(hex.dropFirst(),radix: 16) else { return nil }; return NSColor(srgbRed: CGFloat((value>>16)&255)/255, green: CGFloat((value>>8)&255)/255, blue: CGFloat(value&255)/255, alpha: 1) }
    static func make(_ r: EditorRun) -> [NSAttributedString.Key:Any] {
        let sizes: [CGFloat] = [16,32,28,24,20,18,16]
        var font = r.block == 8 ? NSFont.monospacedSystemFont(ofSize: 16, weight: .regular) : NSFont.systemFont(ofSize: (1...6).contains(r.block) ? sizes[Int(r.block)] : 16)
        if r.flags&1 != 0 || (1...6).contains(r.block) { font = NSFontManager.shared.convert(font,toHaveTrait: .boldFontMask) }
        if r.flags&2 != 0 || r.block == 7 { font = NSFontManager.shared.convert(font,toHaveTrait: .italicFontMask) }
        var attrs: [NSAttributedString.Key:Any] = [.font:font,.foregroundColor:color(r.foreground) ?? NSColor.textColor,
            .editorFlags:r.flags,.editorBlock:r.block,.editorForeground:r.foreground,.editorBackground:r.background,.editorLink:r.link]
        if r.flags&4 != 0 { attrs[.underlineStyle] = NSUnderlineStyle.single.rawValue }
        if r.flags&8 != 0 { attrs[.strikethroughStyle] = NSUnderlineStyle.single.rawValue }
        if let color = color(r.background) { attrs[.backgroundColor] = color }
        if !r.link.isEmpty { attrs[.link] = r.link }
        if r.block >= 9 {
            let paragraph = NSMutableParagraphStyle(); paragraph.headIndent = 24; paragraph.firstLineHeadIndent = 24
            attrs[.paragraphStyle] = paragraph
        }
        return attrs
    }
}
@MainActor final class NativeEditorTextView: NSTextView {
    var command: ((String) -> Void)?
    override func performKeyEquivalent(with event: NSEvent) -> Bool {
        if event.modifierFlags.contains(.command), let key = event.charactersIgnoringModifiers?.lowercased(), ["b","i","u","z"].contains(key) {
            command?(key == "z" && event.modifierFlags.contains(.shift) ? "redo" : key); return true
        }
        return super.performKeyEquivalent(with: event)
    }
    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)
        guard let storage = textStorage, let layout = layoutManager, textContainer != nil else { return }
        let text = storage.string as NSString; var at = 0, prior: Int32 = -1, item = 0
        while at < text.length {
            let paragraph = text.paragraphRange(for:NSRange(location:at,length:0))
            let block = (storage.attribute(.editorBlock,at:at,effectiveRange:nil) as? NSNumber)?.int32Value ?? 0
            if block >= 9 {
                item = prior == block ? item+1 : 1
                let formats: [NSTextList.MarkerFormat] = [.disc,.decimal,.lowercaseRoman,.lowercaseAlpha]
                let marker = NSTextList(markerFormat:formats[Int(block-9)],options:0).marker(forItemNumber:item)
                let glyph = layout.glyphRange(forCharacterRange:NSRange(location:at,length:1),actualCharacterRange:nil)
                let rect = layout.lineFragmentRect(forGlyphAt:glyph.location,effectiveRange:nil)
                (marker as NSString).draw(at:NSPoint(x:textContainerOrigin.x,y:textContainerOrigin.y+rect.origin.y),withAttributes:[.font:NSFont.systemFont(ofSize:16),.foregroundColor:NSColor.textColor])
            }
            prior = block; at = NSMaxRange(paragraph)
        }
    }
    override func paste(_ sender: Any?) { if let text = NSPasteboard.general.string(forType: .string) { insertText(text, replacementRange: selectedRange()) } }
}
struct NativeEditorText: NSViewRepresentable {
    @ObservedObject var model: NativeEditorModel
    @MainActor final class Coordinator: NSObject, NSTextViewDelegate {
        let model: NativeEditorModel; var revision = UInt64.max; var updating = false
        init(_ model: NativeEditorModel) { self.model = model }
        func textDidChange(_ notification: Notification) {
            guard !updating, let view = notification.object as? NSTextView else { return }
            if !model.richMode { model.accept(text: view.string); return }
            var runs: [EditorRun] = []
            view.attributedString().enumerateAttributes(in: NSRange(location: 0,length: view.attributedString().length)) { attrs, range, _ in
                runs.append(EditorRun(text: (view.string as NSString).substring(with: range), flags: (attrs[.editorFlags] as? NSNumber)?.uint32Value ?? 0,
                    block: (attrs[.editorBlock] as? NSNumber)?.int32Value ?? 0, foreground: attrs[.editorForeground] as? String ?? "",
                    background: attrs[.editorBackground] as? String ?? "", link: attrs[.editorLink] as? String ?? ""))
            }
            model.accept(runs: runs)
        }
        func textViewDidChangeSelection(_ notification: Notification) { if !updating, let view = notification.object as? NSTextView { model.selection = view.selectedRange() } }
        func textView(_ textView: NSTextView, clickedOnLink link: Any, at charIndex: Int) -> Bool { true }
    }
    func makeCoordinator() -> Coordinator { Coordinator(model) }
    func makeNSView(context: Context) -> NSScrollView {
        let scroll = NSScrollView(); scroll.hasVerticalScroller = true
        let view = NativeEditorTextView(frame: .zero); view.delegate = context.coordinator
        view.isRichText = true; view.importsGraphics = false; view.allowsUndo = false
        view.isAutomaticQuoteSubstitutionEnabled = false; view.isAutomaticDashSubstitutionEnabled = false
        view.isAutomaticTextReplacementEnabled = false; view.isAutomaticLinkDetectionEnabled = false
        view.isVerticallyResizable = true; view.isHorizontallyResizable = false
        view.autoresizingMask = [.width]; view.textContainer?.widthTracksTextView = true
        view.textContainerInset = NSSize(width: 12,height: 12)
        view.command = { key in switch key { case "b": model.format(0); case "i": model.format(1); case "u": model.format(2); case "z": model.undo(); case "redo": model.redo(); default: break } }
        scroll.documentView = view; return scroll
    }
    func updateNSView(_ scroll: NSScrollView, context: Context) {
        guard let view = scroll.documentView as? NativeEditorTextView else { return }
        view.isEditable = model.ready && !model.isBusy && !model.frozen
        // Replacing the backing string while an input method owns marked text
        // cancels composition. Apply the shared presentation after it commits.
        guard !view.hasMarkedText(), context.coordinator.revision != model.presentation else { return }
        context.coordinator.updating = true; defer { context.coordinator.updating = false }
        context.coordinator.revision = model.presentation
        let selection = model.selection
        let content = NSMutableAttributedString(string: "")
        if model.richMode, let doc = model.rich { for run in doc.runs { content.append(NSAttributedString(string: run.text,attributes: EditorAttributes.make(run))) } }
        else { content.append(NSAttributedString(string: model.source,attributes: [.font:NSFont.monospacedSystemFont(ofSize:16,weight:.regular),.foregroundColor:NSColor.textColor])) }
        view.textStorage?.setAttributedString(content)
        let start = min(selection.location,content.length), length = min(selection.length,content.length-start)
        view.setSelectedRange(NSRange(location:start,length:length))
        if model.richMode, let doc = model.rich {
            var offset = 0; var style = doc.runs.last ?? EditorRun(text: "")
            for run in doc.runs { if start < offset + run.text.utf16.count { style = run; break }; offset += run.text.utf16.count }
            view.typingAttributes = EditorAttributes.make(style)
        } else { view.typingAttributes = [.font:NSFont.monospacedSystemFont(ofSize:16,weight:.regular)] }
    }
}
struct NativeEditorPane: View {
    @ObservedObject var model: NativeEditorModel
    let save: () -> Void, saveAs: () -> Void, clear: () -> Void
    var body: some View {
        VStack(spacing:8) {
            HStack { Text(model.title).font(.headline).lineLimit(1).truncationMode(.middle); if model.dirty { Text("Unsaved").foregroundStyle(.secondary) }; Spacer(); Button("Save",action:save).keyboardShortcut("s"); Button("Save As…",action:saveAs) }
            HStack {
                Button("Rich") { model.setMode(true) }.disabled(!model.permitsRich)
                Button("Source") { model.setMode(false) }
                Button("Undo") { model.undo() }.disabled(!model.canUndo)
                Button("Redo") { model.redo() }.disabled(!model.canRedo)
                Spacer()
                if model.moduleSource != nil { Button("Clear Entry…",action:clear) }
            }
            HStack {
                Button("B") { model.format(0) }.bold(); Button("I") { model.format(1) }.italic()
                Button("Underline") { model.format(2) }; Button("Strike") { model.format(3) }
                Menu("Paragraph") { ForEach(0..<13,id:\.self) { index in Button(["Normal","Heading 1","Heading 2","Heading 3","Heading 4","Heading 5","Heading 6","Address","Preformatted","Bullets","Numbers","Roman list","Alphabetic list"][index]) { model.format(4,value:Int32(index)) } } }
                Menu("Text Color") { Button("Black") { model.format(5,text:"#000000") }; Button("Red") { model.format(5,text:"#ff0000") }; Button("Green") { model.format(5,text:"#008000") }; Button("Blue") { model.format(5,text:"#0000ff") }; Button("Purple") { model.format(5,text:"#800080") }; Button("Default") { model.format(5,text:"") } }
                Menu("Highlight") { Button("Yellow") { model.format(6,text:"#ffff00") }; Button("Green") { model.format(6,text:"#aaffaa") }; Button("Blue") { model.format(6,text:"#aaddff") }; Button("None") { model.format(6,text:"") } }
                Button("Clear Format") { model.format(8) }
            }.disabled(!model.richMode)
            HStack { TextField("sword://, bible:// or web link",text:$model.linkInput); Button("Apply Link") { model.format(7,text:model.linkInput) }; Button("Remove Link") { model.format(7,text:"") } }.disabled(!model.richMode)
            NativeEditorText(model:model).frame(minWidth:720,minHeight:450)
            Text(model.message).font(.caption).foregroundStyle(.secondary).frame(maxWidth:.infinity,alignment:.leading)
        }.padding().background(Color(nsColor:.windowBackgroundColor)).disabled(model.isBusy || model.frozen || !model.ready)
    }
}
enum EditorCloseChoice { case save,discard,cancel }
@MainActor final class NativeEditorWindow: NSObject, NSWindowDelegate {
    let model: NativeEditorModel
    let window: NSWindow
    var onClosed: (() -> Void)?
    var closeDecision: (() -> EditorCloseChoice)?
    var saveDestination: (() -> String?)?
    private var approved = false
    private var observation: AnyCancellable?
    init(worker: CoreWorker, location: NativeEditorLocation, recovered: CoreRecoveryDraft? = nil, show: Bool = true, moduleSaved: @escaping (String) -> Void) {
        model = NativeEditorModel(worker:worker,location:location,recovered:recovered)
        window = NSWindow(contentRect:NSRect(x:0,y:0,width:900,height:680),styleMask:[.titled,.closable,.miniaturizable,.resizable],backing:.buffered,defer:false)
        super.init(); window.minSize = NSSize(width:800,height:650); model.onModuleSaved = moduleSaved; window.delegate = self; window.isReleasedWhenClosed = false
        window.contentView = NSHostingView(rootView:NativeEditorPane(model:model,save:{ [weak self] in self?.save() },saveAs:{ [weak self] in self?.save(asNew:true) },clear:{ [weak self] in self?.clearEntry() }))
        observation = model.$dirty.sink { [weak self] value in self?.window.isDocumentEdited = value }
        window.center(); if show { window.makeKeyAndOrderFront(nil) } else { window.orderOut(nil) }
        Task { await model.start(); window.title = model.title }
    }
    private func destination() -> String? { if let saveDestination { return saveDestination() }; let panel = NSSavePanel(); panel.nameFieldStringValue = "Study.spt"; panel.title = "Save Study Document"; return panel.runModal() == .OK ? panel.url?.path : nil }
    @discardableResult private func saveNow(asNew: Bool = false) async -> Bool {
        var path: String?
        if asNew || { if case .newDocument = model.location { return true }; return false }() { guard let chosen = destination() else { return false }; path = chosen }
        do { try await model.save(to:path); window.title = model.title; return true }
        catch { let alert = NSAlert(); alert.messageText = "Unable to save changes"; alert.informativeText = "The document or module may have changed. Reopen it before replacing newer content. \(error.localizedDescription)"; alert.runModal(); return false }
    }
    func save(asNew: Bool = false) { Task { _ = await saveNow(asNew:asNew) } }
    func clearEntry() {
        let alert = NSAlert(); alert.messageText = "Clear this saved module entry?"; alert.informativeText = model.title
        alert.addButton(withTitle:"Clear Entry"); alert.addButton(withTitle:"Cancel")
        guard alert.runModal() == .alertFirstButtonReturn else { return }
        Task { do { try await model.save(erase:true) } catch { let alert = NSAlert(); alert.messageText = "The entry could not be cleared"; alert.informativeText = error.localizedDescription; alert.runModal() } }
    }
    func confirmClose() async -> Bool {
        let deadline = Date().addingTimeInterval(30)
        while model.isBusy { if Date() >= deadline { return false }; try? await Task.sleep(nanoseconds:10_000_000) }
        if !model.dirty { return true }
        let choice: EditorCloseChoice
        if let closeDecision { choice = closeDecision() }
        else {
            let alert = NSAlert(); alert.messageText = "Save changes to \(model.title)?"
            alert.addButton(withTitle:"Save"); alert.addButton(withTitle:"Discard"); alert.addButton(withTitle:"Cancel")
            let response = alert.runModal(); choice = response == .alertFirstButtonReturn ? .save : response == .alertSecondButtonReturn ? .discard : .cancel
        }
        switch choice { case .save: return await saveNow(); case .discard: return true; case .cancel: return false }
    }
    func windowShouldClose(_ sender: NSWindow) -> Bool {
        if approved { return true }
        Task { if await confirmClose() { approved = true; window.performClose(nil) } }; return false
    }
    func windowWillClose(_ notification: Notification) { model.close(); onClosed?() }
}

// The section is committed before opening an ordinary recoverable module editor.
@MainActor final class NativeBookSectionModel: ObservableObject {
    @Published private(set) var modules: [InstalledModule] = []
    @Published var module = ""
    @Published var path = "/Journals/"
    @Published private(set) var busy = false
    @Published private(set) var message = ""
    private let worker: CoreWorker
    init(worker: CoreWorker) { self.worker = worker }
    func load() async {
        guard !busy else { return }; busy = true; defer { busy = false }
        do {
            modules = try await worker.modules().filter { $0.hasBookTree }
            if !modules.contains(where: { $0.id == module }) { module = modules.first?.id ?? "" }
            message = modules.isEmpty ? "Install a writable personal general book first." : ""
        } catch { message = "The book list could not be loaded. \(error.localizedDescription)" }
    }
    func create() async -> EditorModuleSource? {
        guard !busy, !module.isEmpty else { return nil }
        busy = true; defer { busy = false }
        var tab: UInt64?
        do {
            let opened = try await worker.createTab(); tab = opened
            let prepared = try await worker.prepareBookSection(tab:opened,module:module,path:path)
            let created = try await worker.createBookSection(tab:opened,prepared:prepared)
            try? await worker.closeTab(opened)
            message = "Section created."
            return created
        } catch {
            if let tab { try? await worker.closeTab(tab) }
            message = "Section could not be created. Use a new absolute path in a writable, unencrypted UTF-8 personal RawGenBook. Existing sections are never replaced. \(error.localizedDescription)"
            return nil
        }
    }
}
@MainActor final class NativeBookSectionWindow: NSObject, NSWindowDelegate {
    let model: NativeBookSectionModel
    let window: NSWindow
    init(worker: CoreWorker, created: @escaping (EditorModuleSource) -> Void) {
        model = NativeBookSectionModel(worker:worker)
        window = NSWindow(contentRect:NSRect(x:0,y:0,width:590,height:310),styleMask:[.titled,.closable],backing:.buffered,defer:false)
        super.init(); window.title = "New Journal or Prayer Section"; window.delegate = self; window.isReleasedWhenClosed = false
        window.contentView = NSHostingView(rootView:NativeBookSectionPane(model:model,create:{ [weak self] in
            guard let self else { return }
            Task { if let result = await self.model.create() { self.window.close(); created(result) } }
        }))
        window.center(); window.makeKeyAndOrderFront(nil)
        Task { await model.load() }
    }
    func windowShouldClose(_ sender: NSWindow) -> Bool { !model.busy }
}
private struct NativeBookSectionPane: View {
    @ObservedObject var model: NativeBookSectionModel
    let create: () -> Void
    var body: some View {
        VStack(alignment:.leading,spacing:12) {
            Picker("Personal book",selection:$model.module) { ForEach(model.modules) { Text($0.description + " (" + $0.id + ")").tag($0.id) } }
            TextField("Section path",text:$model.path)
            Text("For example: /Journals/Today or /Prayers/Family. Missing folders are created with the section.").font(.caption).foregroundStyle(.secondary)
            Text("Create saves an empty section immediately, then opens the editor. Closing the editor keeps that section.").font(.callout)
            Text(model.message).font(.caption).foregroundStyle(.secondary).textSelection(.enabled)
            HStack { Spacer(); if model.busy { ProgressView().controlSize(.small) }; Button("Create and Edit",action:create).disabled(model.module.isEmpty || model.path.isEmpty).keyboardShortcut(.defaultAction) }
        }.padding().disabled(model.busy)
    }
}
