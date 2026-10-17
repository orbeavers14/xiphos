# macOS native development preview

For a new chat or computer, first read the [shared handoff](../../doc/modernization/handoff.md)
and [macOS task/evidence instructions](../../doc/modernization/handoff-macos.md).

This preview is an incremental implementation of the
[shared-core/native-frontends goal](../../doc/modernization/architecture.md),
not a complete Xiphos replacement. The existing GTK app remains available.
Current functionality, evidence and acceptance gates are in the
[status ledger](../../doc/modernization/status.md) and
[parity matrix](../../doc/modernization/parity-matrix.md).

## Build and dependencies

Requires a Swift 6-capable Xcode toolchain, CMake >=3.16, SWORD >=1.8.1,
GLib >=2.32 and libxml2 headers/library. Swift compiles in Swift 6 language mode;
the native application core requires C++17 and legacy-linked helpers retain
C++11 compatibility. GLib/libxml2 reuse existing legacy dependencies without GTK.

The requested deployment target defaults to macOS 13.0. Actual compatibility
also depends on SDKs/libraries: the checked local SWORD 1.9.0.3916 and GLib
2.86.5 builds require macOS 26.0. Set CMAKE_OSX_DEPLOYMENT_TARGET accordingly;
local runs do not validate macOS 13. Existing compatible onChange calls emit
newer-SDK deprecation warnings. The app is unsigned/unnotarized and linked to
local SDKs; release bundling, applicable notices/source/relink material, signing
and minimum-version validation remain pending.

With normal pkg-config-enabled SWORD/GLib SDKs and discoverable libxml2:

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel
ctest --test-dir build/macos --output-on-failure
```

A prebuilt SWORD SDK can supply SWORD_INCLUDE_DIR/SWORD_LIBRARY. Static SDK
transitive libraries omitted by sword.pc must be explicit, for example:

```sh
cmake -S platforms/macos -B build/macos \
  '-DXIPHOS_SWORD_EXTRA_LIBRARIES=z;bz2;/absolute/path/to/liblzma.dylib'
```

GLib fallbacks are GLIB_INCLUDE_DIR, GLIB_CONFIG_INCLUDE_DIR, GLIB_LIBRARY and
XIPHOS_GLIB_EXTRA_LIBRARIES. FindLibXml2 discovers its SDK. Verify actual binary
architecture/version/deployment minima. No dependency source is vendored here.
Normal launches use standard SWORD module discovery; XIPHOS_MODULE_ROOT selects
an explicit isolated library for testing. The adapter owns C handles and copied
values; reference parsing, rendering, selection, histories and persistence stay
in C++ rather than a second Swift/SwordKit engine.

## Disposable GUI checks and Documents access

Build/run fixture tests before launching a GUI copy:

```sh
cmake --build build/macos --target preview_fixture
cmake --build build/macos --target preview_reading_options_fixture -j4
```

These opt-in targets build/test and launch only generated original-prose modules.
They are excluded from normal builds/CI. For an already checked build/fixture:

```sh
sh platforms/macos/launch-fixture.sh build/macos reader
sh platforms/macos/launch-fixture.sh build/macos books
sh platforms/macos/launch-fixture.sh build/macos book-documents
sh platforms/macos/launch-fixture.sh build/macos book-presentation
sh platforms/macos/launch-fixture.sh build/macos reading-options
sh platforms/macos/launch-fixture.sh build/macos lexicons
sh platforms/macos/launch-fixture.sh build/macos lexical
```

Each launch copies the app and selected generated fixture to a unique
/private/tmp/xiphos-preview.* directory, then uses Launch Services with an
explicit root/fresh window. It does not access an installed library or change
privacy settings. This avoids the repeatedly reported Documents prompts for
repository-hosted GUI builds. Quit the temporary preview before removing its
printed directory; keep copies only as long as inspection needs them.

Launch from a normal user Terminal/Finder or an authorized app launch tool.
A restricted direct executable launch can abort in AppKit _RegisterApplication
before engine startup; that signature alone does not identify an engine defect.
Launch success alone is not a GUI check: inspect the window and content.
Normal user-library permissions and stable signed app identity remain release
work; do not weaken system security to bypass them.

## Ownership and window behavior

One shared C++ worker per process constructs/uses/destroys SWORD on its engine
thread. Each window owns a core tab, histories and preferences. The Swift actor
submits copied requests and polls without blocking the UI; accepted actions run
FIFO. Loading guards serialize window actions; closed/revised windows discard
late presentation. Canceling observation does not cancel accepted engine/file
work, and shutdown drains/joins away from the UI thread. Active SWORD operation
cancellation and independent concurrent engine instances are not promised.
The synchronous adapter is retained for characterization tests.

## Current controls and contracts

| Area | Locally checked behavior |
|---|---|
| Reading | Module sidebar, reference input, core Book/Chapter/Verse choices, Back/Forward/Clear, whole chapter, adjacent chapters, verse-number navigation and selected-verse scroll |
| Rendering | Declared options for red words, Strong's, morphology, footnotes, cross-references, Greek accents, Hebrew vowels/cantillation, lemmas, transliteration, glosses, segmentation and textual variants |
| Reading preferences | Explicit native import/save for 18 modops.conf engine fields; per-module/window state and unknown/uninstalled values preserved |
| General books | Core-eligible module selection, direct key/path entry, parent/first-child/previous/next sibling commands and bounded contents browsing in the lower-right pane; failed input retains reading; full visual/corpus/resource/restore acceptance pending |
| Dictionary | Separate module/key input, previous/next key, nine nearby keys retaining repeats, recorded lexical-click history and Back/Forward/Clear |
| Lexicons | Five role-filtered choices with unavailable saved names retained; -- Select -- keeps the prior value, Cancel discards a draft, Apply changes this window only |
| Lexicon XML | Explicit Import/Save lexicon choices pickers using the shared reader/verified writer; five values updated while other settings survive |
| Notes/references | Copied body previews, explicit ordered range/relative/reference lists and source-aware passage navigation; current click destinations remain a separate parity gate |
| Hover | Main-reader/information/sidebar/note-body/dictionary semantic lexical/footnote results retained in Information Preview; cross-reference sidebar or information-link mode selected by a core-owned per-window preference |

Reading/display/option refreshes use committed source context and preserve
unfinished reference input without visiting history. Successful navigation is
explicit; failed actions retain prior content. No modules is a supported startup
state; invalid explicit roots and unsupported module kinds report errors. SWORD
normalizes references rather than providing strict input validation. Book menus
use module canon/testament detection; numeric localization/full composition is
pending.

Dictionary previews/direct input/key steps do not record dictionary history;
main lexical clicks do, with shared GLib Unicode-uppercase requested keys and
raw command metadata. SDK-resolved keys may differ from requested keys. Repeated
current visits preserve Forward. Clear/Close keep Bible state; dictionary Close
retains history and Information Preview. Pending module choices must be looked
up before key navigation. Classification excludes DailyDevotion and uses the
exact first Feature entry; declared roles do not prove effective compatibility.

Engine imports replace only this window's effective engine settings. Lexicon
imports retain unavailable names and publish all five choices after successful
parsing. Explicit saves preserve unrelated values and verify a staged file
before replacement; malformed/empty existing XML files and observed external
changes reject. XML serialization normalizes to UTF-8; null/empty choices cannot
round trip through legacy defaults and reject. Canceling a picker does nothing.
No automatic profile restoration/save occurs. Startup initializes environment
locale before the worker; saved forced locale is read as data and not applied.
Changing process locale while engines run is unsupported.

Hover results preserve drafts and histories without disabling reading controls.
Leave/no-presentation/error keeps the last information/status. A latest-result
and display/window revision guard prevents stale publication; reading/profile/
preference actions invalidate pending hover work. Cross-reference mode defaults
enabled and is currently in memory per window. List mode retains the last
information; information mode shows links and Back to committed context. The
trusted main-frame WebKit transport runs in the isolated client world, bounds
payloads and keeps module JavaScript/network content disabled. All URI/type/
source selection/render decisions remain in C++. See
[hover contracts](../../doc/modernization/hover-workflows.md).

## Validation and remaining work

Run the complete local suite after a checked milestone. Current tests include
core characterization/fixtures, C and Swift ownership, profile/hover adapters,
and reader model options/lexicons/profile/hover/locale modes. Temporary GUI checks
cover visible controls and real pointer events; they do not establish whole-app
visual/workflow parity. Both hosted workflows remain paused; use local checks and reserve
platform CI at meaningful review/release checkpoints.

Representative modules, macOS minimum-version validation, complete main-pane
composition, complete commentary/parallel/book/devotional workflows, remaining click/hover
families, heading/alias/global-source behavior, keyboard/accessibility,
localization/styles/images, full settings/session restoration, interprocess
storage locking/power-loss durability, search/editors/installation/BibleSync and
native Windows/Linux validation remain pending. No complete feature group or
native release is accepted yet; passing local checks do not authorize retirement
of required legacy source, regression tests or license material.

The [prior guide at an immutable pushed commit](https://github.com/orbeavers14/xiphos/blob/c2937a40a1e297bdd2e74907fe5f7af2be6ef688/platforms/macos/README.md)
retains chronological slice notes/evidence. Use this guide and current status
for today's scope; older pending statements describe earlier checkpoints.

## Initial synchronized commentary pane

Select a commentary from the installed-module sidebar or the commentary picker
to display it at the committed Bible verse. Commentary occupies the upper-right
resizable pane; dictionary content appears below. Passage changes and Back/Forward
use shared core module pairs. Changing/closing commentary preserves unsent Bible
reference input. Bible and commentary have independent whole-chapter controls. The Display menu
applies verse numbering/highlighting to either chapter view. Core-owned chapter mode follows the legacy per-module default and participates in
explicit reading-options import/save. Commentary headings use owned core strings
and the shared per-module preference in verse/chapter views. The control governs
preverse headings; interverse titles remain in SDK body output. Saved pane settings/
startup, complete heading transformations, personal editors and detached windows are
pending; see [scope and acceptance](../../doc/modernization/commentary-workflows.md).

After local fixture checks, `sh platforms/macos/launch-fixture.sh build/macos commentary`
opens generated original-prose Bible/commentary/dictionary data outside Documents.
This avoids repeatedly asking the preview for Documents access; it does not change
macOS permissions or read the installed user library.

Use fixture kind `headings` to inspect original-prose OSIS headings and heading-only
entries after the local heading checks pass.

## Image and font display controls

Reading options and Commentary options show Image content/Respect font faces once
the shared core knows each setting through content discovery or explicit import.
Unknown fields remain hidden. Toggles refresh the committed passage, retain draft
input/history and use per-module/window state. Explicit reading-options save/import
persists known states. The `headings` fixture includes original styled Bible and
commentary data; its resource/font names are synthetic, so resource loading and
installed-font appearance require separate validation. See
[scope and acceptance](../../doc/modernization/html-cleanup-workflows.md).

## Shared lexical layout checkpoint

The reader opts its core tab into copied lexical presentation. Main/commentary
entry and chapter views publish matching HTML/CSS; CSS-only changes reload the
view. Layout, whole-document fallback and format-before-cleanup decisions remain
in C++. Disabled options or unsupported documents remove annotation CSS. The
private `lexical` fixture exercises real declared SWORD filters, headings, safe
word layout and unsupported cross-word font nesting without installed modules.

At `d953adc`, the integrated local suite passes 68/68. Actual temporary-window
checks covered Strong's enablement, its positioning beneath the word, chapter
selection/scroll and readable raw fallback. Model checks cover paired commentary,
drafts/history, cleanup and window/closed-state isolation. Representative module,
RTL/font, accessibility, minimum-OS and full visual parity remain unaccepted.

The later [double-spacing checkpoint](../../doc/modernization/double-space-workflows.md)
adds separate main/commentary controls through the shared preference command.
The current suite passes 70/70; a temporary-window check confirms visible on/off
chapter spacing refresh with unchanged passage/selection. CSS may contain only
spacing while annotation requested/applied flags are false; views use copied CSS.

## Shared note-number controls

Reading options and Commentary options provide independent “Footnote and
cross-reference numbers” toggles using the shared queued core setting. Profiles
save/restore it; toggles refresh committed context without adding history or
replacing a typed reference. Supported supplied labels and legacy document
counters come from C++, including fragment cleanup order and lexical fallback.

At `1d3612f`, 75/75 local Mac/core tests and a disposable window check cover the
controls. To reproduce using original-prose data, run local tests first, then:

```sh
sh platforms/macos/launch-fixture.sh build/macos notes
```

The [note contract](../../doc/modernization/note-numbering-workflows.md) records
remaining WORD lexical markup, heading/TEI identifiers and full corpus limits.

## General-book navigation checkpoint

The Mac adapter copies core tree/entry/page values before releasing C results.
Swift owns controls and visibility only; typed key interpretation, navigation,
SDK relations and successful selection publication stay in the shared worker.
The book pane uses the dictionary/book area below any commentary pane. Browsing
contents does not change the reading selection. Closing/reopening the current
book retains its source and draft key; window selections remain isolated.

`reader_model_books` covers module routing, hierarchy, shared commands, entryless
branches, rejected input, preserved Bible presentation and closed-window results.
A disposable original-prose window checked parent/child/sibling controls,
individually accessible contents buttons, branch browsing and reading, and rejected
key retention. Complete keyboard/VoiceOver, pagination/corpus, layout and platform
acceptance remain open. Book ordinary footnote clicks now use the shared exact-source command and the
sidebar preview. Failed/mismatched links retain the previous preview; successful
reading removes stale notes. Book-note and verse-reference previews replace each
other without changing the reading source. A disposable window verified the
marker/body and navigation cleanup. Other book links/resources and full saved-tab
restoration remain subsequent work. No installed module library or personal profile was used.

`preview_books_fixture` is opt-in and local, like the other fixture launch targets;
it never runs as part of a normal build or enables GitHub Actions.

## Configured book documents

The book pane now presents complete shared-core DisplayLevel documents. Swift
copies selected root, ordered sections, combined HTML and optional results; depth,
traversal, publication and descendant-note routing remain in C++. Raw single-entry
and read-only document APIs keep their existing behavior. A traversal/render
failure retains the previous reading, contents and note; successful navigation
clears stale note previews. Entryless branches can display their descendants.

`swift_book_document_adapter` checks ownership, depth/order, exact note context,
restore modes, explicit read-only XML import and failure/window isolation.
`reader_model_book_documents` checks the pane and core-owned workflow. A disposable
original-prose window verified combined text/separators, a descendant marker/body,
entryless-branch content and retained reading after a document-limit failure.
The current full local Mac/core suite passes 102/102. Display cleanup/annotation
layout, images/resources, generalized book links, saved tabs and complete visual/
corpus/platform acceptance remain pending. No complete parity is implied.

Book controls live in `BookPane.swift`; `ReaderApp.swift` retains the window model
and composition. The view move preserves the checked control implementation;
app/model builds and six focused book/adapter checks pass after extraction.

## Book display snapshots and controls

Selected book documents now also copy a separate shared presentation: visible
HTML, matching annotation/spacing CSS, image/font states and cleanup diagnostics.
Raw combined and per-section SDK HTML remains available. Raw document reads have
no presentation. The book pane consumes the matching HTML/CSS packet and exposes
Double space, Image content, Respect font faces and Refresh reading through native
controls. Core refresh retains the committed requested target, including explicit
root paths; controls preserve an unsubmitted key and do not visit history.

The full local Mac/core suite passed 105/105; the compact menu adjustment then
passed app/model builds and nine focused checks. A disposable original-prose
window checked menu state, image/font toggles, spacing, explicit-root refresh with
an unsubmitted draft, and the compact header layout. Actual image/resource loading,
book note numbering, generalized book links and full font/RTL/corpus acceptance
remain pending. No installed content or personal profiles were used.

### Separate book image preview (local checkpoint)

Book image commands now use shared-core typed link dispatch and bounded local
resource bytes. Swift copies the result, decodes an ImageIO thumbnail off the UI
thread (maximum 2048 pixels), and presents a native sheet with Close. Failed
loads retain the current reading and existing preview. This does not yet provide
inline book images, full-size/animated viewing, or validated support for every
legacy image format. Run `sh platforms/macos/launch-fixture.sh build/macos book-images`
for the original-prose PNG fixture. The separate preview opened and closed in a
local window; the accessibility label subsequently compiled and passed focused
model checks, but full VoiceOver acceptance remains pending.

### Inline book images

The book pane consumes shared-core resource-prepared HTML separately from raw
SDK and ordinary display HTML. Only this pane permits data images; scripts,
network and file subresources remain disabled. The shared image-content control
refreshes cleanup/resource preparation. Missing resources report a count while
retaining book text. The `book-images` fixture verified the PNG inline, its
native preview click/Close, image-off text retention, and the preview image's
accessibility label. Arbitrary corpus, sizing, CSS assets and full format/visual
acceptance remain pending; libxml recovery can normalize prepared markup.

### Module stylesheets

Book documents now consume core-owned module CSS, including PreferredCSSXHTML
and the legacy style.css fallback. The adapter copies raw CSS and separately
prepared style text; it performs no path selection or file reads. Global profile
CSS and recursive assets/fonts/imports remain pending. Unusable styles retain
reading text with a status message. `book-stylesheets` launches an original-prose
fixture; its preferred color/pseudo-element and unavailable-style text fallback
were checked in a local window. Full fonts/RTL/corpus/design acceptance is open.

### Combined book note numbering

The book display menu now controls shared footnote/cross-reference numbering.
The core preserves Xiphos's combined-document counters and terminal-label
suppression; Swift only copies metadata, toggles the existing setting and
requests refresh. `book-note-numbers` launches the generated original-prose
fixture. A local window checked cross-section n1/x1/n2/x2, descendant-note body
identity, and Off/On refresh with the same selected root. Full corpus, hover and
saved-tab/visibility acceptance remain pending.
