# Native Xiphos migration: architecture and baseline

Baseline: `orbeavers14/xiphos`, `master` at
`c9976a873bf77630af67ec8df84642225e33f6b3` (2026-10-10 inspection).
GitHub metadata confirms parent `crosswire/xiphos`. Initial slices used fork
branches; the owner now authorizes locally checked commits directly to the fork's
`master`. Existing files and user data are preserved. This is an incremental migration,
not a claim that native Xiphos has full parity.

## Build and dependency audit

The top-level CMake build creates static backend/main/editor/WebKit/HTML
libraries and the GTK executable. C and C++ are mixed; `src/gui` defines the
GTK-facing headers, and `ui/*.gtkbuilder` describes menus and dialogs.
`src/backend` is NOT platform independent: sword_main.cc includes GTK,
settings, search UI, and dialog callbacks; module_manager.cc uses application
state. `src/main` also directly references widgets. Moving those directories
unchanged would preserve coupling rather than create a core.

CMake requires SWORD >=1.8.1, BibleSync (>=1.2.0 Unix / >=2.0.1 Windows),
SQLite (AndBible imports), GLib/GModule, ICU, UUID (Unix), libssh2 (Windows),
libsoup 3 or 2.4, GTK3, ATK, Cairo, GDK, Pango, minizip, zlib, libxml2,
and WebKitGTK 4.1 or 4.0. Legacy WebKit1/GtkHTML paths are optional.
Gettext is optional; build tools include glib-genmarshal, xmllint, xsltproc,
itstool, desktop-file-validate, appstreamcli, and selected help/package tools.
GTKTVEDITOR must be selected for a usable editor with WebKit2; the default
otherwise warns that notes/journal editing is unavailable.

Existing Actions build Linux distributions and cross-compile Win32/Win64;
Windows packaging uses a Fedora 30 Docker image and MinGW/GTK, not WinUI.
Existing jobs also build help and release packages. They remain intact.
The new standalone core build avoids desktop/help dependencies and permits
engine integration tests on macOS and Linux. Windows can use SWORD include/
library SDK paths; an actual MSVC SWORD SDK and WinUI build remain unvalidated.
Local SDK: AppleClang 21, SWORD 1.9.0.3916; its static sword.pc omits zlib,
bzip2 and lzma link requirements, so explicit extra libraries are needed.
That SDK was built for macOS 26.0; local preview validation uses that target,
not evidence of compatibility on macOS 13.

### Development hosts and platform acceptance

Three target operating systems do not require three physical computers. A Windows
host can develop WinUI 3 directly and build/test Linux code in WSL2 or a Linux VM.
[Microsoft documents Linux GUI support in WSL2](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps)
on Windows 10 build 19044 or later and Windows 11, including X11 and Wayland apps.
WSLg can support iterative GTK4 interaction checks, but it does not provide a full
Linux desktop. Distribution packaging, desktop integration and release acceptance
still need a representative Linux desktop, which can run in a VM.

The SwiftUI/AppKit frontend requires access to a Mac for native builds and testing;
[Xcode's supported host systems are macOS](https://developer.apple.com/xcode/system-requirements).
A Mac plus a Windows host with Linux virtualization can therefore cover the three
development targets. Shared core changes should be checked with each target's
compiler and dependencies before claiming cross-platform acceptance. Separate
platform branches are optional; frontend directories and the shared core remain
part of the same repository and implementation.

This is a supported development plan, not evidence that Windows or Linux checks
have run. The current connected environment is macOS; WinUI 3 and GTK4 runtime
validation remain pending. Routine builds stay local and hosted Actions remain
paused to conserve the owner's GitHub allowance.

## License and attribution audit

COPYING contains GPL v2. Inspected C/C++ headers, build scripts, and manual
legal.xml grant GPL v2 **or later**. Preserve those notices, AUTHORS, and
COPYING; new source uses SPDX GPL-2.0-or-later. GitHub's GPL-2.0 label alone
is less specific than these source notices. The original project credits
SWORD, BibleTime and BibleCS contributions. Do not erase those acknowledgments.
The inspected installed SWORD swmgr.h grants GPL version 2 (without an
or-later clause). Its dependency terms constrain the linked distribution.
SWORD and each bundled/transitive library must retain their own license and
source/distribution obligations; modules have independent content licenses.
The generated test module contains original prose and downloads no Bible text.
Vendored Windows Festival resources have separate COPYING files (including
voice/dictionary terms): review each before reuse in native packages.
SwordKit was subsequently inspected locally: README declares GPL-2.0-only
and LICENSE contains GPL v2. The initial reuse screen and constraints are in
[status.md](status.md#reuse-from-related-projects). No SwordKit code is imported
by this slice; actual imports need file-level provenance/dependency review.
Packaging and store distribution require a separate dependency/notice audit;
this engineering inventory is not a completed distribution license review.

The shared dictionary-click casing seam reuses GLib, already required by the
legacy app, without GTK/GModule in the native core. The inspected GLib 2.86.5
`glib/gutf8.c` is LGPL-2.1-or-later; its COPYING and per-file notices remain
external dependency material. Local-only characterization used a pinned
source archive from GNOME with SHA-256
`ce85a947bb8b3c0204dbeff79aec39bcb46371c6fafb64ba5b8726c71e038d5f`, matching
GNOME's published checksum. No GLib source is vendored into this repository.
Release packaging must account for its linked form, source/notice and relinking
requirements alongside SWORD and other dependencies. Unicode database versions
and process locale are part of cross-platform acceptance, not assumed identical.
See [GLib's casing contract](https://docs.gtk.org/glib/func.utf8_strup.html).

## Boundaries and ownership

SWORD -> shared Xiphos C++ services -> versioned C ABI -> native frontends.
The core owns reference resolution, versification, module policy, rendering
options, search scope/results, history/session models, parallel sets,
annotations/bookmarks/imports and BibleSync state. Platform code owns controls,
window layout, accessibility, file dialogs, clipboard/printing, scheduling and
native integration. Introduce these services incrementally with legacy tests.

* Reader/module seam: SWMgr/SWModule and owned immutable metadata/results.
  Extracted `render_entry` is called by BOTH legacy BackEnd and new Reader.
  The legacy call retains its strdup ownership and module-key side effect.
* Display seam: split `src/main/display.cc`, modulecache/global_ops and URL
  handling into content + semantic actions, then native web renderers. Raw
  The shared content service now produces chapter fragments with verse anchors,
  optional numbers/highlights and escaped reference labels. Hosts supply theme
  CSS and scrolling. Full legacy decorations, annotation coloring, semantic link
  dispatch and preview behavior remain pending. Single-reference sword/bible
  URI decoding now lives in a core link service; the preview routes clicks to
  the shared session and never loads the target URI in its web view. Legacy
  URL parsing and GTK dispatch remain intact until broader characterization.
* Navigation seam: navbar_versekey/navbar_book/tab_history becomes a per-tab
  study session; no frontend-maintained Bible canon or history algorithms.
  Shared reference lookup/history and native session navigation are implemented.
  GTK keeps its existing callbacks/storage, calling the shared lookup rule.
  Chapter snapshots and adjacent-chapter navigation use cloned module VerseKeys.
  Owned book/chapter/verse choices now reproduce the legacy selector targets
  and testament filtering. The core preserves module position during discovery;
  native menus consume copied labels/targets. Full chapter decorations/options,
  number localization, selector composition and persistence remain pending. See status for characterized rules and validation limits.
* Search seam: backend search plus main/search_* becomes an asynchronous
  service with cancellation/progress/result sets; GUI callbacks become events.
* Storage seam: settings.c/configs.cc/xml.c and bookmark GTK data structures
  become repositories with read-compatible loaders. No migration writes until
  backup/round-trip tests validate settings, sessions, bookmarks and notes.
* Installation seam: module_manager/mod_mgr becomes an install service with
  explicit source/trust/cipher/progress policy; platform code supplies dialogs.
* Sync seam: biblesync_glue becomes protocol/session policy with injected
  networking and UI events. Match all modes and privacy controls.

The initial Reader owns one SWMgr. Its reads are serialized per instance;
C handles must also be serialized, and cannot be closed during use. Results
own copies and survive later reads/reader destruction. SWORD has global locale/
filter state: this slice does not promise concurrent independent readers or
runtime locale switching. StudyWorker now constructs/uses/destroys the reader
and tab sessions on one engine thread, serializing copied requests and results
through futures. Native hosts share one worker per process. Initialization/job
errors propagate without stopping the queue; shutdown drains accepted work.
The C ABI now exposes nonblocking worker requests, copied module/history
snapshots and one-time result transfer. The macOS preview consumes requests
through a Swift actor and shares one worker across windows, each with a core
study tab. Main-actor presentation guards closed-window results and serializes
window actions. Worker shutdown drains/joins on a background queue. Callers
must not wait on futures or join shutdown on a UI thread. Cancellation and
interruption of an active SWORD operation remain separate work.

C ABI v1 is experimental, static-linked, and exposes opaque handles, status
codes and UTF-8 borrowed strings with explicit lifetimes. It is not yet a
stable plugin/shared-library ABI; export decoration, dynamic packaging,
semantic action events and asynchronous cancellation are later work.

## SwordKit decision

Evaluate SwordKit as Swift conveniences over this ABI. Its current direct
SWORD bridge must not create a second SWMgr/search/history/annotation engine
inside native Xiphos. Adapt its bridge or introduce a Xiphos-backed protocol;
keep core behavior here, use Swift for presentation and lifetime management.
Do not assume SwordKit's direct SWORD API is the shared application core.

## Native design contract

Keep the established reading/study composition, pane relationships, names,
context actions and workflows. macOS uses SwiftUI/AppKit + WKWebView,
Windows WinUI 3 + WebView2, Linux GTK4 + WebKitGTK. Native menu placement,
keyboard equivalents, focus/accessibility, file dialogs, appearance, window
restoration and platform packaging may differ without dropping capabilities.
The initial macOS shell presents a module sidebar and verse pane only. It is
not a replacement design or a reproduction of the full screenshot.
Windows and GTK4 frontends are planned, not implemented by placeholder code.

## Reviewable sequence and release gates

1. This PR: inventory, rendering extraction, read-only C ABI, generated engine
   fixture, macOS reader/adapter smoke test and additive CI.
2. Shared study-session navigation and chapter rendering/options; compare
   against legacy using a licensed representative module set and screenshots.
3. Equivalent read panes on GTK4 and WinUI 3, accessibility/keyboard baselines.
4. Search/parallel/preview, then persistence/import/editor/installation/sync
   service extractions, each with legacy characterization and round-trip tests.
5. Platform packaging, signing, localization and broad parity acceptance.

The GTK app remains the supported complete implementation during migration.
No release switch until every matrix row is independently validated on each
platform, user data is proven compatible and regression checks pass. Build
success alone never establishes functional, visual or workflow parity.

## Status and retirement

The [status page](status.md) records evidence, acceptance and remaining work.
Follow its file-retirement gates after each accepted milestone. Generated
inventories belong under ignored `build/modernization/`; retain their generator.

## Shared engine filter policy

`CoreOptions` is a C++11 seam used by the existing GTK application's
`main_set_global_options`. It owns the legacy SWORD option names, value mapping,
ordering, always-on engine headings and textual-variant precedence. It accepts
copied booleans and applies them to a caller-owned SWMgr; it does not own a
second engine, persistence, GUI controls or display heading visibility.
The default legacy route builds this seam alongside rendering and history.
Native study sessions now own copied optional settings. Worker updates and tab
chapter reads execute on the engine thread. Reader rendering scopes snapshot
and restore individual filters, because same-name SWORD filters can have
different initial values. The native EngineManager remains the reader's single
owned SWMgr; no second engine is introduced. The nonblocking C bridge exposes named flags, tab set/clear, configured tab
chapters and history-free entry refreshes. Copied module catalogs now expose exact legacy filter-declaration eligibility,
separate headings metadata and global SDK transliteration availability. Those
snapshots do not imply filter/markup support or enabled preferences. Swift copies these values through the async adapter and wraps tab settings and
history-free configured reads. Four native reading controls now bind this state;
remaining controls and full menu composition are pending. Native frontends must not reproduce this
policy or mutate engine filter state themselves.


`CoreOptions::engine_options_from_preferences` also owns the legacy engine
preference defaults and exact value interpretation. The existing GTK loader
copies its SWORD configuration section into this parser; all display/layout
preferences stay on their existing path. Parsing is independent of filesystem
locations and storage writes. Native frontends must obtain this policy from the
core rather than initialize flags independently. Native module preference
storage/import and controls remain pending; existing unconfigured native
sessions continue to use their previous SDK defaults until explicitly configured.


Native sessions now own in-memory module overrides, serialized by StudyWorker.
Module overrides take precedence over an explicit tab fallback; opt-in legacy
module defaults follow that, otherwise unconfigured SDK behavior is retained.
Setting/clearing/querying preferences does not visit or move history. The C
bridge returns copied flags plus a configured marker, so SDK defaults are never
mistaken for an explicit all-off mask. Persistence/import remains separate work. The macOS preview now binds four
reading options and uses configured tab rendering for both verse and chapter
views; frontends obtain defaults and module state through the async adapter.


Explicit legacy engine-preference imports decode through `CoreOptions` using
SWORD's configuration parser on the serialized worker. A session first prepares
settings for every installed module, then commits them as one replacement;
missing sections receive shared legacy defaults. Import resets previous engine
overrides for that tab without visiting history. Unknown/uninstalled sections
are ignored for rendering, and the original file is never rewritten. The macOS picker now binds this import and refreshes the committed passage
without visiting history. Full display preferences, automatic restoration and
compatible save/export remain separate gates.


The native preference save layer uses a shared encoder for the 18 legacy engine
keys and SWORD's writer to retain unrelated semantic values in the chosen file.
It verifies a same-directory temporary configuration before rename replacement,
with permission preservation and explicit unconfigured-state rejection. No
frontend writes configuration or duplicates its schema. Existing source files
are touched only by an explicit save path; import is still read-only. SWORD
normalizes formatting/comment spacing, and its no-variant reload fallback stays
primary. Native save picker, automatic restore, external-writer coordination,
power-loss durability and Windows file-replacement validation are separate gates.

## Language baseline and profile characterization

The native application core targets C++17. Helpers linked into the existing
application remain C++11 compatible; C and the public C ABI remain C11.
Modernization replaces borrowed/global state with owned results, serialized
engine access and checked errors while preserving established workflows. A
later C++20 requirement is a separate toolchain/validation milestone, not a
prerequisite for extracting these services. Keep requirements per CMake target.

The native core additionally requires libxml2 headers/library, an existing
legacy dependency. The current XML helper characterization uses six complete
frozen/actual functions with real libxml2 and no GTK or user-profile access.
The [profile contract](hover-workflows.md#lexicon-xml-profile)
records duplicate/missing-node and escaping behavior. The owned profile reader now parses
explicit files/in-memory XML and returns UTF-8 lexicon preferences plus saved
locale as data. It reuses libxml2 without GTK and preserves the actual getter/
default rules. DTD documents, malformed/wrong-root input, NUL code units and
over-16-MiB documents reject. Parser contexts stop at DTD declarations before
loading resources; process-wide loaders/substitution defaults remain untouched.
The explicit lexicon writer safely encodes literal values and preserves
unrelated XML outside edited values, shadowed duplicates and saved locale. It
verifies serialized/reloaded values and stages an explicit same-directory file
before replacement, sharing permission/symlink/cleanup/change-detection policy
with the existing engine-preference writer. Empty/null choices reject rather
than silently reloading defaults. Existing empty/malformed files are preserved
on error. XML formatting/encoding normalizes to UTF-8. Native XML import/
save controls now delegate through the Swift adapter and serialized core;
saved-locale startup application remains pending.
The serialized worker and C API now expose explicit selected-file import/save:
import prepares copied preferences before tab publication; save rejects an
unconfigured tab before file work. Existing history/engine settings are retained.
Interprocess locking, power-loss durability and Windows replacement acceptance
remain pending. Linux
review CI installs libxml2-dev; macOS uses the SDK and explicit Swift link inputs.

The inspected [libxml2 2.9.13 copyright notice](https://raw.githubusercontent.com/GNOME/libxml2/v2.9.13/Copyright)
grants an MIT-style permission with notice-retention obligations and identifies
per-file exceptions. No libxml2 source is vendored. Native package/release
validation must check the actual linked artifact and retain applicable notices.

### Commentary presentation preference seam

Engine filters and Xiphos presentation settings remain separate value types.
The initial `ModulePresentationOptions` follows the characterized regular/personal
commentary classification and legacy chapter default. Session/worker commands
own per-module state; C/Swift adapters only copy results. A single SWConfig decode
stages engine/presentation import together, and both reuse the existing verified
replacement transaction for explicit save. The C++11 policy helpers remain
available to the legacy build; native ownership/persistence stays C++17. See
[commentary scope and acceptance](commentary-workflows.md) for checked behavior
and remaining headings, editors, corpus and platform work.

### Owned note-renderer state and source snapshots

The private native Reader owns a note-renderer state registry alongside its
manager. SWORD shares XHTML filter objects across modules and exposes label
setters without getters. Registration establishes an explicit Off baseline,
verified against the checked SDK; scoped reads track/restore values by filter
pointer rather than module name. All allocation/registration validation precedes
mutation; the owner/filters outlive scopes and all operations stay serialized.
Legacy unscoped setters remain a separate retained-adapter path.

Configured opt-in C++ Reader calls copy raw body/SDK-heading cleanup-point metadata
into `Entry.note_source` before later renders replace attributes. Captured empty
values differ from unavailable metadata. The Reader flag controls SDK supplied
labels; opt-in StudySession presentation adds owned document counters after
per-fragment cleanup. Queued worker/C capability/settings and typed copied state
serve both native preview adapters. Existing other consumers remain default-off;
raw source metadata remains internal C++ input. Top-level SDK paragraph/heading
wrappers stay outside inline word spans, with one pane-wide fallback decision.
See [the note-numbering seam](note-numbering-workflows.md) for evidence and limits.

### Isolated heading-note source capture

The C++11 capture service refreshes a selected source entry, copies its attributes
before heading filtering, and restores source attributes and SDK attribute
processing on success/exception. Raw heading notes are filtered separately; saved
headings already processed by Footnotes retain their source IDs/bodies. Repeated
heading extraction is skipped to retain ThML visibility. All metadata is copied
before optional note-body rendering. These fragment-local IDs remain internal
until a shared heading namespace/lookup is supplied; body-note lookup cannot be
used for them. Existing native snapshots remain unchanged at `91f83c9`.

### Scoped heading identity and presentation gate

A reserved canonical heading namespace carries fragment index plus encoded opaque
SDK ID through existing note URLs and copied results. Configured shared lookup
regenerates source metadata, scopes Headings/Footnotes as needed, and never treats
these IDs as body-note IDs. Session replacement is default-off for other consumers
and requires note-source capability. It preserves raw Reader metadata and accepts
only HTML matching byte-for-byte outside validated note IDs; known IDs, source
module/reference/type and counter-suppression label must agree. The Mac and GTK
models opt in with queued calls, keeping all association/formatting policy in core.
Unsupported candidates retain original rendering. See the note contract for
checked subset behavior and remaining native/corpus gates.

### SDK text-render failure recovery

`2b756bf` centralizes all shared SDK `renderText` calls in a private C++11 boundary.
A local guard restores the caller's entry-attribute processing flag if a filter
throws, preserving both initially enabled and deliberately disabled callers.
Healthy SWBuf output and ordinary source-attribute population remain unchanged;
this does not roll back selected keys or attribute maps. Heading source capture
retains its separate stronger attribute transaction. No platform policy or new
public C API is introduced by this recovery guard.

### General-book source boundary

The initial [book source seam](book-tree-workflows.md) returns owned C++17
indexed-tree node/relationship snapshots from private SDK query keys in Reader.
Typed offsets and exact canonical paths avoid legacy text-input ambiguity;
invalid SDK snaps cannot publish stale nodes. Actual tree root remains distinct
from legacy initial reading offset 4. Configured raw entry rendering and queued worker/C transport now retain these
source identities and existing tab/module option policy. Bounded child pages
keep lazy-tree order/cursor rules in core and travel as owned queued C snapshots.
Native controls and
book-specific display/link/history rules remain subsequent seams. No additional
engine or frontend navigation policy is introduced; GTK3 book/editor paths remain
intact.
