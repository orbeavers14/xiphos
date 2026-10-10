# macOS native reader preview

This is a read-only vertical slice, not a complete Xiphos replacement. It has
installed module discovery, a module sidebar, direct reference entry, shared Back/Forward/Clear History, and a
WKWebView verse pane backed by the shared C++ core. The adapter owns only C
handles and copied presentation values. It does not implement SWORD logic.

The application shares one C++ engine worker across windows. Each window owns
its own core study tab and history. The Swift actor submits copied requests and
polls without blocking the main thread; discovery, rendering and navigation
all run on the engine thread. Loading controls serialize window actions, and
closed windows discard late presentation results. Closing a tab or draining
the engine runs away from the main thread. Canceling a Swift task does not
cancel an accepted engine action; active SWORD cancellation is pending.
The synchronous adapter remains available for characterization tests.

Requires macOS 13+, a Swift 6-capable Xcode toolchain, CMake and SWORD >=1.8.1.
Swift sources compile in Swift 6 language mode.
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

The app uses standard SWORD module discovery for normal launches. For an
isolated GUI test, use the opt-in target after configuring the macOS build:

```sh
cmake --build build/macos --target preview_fixture
```

This builds the preview, runs the core fixture test, then copies the app and
only the generated primary fixture into a unique `/private/tmp/xiphos-preview.*`
directory. Launch Services starts that copy with an explicit module root and a
fresh window. It does not request access to the Documents-hosted repository or
change privacy settings. Temporary copies stay available for inspection; quit
that preview before removing its printed temporary directory. This target is
not part of default builds or CI. To launch an already-built/tested fixture:

```sh
/bin/sh platforms/macos/launch-fixture.sh build/macos
```

Run GUI launches from a normal user Terminal or Finder. In a restricted agent
command environment, request an approved launch outside that environment.
A sandboxed direct executable launch can abort during AppKit's
`_RegisterApplication` before the reader starts; this signature alone does not
identify an engine defect. Do not change system security settings to work
around it. `open` returning success is not a GUI smoke test: verify the window
and rendered content. Core/adapter tests remain separate from GUI checks.
macOS can prompt when an app reads protected Documents storage. The user
confirmed repeated Documents permission prompts during these checks. Use the
fixture target above for development rather than repeatedly granting access.
Normal user-library permissions and stable app signing remain release work.

Select `ParityTest`, enter `John 3:16`, then `John 3:17`: the displayed original
fixture text should change. Back should restore John 3:16, Forward John 3:17,
and Clear History should disable both buttons while keeping the current text. No test text comes from a copyrighted Bible.
No modules is a supported startup state. A nonexistent explicit root reports
an error. A non-verse-key module reports that this slice does not support it.
SWORD normalizes references; this is not strict input validation.

Book/Chapter/Verse menus use shared-core choices for the committed passage.
The module supplies its canon, labels and normalized targets. Successful choices
participate in history; the book menu hides absent testaments using legacy
first/last-entry detection. Numeric localization and legacy menu composition
are still pending.

Select Whole chapter to display all verse positions in the module's chapter.
Previous/Next chapter use shared C++ navigation and participate in history.
Clicking a verse number visits it through the shared session. Exact-ID single
reference sword/bible links are supported; ranges, notes and other actions are
pending and leave the pane unchanged.
The shared core supplies chapter fragments, selected-verse anchors and optional
verse numbers/highlights. Display controls refresh the committed passage; the
native web view scrolls to the selected verse with module scripts disabled.
Full Xiphos decorations/options, synchronized commentary,
parallel views, semantic links, images, search, persistence, editors,
installation and BibleSync are pending. JavaScript and network content are
disabled in this development pane. The app is unsigned, unnotarized and linked
against the installed SDK; it is not a redistributable standalone package.
See [parity ledger](../../doc/modernization/parity-matrix.md).
