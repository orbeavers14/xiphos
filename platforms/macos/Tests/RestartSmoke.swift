// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit

@main struct RestartSmoke {
    struct Manifest: Codable { let keys: [String]; let xml: String; let dictionaryDraft: String }

    @MainActor static func idle(_ model: ReaderModel) async throws {
        let deadline = Date().addingTimeInterval(15)
        while model.isBusy || model.search.isBusy {
            guard Date() < deadline else { fatalError("Restart model timeout") }
            try await Task.sleep(nanoseconds: 1_000_000)
        }
    }
    @MainActor static func main() async throws {
        guard [3,4].contains(CommandLine.arguments.count) else { fatalError("Disposable module root/output directory required") }
        let root = CommandLine.arguments[1], folder = URL(fileURLWithPath: CommandLine.arguments[2])
        guard folder.lastPathComponent == "restart-smoke-fixture" else { fatalError("Only disposable restart output can be removed") }
        let phase = CommandLine.arguments.count == 4 ? CommandLine.arguments[3] : "all"
        if phase == "all" {
            for step in ["write", "resume", "windows"] {
                let process = Process(); process.executableURL = URL(fileURLWithPath: CommandLine.arguments[0])
                process.arguments = [root, folder.path, step]
                try process.run(); process.waitUntilExit()
                guard process.terminationStatus == 0 else { fatalError("Restart subprocess failed: \(step)") }
            }
            print("Native session persisted across two fresh processes")
            return
        }
        guard ["write","resume","windows"].contains(phase) else { fatalError("Unknown restart test phase") }
        if phase == "windows" {
            _ = NSApplication.shared
            NSApplication.shared.setActivationPolicy(.prohibited)
            let windowPath = folder.appendingPathComponent("appkit.xml").path
            do {
                let worker = try CoreWorker(root: root)
                let owner = NativeSessionController(worker: worker, path: windowPath, showsWindows: false, offscreenWindows: true)
                try await owner.start(); try await idle(owner.records[0].model)
                guard let window = owner.records[0].window else { fatalError("Offscreen NSWindow missing") }
                window.setFrame(NSRect(x: 73, y: 85, width: 900, height: 650), display: false)
                window.performClose(nil)
                let deadline = Date().addingTimeInterval(10)
                while !owner.records.isEmpty {
                    guard Date() < deadline else { fatalError("NSWindow close/save timeout") }
                    try await Task.sleep(nanoseconds: 1_000_000)
                }
                guard FileManager.default.fileExists(atPath: windowPath) else { fatalError("NSWindow delegate did not save before close") }
                await worker.shutdown()
            }
            do {
                let worker = try CoreWorker(root: root)
                let owner = NativeSessionController(worker: worker, path: windowPath, showsWindows: false, offscreenWindows: true)
                try await owner.start()
                guard owner.records.count == 1, let window = owner.records[0].window,
                      window.frame.width == 900, window.frame.height == 650 else { fatalError("Native NSWindow frame not restored") }
                try await owner.prepareToQuit()
                let saved = try Data(contentsOf: URL(fileURLWithPath: windowPath))
                window.performClose(nil)
                guard owner.records.isEmpty, try Data(contentsOf: URL(fileURLWithPath: windowPath)) == saved else { fatalError("Quit close altered the prepared session") }
                await worker.shutdown()
            }
            print("Offscreen AppKit close, frame restore and prepared quit passed")
            return
        }
        if phase == "write" {
            try? FileManager.default.removeItem(at: folder)
            try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        }
        let path = folder.appendingPathComponent("session.xml").path
        let manifestURL = folder.appendingPathComponent("expectations.json")
        var savedKeys: [String] = [], savedXML = "", dictionaryDraft = ""
        if phase == "resume" {
            let manifest = try JSONDecoder().decode(Manifest.self, from: Data(contentsOf: manifestURL))
            savedKeys = manifest.keys; savedXML = manifest.xml; dictionaryDraft = manifest.dictionaryDraft
        }
        // First launch, two independently owned reading windows and host drafts.
        if phase == "write" {
            let worker = try CoreWorker(root: root)
            let owner = NativeSessionController(worker: worker, path: path, showsWindows: false)
            try await owner.start()
            let first = owner.records[0].model, second = owner.addWindow()
            try await idle(first); try await idle(second)
            first.reference = "John 3:17"; first.read(); try await idle(first)
            first.reference = "John 3:18"; first.read(); try await idle(first)
            first.navigate(-1); try await idle(first)
            first.followLink("sword://Dictionary/alpha"); try await idle(first)
            first.followLink("sword://Dictionary/beta"); try await idle(first)
            first.navigateDictionaryHistory(-1); try await idle(first)
            first.dictionaryKey = "beta"; first.lookupDictionary(); try await idle(first)
            first.showChapter = true; first.verseNumbers = false; first.highlightSelected = false
            first.reference = "unsubmitted reading draft"
            first.search.expression = "unsubmitted search draft"; first.search.scopeChoice = .last
            first.search.selectedModules = ["TextAlt","Text"]; first.search.searchMultipleModules = true
            second.chooseBook("CaptureBook"); try await idle(second)
            second.bookKey = "/unsubmitted book draft"
            dictionaryDraft = first.dictionaryKey
            savedKeys = owner.records.map(\.key)
            try await owner.prepareToQuit()
            savedXML = try String(contentsOfFile: path, encoding: .utf8)
            guard savedXML.contains("host=") else { fatalError("Host payload not stored with core snapshot") }
            for record in owner.records { record.model.close() }
            await worker.shutdown()
        }
        if phase == "write" {
            try JSONEncoder().encode(Manifest(keys: savedKeys, xml: savedXML, dictionaryDraft: dictionaryDraft)).write(to: manifestURL)
            return
        }
        // Fresh process/engine owner: rehydrate without visits or default selections.
        do {
            let worker = try CoreWorker(root: root)
            let owner = NativeSessionController(worker: worker, path: path, showsWindows: false)
            try await owner.start()
            guard owner.records.map(\.key) == savedKeys, owner.activeKey == savedKeys[1] else { fatalError("Window order/active identity lost") }
            let first = owner.records[0].model, second = owner.records[1].model
            guard first.renderedReference.contains("John"), first.reference == "unsubmitted reading draft",
                  first.showChapter, !first.verseNumbers, !first.highlightSelected,
                  first.dictionaryEntry != nil, first.dictionaryKey == dictionaryDraft,
                  first.search.expression == "unsubmitted search draft", first.search.scopeChoice == .all,
                  first.search.selectedModules == ["TextAlt","Text"], first.search.hits.isEmpty,
                  second.showingBook, second.bookDocument != nil, second.bookKey == "/unsubmitted book draft" else {
                fatalError("Restored reference=\(first.renderedReference) draft=\(first.reference) chapter=\(first.showChapter) numbers=\(first.verseNumbers) highlight=\(first.highlightSelected) dictionary=\(first.dictionaryEntry != nil) key=\(first.dictionaryKey) expected=\(dictionaryDraft) query=\(first.search.expression) scope=\(first.search.scopeChoice) modules=\(first.search.selectedModules) book=\(second.showingBook)/\(second.bookDocument != nil)/\(second.bookKey)")
            }
            let history = try await worker.history(first.sessionTab!)
            let dictionary = try await worker.dictionaryHistory(tab: first.sessionTab!)
            guard history.count == 3, history.canForward, dictionary.entries.count == 2, dictionary.canForward else { fatalError("Hydration replayed visits or lost cursors") }
            // Other platform metadata remains opaque and survives a macOS save.
            var host = SessionHostState(first)
            let foreign = "{\"version\":1,\"hosts\":{\"windows\":\"opaque foreign state\"}}"
            let encoded = try host.encoded(preserving: foreign)
            guard encoded.contains("opaque foreign state"), try SessionHostState.decode(encoded) != nil else { fatalError("Cross-host metadata was discarded") }
            host.version = 99
            do { _ = try SessionHostState.decode(try host.encoded()); fatalError("Future host version accepted") } catch {}
            // Closing one window saves only the remaining one; closing the last
            // preserves it for the next launch instead of writing an empty session.
            try await owner.closeWindow(key: savedKeys[0])
            guard owner.records.count == 1 else { fatalError("Window close ownership") }
            try await owner.closeWindow(key: savedKeys[1])
            guard owner.records.isEmpty else { fatalError("Last window close ownership") }
            let last = try String(contentsOfFile: path, encoding: .utf8)
            let emptyPath = folder.appendingPathComponent("empty.xml").path
            try await owner.save(to: emptyPath)
            let empty = try await worker.readNativeSession(path: emptyPath)
            guard empty.windows.isEmpty else { fatalError("Explicit empty Save As did not write a zero-window session") }
            try await owner.prepareToQuit()
            guard try String(contentsOfFile: path, encoding: .utf8) == last else { fatalError("Quit after last close erased session") }
            await worker.shutdown()
        }
        // Failed restore cannot be replaced implicitly during close/quit.
        do {
            let invalid = folder.appendingPathComponent("invalid.xml").path
            let corrupted = savedXML.replacingOccurrences(of: "version=\"1\"", with: "version=\"99\"")
            try corrupted.write(toFile: invalid, atomically: true, encoding: .utf8)
            let worker = try CoreWorker(root: root)
            let owner = NativeSessionController(worker: worker, path: invalid, showsWindows: false)
            do { try await owner.start(); fatalError("Future core version accepted") } catch {}
            let fresh = owner.addWindow(); try await idle(fresh)
            do { try await owner.save(); fatalError("Failed-restore source overwritten") } catch {}
            guard try String(contentsOfFile: invalid, encoding: .utf8) == corrupted else { fatalError("Failed file changed") }
            let separate = folder.appendingPathComponent("separate.xml").path
            try await owner.save(to: separate)
            guard owner.restoreError == nil, owner.path == separate else { fatalError("Explicit save-as recovery") }
            fresh.close(); await worker.shutdown()
        }
        print("Native restart, ordered windows, nonrecording hydration, host drafts, close/save and failure retention passed")
    }
}
