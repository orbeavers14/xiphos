# 2026 native-platform modernization decision

Owner direction (2026-11-09): reduce the maintenance cost of the native Xiphos applications by dropping obsolete OS and toolchain compatibility. Adopt newer tools when they simplify maintenance or resolve a concrete build or runtime requirement.

## Target policy

- **macOS:** use SwiftUI/AppKit, Swift 6 and the current supported Xcode/Apple SDK. The native preview defaults to macOS 27.0 and rejects lower targets. The development host runs macOS 27.2. Evaluate macOS 26.7 as the candidate minimum. Lower the floor only after rebuilding or replacing dependencies that require 27.0, auditing the complete bundle and passing a launch on 26.7. Validate the existing 27.0 floor on an actual target as well.
- **Windows:** Windows 11 version 25H2 as the minimum development and runtime baseline, supported Windows SDK/MSVC, WinUI 3 and WebView2; no earlier Windows releases, MinGW/GTK desktop or old installer compatibility requirement. Validate native MSVC builds on 25H2 and run a compatibility check on 26H2 before frontend acceptance.
- **Linux:** maintained 2026 distributions and SDKs, GTK4/WebKitGTK. Use Fedora 45 ARM64 as the primary Fedora target and check Fedora 44 for compatibility. Validate Ubuntu 24.04 LTS and Debian 13 (Trixie) on x86-64. Fedora 45 has passed a build and desktop smoke. The other entries are validation targets pending native build and desktop checks.
- **Core C++:** retain C++17 for the shared core, C++11 for retained policy helpers and C11 for the C ABI. Consider a language upgrade when it solves a specific maintenance or portability problem and can be validated on Apple Clang, GCC and MSVC.
- **Build tooling/dependencies:** raise outdated minimum versions in coordinated changes, remove obsolete fallback branches only after checking all supported platforms, and pin/record reproducible SDK versions, architectures and licenses. Do not make a blanket mass-update without successful builds.

## Adoption evidence

As of 2026-11-09, [OpenMPT telemetry](https://buildbot.openmpt.org/statistics/)
favored Windows 11 25H2. [TelemetryDeck](https://telemetrydeck.com/survey/apple/macOS/versions/)
showed macOS 26.7 leading its sample in late October. [Homebrew event counts](https://formulae.brew.sh/analytics/os-version/30d/)
favored macOS 26 over 27, Ubuntu 24.04 over 26.04, Fedora 44 over the newly
released Fedora 45, and Debian 13 over Debian 12. The [Steam Linux survey](https://store.steampowered.com/hwsurvey/?platform=linux)
favored CachyOS among named distributions in its gaming sample. These are
audience-specific signals. The Linux matrix also reflects existing Fedora
validation and the GTK4/WebKitGTK package requirements.

## Compatibility intentionally retained

Keep SWORD module interoperability, existing user bookmarks/annotations/notes, profile/session recovery and import/migration safety. Dropping old application binaries and OS versions is **not** authorization to discard users' study data or third-party module formats. Continue licensing, translator-credit and attribution obligations.

## Execution order and release gates

1. Inventory version floors and obsolete compatibility branches with repository search/Graphify; record impacted tests.
2. Align the Mac CMake, Swift and bundle deployment floor with the SDK closure and verify local builds and launches.
3. Upgrade Linux/Windows dependency definitions and native builds on their actual target hosts.
4. Remove now-unused legacy shims and build fallbacks with regression evidence; preserve frozen characterization fixtures until replacement tests cover their behaviors.
5. Update deployment and user documentation to match **verified**, not merely planned, supported systems. Bundle dependencies, audit runtime closure, verify signing/install/upgrade workflows.

All changes require local build/test evidence on affected platforms. Keep hosted GitHub Actions disabled unless separately authorized. Do not equate a modern build configuration with completed feature parity or release readiness.
