# Dependency directory and inventory

This is the versioned starting point for finding Xiphos dependencies. It contains
inventory/documentation, not copies of external libraries. Installed SDKs remain
in their package-manager/system locations or ignored local build storage. Moving
an installed SDK can break its metadata and existing build configurations.

## Shared native core

| Dependency | Purpose | Requirement | Inspected macOS development setup (2026-10-21) |
|---|---|---|---|
| CrossWire SWORD | Modules, versification, rendering and search | >=1.8.1 | 1.9.0.3916; installed SDK under `/usr/local`, static library |
| GLib | Existing Unicode casing and shared utility behavior | >=2.32 | 2.90.1 selected through installed pkg-config metadata; older 2.86.5 source/build remains at `../glib-2.86.5` and `../glib-build` |
| libxml2 | XML persistence and HTML parsing | Required; standalone finder has no explicit version floor | Apple SDK headers/library; paths recorded by CMake |
| libpng | Shared bounded static PNG validation/decoding | >=1.6, simplified read API enabled | 1.6.59, official-source static SDK at `build/libpng-sdk` |
| zlib | PNG decompression/checksums and SWORD linkage | Required through PNG discovery | Apple SDK headers/library; headers report 1.2.12, not a verified runtime version |
| System threads and C/C++ runtimes | Worker and standard library | Target toolchain | Apple toolchain/system libraries on this host |

These are minimum requirements, not a universal dependency lockfile. The local
libpng source is pinned by version and checksum in the [PNG contract](../doc/modernization/png-export-workflows.md).
Other exact SDK versions describe this checked host, not a promise that every
platform uses those versions. Verify each target's architecture, runtime,
configuration and minimum operating-system version.

Static SDKs can require additional libraries. This Mac's SWORD linkage includes
zlib, bzip2 and liblzma; GLib linkage includes libintl/gettext, iconv and Foundation.
Those inputs are explicit in the local CMake cache; only the selected target
and actual final link command establish which library was used. They are not necessarily the
same on Linux or Windows. SDK/package dependencies must also be inventoried when
preparing release bundles; this document is not a complete transitive lockfile.

## Platform interfaces and tools

| Area | Dependencies / state | Setup guide |
|---|---|---|
| Core development | CMake >=3.16, C++17 compiler, C11 compiler; legacy helpers keep C++11; pkg-config preferred for SDK discovery | [Core](../src/core/README.md) |
| macOS preview | Swift 6-capable Xcode/toolchain; SwiftUI, AppKit, WebKit, Foundation, ImageIO/CoreGraphics from Apple SDKs | [macOS](../platforms/macos/README.md) |
| Linux preview | GTK4 >=4.6 and WebKitGTK API 6.0 >=2.40, plus core dependencies; GTK can be disabled for portable model checks | [Linux](../platforms/linux/README.md) |
| Windows target | MSVC C++ tools, Windows SDK, CMake/Ninja and compatible core SDKs; WinUI 3 frontend remains planned | [Windows](../platforms/windows/README.md) |

SwordKit is a possible future Swift adapter, not a current linked dependency.
Its application logic must not duplicate the shared core.

## Existing GTK application

The retained reference application additionally uses BibleSync, SQLite, GModule,
ICU, UUID on Unix (libssh2 on Windows), libsoup, GTK3, ATK, Cairo, GDK/GdkPixbuf,
GIO/GObject, Pango, minizip and WebKitGTK. Optional historical WebKit1/GtkHTML
editor configurations and help/translation/packaging tools have separate needs.
These dependencies cannot be removed until their remaining callers and parity
retirement gates are checked.

The authoritative build declarations are [legacy dependencies](../cmake/XiphosDependencies.cmake),
[legacy tools](../cmake/XiphosBuildTools.cmake) and
[native core dependencies](../src/core/CMakeLists.txt). See
[INSTALL](../INSTALL.md) and the [architecture/license audit](../doc/modernization/architecture.md).
The standalone native core does not require GTK, WebKitGTK, BibleSync or SQLite.

## Where to look on this checkout

- `dependencies/`: this tracked inventory and future dependency documentation.
- `build/dependencies/`: ignored libpng source archive, extracted source and build
  directory on the current Mac. These files are local and do not arrive with Git.
- `build/libpng-sdk/`: ignored installed static libpng SDK used by current checks.
- Sibling GLib source/build directories: existing local setup, outside this Git
  repository; they also do not arrive with Git.
- System/package-manager SDK prefixes: SWORD, Apple SDKs and supporting libraries.
- `build/<configuration>/CMakeCache.txt`: exact resolved headers/libraries, SDK
  paths and static extra link inputs for that configuration. Never commit these
  machine-specific caches; inspect them locally when reproducing a build.

For future source-built SDKs, prefer ignored `build/dependencies/<name>-<version>`
and `build/<name>-sdk` locations, and record the source URL/revision, checksum,
license/notices, compiler, architecture, configuration and deployment target here
or in a linked contract. Use a fresh build directory when changing SDKs. Do not
relocate existing working installations merely to match that convention.

Current Mac checks target macOS 26 because of the local SDKs. Homebrew's inspected
libpng 1.6.59 bottle targets macOS 27; the separately built static SDK targets 26.
The requested macOS 13 minimum, native Windows validation and release dependency
bundling remain open. Preserve upstream licenses and distribution obligations;
this inventory does not replace a release notice/source audit.

### Discovery change observed 2026-10-21

The resumed configure discovers installed GLib 2.90.1 through pkg-config. Its
imported target takes precedence over the older `GLIB_LIBRARY` fallback cache
entry, even though that entry still names the local 2.86.5 SDK. Inspect selected
pkg-config targets and actual link inputs, not only a fallback variable. The
linked Homebrew GLib and liblzma report a macOS27 minimum while this development
configuration requests26; current tests run on macOS27.0.1 and establish neither
macOS26 nor macOS13 runtime compatibility. No installed package was moved or
changed by this source work. The source-pinned static libpng SDK remains1.6.59.

For a deliberate fallback-only configuration, CMake can disable PkgConfig
discovery with `CMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE`; then supply coherent
SWORD/GLib fallback headers, libraries and transitive link inputs through the
platform guide. Use a fresh build directory and validate the complete chosen
SDK set. Do not assume an explicit fallback cache path overrides an available
imported pkg-config target. This is a reproducibility choice, not a package update
or a claim that every older local SDK is release-compatible.
