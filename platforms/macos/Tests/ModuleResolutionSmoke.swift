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
        let passage = try await worker.resolveLink("sword://Shared/John1:1", currentModule: "AFirst")
        guard passage.module == "BSecond", passage.reference == "John1:1",
              try await worker.resolveLink("bible://文字+caf%C3%A9/John1:1", currentModule: "AFirst").module == "Unicode",
              try await worker.resolveLink("sword:///John1:1", currentModule: "Shared").module == "BSecond" else { fatalError("Typed single-passage aliases use the shared catalog") }
        guard try await worker.resolveLink("bible://Missing/John1:1", currentModule: "AFirst").module == "AFirst" else { fatalError("Unknown passage URI uses exact main fallback") }
        let contextual = try await worker.resolveStudyLinkAt(tab: tab, currentModule: "AFirst", currentReference: "John1:1", uri: "sword://Shared/John1:1")
        guard case .passage(let target) = contextual, target.module == "BSecond",
              try await worker.history(tab).count == 0 else { fatalError("Contextual passage alias read must not navigate") }
        for (uri, module) in [("sword://Shared/John1:1\0hidden", "AFirst"), ("sword://Shared/John1:1", "AFirst\0hidden")] {
            do { _ = try await worker.resolveLink(uri, currentModule: module); fatalError("NUL link input accepted") }
            catch is CoreError { }
        }
        let command = try await worker.followStudyLinkAt(tab: tab, currentModule: "BSecond", currentReference: "John1:1", uri: "passagestudy.jsp?action=showRef&type=scripRef&module=Shared&value=1%3B1", navigationModule: "AFirst")
        guard case .references(let references) = command, references.module == "AFirst", references.entries.count == 2,
              references.entries[0].html.contains("prose for AFirst"), try await worker.history(tab).count == 0 else { fatalError("Swift reference command uses shared main-module fallback without URI aliases or history") }
        let range = try await worker.followStudyLinkAt(tab: tab, currentModule: "Unicode", currentReference: "John1:2", uri: "bible://Shared/John1:1-2%3B1", navigationModule: "AFirst")
        guard case .references(let rangePreview) = range, rangePreview.module == "BSecond", rangePreview.context == "John 1:2", rangePreview.entries.count == 3,
              rangePreview.entries[0].html.contains("prose for BSecond"), rangePreview.entries[0].reference == rangePreview.entries[2].reference,
              try await worker.history(tab).count == 0 else { fatalError("Swift Bible range alias returns copied ordered actual-source preview") }
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
