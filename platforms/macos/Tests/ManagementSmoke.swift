// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit
import XiphosCore
@main struct ManagementSmoke {
    @MainActor static func check(_ ok: Bool,_ why: String) { if !ok { fatalError(why) } }
    @MainActor static func main() async throws {
        guard CommandLine.arguments.count == 3, let port = UInt32(CommandLine.arguments[2]) else { fatalError("Loopback fixture and port required") }
        _ = NSApplication.shared;NSApplication.shared.setActivationPolicy(.prohibited)
        let root = URL(fileURLWithPath:CommandLine.arguments[1]);check(root.lastPathComponent == "remote-install-fixture","Named fixture only")
        let worker = try CoreWorker(root:root.appendingPathComponent("installed").path)
        let file = root.appendingPathComponent("sources.profile").path, ca = root.appendingPathComponent("certificate.pem").path
        var profile = try await worker.readRepositories(path:file)
        profile = try await worker.editRepository(path:file,profile:profile,endpoint:CoreRepositoryEndpoint(id:"original",host:"127.0.0.1",catalog:"/catalog",packages:"/packages",port:port),name:"Original fixture")
        let stale = profile
        profile = try await worker.editRepository(path:file,profile:profile,endpoint:profile.rows[0].endpoint,name:"Renamed fixture")
        do { _ = try await worker.editRepository(path:file,profile:stale,remove:"original");fatalError("Stale profile overwrite") } catch {}
        profile = try await worker.refreshRepository(path:file,profile:profile,index:0,workspace:root.path,caBundle:ca)
        let catalog = try await worker.repositoryChoices(profile:profile,index:0,planning:"sword|Added");check(catalog.count == 1 && catalog[0].module == "Added" && catalog[0].decision == 0,"HTTPS catalog and core install planner")
        let installed = try await worker.installRemote(path:file,profile:profile,index:0,module:"Added",replace:false,workspace:root.path,caBundle:ca);check(installed.profile != nil,"Install refreshes source destination state");profile = installed.profile!
        let inventory = try await worker.libraryInventory();check(inventory.contains(where: { $0.module == "Added" }),"Fresh native inventory sees installation")
        let source = root.appendingPathComponent("source").path
        let local = try await worker.prepareLocalInstall(source:source,module:"Added");check([1,2].contains(local.decision),"Local replacement requires explicit choice")
        try await worker.installLocal(local,replace:true)
        // A slow transfer can be stopped without changing installed content.
        let slow = CoreRepositoryEndpoint(id:"original",host:"127.0.0.1",catalog:"/catalog",packages:"/slow",port:port)
        profile = try await worker.editRepository(path:file,profile:profile,endpoint:slow,name:"Slow fixture")
        profile = try await worker.refreshRepository(path:file,profile:profile,index:0,workspace:root.path,caBundle:ca)
        let snapshot = profile
        let transfer = Task { try await worker.installRemote(path:file,profile:snapshot,index:0,module:"Added",replace:true,workspace:root.path,caBundle:ca) }
        try await Task.sleep(nanoseconds:100_000_000);transfer.cancel()
        do { _ = try await transfer.value;fatalError("Slow cancelled transfer committed") } catch { check((error as? CoreError)?.status == XC_CANCELLED,"Transfer must report actual cancellation") }
        let added = try await worker.maintenanceOffer(module:"Added");try await worker.maintainModule(added,action:0)
        check(!(try await worker.libraryInventory()).contains(where: { $0.module == "Added" }),"Native maintenance removes selected module")
        let readOnly = LibraryManagementModel(worker:worker,path:file,workspace:root.path,allowsChanges:false);readOnly.reload();await readOnly.waitUntilIdle()
        readOnly.selectSource("original");await readOnly.waitUntilIdle();readOnly.refreshSource();check(!readOnly.isBusy,"Auto-discovered libraries do not start transfers without an explicit managed root")
        readOnly.installedID = readOnly.installed.first?.id;readOnly.inspectModule();check(readOnly.maintenance == nil && !readOnly.isBusy,"Auto-discovered libraries stay read-only until folder selection")
        let window = NativeLibraryWindow(worker:worker,path:file,workspace:root.path,show:false);await window.model.waitUntilIdle()
        check(window.model.profile?.rows.count == 1 && !window.model.installed.isEmpty,"Actual management window model loads")
        window.model.selectSource("original");await window.model.waitUntilIdle();window.model.host = "unsaved.invalid";check(window.model.hasUnsavedSourceChanges,"Unsaved endpoint edits prevent stale-server actions")
        window.window.contentView?.layoutSubtreeIfNeeded();try await Task.sleep(nanoseconds:100_000_000)
        if let view = window.window.contentView, let bitmap = view.bitmapImageRepForCachingDisplay(in:view.bounds) { view.cacheDisplay(in:view.bounds,to:bitmap);if let data = bitmap.representation(using:.png,properties:[:]) { try data.write(to:root.appendingPathComponent("management-window.png")) } }
        window.window.close();await worker.shutdown();print("Native management HTTPS/local/maintenance and window checks passed")
    }
}
