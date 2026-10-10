// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
@main
struct AdapterSmoke {
    static func main() throws {
        guard CommandLine.arguments.count == 2 else { fatalError("fixture root required") }
        let reader = try CoreReader(root: CommandLine.arguments[1])
        guard reader.modules.count == 1, reader.modules[0].id == "ParityTest" else { fatalError("module snapshot") }
        let first = try reader.read(module: "ParityTest", reference: "John 3:16")
        let second = try reader.read(module: "ParityTest", reference: "John 3:17")
        guard first.html.contains("café"), second.html.contains("Second fixture"),
              first.reference.contains("3:16") else { fatalError("adapter results") }
        do {
            _ = try reader.read(module: "Missing", reference: "John 3:16")
            fatalError("missing module succeeded")
        } catch is CoreError { }
        print("Swift adapter smoke checks passed")
    }
}
