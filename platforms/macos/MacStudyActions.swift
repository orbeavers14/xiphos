// SPDX-License-Identifier: GPL-2.0-or-later
import AppKit
import AVFoundation
import SwiftUI

@MainActor enum MacStudyActions {
    private static let speech = AVSpeechSynthesizer()

    static func speak(_ text: String) {
        let spoken = String(text.prefix(16_384)).trimmingCharacters(in: .whitespacesAndNewlines)
        guard !spoken.isEmpty else { return }
        stopSpeech()
        speech.speak(AVSpeechUtterance(string: spoken))
    }

    static func stopSpeech() {
        if speech.isSpeaking { speech.stopSpeaking(at: .immediate) }
    }

    static func selectionURL(_ text: String, translating: Bool) -> URL? {
        guard !text.isEmpty, text.utf8.count <= 4096, !text.utf8.contains(0) else { return nil }
        var url = URLComponents()
        url.scheme = "https"
        url.host = translating ? "translate.google.com" : "maps.apple.com"
        url.path = "/"
        url.queryItems = translating
            ? [URLQueryItem(name: "sl", value: "auto"),
               URLQueryItem(name: "tl", value: Locale.current.language.languageCode?.identifier ?? "en"),
               URLQueryItem(name: "text", value: text), URLQueryItem(name: "op", value: "translate")]
            : [URLQueryItem(name: "q", value: text)]
        return url.url
    }

    static func openSelection(_ text: String, translating: Bool) {
        if let url = selectionURL(text, translating: translating) { NSWorkspace.shared.open(url) }
    }

    static func about() {
        let credits = Bundle.main.url(forResource: "AUTHORS", withExtension: nil)
            .flatMap { try? String(contentsOf: $0, encoding: .utf8) } ?? "The Xiphos contributors"
        NSApp.orderFrontStandardAboutPanel(options: [
            .applicationName: "Xiphos",
            .credits: NSAttributedString(string: credits)
        ])
    }
}

struct NativeHelpView: View {
    @State private var showLicense = false
    private var license: String {
        Bundle.main.url(forResource: "COPYING", withExtension: nil)
            .flatMap { try? String(contentsOf: $0, encoding: .utf8) } ?? ""
    }

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 14) {
                Text("Xiphos Help").font(.largeTitle)
                Group {
                    topic("Reading", "Choose an installed Bible and enter a passage. Use the Passage menu for export, printing, and separate reading windows.")
                    topic("Search and comparison", "Use the Search tab to look up words, phrases, or lemmas. Open a verse result in Parallel Reading to compare installed translations.")
                    topic("Modules and personal study", "Use File → Module Library to install or archive modules. Use New Study Module in the sidebar to create a book, organizer, or personal commentary.")
                    topic("Settings and recovery", "Open Xiphos → Settings for appearance, default modules, reading choices, and BibleSync. Save the reading session from the File menu.")
                }
                Link("Open the upstream Xiphos manual", destination: URL(string: "https://xiphos.org/help/")!)
                DisclosureGroup("Software license", isExpanded: $showLicense) {
                    Text(license).font(.system(.caption, design: .monospaced)).textSelection(.enabled)
                }
            }.frame(maxWidth: .infinity, alignment: .leading).padding(20)
        }.frame(minWidth: 560, minHeight: 440)
    }

    private func topic(_ title: LocalizedStringKey, _ detail: LocalizedStringKey) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(title).font(.headline).accessibilityAddTraits(.isHeader)
            Text(detail)
        }
    }
}
