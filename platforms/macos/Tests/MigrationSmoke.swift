// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
@main struct MigrationSmoke {
    @MainActor static func check(_ condition: Bool,_ message: String) { if !condition { fatalError(message) } }
    @MainActor static func main() async throws {
        _ = NSApplication.shared;NSApplication.shared.setActivationPolicy(.prohibited)
        let root = URL(fileURLWithPath:CommandLine.arguments[1]);check(root.lastPathComponent == "legacy-migration-fixture","Named fixture only")
        let worker = try CoreWorker(root:root.appendingPathComponent("library").path)
        if CommandLine.arguments.count == 3 {
            let restored = NativeSessionController(worker:worker,path:root.appendingPathComponent("native-ui.xml").path,showsWindows:false)
            try await restored.start();check(restored.records.count == 2,"Fresh-process converted reading windows restore")
            check(restored.records.first?.model.syncTarget?.module == "Text","Fresh-process original reading module retained")
            for row in restored.records { row.model.close() };await worker.shutdown();return
        }
        let model = NativeMigrationModel(worker:worker)
        model.settings = root.appendingPathComponent("settings.xml").path;model.tabs = root.appendingPathComponent("tabs.xml").path;model.options = root.appendingPathComponent("modops.conf").path
        model.prepare();await model.waitUntilIdle()
        check(model.preview?.ready == true && model.preview?.tabs == 2 && !model.needsPreview,"Owned native migration preview copies complete conversion")
        check(model.preview?.settings.contains(where: { $0.contains("layout/hight") && $0.contains("preserved") }) == true,"Unsupported layout values are disclosed as preserved, not silently applied")
        let window = NativeMigrationWindow(model:model);window.window.orderOut(nil);check(window.window.contentView != nil,"Native migration review screen constructs")
        let protected = root.appendingPathComponent("current-session.xml").path
        model.currentSessionPath = { protected }
        model.exportSession(path:protected);await model.waitUntilIdle();check(!FileManager.default.fileExists(atPath:protected),"Even an unsaved current-session path cannot be used as migration output")
        let backup = root.appendingPathComponent("native-ui.bundle").path
        model.savePackage(path:backup);await model.waitUntilIdle();check(FileManager.default.fileExists(atPath:backup),"Native backup uses shared verified package storage")
        var activated: String?
        model.onUseSession = { path in activated = path }
        model.useOnRestart = true
        let destination = root.appendingPathComponent("native-ui.xml").path
        model.exportSession(path:destination);await model.waitUntilIdle();check(activated == destination,"Startup selection happens only after successful native export")
        check(try await worker.readNativeSession(path:destination).windows.count == 2,"Exported native file has every converted window")
        model.restoreOriginals(folder:root.appendingPathComponent("native-ui-originals").path);await model.waitUntilIdle()
        check(try Data(contentsOf:root.appendingPathComponent("native-ui-originals/settings.xml")) == Data(contentsOf:root.appendingPathComponent("settings.xml")),"Native restore preserves exact original settings bytes")
        model.tabs += ".changed";let guardFile = root.appendingPathComponent("should-not-exist.xml").path
        model.exportSession(path:guardFile);await model.waitUntilIdle();check(!FileManager.default.fileExists(atPath:guardFile),"Changed selections require a new preview")
        model.package = backup;model.mappingText = "";model.prepare();await model.waitUntilIdle();check(model.preview?.ready == true,"Saved package reopens and resolves against the current SDK")
        let original = try Data(contentsOf:root.appendingPathComponent("settings.xml"))
        try Data("Changed externally".utf8).write(to:root.appendingPathComponent("settings.xml"))
        let reloaded = try await worker.previewMigration(settings:"",package:backup)
        check(reloaded.ready,"A package does not depend on old machine paths or changed originals")
        try original.write(to:root.appendingPathComponent("settings.xml"))
        model.mappingText = "bad mapping";model.prepare();await model.waitUntilIdle();check(model.preview == nil && model.message.contains("per line"),"Invalid mapping cannot reuse a stale preview")
        await worker.shutdown()
        print("Native legacy migration preview, backup, restore, explicit activation and stale-selection checks passed")
    }
}
