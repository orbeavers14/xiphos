// SPDX-License-Identifier: GPL-2.0-or-later
import SwiftUI
import AppKit

enum NativeAppearance: String, CaseIterable {
    case system, light, dark

    var appKitAppearance: NSAppearance? {
        switch self {
        case .system: nil
        case .light: NSAppearance(named: .aqua)
        case .dark: NSAppearance(named: .darkAqua)
        }
    }
}

@MainActor final class NativeAppPreferences: ObservableObject {
    private let store: UserDefaults
    private let prefix: String
    var onAppearanceChanged: ((NativeAppearance) -> Void)?
    @Published var appearance: NativeAppearance {
        didSet {
            store.set(appearance.rawValue, forKey: prefix + "appearance")
            onAppearanceChanged?(appearance)
        }
    }
    @Published var restoreSession: Bool { didSet { store.set(restoreSession, forKey: prefix + "restore") } }
    @Published var recoverySeconds: Int { didSet { store.set(recoverySeconds, forKey: prefix + "recovery") } }
    @Published var language: String {
        didSet {
            store.set(language, forKey: prefix + "language")
            if language == "system" { store.removeObject(forKey: "AppleLanguages") }
            else { store.set([language], forKey: "AppleLanguages") }
        }
    }
    @Published var bible: String { didSet { store.set(bible, forKey: prefix + "bible") } }
    @Published var devotional: String { didSet { store.set(devotional, forKey: prefix + "devotional") } }
    @Published var commentary: String { didSet { store.set(commentary, forKey: prefix + "commentary") } }
    init(store: UserDefaults = .standard, profile: String) {
        self.store = store; prefix = "NativeAppPreferences:" + profile + ":"
        appearance = store.string(forKey: prefix + "appearance").flatMap(NativeAppearance.init(rawValue:)) ?? .system
        restoreSession = store.object(forKey: prefix + "restore") as? Bool ?? true
        let seconds = store.integer(forKey: prefix + "recovery")
        recoverySeconds = [5,15,30,60].contains(seconds) ? seconds : 5
        let savedLanguage = store.string(forKey: prefix + "language") ?? "system"
        language = ["system","en","fr","de","es"].contains(savedLanguage) ? savedLanguage : "system"
        bible = store.string(forKey: prefix + "bible") ?? ""
        devotional = store.string(forKey: prefix + "devotional") ?? ""
        commentary = store.string(forKey: prefix + "commentary") ?? ""
    }
    func loadSync() -> CoreSyncSettings? {
        guard let data = store.data(forKey: prefix + "sync"), data.count <= 65536 else { return nil }
        return try? JSONDecoder().decode(CoreSyncSettings.self, from: data)
    }
    func saveSync(_ settings: CoreSyncSettings) {
        // Never persist identity, session phrase, consent or connection state.
        if let data = try? JSONEncoder().encode(settings) { store.set(data, forKey: prefix + "sync") }
    }
}

private enum SettingsSection: String, CaseIterable, Identifiable {
    case general, appearance, reading, lexicons, sync
    var id: Self { self }
    var title: LocalizedStringKey {
        switch self {
        case .general: "General"
        case .appearance: "Appearance"
        case .reading: "Reading"
        case .lexicons: "Lexicons"
        case .sync: "BibleSync"
        }
    }
    var icon: String {
        switch self {
        case .general: "gearshape"
        case .appearance: "circle.lefthalf.filled"
        case .reading: "book"
        case .lexicons: "character.book.closed"
        case .sync: "antenna.radiowaves.left.and.right"
        }
    }
}

@MainActor struct NativeSettingsView: View {
    @ObservedObject var delegate: SessionAppDelegate
    var body: some View {
        if let controller = delegate.controller {
            SessionSettingsView(controller: controller, delegate: delegate, preferences: delegate.preferences)
        } else {
            Text("Settings will be available when the reading session is ready.").padding()
        }
    }
}

@MainActor private struct SessionSettingsView: View {
    @ObservedObject var controller: NativeSessionController
    @ObservedObject var delegate: SessionAppDelegate
    @ObservedObject var preferences: NativeAppPreferences
    @State private var section: SettingsSection? = .general

    private var model: ReaderModel? {
        controller.records.first(where: { $0.key == controller.activeKey })?.model
    }

    var body: some View {
        NavigationSplitView {
            List(SettingsSection.allCases, selection: $section) { item in
                Label(item.title, systemImage: item.icon).tag(item)
            }.navigationSplitViewColumnWidth(min: 145, ideal: 165)
        } detail: {
            ScrollView {
                Group {
                    switch section ?? .general {
                    case .general: general
                    case .appearance: appearance
                    case .reading:
                        if let model { ReadingSettingsView(model: model, controller: controller).id(controller.activeKey) }
                        else { requiresWindow }
                    case .lexicons:
                        if let model, let choices = model.lexiconPreferences {
                            LexiconPreferencesView(model: model, preferences: choices, inline: true)
                                .id(controller.activeKey)
                        } else { requiresWindow }
                    case .sync:
                        if let sync = delegate.sync { NativeSyncView(model: sync, inline: true) }
                        else { Text("BibleSync settings are unavailable.").foregroundStyle(.secondary) }
                    }
                }.frame(maxWidth: .infinity, alignment: .leading).padding(20)
            }.navigationTitle((section ?? .general).title)
        }
        .frame(minWidth: 730, idealWidth: 850, minHeight: 500, idealHeight: 650)
        .onChange(of: preferences.bible) { _, bible in controller.preferredBible = bible }
        .onChange(of: preferences.devotional) { _, module in controller.preferredDevotional = module }
        .onChange(of: preferences.commentary) { _, commentary in controller.preferredCommentary = commentary }
    }

    private var requiresWindow: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Open a reading window to change these settings.")
            Button("New Reading Window") { delegate.openNewWindow() }
        }
    }

    private var appearance: some View {
        Form {
            Picker("Appearance", selection: $preferences.appearance) {
                Text("System").tag(NativeAppearance.system)
                Text("Light").tag(NativeAppearance.light)
                Text("Dark").tag(NativeAppearance.dark)
            }
            Text("Changes apply to all Xiphos windows.").foregroundStyle(.secondary)
        }
    }

    private var general: some View {
        Form {
            Picker("Interface language", selection: $preferences.language) {
                Text("System language").tag("system")
                Text("English").tag("en")
                Text("Français — partial translation").tag("fr")
                Text("Deutsch — partial translation").tag("de")
                Text("Español — partial translation").tag("es")
            }
            Text("Restart to apply the interface language. Untranslated text uses English.").font(.callout)
            Picker("Default Bible for new windows", selection: $preferences.bible) {
                Text("First available Bible").tag("")
                let bibles = model?.modules.filter { $0.type == "Biblical Texts" } ?? []
                if !preferences.bible.isEmpty && !bibles.contains(where: { $0.id == preferences.bible }) {
                    Text(preferences.bible + " (unavailable)").tag(preferences.bible)
                }
                ForEach(bibles) { Text($0.id).tag($0.id) }
            }
            Picker("Default commentary for new windows", selection: $preferences.commentary) {
                Text("None").tag("")
                let commentaries = model?.modules.filter { $0.type == "Commentaries" } ?? []
                if !preferences.commentary.isEmpty && !commentaries.contains(where: { $0.id == preferences.commentary }) {
                    Text(preferences.commentary + " (unavailable)").tag(preferences.commentary)
                }
                ForEach(commentaries) { Text($0.id).tag($0.id) }
            }
            Picker("Default daily devotional", selection: $preferences.devotional) {
                Text("First available devotional").tag("")
                let choices = model?.modules.filter(\.isDevotional) ?? []
                if !preferences.devotional.isEmpty && !choices.contains(where: { $0.id == preferences.devotional }) {
                    Text(preferences.devotional + " (unavailable)").tag(preferences.devotional)
                }
                ForEach(choices) { Text($0.id).tag($0.id) }
            }
            Toggle("Restore reading session on launch", isOn: $preferences.restoreSession)
            Picker("Recovery autosave interval", selection: $preferences.recoverySeconds) {
                Text("5 seconds").tag(5); Text("15 seconds").tag(15)
                Text("30 seconds").tag(30); Text("60 seconds").tag(60)
            }
            Text("Startup and recovery changes take effect next launch. Module defaults apply to new windows.")
                .font(.callout).foregroundStyle(.secondary)
        }
    }
}

@MainActor private struct ReadingSettingsView: View {
    @ObservedObject var model: ReaderModel
    let controller: NativeSessionController
    @State private var message = ""
    @State private var saving = false
    private func flag(_ title: LocalizedStringKey, _ option: EngineOptionFlags) -> some View {
        Toggle(title, isOn: Binding(get: { model.engineOptions?.contains(option) ?? false },
                                   set: { model.setReadingOption(option, enabled: $0) }))
            .disabled(!model.moduleOptions.declared.contains(option))
    }
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Reading settings").font(.title2)
            Text("Changes apply to the selected reading window.").foregroundStyle(.secondary)
            Form {
                Toggle("Chapter view", isOn: $model.showChapter)
                Toggle("Verse numbers", isOn: $model.verseNumbers)
                Toggle("Highlight selected verse", isOn: $model.highlightSelected)
                if let value = model.contentOptions.doubleSpace {
                    Toggle("Double space", isOn: Binding(get: { value }, set: { model.setDoubleSpace($0) }))
                }
                if let value = model.contentOptions.images {
                    Toggle("Image content", isOn: Binding(get: { value }, set: { model.setImageContent($0) }))
                }
                flag("Words of Christ in Red", .wordsInRed)
                flag("Strong’s numbers", .strongs)
                flag("Footnotes", .footnotes)
                flag("Cross-references", .crossReferences)
            }
            HStack {
                Spacer()
                Button("Save Settings and Session") {
                    saving = true; message = ""
                    Task {
                        do { try await controller.save(); message = "Settings and session saved." }
                        catch { message = "Settings could not be saved. " + error.localizedDescription }
                        saving = false
                    }
                }.keyboardShortcut("s", modifiers: .command)
            }
            if !message.isEmpty { Text(message).font(.callout).accessibilityLabel(message) }
        }.disabled(model.isBusy || saving)

    }
}
