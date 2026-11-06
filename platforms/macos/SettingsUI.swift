// SPDX-License-Identifier: GPL-2.0-or-later
import SwiftUI

@MainActor final class NativeAppPreferences: ObservableObject {
    private let store: UserDefaults
    private let prefix: String
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
    @Published var commentary: String { didSet { store.set(commentary, forKey: prefix + "commentary") } }
    init(store: UserDefaults = .standard, profile: String) {
        self.store = store; prefix = "NativeAppPreferences:" + profile + ":"
        restoreSession = store.object(forKey: prefix + "restore") as? Bool ?? true
        let seconds = store.integer(forKey: prefix + "recovery")
        recoverySeconds = [5,15,30,60].contains(seconds) ? seconds : 5
        let savedLanguage = store.string(forKey: prefix + "language") ?? "system"
        language = ["system","en","fr","de","es"].contains(savedLanguage) ? savedLanguage : "system"
        bible = store.string(forKey: prefix + "bible") ?? ""
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

/// Presentation of the existing session and FIFO services; no second settings engine.
@MainActor struct NativeSettingsView: View {
    @ObservedObject var delegate: SessionAppDelegate
    var body: some View {
        ScrollView {
            if let controller = delegate.controller {
                SessionSettingsView(controller: controller, delegate: delegate, preferences: delegate.preferences)
            } else {
                Text("Settings will be available when the reading session is ready.").padding()
            }
        }.frame(minWidth: 620, idealWidth: 680, minHeight: 540, idealHeight: 680)
    }
}

@MainActor private struct SessionSettingsView: View {
    @ObservedObject var controller: NativeSessionController
    let delegate: SessionAppDelegate
    @ObservedObject var preferences: NativeAppPreferences
    var body: some View {
        VStack(alignment: .leading) {
            if let model = controller.records.first(where: { $0.key == controller.activeKey })?.model {
                ReadingSettingsView(model: model, controller: controller)
                    .id(controller.activeKey)
            } else {
                Text("Open a reading window to change reading settings.")
                Button("New Reading Window") { delegate.openNewWindow() }
            }
            Divider()
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
                    if let model = controller.records.first?.model {
                        if !preferences.bible.isEmpty && !model.modules.contains(where: {$0.id == preferences.bible}) {
                            Text(preferences.bible + " (unavailable)").tag(preferences.bible)
                        }
                        ForEach(model.modules.filter { $0.type == "Biblical Texts" }) { Text($0.id).tag($0.id) }
                    }
                }
                Picker("Default commentary for new windows", selection: $preferences.commentary) {
                    Text("None").tag("")
                    if let model = controller.records.first?.model {
                        if !preferences.commentary.isEmpty && !model.modules.contains(where: {$0.id == preferences.commentary}) {
                            Text(preferences.commentary + " (unavailable)").tag(preferences.commentary)
                        }
                        ForEach(model.modules.filter { $0.type == "Commentaries" }) { Text($0.id).tag($0.id) }
                    }
                }
                Toggle("Restore reading session on launch", isOn: $preferences.restoreSession)
                Picker("Recovery autosave interval", selection: $preferences.recoverySeconds) {
                    Text("5 seconds").tag(5); Text("15 seconds").tag(15)
                    Text("30 seconds").tag(30); Text("60 seconds").tag(60)
                }
            }
            Text("Startup and recovery changes take effect next launch. Module defaults apply to new windows; restored windows retain their saved choices.")
                .font(.callout).foregroundStyle(.secondary)
            Divider()
            Text("Library and study preferences").font(.headline)
            HStack {
                Button("Module Library…") { delegate.openModuleLibrary() }
                Button("Parallel Reading Sets…") { delegate.openParallelSets() }
                Button("BibleSync…") { delegate.openSync() }
            }
            HStack {
                Button("Personal Verse Notes…") { delegate.openPersonalNotes() }
                Button("Bookmarks and Imports…") { delegate.openBookmarks() }
            }
            Text("Sessions restore the selected modules, reading options and window layout. Recovery also protects unsaved study drafts.")
                .font(.callout).foregroundStyle(.secondary)
            Text("BibleSync shares passages. Open BibleSync to review its settings and connect explicitly.")
                .font(.callout).foregroundStyle(.secondary)
        }.padding(20)
        .onChange(of: preferences.bible) { controller.preferredBible = $0 }
        .onChange(of: preferences.commentary) { controller.preferredCommentary = $0 }
    }
}

@MainActor private struct ReadingSettingsView: View {
    @ObservedObject var model: ReaderModel
    let controller: NativeSessionController
    @State private var message = ""
    @State private var saving = false
    @State private var lexicons = false
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
                Button("Lexicon dictionaries…") { lexicons = true }.disabled(model.lexiconPreferences == nil)
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
        .sheet(isPresented: $lexicons) {
            if let preferences = model.lexiconPreferences {
                LexiconPreferencesView(model: model, preferences: preferences)
            }
        }
    }
}
