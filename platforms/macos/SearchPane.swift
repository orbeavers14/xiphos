// SPDX-License-Identifier: GPL-2.0-or-later
import SwiftUI
import AppKit
import UniformTypeIdentifiers

struct SearchPane: View {
    @ObservedObject var model: SearchModel
    let hostBusy: Bool
    let canRead: Bool
    let onRead: (SearchHit) -> Void
    let onLink: (String, String, String) -> Void
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            VStack(alignment: .leading, spacing: 6) {
                Picker("Module", selection: $model.module) {
                    ForEach(model.modules) { Text($0.id).tag($0.id) }
                }.accessibilityLabel("Search module")
                Menu("Saved search lists") {
                    Button("Load from Xiphos profile…") { chooseSearchProfile() }
                    if model.savedSearchLists.moduleLists.contains(where: { $0.label != nil }) {
                        Section("Module lists") {
                            ForEach(Array(model.savedSearchLists.moduleLists.enumerated()), id: \.offset) { item in
                                if let label = item.element.label {
                                    Button(label.isEmpty ? "(Empty label)" : label) { model.applySavedModuleList(at: item.offset) }
                                }
                            }
                        }
                    }
                    if model.savedSearchLists.ranges.contains(where: { $0.label != nil }) {
                        Section("Ranges") {
                            ForEach(Array(model.savedSearchLists.ranges.enumerated()), id: \.offset) { item in
                                if let label = item.element.label {
                                    Button(label.isEmpty ? "(Empty label)" : label) { model.applySavedRange(at: item.offset) }
                                }
                            }
                        }
                    }
                }.accessibilityLabel("Saved search lists")
                Toggle("Search multiple modules", isOn: $model.searchMultipleModules)
                if model.searchMultipleModules {
                    Button("Add selected module") { model.addModule() }
                        .disabled(model.selectedModules.count >= 32)
                    if model.selectedModules.isEmpty {
                        Text("Add modules in the order to search.").font(.caption).foregroundStyle(.secondary)
                    } else {
                        ScrollView {
                            VStack(alignment: .leading, spacing: 4) {
                                ForEach(Array(model.selectedModules.enumerated()), id: \.offset) { item in
                                    HStack {
                                        Text("\(item.offset + 1). \(item.element)")
                                        Spacer()
                                        Button { model.moveModule(at: item.offset, by: -1) } label: { Image(systemName: "chevron.up") }
                                            .disabled(item.offset == 0).accessibilityLabel("Move \(item.element) at position \(item.offset + 1) up")
                                        Button { model.moveModule(at: item.offset, by: 1) } label: { Image(systemName: "chevron.down") }
                                            .disabled(item.offset + 1 == model.selectedModules.count).accessibilityLabel("Move \(item.element) at position \(item.offset + 1) down")
                                        Button { model.removeModule(at: item.offset) } label: { Image(systemName: "minus.circle") }
                                            .accessibilityLabel("Remove \(item.element) at position \(item.offset + 1)")
                                    }.buttonStyle(.borderless)
                                }
                            }
                        }.frame(height: min(120, CGFloat(model.selectedModules.count) * 28))
                    }
                }
                TextField("Search text", text: $model.expression).onSubmit { if !hostBusy { model.search() } }
                    .accessibilityLabel("Search text")
                Picker("Match", selection: $model.mode) {
                    Text("Phrase").tag(CoreSearchMode.phrase)
                    Text("Words (automatic)").tag(CoreSearchMode.words)
                    Text("Regular expression").tag(CoreSearchMode.regularExpression)
                    Text("Lemma").tag(CoreSearchMode.lemma)
                    Text("Morphology").tag(CoreSearchMode.morphology)
                    Text("Footnote text").tag(CoreSearchMode.footnote)
                    Text("Multiword").tag(CoreSearchMode.multiword)
                    Text("Indexed query").tag(CoreSearchMode.indexed)
                }.accessibilityLabel("Search match mode")
                Picker("Scope", selection: $model.scopeChoice) {
                    ForEach(SearchScopeChoice.allCases) { Text($0.rawValue).tag($0) }
                }.accessibilityLabel("Search scope")
                if model.scopeChoice == .bounds {
                    Picker("Bounds from", selection: $model.boundsModule) {
                        ForEach(model.modules) { Text($0.id).tag($0.id) }
                    }.accessibilityLabel("Search bounds module")
                    TextField("John 1:1 - John 1:3", text: $model.bounds).accessibilityLabel("Search bounds")
                }
                Toggle("Case sensitive", isOn: $model.caseSensitive)
                Toggle("Set annotation options", isOn: $model.setAnnotationOptions)
                    .disabled(model.mode == .phrase)
                    .help("Otherwise retain the previous search annotation settings. Phrase searches always retain prior settings.")
                if model.setAnnotationOptions {
                    VStack(alignment: .leading) {
                        Toggle("Include Strong’s annotations", isOn: $model.searchStrongs)
                        Toggle("Include morphology annotations", isOn: $model.searchMorphology)
                        Toggle("Include footnotes", isOn: $model.searchFootnotes)
                    }.disabled(model.mode == .phrase)
                }
                Button("Reset search annotations") { model.resetSearchAnnotations() }
                    .help("Clear retained search-only settings without changing reading preferences or previous results.")
                if model.mode == .phrase {
                    Text("Phrase search keeps prior annotation settings.").font(.caption).foregroundStyle(.secondary)
                }
            }.disabled(model.isBusy || hostBusy)
            HStack {
                if model.isSearching || model.isExporting {
                    Button("Stop") { model.stop() }.accessibilityLabel(model.isExporting ? "Stop export" : "Stop search")
                } else {
                    Button("Search") { model.search() }.disabled(!model.canSearch || hostBusy)
                }
                Spacer()
                Button("Clear last") { model.clearScope() }
                    .disabled(model.isBusy || hostBusy || !model.scope.present)
                    .help("Clear the scope used by Search within last results")
            }
            HStack {
                Menu("Export") {
                    Button("Copy selected passage") { export(all: false, save: false) }.disabled(!model.canExportSelected)
                    Button("Save selected passage…") { export(all: false, save: true) }.disabled(!model.canExportSelected)
                    Divider()
                    Button("Copy all search matches") { export(all: true, save: false) }.disabled(!model.canExportAll)
                    Button("Save all search matches…") { export(all: true, save: true) }.disabled(!model.canExportAll)
                    Divider()
                    Picker("Format", selection: $model.exportFormat) {
                        ForEach(SearchExportFormat.allCases) { Text($0.rawValue).tag($0) }
                    }.accessibilityLabel("Search export format")
                    Toggle("Include scripture", isOn: $model.exportScripture)
                    Toggle("Include search details", isOn: $model.exportSearchContext)
                    if model.hasIncompleteExportSource {
                        Toggle("Export returned matches from incomplete search", isOn: $model.exportPartialSource)
                            .disabled(!model.exportSearchContext)
                            .help("Enable Include search details to label incomplete output. Only returned matches are exported; this choice resets for each new search result.")
                    }
                    if model.exportFormat == .html {
                        Toggle("Embed PNG images", isOn: $model.exportImages).disabled(!model.exportScripture)
                    }
                }.disabled(model.isBusy || hostBusy || model.hits.isEmpty).accessibilityLabel("Export search results")
                Spacer()
            }
            if model.isSearching || model.isExporting {
                ProgressView(value: Double(model.progress?.percent ?? 0), total: 100)
                    .accessibilityLabel(model.isExporting ? "Export progress" : "Search progress")
            }
            Text(model.message).font(.caption).foregroundStyle(.secondary).accessibilityLabel("Search status: \(model.message)")
            VSplitView {
                VStack(spacing: 4) {
                    List(selection: Binding(get: { model.selection }, set: { model.select($0) })) {
                        if let batch = model.batchResult {
                            ForEach(batch.requestedModules.indices, id: \.self) { index in
                                Section {
                                    ForEach(model.hits.filter { $0.group == index }) { hit in resultRow(hit) }
                                    let status = model.groupMessage(index)
                                    if !status.isEmpty { Text(status).font(.caption).foregroundStyle(.secondary) }
                                } header: {
                                    Text("\(batch.requestedModules[index]) • group \(index + 1)")
                                }
                            }
                        } else {
                            ForEach(model.hits) { hit in resultRow(hit) }
                        }
                    }.accessibilityLabel("Search results").disabled(model.isSearching || model.isClearing || model.isExporting || hostBusy)
                    Button("Read in main pane") {
                        if let hit = model.hits.first(where: { $0.id == model.selection }) { onRead(hit) }
                    }.disabled(model.isBusy || hostBusy || !canRead || model.selection == nil)
                }.frame(minHeight: 90)
                if let entry = model.preview, let source = model.previewSource {
                    VStack(alignment: .leading, spacing: 3) {
                        Text("\(source.module) • \(entry.reference)").font(.caption)
                        PassageView(html: entry.headingHTML + entry.html, css: entry.lexical.css, anchor: nil,
                            onScrollFailure: { _ in }, onLink: { onLink($0, source.module, entry.reference) })
                            .accessibilityLabel("Search result preview")
                    }.frame(minHeight: 100)
                } else if model.isPreviewLoading { ProgressView("Loading preview…").frame(minHeight: 100) }
            }
        }.padding(8)
    }
    private func resultRow(_ hit: SearchHit) -> some View {
        Text(hit.reference).tag(hit.id).contentShape(Rectangle())
            .onTapGesture(count: 2) { if canRead && !hostBusy { onRead(hit) } }
    }
    private func chooseSearchProfile() {
        let panel = NSOpenPanel()
        panel.title = "Load saved search lists"
        panel.message = "Choose a Xiphos settings file containing named module lists and ranges."
        panel.prompt = "Load"
        panel.canChooseFiles = true; panel.canChooseDirectories = false; panel.allowsMultipleSelection = false
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            model.loadSearchLists(path: url.path)
        }
    }
    private func export(all: Bool, save: Bool) {
        model.exportResults(allMatches: all, forClipboard: !save) { payload in
            if save { chooseSaveFile(payload) }
            else {
                do {
                    try SearchExportDestination.copy(payload)
                    model.exportDestinationMessage(payload.selection == .visible ? "Selected passage copied." : "Search matches copied.")
                } catch { model.exportDestinationMessage(error.localizedDescription) }
            }
        }
    }
    private func chooseSaveFile(_ payload: SearchExportPayload) {
        let panel = NSSavePanel()
        panel.title = payload.selection == .visible ? "Save selected search passage" : "Save search matches"
        panel.message = "Save the search results from when you chose Export."
        panel.allowedContentTypes = payload.format == .html ? [.html] : [.plainText]
        panel.nameFieldStringValue = payload.format == .html ? "Xiphos search results.html" : "Xiphos search results.txt"
        panel.begin { response in
            guard response == .OK, let url = panel.url else { return }
            Task {
                do {
                    try await SearchExportDestination.save(payload, to: url)
                    model.exportDestinationMessage(payload.format == .html ? "HTML document saved." : "Search text saved.")
                } catch { model.exportDestinationMessage(error.localizedDescription) }
            }
        }
    }

}
