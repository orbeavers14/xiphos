# 2026 native-platform modernization decision

Owner direction (2026-11-09): the new native Xiphos application may drop support for obsolete operating systems, toolchains, build machinery and legacy UI implementations. Optimize the new shared-core/native-frontend design for a current 2026 development stack. This is a forward-looking compatibility policy, not evidence that the upgrades below have been implemented or tested.

## Target policy

- **macOS:** modern SwiftUI/AppKit and Swift 6 language mode, built with the current supported Xcode/Apple SDK. Target macOS 26.6 or later initially so the current macOS 26.6 development host remains usable. The actual deployment floor must match every linked runtime dependency; presently recorded development bottles require macOS 27, so either rebuild/pin dependencies compatible with 26.6 or make a separately authorized move to macOS 27. Do not claim macOS 26 compatibility until a strict runtime audit and actual launch pass.
- **Windows:** Windows 11 only, supported Windows SDK/MSVC, WinUI 3 and WebView2; no Windows 10, MinGW/GTK desktop or old installer compatibility requirement. Validate native MSVC builds before frontend acceptance.
- **Linux:** maintained 2026 distributions and SDKs, GTK4/WebKitGTK. Define a small explicit distro/test matrix when native Linux validation resumes; do not spend work keeping legacy GTK3/distribution targets alive.
- **Core C++:** migrate the core and retained C++11 policy targets toward C++23 as a portable production baseline in a separate, tested sweep. C++26 features are opt-in after proven Apple Clang, GCC, MSVC and library support; newer syntax alone is not a delivery goal. Preserve the stable C ABI where needed for cross-language integration.
- **Build tooling/dependencies:** raise outdated minimum versions in coordinated changes, remove obsolete fallback branches only after checking all supported platforms, and pin/record reproducible SDK versions, architectures and licenses. Do not make a blanket mass-update without successful builds.

## Compatibility intentionally retained

Keep SWORD module interoperability, existing user bookmarks/annotations/notes, profile/session recovery and import/migration safety. Dropping old application binaries and OS versions is **not** authorization to discard users' study data or third-party module formats. Continue licensing, translator-credit and attribution obligations.

## Execution order and release gates

1. Inventory version floors and obsolete compatibility branches with repository search/Graphify; record impacted tests.
2. Resolve the Mac SDK/runtime OS-floor mismatch, then update Mac CMake deployment policy and local build/launcher checks together.
3. Upgrade the shared core to C++23 incrementally, including UTF-8/char8_t and SWORD boundary fixes; test C/C++ and Swift interfaces.
4. Upgrade Linux/Windows dependency definitions and native builds on their actual target hosts.
5. Remove now-unused legacy shims and build fallbacks with regression evidence; preserve frozen characterization fixtures until replacement tests cover their behaviors.
6. Update deployment and user documentation to match **verified**, not merely planned, supported systems. Bundle dependencies, audit runtime closure, verify signing/install/upgrade workflows.

All changes require local build/test evidence on affected platforms. Keep hosted GitHub Actions disabled unless separately authorized. Do not equate a modern build configuration with completed feature parity or release readiness.
