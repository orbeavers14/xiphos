# macOS native reader preview

This is a read-only vertical slice, not a complete Xiphos replacement. It has
installed module discovery, a module sidebar, direct reference entry, shared Back/Forward/Clear History, and a
WKWebView verse pane backed by the shared C++ core. The adapter owns only C
handles and copied presentation values. It does not implement SWORD logic.

Requires macOS 13+, Xcode Command Line Tools, CMake and SWORD >=1.8.1.
The requested deployment target defaults to 13.0; actual compatibility also
depends on the SWORD SDK minimum OS. Set `CMAKE_OSX_DEPLOYMENT_TARGET` to match
a newer SDK if necessary (the inspected local SDK requires 26.0). With a
normal pkg-config-enabled SDK:

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel
ctest --test-dir build/macos --output-on-failure
open build/macos/XiphosNativePreview.app
```

For a static SDK whose sword.pc omits dependencies, pass those explicitly:

```sh
cmake -S platforms/macos -B build/macos \
  '-DXIPHOS_SWORD_EXTRA_LIBRARIES=z;bz2;/absolute/path/to/liblzma.dylib'
```

The app uses standard SWORD module discovery. For an isolated preview with the
test module, launch a fresh app instance through macOS Launch Services after tests:

```sh
/usr/bin/open -n --env "XIPHOS_MODULE_ROOT=$PWD/build/macos/core/fixture" \
  build/macos/XiphosNativePreview.app
```

Run GUI launches from a normal user Terminal or Finder. In a restricted agent
command environment, request an approved launch outside that environment.
A sandboxed direct executable launch can abort during AppKit's
`_RegisterApplication` before the reader starts; this signature alone does not
identify an engine defect. Do not change system security settings to work
around it. `open` returning success is not a GUI smoke test: verify the window
and rendered content. Core/adapter tests remain separate from GUI checks.

Select `ParityTest`, enter `John 3:16`, then `John 3:17`: the displayed original
fixture text should change. Back should restore John 3:16, Forward John 3:17,
and Clear History should disable both buttons while keeping the current text. No test text comes from a copyrighted Bible.
No modules is a supported startup state. A nonexistent explicit root reports
an error. A non-verse-key module reports that this slice does not support it.
SWORD normalizes references; this is not strict input validation.

Only SWORD entry XHTML is shown. Chapter display, synchronized commentary,
parallel views, semantic links, images, search, persistence, editors,
installation and BibleSync are pending. JavaScript and network content are
disabled in this development pane. The app is unsigned, unnotarized and linked
against the installed SDK; it is not a redistributable standalone package.
See [parity ledger](../../doc/modernization/parity-matrix.md).
