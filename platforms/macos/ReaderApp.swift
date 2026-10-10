// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import SwiftUI
import WebKit

@MainActor
final class ReaderModel: ObservableObject {
    @Published var modules: [InstalledModule] = []
    @Published var selection: String?
    @Published var reference = "John 3:16"
    @Published var renderedReference = ""
    @Published var html = ""
    @Published var message = ""
    private var reader: CoreReader?
    init() {
        do {
            reader = try CoreReader(root: ProcessInfo.processInfo.environment["XIPHOS_MODULE_ROOT"])
            modules = reader!.modules
            selection = modules.first(where: { $0.type == "Biblical Texts" })?.id
            if modules.isEmpty { message = "No installed SWORD modules found. Install modules with the existing Xiphos application, then reopen this preview." }
            else if selection == nil { message = "Select a Bible or commentary module to read a verse." }
            else { read() }
        } catch { message = error.localizedDescription }
    }
    func read() {
        guard let reader, let selection else { return }
        do {
            let entry = try reader.read(module: selection, reference: reference)
            html = entry.html
            renderedReference = entry.reference
            message = entry.html.isEmpty ? "This module has no text at this reference." : ""
        } catch {
            html = ""
            renderedReference = ""
            message = error.localizedDescription
        }
    }
}

struct PassageView: NSViewRepresentable {
    let html: String
    final class Coordinator: NSObject, WKNavigationDelegate {
        var loaded: String?
        func webView(_ webView: WKWebView, decidePolicyFor action: WKNavigationAction,
                     decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
            // Semantic link dispatch is a later parity slice. Keep this preview
            // offline and do not accidentally navigate away from the study pane.
            decisionHandler(action.navigationType == .other && action.request.url?.scheme == "about" ? .allow : .cancel)
        }
    }
    func makeCoordinator() -> Coordinator { Coordinator() }
    func makeNSView(context: Context) -> WKWebView {
        let config = WKWebViewConfiguration()
        config.websiteDataStore = .nonPersistent()
        config.defaultWebpagePreferences.allowsContentJavaScript = false
        let view = WKWebView(frame: .zero, configuration: config)
        view.navigationDelegate = context.coordinator
        return view
    }
    func updateNSView(_ view: WKWebView, context: Context) {
        guard context.coordinator.loaded != html else { return }
        context.coordinator.loaded = html
        view.loadHTMLString("""
            <!doctype html><html><head><meta charset="utf-8">
            <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'">
            <style>:root { color-scheme: light dark; } body { font: 18px Georgia, serif; line-height: 1.55; margin: 24px; }</style>
            </head><body>\(html)</body></html>
            """, baseURL: nil)
    }
}

struct ReaderWindow: View {
    @StateObject private var model = ReaderModel()
    var body: some View {
        HSplitView {
            List(selection: $model.selection) {
                ForEach(Array(Set(model.modules.map(\.type))).sorted(), id: \.self) { type in
                    Section(type) {
                        ForEach(model.modules.filter { $0.type == type }) { module in
                            VStack(alignment: .leading, spacing: 3) {
                                Text(module.id)
                                Text(module.description).font(.caption).foregroundStyle(.secondary).lineLimit(2)
                            }.tag(module.id)
                        }
                    }
                }
            }.frame(minWidth: 190, idealWidth: 230, maxWidth: 320)
                .accessibilityLabel("Installed SWORD modules")
            VStack(alignment: .leading, spacing: 0) {
                HStack {
                    TextField("Passage reference", text: $model.reference).onSubmit { model.read() }
                        .accessibilityLabel("Passage reference")
                    Button("Read") { model.read() }.disabled(model.selection == nil)
                }.padding(12)
                Divider()
                if !model.renderedReference.isEmpty {
                    Text(model.renderedReference).font(.headline).padding(12)
                }
                PassageView(html: model.html)
                if !model.message.isEmpty {
                    Text(model.message).foregroundStyle(.secondary).padding(12)
                        .accessibilityLabel(model.message)
                }
                Divider()
                Text("Xiphos native preview • Verse reading slice").font(.caption).foregroundStyle(.secondary).padding(8)
            }.frame(minWidth: 450)
        }.frame(minWidth: 700, minHeight: 450)
            .onChange(of: model.selection) { _ in model.read() }
    }
}

@main
struct XiphosNativePreview: App {
    var body: some Scene {
        WindowGroup("Xiphos Native Preview") { ReaderWindow() }
    }
}
