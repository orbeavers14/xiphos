# Xiphos native development

A Bible-study application in development, based on [CrossWire Xiphos](https://github.com/crosswire/xiphos) and The SWORD Project. One shared C++ core powers native interfaces.

| Platform | Current state |
|---|---|
| macOS | SwiftUI/AppKit development preview |
| Linux | GTK4/WebKitGTK frontend in development |
| Windows | Core validation tooling; WinUI 3 frontend planned |

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
