# Shared application core

C++/SWORD services for reading, navigation/history, search, parallel passages,
bookmarks/annotations, editing, library installation, sessions, migration/recovery
and optional passage sync. Native hosts share this implementation through an
owned C11 API and one FIFO worker per process. GTK is not a core dependency.

## Build and requirements

From the repository root:

```sh
cmake -S . -B build/core
cmake --build build/core --parallel
ctest --test-dir build/core --output-on-failure
```

Building directly with `-S src/core` also works. Use C++17 and CMake3.16+.
SWORD1.8.1+, GLib2.32+, libxml2, PNG1.6+, zlib, libcurl7.62+, libarchive3.6+
and SQLite3.31+ are required for the full native core. See the
[dependency inventory](../../dependencies/README.md) and CMakeLists.txt for
actual discovery rules/options. Compatible static SDKs may require explicit
transitive libraries; version numbers alone do not prove architecture/OS compatibility.

Pkg-config is preferred. Prebuilt SDK fallbacks include SWORD_INCLUDE_DIR,
SWORD_LIBRARY, GLIB_INCLUDE_DIR, GLIB_CONFIG_INCLUDE_DIR and GLIB_LIBRARY, with
XIPHOS_SWORD_EXTRA_LIBRARIES/XIPHOS_GLIB_EXTRA_LIBRARIES for missing link inputs.
Optional BibleSync must use the reviewed safe API/patch; see dependencies.

## Contracts and verification

Public C++ headers are in include/xiphos/core; c_api.h documents C ownership,
request polling and handle lifetimes. Native UIs copy result data and await work;
they must not create another navigation/search/persistence engine or access SDK
objects concurrently. C++11-compatible policy helpers remain independently checked.

Tests create original-prose fixtures. Frozen legacy-reference inputs retain
behavior/provenance after the GTK application retirement. Real-module corpus
runners are manual, with no automatic downloads or CTest registration. Use
appropriate local normal/sanitizer/platform checks for each change.

[Architecture](../../doc/modernization/architecture.md) ·
[Current status](../../doc/modernization/status.md) ·
[Service contracts](../../doc/modernization/README.md) ·
[Historical extraction notes](../../doc/modernization/history/core-readme.md)
