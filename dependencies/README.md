# New application dependency inventory

**Scope:** the shared C++ core and its macOS, Windows and Linux native frontends.
The retained GTK3 reference application and its exclusive dependencies are excluded.
All three frontends consume the shared-core dependencies in the first table.

**Last inspected: 2026-10-31.** Installed versions describe the current macOS
27.2 development host, not verified Windows/Linux installations. Minimums are
those declared by the new application's build/setup files. **Not specified**
means no numeric minimum is declared, not compatibility with every old version.
**Not discovered** describes the inspected discovery tools, not proof of absence.
Planned frontend dependencies are identified explicitly.

## Shared core dependencies

SDK-specific transitive libraries and build/test tools are labeled below. They
are included because they support the new core, not because the old GTK app used
them. Transitive libraries vary with the selected SDK and TLS backend.

| Dependency | Minimum version | Installed / selected version | Purpose | Areas of use / location |
|---|---|---|---|---|
| CrossWire SWORD | 1.8.1 | 1.9.0.3916; runtime verified | Module discovery, versification, rendering, search and editable module drivers | `src/core/reader.cpp`, session/search/book services; selected static SDK under `/usr/local` |
| GLib | 2.32 | 2.90.1; runtime verified | Unicode casing, UTF-8 validation, SHA256 and shared utilities | Core rendering/persistence/installation helpers; selected Homebrew SDK |
| libxml2 | Not specified by native finder | 2.9.13; selected headers and runtime verified | XML persistence and HTML/rich-document parsing | `xml_document.cpp`, `session_snapshot.cpp`, `rich_document.cpp`, HTML preparation; Apple SDK/system library |
| libpng | 1.6; simplified read API enabled | 1.6.59; selected headers and static runtime verified | Bounded PNG validation, decoding and export | `src/core/export_png.cpp`, image preparation; `build/libpng-sdk` |
| zlib | Not specified | Direct runtime 1.2.12; effective core headers 1.3.2; see notes below | Catalog gzip expansion, PNG/ZIP support and SWORD linkage | `repository_archive.cpp`, libpng/libarchive/SWORD; direct link uses Apple SDK/system zlib |
| libcurl | 7.62 | 8.23.0; runtime verified; OpenSSL 3.6.5 TLS backend | HTTPS download, certificate/redirect checks, transfer limits, progress and cancellation | `src/core/https_download.cpp`; selected MacPorts dynamic SDK |
| libarchive | 3.6 | 3.8.11; static runtime verified | ZIP/tar parsing beneath the core extraction policy | `src/core/repository_archive.cpp`; verified source-built SDK at `build/libarchive-sdk` |
| Threads / C and C++ runtimes | Target toolchain; C11 / C++17 | Apple toolchain/system runtime on macOS 27.2; no independent package version assigned | FIFO SDK ownership, atomics, filesystem and standard-library facilities | `src/core/worker.cpp`, transfer controls and shared services; CMake Threads target / libc++ |
| bzip2 — SWORD transitive dependency | SDK-selected; no separate Xiphos floor | 1.0.8; system runtime verified | SWORD compression support | Explicit SWORD static extra link input; Apple system library |
| XZ / liblzma — SWORD transitive dependency | SDK-selected; no separate Xiphos floor | 5.8.4; installed metadata/prefix inspected | SWORD compression support | Explicit SWORD static extra link input; Homebrew SDK |
| gettext / libintl — GLib transitive dependency | SDK-selected; no separate Xiphos floor | 1.0; installed package/tool inspected | Localization runtime supporting the selected GLib build | Selected Homebrew `libintl`; not a new-core gettext tool requirement |
| libiconv — SDK transitive dependency | SDK-selected; no separate Xiphos floor | Apple system library; semantic package version not independently verified | Character-set conversion | GLib/gettext supporting linkage |
| PCRE2 — GLib transitive dependency | SDK-selected; no separate Xiphos floor | 10.49; build-selected package metadata | GLib regular-expression support | Selected Homebrew GLib dependency |
| OpenSSL libraries — selected curl backend | Backend-selected; not required by every curl build | 3.6.5; curl runtime reports `OpenSSL/3.6.5` | TLS for the installed curl SDK | MacPorts curl's `libssl` / `libcrypto`; other platforms may use a different backend |
| nghttp2 — curl transitive dependency | SDK-selected; no separate Xiphos floor | 1.70.0; installed metadata | HTTP/2 capability built into curl | Selected curl dynamic dependencies |
| libidn2 — curl transitive dependency | SDK-selected; no separate Xiphos floor | 2.3.8; installed metadata | Internationalized-domain capability built into curl | Selected curl dynamic dependencies; repository policy currently accepts ASCII DNS/IPv4 |
| libpsl — curl transitive dependency | SDK-selected; no separate Xiphos floor | 0.23.3; installed metadata | Public-suffix support built into curl | Selected curl dynamic dependencies |
| Zstandard — curl transitive dependency | SDK-selected; no separate Xiphos floor | 1.5.7; installed metadata | Compression capability built into curl | Selected curl dynamic dependencies |
| Brotli decoder/common — curl transitive dependency | SDK-selected; no separate Xiphos floor | 1.2.0; installed metadata | Compression capability built into curl | Selected curl dynamic dependencies |
| curl-private zlib — transitive dependency | SDK-selected; no separate Xiphos floor | 1.3.2; installed metadata/link inspection | Compression support inside curl | MacPorts library; separate from directly linked Apple zlib above |
| CMake / CTest — build/test tool | 3.16 | 4.4.0 | Configure, generate builds and run checks | `src/core/CMakeLists.txt`, native platform CMake files |
| C/C++ compiler — build tool | C11 / C++17; no compiler release floor declared | Apple Clang 21.0.0 | Compile shared code and adapters | Core build; reused helper interfaces retain C++11 compatibility |
| pkg-config / pkgconf — discovery tool | Not specified; preferred, with explicit SDK fallback support | Build-selected pkgconf 3.0.10; another PATH installation reports 0.29.2 | Discover SDK versions, headers and link inputs | Core CMake dependency discovery |
| Make / Ninja — build tool | A supported CMake generator; no numeric floor declared | GNU Make 3.81; Ninja not on this Mac's PATH | Run generated build rules | Current Mac configuration uses Unix Makefiles; Windows setup selects Ninja |
| Python3 — test tool | Python3; finder declares no tighter numeric floor | 3.14.7; CMake-selected interpreter | Generate/run local HTTPS and archive fixtures | `src/core/tests/remote_install_fixture.py` |
| OpenSSL CLI — test tool | Required by HTTPS fixtures; no numeric floor declared | 3.6.5; CMake-selected executable | Generate disposable local TLS certificates | HTTPS tests only; separate from curl's choice of runtime TLS backend |

Authoritative requirements: [core CMake](../src/core/CMakeLists.txt).
Source provenance, limits and notices: [PNG contract](../doc/modernization/png-export-workflows.md)
and [HTTPS/extraction contract](../doc/modernization/https-transfer.md).
The shared core does not require GTK, WebKitGTK, BibleSync or SQLite.
Installed SDK capabilities are not necessarily features actively used by the app.

## Native frontend dependencies

Rows are grouped by platform. Shared libraries and common tools from the first
table also apply; they are not repeated as separate platform implementations.
Windows interface dependencies remain planned until its native frontend exists.

| Frontend | Dependency | Minimum version | Installed / observed version | Purpose | Areas of use / location |
|---|---|---|---|---|---|
| **macOS** | Swift compiler | Swift 6 language mode | 6.4.2 | Compile native Apple adapters and UI | `platforms/macos/CMakeLists.txt` |
| **macOS** | Xcode / Apple SDK | Swift 6-compatible toolchain; no independent Xcode release floor declared | Xcode 27.2; active SDK 27.2 | Apple compiler, framework and platform SDKs | Native Mac build; libxml2 retains cached CLT 26 SDK headers |
| **macOS** | SwiftUI | macOS 13 project baseline; runtime compatibility still needs target validation | Bundled framework; developer SDK 27.2 | Reader/editor presentation | `platforms/macos/ReaderApp.swift`, editor/search panes |
| **macOS** | AppKit | macOS 13 project baseline | Bundled framework; developer SDK 27.2 | Windows, text editing, panels, clipboard and lifecycle | `SessionApp.swift`, `EditorWindow.swift`, native adapters |
| **macOS** | Foundation / Combine / Darwin runtime | Target Apple SDK; no independent library floor declared | Bundled SDK/runtime; developer SDK/host 27.2 | Files/URLs, observation and system facilities | `platforms/macos/*.swift` |
| **macOS** | WebKit | Target Apple SDK; macOS 13 project baseline | Bundled framework; developer SDK 27.2 | Native HTML reading view | macOS reading/study presentation |
| **macOS** | ImageIO / CoreGraphics | Target Apple SDK; no independent floor declared | Bundled SDK/runtime; developer SDK 27.2 | Images and native graphics | Image/clipboard adapters; CoreGraphics supplied through Apple UI frameworks |
| **macOS** | UniformTypeIdentifiers | Target Apple SDK; no independent floor declared | Bundled framework; developer SDK 27.2 | File/export type descriptions | Native export/save adapters |
| **Windows** | MSVC C++ Build Tools / runtime | C++17-capable native MSVC toolchain; no numeric toolset floor declared | Requires Windows-host inspection | Compile the core and future native frontend | `platforms/windows/core.ps1`; matching target architecture/runtime required |
| **Windows** | Windows SDK | Compatible target SDK; numeric floor not declared here | Requires Windows-host inspection | Windows headers, import libraries and platform APIs | Native validation lane; `platforms/windows/README.md` |
| **Windows** | Windows App SDK / WinUI 3 | Planned; package/minimum version not yet selected | Not integrated/verified | Planned native Windows widgets and application UI | Future WinUI frontend; not the retained MinGW/GTK application |
| **Windows** | WebView2 | Planned; SDK/runtime version not yet selected | Not integrated/verified | Planned HTML reading presentation | Future Windows reading view |
| **Windows** | PowerShell | Windows PowerShell 5.1 or PowerShell 7, per setup guide | Requires Windows-host inspection | Toolchain inventory and native validation entry point | `platforms/windows/core.ps1` |
| **Windows** | Ninja | Required by current native Windows validation script; no numeric floor | Requires Windows-host inspection | Build generator used with the selected MSVC developer shell | Windows core validation; shared CMake/CTest requirement is above |
| **Linux** | GTK4 | 4.6 | Not discovered on this Mac; Linux-host inspection pending | Native Linux widgets and application interface | `platforms/linux/reader_app.cpp`, `study_ui.cpp` |
| **Linux** | WebKitGTK API 6.0 | 2.40 | Not discovered on this Mac; Linux-host inspection pending | Native Linux HTML reading view | `platforms/linux/CMakeLists.txt` and GTK4 preview |

Setup/status: [macOS](../platforms/macos/README.md),
[Windows](../platforms/windows/README.md), [Linux](../platforms/linux/README.md).
GTK4 is confined to Linux presentation; GTK3 is not a dependency of the new app.
Portable Linux-model checks on this Mac disable the GTK4/WebKitGTK interface.
SwordKit is a reference/potential adapter, not a linked dependency; the new
HTTPS service does not link its separate SWORD bridge.

## Inspection and SDK selection notes

Versions were checked against `build/macos/CMakeCache.txt`, generated compiler/link
inputs, package metadata, tool versions and a temporary selected-library runtime
probe. This update did not install, relocate or upgrade packages or modify app code.

- GLib's selected imported target is 2.90.1, ahead of old 2.86.5 fallback paths.
  Older local GLib source/build directories are unselected and not delivered by Git.
- CMake's zlib finder records Apple SDK 1.2.12 headers/library, but curl's public
  include directory precedes system headers: actual core preprocessing selects
  `zlib.h` 1.3.2. The direct runtime reports 1.2.12; curl separately links 1.3.2.
  A finder cache value alone does not establish the effective header version.
- curl's auxiliary pkg-config cache reports 8.7.1; selected headers/library and
  runtime are 8.23.0. The two installed pkg-config implementations also search
  different prefixes. Use actual selected inputs, not just the shell default.
- Apple framework versions are reported as their SDK context, not invented
  independent semantic versions. libxml2 runtime value `20913` means 2.9.13.

SDK-specific static linkage can require `XIPHOS_SWORD_EXTRA_LIBRARIES`,
`XIPHOS_GLIB_EXTRA_LIBRARIES` and `XIPHOS_TRANSFER_EXTRA_LIBRARIES`; native Swift
custom links consume those inputs too. This inventory is not a complete transitive
lockfile or a release license/source audit.

Local external SDK storage stays outside tracked application source:

- `build/dependencies/`, `build/libpng-sdk/`: ignored libpng source/build/SDK.
- `build/libarchive-3.8.11/`, `build/libarchive-build/`, `build/libarchive-sdk/`:
  ignored verified libarchive source/build/SDK; provenance/notices are linked above.
- System/package-manager prefixes: selected SWORD, GLib, curl and supporting SDKs.
- `build/<configuration>/CMakeCache.txt` and generated flags/link commands: local
  resolved inputs; do not commit machine-specific caches.

For future source builds, prefer ignored `build/dependencies/<name>-<version>` and
`build/<name>-sdk`, recording source URL/revision, checksum, notices, compiler,
architecture, configuration and deployment target. Use a fresh build directory
when changing SDKs; do not move working installations to match a convention.

Current checks run on macOS27.2 with configured deployment target26; some selected
dylibs require27. The requested macOS13 baseline, macOS26 runtime, physical Windows/
Fedora validation and release bundling remain open. Preserve dependency notices.
