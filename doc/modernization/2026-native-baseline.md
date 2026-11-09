# 2026 native-platform modernization decision

Owner direction (2026-11-09): reduce the maintenance cost of the native Xiphos applications by dropping obsolete OS and toolchain compatibility. Adopt newer tools when they simplify maintenance or resolve a concrete build or runtime requirement.

## Target policy

- **macOS:** use SwiftUI/AppKit, Swift 6 and the current supported Xcode/Apple SDK. The native preview defaults to macOS 27.0 and rejects lower targets. The development host runs macOS 27.2. Audit the complete dependency closure and validate the minimum OS on an actual target before claiming release support.
- **Windows:** Windows 11 only, supported Windows SDK/MSVC, WinUI 3 and WebView2; no Windows 10, MinGW/GTK desktop or old installer compatibility requirement. Validate native MSVC builds before frontend acceptance.
- **Linux:** maintained 2026 distributions and SDKs, GTK4/WebKitGTK. Define a small explicit distro/test matrix when native Linux validation resumes; do not spend work keeping legacy GTK3/distribution targets alive.
- **Core C++:** retain C++17 for the shared core, C++11 for retained policy helpers and C11 for the C ABI. Consider a language upgrade when it solves a specific maintenance or portability problem and can be validated on Apple Clang, GCC and MSVC.
- **Build tooling/dependencies:** raise outdated minimum versions in coordinated changes, remove obsolete fallback branches only after checking all supported platforms, and pin/record reproducible SDK versions, architectures and licenses. Do not make a blanket mass-update without successful builds.

## Compatibility intentionally retained

Keep SWORD module interoperability, existing user bookmarks/annotations/notes, profile/session recovery and import/migration safety. Dropping old application binaries and OS versions is **not** authorization to discard users' study data or third-party module formats. Continue licensing, translator-credit and attribution obligations.

## Execution order and release gates

1. Inventory version floors and obsolete compatibility branches with repository search/Graphify; record impacted tests.
2. Align the Mac CMake, Swift and bundle deployment floor with the SDK closure and verify local builds and launches.
3. Upgrade Linux/Windows dependency definitions and native builds on their actual target hosts.
4. Remove now-unused legacy shims and build fallbacks with regression evidence; preserve frozen characterization fixtures until replacement tests cover their behaviors.
5. Update deployment and user documentation to match **verified**, not merely planned, supported systems. Bundle dependencies, audit runtime closure, verify signing/install/upgrade workflows.

All changes require local build/test evidence on affected platforms. Keep hosted GitHub Actions disabled unless separately authorized. Do not equate a modern build configuration with completed feature parity or release readiness.
