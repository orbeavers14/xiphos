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

Requires macOS 13+, a Swift 6-capable Xcode toolchain, CMake, SWORD >=1.8.1
and GLib >=2.32 (the existing portable Unicode utility, without GTK).
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

GLib also uses pkg-config discovery; prebuilt SDKs can set `GLIB_INCLUDE_DIR`,
`GLIB_CONFIG_INCLUDE_DIR`, `GLIB_LIBRARY` and static transitive dependencies in
`XIPHOS_GLIB_EXTRA_LIBRARIES`. The preview is a development build. A distributable
app still needs dependency bundling, notices, source/relink materials as applicable,
signing and compatibility validation; the build does not establish release readiness.

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
reference sword/bible links are supported; unsupported URI actions leave the
pane unchanged. Generated study-note links are described below.
The shared core supplies chapter fragments, selected-verse anchors and optional
verse numbers/highlights. Display controls refresh the committed passage; the
native web view scrolls to the selected verse with module scripts disabled.
Full Xiphos decorations/options, synchronized commentary,
parallel views, semantic links, images, search, persistence, editors,
installation and BibleSync are pending. JavaScript and network content are
disabled in this development pane. The app is unsigned, unnotarized and linked
against the installed SDK; it is not a redistributable standalone package.
See [parity ledger](../../doc/modernization/parity-matrix.md).

The async adapter exposes copied module option eligibility, typed engine flags,
and module-specific settings from the shared core. The native Reading options
menu currently binds Words of Christ in Red, Strong's Numbers, Morphological
Tags, Footnotes, Cross-references, Greek Accents, Hebrew Vowel Points and Hebrew
Cantillation, Lemmas, Transliterated Forms, Glosses and Morpheme Segmentation
when the displayed module declares those options. Textual Variants offers Primary Reading, Secondary
Reading and All Readings for modules with declared variant filters; selecting
one updates all three legacy variant fields in the shared core. New native
tabs opt in to the shared legacy defaults. Preferences → Import reading options
opens a native picker for an existing Xiphos `modops.conf`; its engine settings
replace this window's options without changing the file. Other windows keep
their settings. Preferences → Save reading options opens the native Save panel
for an explicit `modops.conf` destination, including the system overwrite
confirmation. Saving preserves unrelated settings through the shared core.
Canceling either picker does nothing. Automatic restoration on launch remains
pending. Settings remain separate for each module and window.
Changing an option refreshes the committed passage without adding history or
replacing unsubmitted reference input. Verse and chapter views use the same
configured tab. The SWORD red-letter class uses the existing Xiphos editor's
red style; user-defined colors and full theme parity remain pending.

`reader_model_options` checks these controls with two original generated OSIS
modules and two model windows. `swift_option_adapter` also exercises the typed
bindings. Declared eligibility does not prove filter/markup support across all
SWORD versions. Clicking a generated study-note or cross-reference marker opens
a core-rendered body preview below the module list without changing the main
passage, reference draft or history. The preview identifies its source module
and passage; Close dismisses it, and a successful passage refresh clears stale
preview content. Links inside a note use that note's module as their fallback.
Scripture-reference links inside notes open a sidebar passage list through the
core using the note's module and committed source context. Single references,
ranges and relative lists preserve the main passage, draft and history.
Show referenced passages expands a note's reference list through the shared
core, keeping order, duplicates, module settings and source context. It leaves
the main passage, draft and history unchanged. Back to note restores the body;
a passage heading navigates the main reader. Empty results and the existing
2000-verse range limit are reported.
Hover/pin behavior, legacy unknown-module fallback, Strong's/morphology
lookup, remaining option menus, full display-preference
migration and visual/workflow parity remain pending.

For a local GUI check with those generated modules, run:

```sh
cmake --build build/macos --target preview_reading_options_fixture -j4
```

This opt-in target tests and copies only the generated fixture and preview to a
temporary directory outside Documents. It does not launch during ordinary
builds/CI or access an installed user library. Launch success alone is not a
GUI check; quit the temporary app before removing its copy.


The Swift worker also exposes explicit dictionary previews as copied
`RenderedDictionaryEntry` values using the shared C API. Original RawLD
fixtures check settings isolation, Unicode, failure recovery and lifetime;
the native dictionary pane and Strong's/morphology link dispatch are pending.


CoreWorker now supports copied lexicon preferences and typed lexical previews.
Explicit default enablement obtains the five legacy names from the shared core;
nullable/empty names remain distinct. The adapter rejects embedded NULs and
owns source/kind/raw value plus dictionary key/HTML after shutdown. Fixture
checks cover OT/NT context and separate tabs. Native preference/pane/hover/click
workflows and lexicon profile persistence are the next integration work.


The copied catalog now exposes dictionary classification and declared lexicon
roles. Classification follows the legacy dictionary type and excludes
DailyDevotion, with SWLD validation; role matching uses the exact first SWORD
Feature entry (GreekDef/HebrewDef/GreekParse/HebrewParse). It does not union all
Feature entries or promise effective lookup compatibility. Both synchronous
and worker Swift adapters expose the same copied values. Native selection
controls can use these flags without accessing SWORD or duplicating matching.


The Preferences menu now opens a native Lexicon dictionaries sheet with Greek
and Hebrew lexicons, Greek NT/OT morphology and Hebrew morphology. Choices use
core catalog roles; unavailable current names are retained until a valid choice
is applied. Cancel discards the draft, and the legacy -- Select -- placeholder
preserves the current choice. Apply changes only this window's copied settings,
without changing Bible passage/history or unfinished reference input. Profile
persistence, default general dictionary and lookup-pane interactions are pending.
For disposable UI checks, run the local fixture tests and launch
`sh platforms/macos/launch-fixture.sh build/macos lexicons`; this copies only
original generated data and the preview into temporary storage.


Contextual Strong's and morphology clicks now receive copied lexical results
from the shared worker and show a dictionary column on the right. Selection,
source testament and rendering stay in C++; Swift only presents the returned
module/key/HTML. Nested links use the captured source passage. Closing or
replacing the result preserves Bible history and unfinished reference input.
The right column is an initial composition: commentary above it, pane toggles,
manual lookup, dictionary key navigation/history, hover and pinning remain
pending. This is not full dictionary or visual parity. Generated dictionary
fixtures now include original word/morphology markup for real WebKit click checks.


Selecting a classified dictionary in the module list now opens its own lookup
column and keeps the committed Bible selected. The column has a filtered module
picker and word/key field, and uses the shared dictionary reader. A successful
lookup copies the SDK-resolved key into the dictionary field; errors retain the
previous definition. This does not implement legacy click Unicode normalization,
key browsing or dictionary history. Devotional classification remains separate.


Previous key / Next key call the shared navigation request with the committed
dictionary module/key, replacing unfinished dictionary input only after success.
A pending module choice must be looked up before key navigation is enabled.
Source passage context, Bible history and unfinished Bible reference remain
unchanged. SDK boundary behavior is retained; these are key steps, not dictionary
history Back/Forward. Separate history and the nine-key chooser are available
as described below.


CoreWorker now exposes copied Sendable dictionary-history entries/cursor/flags,
explicit visit, Back/Forward/Clear commands and recorded contextual lexical
clicks. Preview and recorded paths share their Swift ownership bridge and the
same C++ parser/renderer. Core snapshots are released after copying and values
survive engine shutdown. Native history controls and shared main-click Unicode
normalization are described below.


The native dictionary column now has its own Back/Forward/History menu. Lexical
clicks use recorded core dispatch and refresh copied history flags; repeating
the current click preserves Forward. Direct input and previous/next key steps
retain the existing no-record workflow. Clear retains the displayed definition
and all Bible state, and closing the dictionary retains its history. History
navigation uses committed current Bible context for nested links. Detached
windows, hover/pinning, persistence and full pane parity remain pending, so
dictionary feature acceptance is still partial.


The nearby-key menu presents nine copied core choices, retaining boundary repeats
with distinct row identities. Each successful dictionary result refreshes choices
through one guarded presentation path; selecting a choice uses direct lookup
and does not record history. Invalid/stale selections are ignored. The module
picker now uses the full column width; nearby-key and history icon menus keep
accessible names and help text while fitting the narrow column.

Main-pane lexical clicks now use the shared GLib Unicode-uppercase requested
key for reading/history while retaining raw link metadata. Hover/preview reads
and dialog-origin clicks use raw keys and leave main dictionary history alone.
Swift contains no duplicate casing implementation. Host process-locale setup,
representative real modules and cross-platform acceptance remain separate gates.
