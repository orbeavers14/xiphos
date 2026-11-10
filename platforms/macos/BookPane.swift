// SPDX-License-Identifier: GPL-2.0-or-later
import SwiftUI

struct BookCreationMenu: View {
    @ObservedObject var model: ReaderModel
    @State private var showing = false
    @State private var organizer = false
    @State private var commentary = false
    @State private var module = ""

    var body: some View {
        Menu("New study module") {
            Button("General Book…") { begin(organizer: false) }
            Button("Journal and Prayers…") { begin(organizer: true) }
            Button("Personal Commentary…") { begin(organizer: false, commentary: true) }
        }
        .disabled(model.isBusy)
        .alert(commentary ? "New Personal Commentary" : organizer ? "New Journal and Prayers Book" : "New General Book", isPresented: $showing) {
            TextField("Module ID", text: $module)
            Button("Cancel", role: .cancel) {}
            Button("Create") {
                if commentary { model.createPersonalCommentary(module) }
                else { model.createStudyBook(module, organizer: organizer) }
            }.disabled(module.isEmpty)
        } message: {
            Text("Choose a unique ID using letters, numbers, or underscores.")
        }
    }

    private func begin(organizer: Bool, commentary: Bool = false) {
        self.organizer = organizer; self.commentary = commentary
        module = ""
        showing = true
    }
}

private struct BookSectionMenu: View {
    @ObservedObject var model: ReaderModel
    let module: String
    let node: BookNode
    @State private var naming = false
    @State private var moving = false
    @State private var deleting = false
    @State private var input = ""

    var body: some View {
        Menu("Section") {
            Button("Rename…") { input = node.title; naming = true }
            Button("Move…") { input = ""; moving = true }
            Button(node.hasChildren ? "Delete Section and Children…" : "Delete Section…", role: .destructive) {
                deleting = true
            }
        }
        .accessibilityLabel("Organize book section")
        .alert("Rename Section", isPresented: $naming) {
            TextField("New name", text: $input)
            Button("Cancel", role: .cancel) {}
            Button("Rename") { model.organizeBookSection(module: module, path: node.path, action: .rename(input)) }
                .disabled(input.isEmpty || input == node.title)
        }
        .alert("Move Section", isPresented: $moving) {
            TextField("Destination parent path", text: $input)
            Button("Cancel", role: .cancel) {}
            Button("Move") { model.organizeBookSection(module: module, path: node.path, action: .move(input)) }
                .disabled(!input.hasPrefix("/"))
        } message: {
            Text("Enter an existing parent path. Enter / to move to the top level.")
        }
        .alert("Delete Section?", isPresented: $deleting) {
            Button("Cancel", role: .cancel) {}
            Button("Delete", role: .destructive) {
                model.organizeBookSection(module: module, path: node.path, action: .delete(descendants: node.hasChildren))
            }
        } message: {
            Text(node.hasChildren ? "This permanently removes \(node.path) and all its children." :
                                    "This permanently removes \(node.path).")
        }
    }
}

struct BookPane: View {
    @ObservedObject var model: ReaderModel
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack {
                Text("General book").font(.headline)
                Spacer()
                Menu {
                    if let options = model.bookDocument?.presentation?.options {
                        if let spacing = options.doubleSpace {
                            Toggle("Double space", isOn: Binding(get: { spacing }, set: { model.setBookDoubleSpace($0) }))
                        }
                        if let images = options.images {
                            Toggle("Image content", isOn: Binding(get: { images }, set: { model.setBookImageContent($0) }))
                        }
                        if let fonts = options.fontFaces {
                            Toggle("Respect font faces", isOn: Binding(get: { fonts }, set: { model.setBookRespectFontFaces($0) }))
                        }
                    }
                    if let numbers = model.bookDocument?.presentation?.noteNumbers {
                        Toggle("Footnote and cross-reference numbers", isOn: Binding(get: { numbers }, set: { model.setBookNoteNumbers($0) }))
                    }
                    Button("Refresh reading") { model.refreshBookDisplay() }
                } label: { Image(systemName: "textformat") }
                    .menuStyle(.borderlessButton).fixedSize()
                    .accessibilityLabel("Book display options").help("Book display options")
                if let tree = model.bookEntry?.tree, !tree.node.path.isEmpty {
                    BookSectionMenu(model: model, module: tree.module, node: tree.node)
                }
                Button("Close") { model.closeBook() }
            }.padding(8)
            Picker("Book module", selection: Binding(
                get: { model.bookEntry?.tree.module ?? model.bookBrowser?.module ?? "" }, set: { model.chooseBook($0) })) {
                ForEach(model.modules.filter(\.hasBookTree)) { module in Text(module.id).tag(module.id) }
            }.padding(.horizontal, 8)
            HStack {
                TextField("Book key or path", text: $model.bookKey).onSubmit { model.readBookKey() }
                    .accessibilityLabel("Book key or path")
                Button("Read") { model.readBookKey() }
            }.padding(8)
            HStack {
                Button { model.navigateBook(.parent) } label: { Image(systemName: "arrow.left") }
                    .accessibilityLabel("Book parent").help("Parent")
                    .disabled(model.bookEntry?.tree.parent == nil)
                Button { model.navigateBook(.firstChild) } label: { Image(systemName: "arrow.right") }
                    .accessibilityLabel("Book first child").help("First child")
                    .disabled(model.bookEntry?.tree.firstChild == nil)
                Button { model.navigateBook(.previousSibling) } label: { Image(systemName: "arrow.up") }
                    .accessibilityLabel("Book previous sibling").help("Previous sibling")
                    .disabled(model.bookEntry?.tree.previousSibling == nil)
                Button { model.navigateBook(.nextSibling) } label: { Image(systemName: "arrow.down") }
                    .accessibilityLabel("Book next sibling").help("Next sibling")
                    .disabled(model.bookEntry?.tree.nextSibling == nil)
            }.padding(.horizontal, 8).padding(.bottom, 8)
            if let page = model.bookChildren {
                HStack {
                    Button("Contents") { model.browseBook(0) }
                    if let parent = model.bookBrowser?.parent {
                        Button("Up a level") { model.browseBook(parent.offset) }
                    }
                    Text(page.parent.path.isEmpty ? page.module : page.parent.path).font(.caption).lineLimit(1)
                }.padding(.horizontal, 8)
                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 4) {
                        ForEach(page.nodes) { node in
                            HStack {
                                Button(node.title) { model.chooseBookNode(node) }
                                    .accessibilityLabel("Read book section \(node.title)")
                                Spacer()
                                if node.hasChildren {
                                    Button { model.browseBook(node.offset) } label: { Image(systemName: "folder") }
                                        .accessibilityLabel("Browse book section \(node.title)").help("Browse children")
                                }
                            }.buttonStyle(.borderless)
                            Divider()
                        }
                    }.padding(.horizontal, 8)
                }.frame(minHeight: 60, idealHeight: 110, maxHeight: 170)
                    .accessibilityLabel("Book contents")
                if page.nextAfter != nil {
                    Button("More sections") { model.moreBookChildren() }.padding(.horizontal, 8)
                }
            }
            Divider()
            if let entry = model.bookEntry {
                Text(entry.tree.node.path.isEmpty ? entry.tree.module : entry.tree.node.path).font(.caption).padding(8)
                PassageView(html: model.bookDocument?.resourceHTML ?? entry.html, css: model.bookDocument?.resourceCSS ?? "", styleModule: entry.tree.module, anchor: model.bookAnchor, anchorRevision: model.bookAnchorRevision, onScrollFailure: { model.message = $0 },
                    onLink: { model.followBookLink($0) }, allowsInlineImages: model.bookDocument?.presentation?.inlineImages != nil)
                if let issues = model.bookDocument?.presentation?.inlineImages?.issues, !issues.isEmpty {
                    Text("Some book images could not be loaded (\(issues.count)).")
                        .font(.caption).foregroundStyle(.secondary).padding(.horizontal, 8)
                }
                if model.bookDocument?.presentation?.stylesheetIssue != nil {
                    Text("The module’s stylesheet could not be loaded.")
                        .font(.caption).foregroundStyle(.secondary).padding(.horizontal, 8)
                }
                if !entry.hasEntry && (model.bookDocument?.displayHTML.isEmpty ?? true) {
                    Text("This section has no text. Browse its children to continue reading.")
                        .foregroundStyle(.secondary).padding(8)
                }
            }
        }.frame(minWidth: 230, idealWidth: 300, minHeight: 200).disabled(model.isBusy)
            .sheet(item: Binding(get: { model.bookImage }, set: { if $0 == nil { model.closeBookImage() } })) { image in
                BookImagePreviewView(image: image, close: model.closeBookImage)
            }
    }
}

