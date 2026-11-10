// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import SwiftUI

struct ReadingStyle: Codable, Equatable {
    enum Font: String, Codable, CaseIterable {
        case serif, sansSerif, monospace

        var title: LocalizedStringKey {
            switch self {
            case .serif: "Serif"
            case .sansSerif: "Sans Serif"
            case .monospace: "Monospace"
            }
        }
        var css: String {
            switch self {
            case .serif: "Georgia, serif"
            case .sansSerif: "Helvetica, sans-serif"
            case .monospace: "Menlo, monospace"
            }
        }
    }

    var font: Font = .serif
    var size = 18
    var customColors = false
    var foreground = "#242424"
    var background = "#FFFFFF"

    @MainActor static var storageKey: String {
        "NativeReadingStyles:" + ((try? NativeSessionController.profileFolder().path) ?? "default")
    }

    static func styles(from data: Data) -> [String: Self] {
        guard data.count <= 256 * 1024 else { return [:] }
        return (try? JSONDecoder().decode([String: Self].self, from: data)) ?? [:]
    }

    static func css(from data: Data, module: String?) -> String {
        let saved = styles(from: data)
        guard let style = module.flatMap({ saved[$0] }) ?? saved[""] else { return "" }
        let colors = style.customColors && isHex(style.foreground) && isHex(style.background)
            ? "color:\(style.foreground);background-color:\(style.background);" : ""
        return "body{font-family:\(style.font.css);font-size:\(min(36,max(12,style.size)))px;\(colors)}"
    }

    static func isHex(_ value: String) -> Bool {
        value.utf8.count == 7 && value.first == "#" &&
            value.utf8.dropFirst().allSatisfy { (48...57).contains($0) || (65...70).contains($0) || (97...102).contains($0) }
    }
}

@MainActor struct ReadingStyleSettings: View {
    let modules: [InstalledModule]
    @AppStorage(ReadingStyle.storageKey) private var stored = Data()
    @State private var module = ""
    @State private var draft = ReadingStyle()
    @State private var message = ""

    var body: some View {
        Form {
            Picker("Apply style to", selection: $module) {
                Text("All modules by default").tag("")
                ForEach(modules) { Text($0.id).tag($0.id) }
            }
            Picker("Font", selection: $draft.font) {
                ForEach(ReadingStyle.Font.allCases, id: \.self) { font in
                    Text(font.title).tag(font)
                }
            }
            Stepper("Text size: \(draft.size)", value: $draft.size, in: 12...36)
            Toggle("Use custom text and background colors", isOn: $draft.customColors)
            if draft.customColors {
                ColorPicker("Text", selection: color(\.foreground), supportsOpacity: false)
                ColorPicker("Background", selection: color(\.background), supportsOpacity: false)
            }
            HStack {
                Button("Reset") { save(nil); load() }
                Button("Apply") { save(draft) }.keyboardShortcut(.defaultAction)
            }
            if !message.isEmpty { Text(message).foregroundStyle(.secondary) }
        }
        .onAppear { load() }
        .onChange(of: module) { _, _ in load() }
    }

    private func load() {
        let styles = ReadingStyle.styles(from: stored)
        draft = styles[module] ?? styles[""] ?? ReadingStyle()
        draft.size = min(36,max(12,draft.size))
        message = ""
    }

    private func save(_ style: ReadingStyle?) {
        var styles = ReadingStyle.styles(from: stored)
        styles[module] = style
        guard let data = try? JSONEncoder().encode(styles), data.count <= 256 * 1024 else {
            message = "Reading styles could not be saved."
            return
        }
        stored = data
        message = "Reading style updated."
    }

    private func color(_ key: WritableKeyPath<ReadingStyle,String>) -> Binding<Color> {
        Binding(get: {
            let value = UInt32(draft[keyPath:key].dropFirst(),radix:16) ?? 0
            return Color(red: Double((value >> 16) & 255)/255,
                         green: Double((value >> 8) & 255)/255,
                         blue: Double(value & 255)/255)
        }, set: {
            guard let rgb = NSColor($0).usingColorSpace(.sRGB) else { return }
            draft[keyPath:key] = String(format: "#%02X%02X%02X",
                Int((rgb.redComponent*255).rounded()), Int((rgb.greenComponent*255).rounded()),
                Int((rgb.blueComponent*255).rounded()))
        })
    }
}
