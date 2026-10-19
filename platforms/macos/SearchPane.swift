// SPDX-License-Identifier: GPL-2.0-or-later
import SwiftUI

struct SearchPane: View {
    @ObservedObject var model: SearchModel
    let hostBusy: Bool
    let canRead: Bool
    let onRead: (String) -> Void
    let onLink: (String, String, String) -> Void
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            VStack(alignment: .leading, spacing: 6) {
                Picker("Module", selection: $model.module) {
                    ForEach(model.modules) { Text($0.id).tag($0.id) }
                }.accessibilityLabel("Search module")
                TextField("Search text", text: $model.expression).onSubmit { if !hostBusy { model.search() } }
                    .accessibilityLabel("Search text")
                Picker("Match", selection: $model.mode) {
                    Text("Phrase").tag(CoreSearchMode.phrase)
                    Text("Words").tag(CoreSearchMode.words)
                    Text("Regular expression").tag(CoreSearchMode.regularExpression)
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
            }.disabled(model.isBusy || hostBusy)
            HStack {
                if model.isSearching {
                    Button("Stop") { model.stop() }.accessibilityLabel("Stop search")
                } else {
                    Button("Search") { model.search() }.disabled(!model.canSearch || hostBusy)
                }
                Spacer()
                Button("Clear last") { model.clearScope() }
                    .disabled(model.isBusy || hostBusy || !model.scope.present)
                    .help("Clear the scope used by Search within last results")
            }
            if model.isSearching {
                ProgressView(value: Double(model.progress?.percent ?? 0), total: 100)
                    .accessibilityLabel("Search progress")
            }
            Text(model.message).font(.caption).foregroundStyle(.secondary).accessibilityLabel("Search status: \(model.message)")
            VSplitView {
                VStack(spacing: 4) {
                    List(selection: Binding(get: { model.selection }, set: { model.select($0) })) {
                        ForEach(model.hits) { hit in
                            Text(hit.reference).tag(hit.id).contentShape(Rectangle())
                                .onTapGesture(count: 2) { if canRead && !hostBusy { onRead(hit.reference) } }
                        }
                    }.accessibilityLabel("Search results").disabled(model.isSearching || model.isClearing || hostBusy)
                    Button("Read in main pane") {
                        if let hit = model.hits.first(where: { $0.id == model.selection }) { onRead(hit.reference) }
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
}
