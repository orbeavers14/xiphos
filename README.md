# Xiphos native development

A Bible-study application in development, based on [CrossWire Xiphos](https://github.com/crosswire/xiphos) and The SWORD Project. One shared C++ core powers native interfaces.

| Platform | Target OS baseline | Current state |
|---|---|---|
| macOS | macOS 27.0 preview minimum. macOS 26.7 is a candidate pending dependency and launch validation. | SwiftUI/AppKit development preview, tested on macOS 27.2 |
| Linux | Fedora 45 (ARM64) is the primary Fedora target. Fedora 44, Ubuntu 24.04 LTS (x86-64), and Debian 13 (x86-64) are validation targets. | GTK4/WebKitGTK frontend in development. Fedora 45 passed a build and desktop smoke test. |
| Windows | Windows 11 25H2 minimum target, with a 26H2 compatibility check planned. | Core validation tooling. Native MSVC validation and the WinUI 3 frontend are pending. |

See the [2026 native baseline](doc/modernization/2026-native-baseline.md) for platform validation and release gates.

The core covers reading, search, parallel passages, bookmarks and notes, editing,
module management, sessions/recovery and passage sync. Native workflows and
release acceptance remain incomplete. The old GTK3 application has been removed;
its source remains in Git history. The final product name is undecided.

[Current status](doc/modernization/status.md) · [Roadmap](doc/modernization/shared-core-roadmap.md) · [Documentation](doc/README.md)

## Build

See [installation](INSTALL.md) and [dependencies](dependencies/README.md) for SDK requirements.

```sh
cmake -S . -B build/core
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

Native setup: [Mac](platforms/macos/README.md) · [Linux](platforms/linux/README.md) · [Windows](platforms/windows/README.md)

For development, start with [AGENTS.md](AGENTS.md). Translation sources and their
workflow are under [localization/](localization/README.md). Routine checks run locally.

## Credits and license

Based on work by the Xiphos Development Team, CrossWire and other contributors.
See [AUTHORS](AUTHORS), [COPYING](COPYING) and [retained notices](doc/legacy-notices/).
Xiphos code is GPL v2 or later; dependencies and Bible modules retain their own licenses.
