// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation

struct NativeLibraryContext: Sendable {
    let worker: CoreWorker
    let managedRoot: String?
}
@MainActor enum NativeLibrarySetup {
    static func preferenceKey(profile: URL) -> String { "NativeModuleRoot:" + profile.path }
    static func create(at root: URL, requireMissing: Bool = true) throws {
        if requireMissing && FileManager.default.fileExists(atPath:root.path) { throw SessionHostError(message:"Choose a new folder name for this module library.") }
        try FileManager.default.createDirectory(at:root.appendingPathComponent("mods.d",isDirectory:true),withIntermediateDirectories:true)
    }
    static func open(profile: URL,explicit: String?,stored: String?,isolated: Bool) async throws -> NativeLibraryContext {
        if let root = [explicit,stored].compactMap({ $0 }).first(where: { !$0.isEmpty }) {
            return NativeLibraryContext(worker:try CoreWorker(root:root),managedRoot:root)
        }
        let root = profile.appendingPathComponent("modules",isDirectory:true)
        if isolated || FileManager.default.fileExists(atPath:root.appendingPathComponent("mods.d").path) {
            try create(at:root,requireMissing:false)
            return NativeLibraryContext(worker:try CoreWorker(root:root.path),managedRoot:root.path)
        }
        // Preserve existing auto-discovered libraries. Installation into a whole
        // library is enabled only after the user chooses an explicit folder.
        let discovered = try CoreWorker()
        let modules = try await discovered.modules()
        if !modules.isEmpty { return NativeLibraryContext(worker:discovered,managedRoot:nil) }
        await discovered.shutdown()
        try create(at:root,requireMissing:false)
        return NativeLibraryContext(worker:try CoreWorker(root:root.path),managedRoot:root.path)
    }
}
