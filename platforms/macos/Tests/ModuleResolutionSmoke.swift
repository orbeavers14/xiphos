// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main struct ModuleResolutionSmoke {
    static func main() async throws {
        guard CommandLine.arguments.count == 2 else { fatalError("Generated alias fixture required") }
        let root = CommandLine.arguments[1]
        let worker = try CoreWorker(root: root)
        let tab = try await worker.createTab()
        let alias = try await worker.resolveModuleName(tab: tab, requested: "Shared")
        guard alias == ModuleNameResolution(requested: "Shared", module: "BSecond", viaAbbreviation: true),
              try await worker.resolveModuleName(tab: tab, requested: "文字 café").module == "Unicode",
              try await worker.resolveModuleName(tab: tab, requested: "AFirst") == ModuleNameResolution(requested: "AFirst", module: "AFirst", viaAbbreviation: false),
              try await worker.resolveModuleName(tab: tab, requested: "Missing") == ModuleNameResolution(requested: "Missing", module: nil, viaAbbreviation: false),
              try await worker.history(tab).count == 0 else { fatalError("Copied shared alias resolution or state isolation") }
        for bad in ["", "bad\0tail", String(repeating: "x", count: 4097)] {
            do { _ = try await worker.resolveModuleName(tab: tab, requested: bad); fatalError("Invalid query accepted") }
            catch is CoreError { }
        }
        let pending = Task { try await worker.resolveModuleName(tab: tab, requested: "Shared") }
        pending.cancel()
        guard try await pending.value == alias else { fatalError("Accepted read-only resolution did not drain") }
        try await worker.closeTab(tab)
        do { _ = try await worker.resolveModuleName(tab: tab, requested: "Shared"); fatalError("Closed tab accepted") }
        catch is CoreError { }
        await worker.shutdown()
        guard alias.requested == "Shared", alias.module == "BSecond" else { fatalError("Owned alias result after shutdown") }
        let bad = try CoreWorker(root: root + "/bad-metadata"), badTab = try await bad.createTab()
        guard try await bad.resolveModuleName(tab: badTab, requested: "Bad").module == "Bad" else { fatalError("Exact ID consulted invalid alias metadata") }
        do { _ = try await bad.resolveModuleName(tab: badTab, requested: "UnknownAlias"); fatalError("Invalid metadata accepted") }
        catch is CoreError { }
        try await bad.closeTab(badTab); await bad.shutdown()
        print("Swift6 copied module resolution, optional identity, Unicode, validation and shutdown passed")
    }
}
