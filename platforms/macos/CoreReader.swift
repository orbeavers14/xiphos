// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import XiphosCore

struct InstalledModule: Identifiable {
    let id: String
    let description: String
    let type: String
    let language: String
}
struct RenderedEntry {
    let reference: String
    let html: String
}
struct CoreError: LocalizedError {
    let status: xc_status
    var errorDescription: String? {
        switch status {
        case XC_INVALID_ARGUMENT: return "Check the module folder and reference. This preview supports verse-key modules."
        case XC_MODULE_NOT_FOUND: return "The selected module is not installed."
        default: return "The SWORD engine could not complete the request."
        }
    }
}
// Presentation adapter only: no SWORD manager, canon tables, navigation or
// rendering policy in Swift. Use this instance serially, including destruction.
final class CoreReader {
    private let handle: OpaquePointer
    let modules: [InstalledModule]
    init(root: String? = nil) throws {
        var opened: OpaquePointer?
        let status: xc_status
        if let root { status = root.withCString { xc_open($0, &opened) } }
        else { status = xc_open(nil, &opened) }
        guard status == XC_OK, let opened else { throw CoreError(status: status) }
        handle = opened
        modules = (0..<xc_module_count(opened)).map { index in
            InstalledModule(id: String(cString: xc_module_id(opened, index)!),
                description: String(cString: xc_module_description(opened, index)!),
                type: String(cString: xc_module_type(opened, index)!),
                language: String(cString: xc_module_language(opened, index)!))
        }
    }
    deinit { xc_close(handle) }
    func read(module: String, reference: String) throws -> RenderedEntry {
        var result: OpaquePointer?
        let status = module.withCString { id in
            reference.withCString { key in xc_read(handle, id, key, &result) }
        }
        guard status == XC_OK, let result else { throw CoreError(status: status) }
        defer { xc_entry_release(result) }
        return RenderedEntry(reference: String(cString: xc_entry_reference(result)!),
                             html: String(cString: xc_entry_html(result)!))
    }
}
