# Mac development preview

SwiftUI/AppKit frontend using the shared C++ core. This is development software,
not a signed/notarized release or a complete replacement workflow acceptance.
The old GTK3 application has been removed.

## Build

Use a Swift 6-capable Xcode toolchain, CMake 3.20 or later, and the SDKs in the
[dependency inventory](../../dependencies/README.md). C++17 and the core's full
library dependencies apply.

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel
ctest --test-dir build/macos --output-on-failure
```

The preview targets macOS 27.0 by default. For an existing build directory
configured below that floor, use a fresh directory or pass
`-DCMAKE_OSX_DEPLOYMENT_TARGET=27.0` when reconfiguring. Choose a higher target
if required by your linked SDKs. Audit the dependency closure before distributing
a bundle. See
[runtime audit/bundling](../../doc/modernization/native-runtime-packaging.md).

Static SDKs can specify SWORD_INCLUDE_DIR/SWORD_LIBRARY,
GLIB_INCLUDE_DIR/GLIB_CONFIG_INCLUDE_DIR/GLIB_LIBRARY and explicit extra link
libraries. Check actual library architecture and deployment minima.

## Running and checking

Use a disposable generated fixture for UI verification:

```sh
cmake --build build/macos --target preview_fixture
sh platforms/macos/launch-fixture.sh build/macos reader
```

The launcher copies the app and fixture to a temporary location and uses a
separate profile there. Inspect the actual window to verify the workflow. Normal
launch discovers installed modules and uses the native profile. Development overrides are
XIPHOS_MODULE_ROOT, XIPHOS_PROFILE_DIRECTORY and XIPHOS_SESSION_PATH. Give GUI
test copies unique bundle identifiers to prevent tools selecting the wrong profile.

Settings, saved module defaults, recovery preferences, reading tabs/detaching,
parallel sets and personal notes are implemented in scoped form. Languages are
English plus partial French/German/Spanish; book labels may be localized while
canonical reference input is unchanged. BibleSync requires explicit connection.

[User guide](../../doc/user-guide.md) · [Status/evidence](../../doc/modernization/status.md) ·
[Mac handoff](../../doc/modernization/handoff-macos.md) ·
[Native UI to-do](../../doc/modernization/native-ui-todo.md) ·
[Historical build/adapter notes](../../doc/modernization/history/macos-readme.md)

## HTTPS transport

The shared core selects URLSession on Apple builds. The Mac adapter uses
system certificate trust by default and supports an explicit PEM trust
bundle for controlled repositories and tests. SWORD module discovery and
installation remain in the shared core. Build the carried SWORD SDK with
curl disabled as described in the [SWORD setup](../../dependencies/sword/README.md).
