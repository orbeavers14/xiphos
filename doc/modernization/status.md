# Modernization status and acceptance

Updated 2026-10-10. Goal: preserve Xiphos functionality, visual composition,
data and workflows through one shared C++ application core and separate
SwiftUI/AppKit, WinUI 3 and GTK4 frontends. Native enhancements supplement
that contract. The [parity matrix](parity-matrix.md) defines acceptance evidence;
[architecture.md](architecture.md) describes boundaries and dependencies.

## Overall completion estimate

Approximately **5% complete** toward a validated native release on all three
platforms (planning range **3–7%**, not a measured parity score). This counts
modernization work, not functionality inherited from the existing GTK app.
The estimate is deliberately provisional and should change as services and
platform workflows are characterized. Reassess after each accepted milestone.

| Work area | Share of total effort | Estimated work completed | Contribution |
|---|---:|---:|---:|
| Inventory, architecture, build/test foundation | 10% | 40% | 4.0% |
| Shared application services | 35% | 2% | 0.7% |
| Three native frontends and design/workflow adaptation | 30% | 1% | 0.3% |
| User-data compatibility and migration | 15% | 0% | 0% |
| Release packaging and comprehensive acceptance | 10% | 0% | 0% |

These weights and within-area percentages are engineering estimates, not
completed issue counts. Foundation still needs a representative module corpus,
Windows native tooling and regression infrastructure. **0 of 38 feature groups
have full accepted parity across the native platforms; 0 of 3 native apps are
release-ready.** PR #1 is implemented and partially verified, pending review
and merge. No milestone has official acceptance yet.

## Reuse from related projects

Inspected 2026-10-10: the local SwordKit source at
`d928c48b37d45502df4466194ab33bdcb1dbd1dd` and its package, bridge,
engine-access lock, metadata attributes and roadmap; public BibleKit, BibleUI
and SwordReader README contracts. Public README evidence is a candidate screen,
not a source/license or integration audit of those three projects. No source
from another project has been copied or linked into this initial slice.

| Project | Useful reuse | Long-term decision and gate |
|---|---|---|
| SwordKit | Swift-owned values, native-handle lifetime patterns, Strong's/morphology attribute types, Apple build/release tooling | Highest-priority implementation reference. Adapt selected Swift presentation/lifetime code to the Xiphos C API; preserve provenance and test behavior. Do not link its existing direct SWORD bridge into the app alongside the Xiphos engine. |
| [BibleUI](https://github.com/SwordReader/BibleUI) | Reference/catalog/attribution controls, cancellation and stale-load protection patterns | Evaluate individual controls after source/license audit and comparison with Xiphos layout, keyboard and accessibility requirements. Its documented formatter removes links, so the current reader cannot preserve Xiphos study-link behavior unchanged. |
| [BibleKit](https://github.com/SwordReader/BibleKit) | Provider-neutral presentation contracts | Optional Xiphos-backed provider for Apple presentation; no reference parsing, search, history or persistence policy moves out of C++. Avoid an extra framework dependency until a needed component justifies it. |
| [SwordReader](https://github.com/SwordReader/SwordReader) | App integration and native platform workflow examples | Inspect focused menu/window/accessibility/test helpers when implementing the corresponding milestone. Its product scope and layout do not replace the Xiphos parity contract. |
| Separate SWORD engine modernization | Potential future dependency update | Keep this separate from extracting the Xiphos application core. Require explicit pinned source/artifact integration, provenance, baseline output/module tests and API/ABI checks before changing the engine dependency. |

SwordKit's README declares GPL-2.0-only; its LICENSE contains GPL v2. Its
package uses Swift tools 6.3, macOS 14+, a native XCFramework and direct
CSwordBridge calls. Xiphos currently targets macOS 13+; adopting that package
unchanged would alter requirements and introduce a second engine boundary.
Its roadmap also leaves deterministic native-shutdown and fixture validation
open. Existing tests/checkmarks do not automatically validate Xiphos.

Expected savings are reduced adapter, UI-control and build-tooling development;
there is insufficient integration evidence for credible time or cost numbers.
Search, navigation/versification, annotations, installation policy and session
state remain shared C++ responsibilities even where Swift implementations exist.
For each actual import, pin the reviewed revision, retain source notices,
record modified files, check dependencies/platform minima and run Xiphos
regressions. Private project code requires a separate publication review before
inclusion in this public fork.

## Next steps in order

1. Finish PR #1 checks and review, validate representative module types and
   rendering against the legacy app, then record scoped acceptance on merge.
2. Review the asynchronous macOS integration separately from the core
   foundation; broaden startup/shutdown and real-module interaction checks.
   Expand decorated chapter options and characterize commentary/parallel panes.
   Screen and workflow comparisons must retain the established study-pane composition.
3. Evaluate selected SwordKit Swift helpers and BibleUI controls against that
   core API; add only a justified, audited adapter/component in a focused PR.
4. Implement equivalent reading panes in GTK4 and WinUI 3 with native toolchain
   CI and interaction checks; retain the legacy app through migration.
5. Extract search/parallel/link/preview, then compatible storage/editors,
   module management and BibleSync, followed by platform release acceptance.

## Shared history slice: pushed, pending acceptance

Implemented and tested locally on 2026-10-10 after the CI-budget change, then
pushed as `6f759e0` at the user's request. Draft CI runner jobs remain gated.
Scoped implementation and local checks are complete; review, platform CI and
acceptance remain pending. Future completed commits are pushed after local
checks while intermediate development stays local.

- [x] Characterized legacy reference-only deduplication, preserved forward
  entries, original text/commentary metadata retention and 50-entry eviction.
- [x] Shared C++11 history policy; the legacy C path calls the same reference
  lookup helper without replacing its GTK/session storage or callbacks.
- [x] C++ study session and additive C ABI: per-session history, Back/Forward,
  Clear History, retained reader ownership, copied results and error handling.
- [x] Thin Swift session adapter and native Back/Forward/Clear History controls.
- [x] Local macOS CTest 3/3 passed, including C ABI and Swift adapter checks.
- [x] Local address/undefined-behavior sanitizer CTest 2/2 passed with the same
  externally built SWORD/leak-detection limitations recorded below.
- [x] Standalone history compiled/tested as C++11 with warnings as errors.
- [x] Original baseline and modified legacy add-history function bodies,
  compiled with a minimal GTK callback stub, agreed over 120 scripted visits,
  duplicates and capacity evictions. This is not a full GTK app runtime test.
- [x] Fresh preview UI: read John 3:17, Back restored John 3:16, Forward restored
  John 3:17; Clear History disabled navigation without clearing the verse pane.

This slice supports a Bible module per session. History stores a commentary
field for the legacy model, but native commentary-pane restoration, history
menu selection/export, synthetic entries, first-back callback side effects,
multiple tab/detached-window UI and persistence remain pending. The original
GTK pathways for these features are retained. Per-reader/session calls and
teardown must be serialized. No cross-reader/global-engine concurrency promise
is added. macOS 13 runtime, full legacy app and Windows/Linux integration are
not validated by these local checks.

Next after the chapter snapshot slice: decorated chapter output/options,
broader reference controls and the established study-pane composition. Keep the overall estimate at approximately
5% until broader milestones and platform parity are accepted.

## Chapter reading slice: local validation, pending acceptance

Implemented 2026-10-10: owned chapter snapshots and adjacent-chapter navigation
in the C++ reader/session, additive C ABI and thin Swift adapter. Enumeration
uses a cloned module VerseKey with that module's locale/versification; no
frontend canon or chapter-length tables. Every verse is retained in order,
including missing text. Chapter reads keep the module at the requested anchor;
chapter navigation records a successful visit and stops at Bible boundaries.

Local macOS CTest **3/3** and sanitizer CTest **2/2** passed (same SDK and leak
limitations below). Tests compare every fixture chapter verse/reference with
the legacy setKey/renderText sequence, exercise Vulg versus KJV verse counts,
check missing/unsupported modules, invalid inputs, snapshot lifetime, cross-book
navigation, Bible boundaries and history rollback on failed navigation. Swift
adapter checks cover copied chapter values and chapter navigation.

Fresh preview check: Whole chapter displayed John 3 with all 36 verse positions
and both original fixture texts; Next chapter selected John 4:1 and displayed
John 4 with a missing-text message; Back restored John 3:16 and both fixture
texts. This is a scoped reading check, not screenshot/design parity.

Full Xiphos chapter decorations/options, introductions/headings, representative selected-verse
highlight/scroll acceptance, semantic study links, synchronized panes and RTL/layout
acceptance remain pending. The preview uses simple numbered sections and still
runs synchronous reading on the main thread; a shared engine executor is later
work. GTK chapter code is retained unchanged; shared render_entry remains its
common engine seam. Full GTK runtime, alternate representative content,
Windows/Linux integration and older macOS runtimes are still unvalidated.

## Chapter presentation slice: local validation, pending acceptance

Implemented 2026-10-10: the shared content service produces chapter fragments
with stable verse anchors, escaped reference labels, selected-verse semantics
and optional verse numbers/highlight classes. Raw SWORD verse markup remains
available unchanged. The additive C ABI owns all strings; Swift copies the
fragment instead of maintaining a separate chapter formatter. Theme colors and
scrolling belong to the native renderer. Options do not alter global SWORD state.

Local macOS CTest **4/4** and sanitizer CTest **3/3** passed. Checks cover
selected positions, escaped attributes, missing entries, preserved engine
markup, option combinations, unknown flags, lifetime and Swift copied values.
Fresh preview verification showed John 3:16 at the top of the chapter pane
with a visible highlight; hiding numbers retained both fixture texts and
turning off highlighting removed the background. Display changes refresh the
committed passage rather than unsubmitted edits in the reference field.
Module scripts remain disabled; trusted scrolling runs in an isolated host world.

Initial GUI inspection timed out; a process sample captured SWORD manager/locale
initialization. Fresh launches against the verified fixture were responsive.
A subsequent timeout was a stale automation connection after process replacement:
selecting the app by its current path worked, and its sample showed an idle event
loop. The initial slow initialization remains uncharacterized; these checks do
not establish startup-performance acceptance. This is separate from the earlier
sandbox abort documented below.

GTK chapter formatting is unchanged. Full chapter decorations, introductions,
headings, whole-book mode, rendering options, RTL, synchronized panes and visual
comparison against legacy remain pending. These scoped checks do not accept F03
or justify retiring legacy display files. Overall estimate remains approximately
**5% (3–7%)** pending broader platform and workflow validation.

## Passage-link slice: local validation, pending acceptance

Implemented 2026-10-10: shared parsing/encoding of single-reference `sword://`
and `bible://` commands. This scoped subset follows `main/url.cc`'s percent and
plus decoding and current-module default. Exact installed verse-key module IDs
are checked in C++; SWORD resolves the reference on a subsequent session visit.
The owned C ABI target survives reader close. Core-generated verse numbers are
links; macOS cancels web navigation and sends the URI through the core resolver
and shared history session. External links are not opened by this preview.

Local macOS CTest **4/4** and sanitizer CTest **3/3** passed. Checks include
explicit/default modules, Unicode round trips, unsupported commands/ranges,
control characters, missing modules, null inputs and result lifetime. Fresh GUI
check: clicking verse 17 changed the reference to John 3:17 and moved the scroll
position; Back restored John 3:16 and its earlier scroll position.

GUI inspection of a Documents-hosted build again timed out. The process sample
showed startup blocked in an engine configuration `open`, rather than an idle
loop. Copying this build and generated fixture to temporary storage produced a
responsive app without changing system permissions. Protected-folder access is
a possible cause, not a confirmed diagnosis. Future GUI fixture tests should use
an isolated temporary copy; normal user-library access/startup remains an open
acceptance check. The earlier stale automation connection was a separate case.

Abbreviation lookup, unknown-module fallback, anchors, passage lists/ranges,
notes, Strong's/morphology actions, hover previews, synchronized panes and
external-link policy remain pending. Unsupported commands preserve the pane;
no legacy URL handler is removed or changed. This implements only a portion
of the link workflow and does not accept its parity row.

## Reference-selector slice: local validation, pending acceptance

Implemented 2026-10-10: the shared navigation service supplies owned book,
chapter and verse choices plus current labels through the C ABI. It clones the
module's VerseKey and follows the existing book/chapter/verse menu selection
sequence in `src/main/navbar_versekey.cc`. SWORD supplies book names, counts,
normalization and targets; the Swift UI has no canon tables or reference parser.
Book choices honor the legacy first/last-entry testament probe. The probe restores
the original module-key object/position and consecutive-link flag. Menu choices
use the committed passage, and successful selections participate in history.

Local macOS CTest **4/4** and sanitizer CTest **3/3** passed. Generated OT-only,
NT-only and mixed fixtures verify 39, 27 and 66 book choices; alternate Vulg
context and KJV checks compare every available menu label/target with the legacy
createKey/setTestament/setBook/setChapter/setVerse sequence. Additional checks
cover module-state preservation, bounds, unsupported dictionaries, errors,
owned snapshot lifetime and Swift copied values. Chapter selection resets to
verse 1 according to SWORD behavior; it is not independently reset in Swift.

Temporary-copy GUI check: verse 17 showed the second fixture text; chapter 4
selected John 4:1; the book menu contained only NT books for the NT fixture;
Luke selected Luke 1:1, and Back restored John 4:1. No repeated Documents access
was needed by this isolated fixture launch.

Legacy GTK selectors remain intact. Localized number formatting, GTK's numeric
menu grid/OT-NT column composition, keyboard comparisons, editor/parallel/dialog
selectors, synchronized commentary and platform-wide visual parity remain
pending. The new menus are native preview controls, not accepted design parity.
Overall planning estimate remains approximately **5% (3–7%)**.

## Shared engine worker: core foundation (historical checkpoint)

Implemented 2026-10-10: `StudyWorker` owns a reader and independent tab sessions
on one C++ engine thread. Discovery, reading, chapter/navigation snapshots,
link resolution, history and teardown run there. Requests copy their inputs,
execute FIFO and return owned values through futures. Initialization failures
and operation errors reach their futures without terminating later jobs.
Closing a tab rejects later requests for that ID; IDs are not reused. Worker
shutdown drains accepted jobs and destroys sessions/reader on the engine thread.

Local macOS CTest **5/5** passed; address/undefined-behavior sanitizer CTest
**4/4** passed. A separate ThreadSanitizer build passed the worker plus required
fixture tests (**2/2**), including four concurrent request producers. External
SWORD is prebuilt and uninstrumented, so race checks cover the instrumented
core/queue and are not a proof of global engine thread safety. Tests also cover
FIFO visits/Back/history, tab isolation, copied input/results, failed operations,
closed tabs, chapter/options/link/selector results, initialization failures and
queued work surviving worker destruction.

At this checkpoint the macOS preview was not connected to the worker.
Nonblocking C request handles are implemented in the following slice; next was
async Swift consumption with stale-result protection and background shutdown,
then verification of a responsive reading window.
No cancellation/interruption of an active SWORD call is provided by this first
worker API. Native hosts must share one worker per process; unrelated SWORD
instances, global locale switching and legacy global calls cannot run alongside
it concurrently. Do not wait on futures or destroy a worker on the UI thread.
The legacy rendering/history path and synchronous C API remain available.

## Nonblocking C worker bridge: local validation (historical checkpoint)

Implemented 2026-10-10: opaque worker/request handles for discovery, tab
lifetime, visits, navigation, history, chapters, reference choices and links.
Opening a worker starts engine initialization on its thread; failures appear
in requests. `xc_request_poll` never waits and caches the terminal status.
Result handles transfer once to the caller, retaining their existing ownership
contracts. Modules and history are copied snapshots. Observer allocation occurs
before accepting potentially mutating work. Releasing an observer does not cancel
its queued operation. Closing a worker drains/joins and must run off the UI thread.

Local macOS CTest **6/6** and address/undefined-behavior sanitizer CTest **5/5**
passed. ThreadSanitizer passed async/worker plus required fixture tests (**3/3**).
The worker and C bridge also passed C++17 warnings-as-errors syntax checks.
Tests exercise all operation families, FIFO history, input ownership, cached
errors/status, wrong-type and repeated takes, dropped observation, closed tabs,
initialization failure and requests/transferred results surviving worker close.
C-header compilation checks the additive declarations and symbols. External
SWORD remains uninstrumented; unrelated engine/global concurrency is unproven.

At this checkpoint the macOS preview used the synchronous adapter. The
following slice connects the worker to the native interface. Active-call
cancellation, search progress and comprehensive startup/shutdown acceptance
remain pending.

## Asynchronous macOS reader: locally validated, pending review

Implemented 2026-10-10 on `modernization/async-macos-reader`, stacked on the
core foundation branch for separate review. The preview shares one C++ engine
worker across windows; each window has its own core tab/history. A Swift 6
actor submits and observes nonblocking C requests, copying owned results into
presentation values. Window controls serialize actions and show loading state.
Closing a window guards late results and schedules tab cleanup; engine draining
runs on a background queue. Accepted actions survive Swift task cancellation.
Unsubmitted reference drafts survive display-option refreshes.

Local macOS build and CTest **8/8 passed**. New tests cover async discovery,
main-actor progress, concurrent request consumers, tab/history isolation,
canceled task observation, initialization errors, copied result lifetime and
background shutdown. Reader-model tests cover initial loading, reading,
history, chapter display, links, draft preservation, ignored overlapping
actions, close-during-startup and stale results after closing.

A rebuilt temporary fixture app was visibly checked: original UTF-8 verse,
John 3:17, Back, whole chapter and verse-number links all worked. A second
native window opened with independent history; closing it retained the first
window's passage and history. No Documents prompt appeared in this test.
This verifies the fixture path, not normal user-library permission behavior.
No GitHub build was requested. Core sanitizer results from the preceding
checkpoint remain applicable to unchanged C++ sources; Swift tests are not
ThreadSanitizer coverage of the GUI.

This is scoped implementation evidence, not full feature acceptance. The
provisional project estimate remains approximately **5% (3–7%)**, with **0/38**
feature groups accepted and **0/3** native platforms release-ready. Stable
signing, real-module equivalence, macOS minimum-version validation, active-call
cancellation, Windows/Linux frontends and full study-pane composition remain
pending. No legacy files are retired by this slice.

## Shared engine option policy: locally characterized, pending review

Implemented 2026-10-10 on `modernization/shared-engine-options`, based on the
core foundation branch for a focused review. `EngineOptions` and the C++11
`CoreOptions` library now own the existing `main_set_global_options` filter
policy. The legacy entry point converts its gboolean fields to core values and
calls that shared policy. The replaced private helper was removed after its
only caller migrated; exported textual setters, menus and preference storage
are retained.

The exact legacy order, On/Off values, Latin transliteration, unconditional
engine headings and Primary/Secondary/All variant precedence are preserved.
With no variant selected, the engine's existing variant value remains untouched.
Display headings are still a separate decision; this extraction does not add
new defaults or reinterpret stored preferences.

Local macOS build and CTest **7/7 passed on this branch**. The new test compiles
the actual legacy adapter body and compares its SWORD setter calls with a frozen
pre-extraction baseline across **2,400 states**, including every individual
boolean filter, mixed states, noncanonical true values, all variant combinations
and both display-heading flags. The targeted address/undefined-behavior
sanitizer test passed **1/1**. New policy source passed C++11 warnings-as-errors
with external SWORD headers treated as system headers. The default legacy seam
build passed and still does not build the experimental reader/worker.

These checks prove option mapping/call equivalence, not full GTK operation or
rendered-option parity. Full GTK/WebKit dependencies are unavailable locally;
that build remains an integration gate. Native request-scoped rendering options,
module capability discovery, real OSIS/ThML/GBF output comparisons and all native
controls remain next work. No GitHub build was dispatched. Full acceptance
remains **0/38**, release-ready platforms **0/3**, estimate approximately **5%**.

The separate [draft PR #2](https://github.com/orbeavers14/xiphos/pull/2) contains
the asynchronous macOS worker integration, with its own **8/8** local checks and
fixture GUI evidence. The 7/7 count above belongs to this core-focused branch;
it does not supersede those tests.

## Scoped native engine options: rendered fixture validation, bridge pending

Implemented 2026-10-10 after the policy extraction on the same focused branch.
The reader accepts explicit filter options for verse and chapter rendering.
Core study sessions own copied optional settings; visits, history navigation
and tab chapter reads use those settings. Worker requests can set/clear tab
options and read a tab chapter. Settings changes do not add history entries;
clearing returns to the unconfigured reader behavior. Existing overloads and
C/Swift callers retain their previous behavior.

Scopes save every actual filter value and restore it after rendering, including
exception unwinding and nested scopes. An initial output test showed that
SWORD's name-based global getter can report one of several same-name filters
with different initial values. Name-based restoration changed subsequent
unconfigured output. The corrected native manager snapshots individual filter
objects instead. Builds without ICU can leave the protected transliterator
pointer uninitialized; it is inspected only when the manager advertises the
Transliteration option. The inspected SDK does not validate an ICU-enabled
transliterator configuration; that remains an explicit integration check.

Local macOS build and CTest **8/8 passed on this branch**. An original-prose
OSIS fixture registers red-letter, Strong's, morphology and note filters.
All 16 filter combinations match direct original-policy SWORD setter/render
calls; enabled and disabled output differ as expected. Tests cover unchanged default output, chapter
options, per-tab opposite settings, history reuse, copied queued settings,
clearing without history changes, and queue continuation after a closed-tab
error. Individual filter values restore after exceptions and nested scopes.
Targeted address/undefined-behavior checks passed **2/2**; targeted ThreadSanitizer
scoped-option/worker test passed **1/1**. External SWORD is uninstrumented.
The default legacy seam build and C++11 policy warnings-as-errors check passed.
Swift static linking now includes CoreOptions because the reader references it.

At this checkpoint native option controls and additive C requests were pending.
The following checkpoint adds requests; next is module capability discovery,
native controls, and comparisons of
broader OSIS/ThML/GBF output and existing Xiphos decorations. This fixture does
not prove all options or complete study-pane parity. No GitHub build was
requested; full acceptance and the provisional estimate remain unchanged.

## Nonblocking tab option bridge: locally tested, native controls pending

Implemented 2026-10-10 on the shared-engine-options branch. The experimental
C API adds 18 named engine-option flag bits, asynchronous tab set/clear,
configured tab chapter reads and history-free configured entry refreshes.
Flags are copied before queueing; unknown bits fail immediately and leave the
output request NULL. Closed-tab errors arrive through normal polling. Zero
flags apply explicit Off filters with engine headings On; clearing removes the
tab override and restores unconfigured behavior. Variant precedence remains in
the shared C++ policy. Existing unconfigured chapter and reader calls remain.

Local macOS CTest **8/8 passed**. The original OSIS fixture tests all 16
combinations of red-letter/Strong's/morphology/note flags through the C API,
comparing owned entry HTML to direct SWORD baseline output. Opposite tab options,
configured chapters, unconfigured chapter compatibility, caller flag mutation,
invalid engine/display bits, missing observers, closed tabs, dropped clear
observation and chapter lifetime after worker shutdown are checked. Refresh
can render a different reference without adding or moving history. The C
compiler checks new declarations and symbols; C++17 warnings-as-errors passed.
Focused address/undefined-behavior and ThreadSanitizer selections each passed
**3/3** (scoped options, async requests and required reader fixture). External
SWORD remains uninstrumented; ICU-enabled and wider markup behavior remain
unvalidated. No GitHub build was dispatched.

Next: module-supported option discovery and native controls consuming this
bridge, with the asynchronous macOS work from PR #2. Preserve Xiphos module
preferences and broader decorations rather than inventing frontend defaults.
No feature group is accepted by this checkpoint; estimate remains about **5%**.

## Module option eligibility: copied catalog metadata, native menus pending

Implemented 2026-10-10 on `modernization/module-option-metadata`, stacked on the
shared-engine-options branch for separate review. Core module snapshots carry
eligibility from the existing exact `GlobalOptionFilter` checks: OSIS/ThML/GBF
aliases, accent/vowel/cantillation filters, variants, Ruby/glosses and segmented
morphemes. Headings are separate metadata; italic-heading eligibility also
requires display headings enabled. Global transliteration availability comes
from the installed manager's advertised options, separately from declarations.
Legacy menu code retains its general transliteration item even without SDK
support; this metadata does not silently reinterpret that menu behavior.

The nonblocking catalog exposes the copied engine eligibility mask, headings
and transliteration availability. Getters do not call the engine and results
survive request/worker shutdown. Missing catalog/index returns zero. Declared
features do not prove actual SDK filter support, markup content, enabled saved
preferences or a working semantic action handler. Unknown filter declarations
remain unrecognized, as with the existing specific menu gates.

Local macOS build and CTest **9/9 passed**. A new characterization test exercises
**30 declaration cases** against the legacy menu alias oracle: all recognized
aliases, unknown/case/whitespace names, duplicate, combined, missing and
non-global declarations. Probes preserve passage/key state. C API checks the
OSIS fixture mask, missing headings, invalid bounds, SDK transliteration
agreement and copied metadata lifetime. Focused address/undefined-behavior
checks passed **3/3** (module metadata, scoped options and reader fixture);
ThreadSanitizer scoped-option/catalog/worker check passed **1/1**. External
SWORD remains uninstrumented. C++11 policy/C++17 reader/bridge warnings-as-errors
and the default legacy seam build passed. No GitHub build was requested.

Next: consume catalog eligibility in the asynchronous Swift adapter and native
menus; preserve display-heading gating and module preference behavior. Full
menu composition, translated labels, effective filter support across SDKs,
images/fonts and broader Xiphos decorations remain pending. No legacy files
are retired and full feature acceptance remains **0/38**.

## Verified implementation, pending acceptance

Evidence below applies to implementation commit
`04f9787da3c66f3d34db2d506ba34e857d31f735` in
[draft PR #1](https://github.com/orbeavers14/xiphos/pull/1). Later commits must
be checked independently. Checkboxes here record the scoped evidence stated,
not full native parity or permission to delete its tests.

- [x] Source/build/dependency/license inventory and 38-group parity ledger.
- [x] Shared rendering seam used by the existing backend and new reader.
- [x] Read-only module/verse C API with ownership and error checks.
- [x] macOS preview and thin Swift adapter compiled and tested.
- [x] Visible macOS reading flow checked with two generated fixture verses.
- [x] Linux core and macOS reader CI passed on the implementation commit.
- [x] Legacy Arch, Fedora, Ubuntu and Debian PR builds passed.
- [ ] Legacy Win32/Win64 cross-builds: still running at this status snapshot.
- [ ] Initial PR review/sign-off and merge: PR remains a draft.
- [ ] Representative module corpus and runtime/visual workflow validation.

CI evidence: [native-core PR run](https://github.com/orbeavers14/xiphos/actions/runs/38079690029)
and [legacy application PR run](https://github.com/orbeavers14/xiphos/actions/runs/38079689911).
Build success checks compatibility at build time; it does not establish runtime
or design parity. No parity row is accepted on all native platforms.

## Development and CI budget policy

Develop, build and run focused regression checks locally. Batch implementation
and documentation into reviewable milestones before publishing; do not push
individual work notes or minor edits to obtain repeated hosted builds.

Both build workflows skip runner jobs for draft PRs and do not build feature
branch pushes. A ready-for-review PR runs CI on opening, reopening, readiness
and subsequent updates. Master integration and tag pushes remain checked;
tag release packaging is retained. Documentation-only events that touch only
Markdown or `doc/modernization/` are filtered. PR path filters evaluate the
whole PR diff, so a documentation update to a ready code PR may still run CI.
Concurrency cancels superseded runs of the same workflow/PR or ref.

Manual workflow dispatch supports deliberate milestone/platform checks once
the workflow is available on the default branch. Use it only when local
checks cannot cover a relevant platform or at a review/release gate. Do not
run the entire matrix for each intermediate fix. Skipped draft jobs are not
validation evidence; relevant checks must pass on the final reviewed revision
before acceptance. Local macOS testing cannot establish Linux/Windows parity.

## Latest CI snapshot

At documentation parent `0bc38acf579e36b5adf9b1d53bc156ce2ecfc964`,
Linux core and macOS reader passed both push and PR checks. Legacy Ubuntu and
Arch passed both; Fedora push passed while its PR check was running. Debian
and Win32/Win64 checks were still running. See the [native run](https://github.com/orbeavers14/xiphos/actions/runs/38080067351)
and [legacy run](https://github.com/orbeavers14/xiphos/actions/runs/38080067352).
This is a dated snapshot, not a live status badge. Any later commit needs its
own CI result before acceptance.

## Remaining milestones

- [ ] Shared per-tab navigation/history and chapter rendering/options.
- [ ] Equivalent reading layout and interactions on GTK4 and WinUI 3.
- [ ] Search, parallel sets, semantic links and preview behavior.
- [ ] Compatible settings, sessions, bookmarks/tags/imports, notes and editors.
- [ ] Module installation/maintenance and BibleSync.
- [ ] Accessibility, localization, keyboard workflows and platform integration.
- [ ] Packaging/signing, license notices, upgrades and complete parity review.

The current macOS preview has a module sidebar and a verse/chapter pane. It
is not yet the established full study layout. Windows and GTK4 native
frontends are not implemented. SwordKit has an initial source/license screen;
actual reuse and integration remain pending.
The GTK app remains the complete implementation; no legacy feature is retired.

## Local validation evidence

Local checks on 2026-10-10, Apple Silicon, AppleClang 21, installed SWORD
1.9.0.3916 (static SDK):

* Standalone Debug core: engine/C ABI test passed.
* macOS preview: app and Swift adapter compiled; CTest 2/2 passed.
* Address + undefined behavior sanitizer build: core test passed, with leak
  detection disabled because the externally built SWORD library is not
  instrumented and has process-global lifetime state. This is not a leak audit.
* Explicit include/library SDK discovery: core build/test passed (macOS only).
* Generated dictionary fixture: discoverable but rejected as unsupported by
  the verse reader. Empty roots, missing module and empty reference tested.
* Visible app check: selected ParityTest; John 3:16 displayed
  “Original fixture: café & peace.”; changing reference to John 3:17 and
  clicking Read displayed “Second fixture verse.” in WKWebView. Confirmed
  both screenshot and accessibility text. This is not visual parity testing.
* Source inventory: 387 UI labels and 101 manual section titles extracted.

The local SDK requires macOS 26.0, so the preview was tested with
`CMAKE_OSX_DEPLOYMENT_TARGET=26.0` and explicit zlib/bzip2/lzma libraries.
The project defaults to 13.0; compatibility on that OS requires an appropriate
SDK and separate runtime verification.

GTK3/WebKitGTK are absent locally, so the legacy desktop app has not been
launched on this Mac. Full native workflows, MSVC/WinUI builds, older macOS
runtime compatibility, storage and distribution packaging remain unvalidated.

## macOS startup report follow-up

On 2026-10-10, a supplied preview crash report showed SIGABRT in HIServices
`_RegisterApplication` during `NSApplication`/SwiftUI startup. There were no
reader/core frames on the faulting stack. The report's binary UUID matched the
locally tested preview. The stack alone does not establish the exact cause.

Relaunched the unchanged binary through `/usr/bin/open -n --env` using the
original generated fixture, with an approved launch outside the restricted
command environment. Confirmed the fresh process remained running, closed the
older test instance, and inspected the fresh window: John 3:16 displayed
“Original fixture: café & peace.” and changing to John 3:17 displayed
“Second fixture verse.” This supports a launch-environment explanation for
this report; it does not prove all startup environments are compatible.

Updated the [preview launch instructions](../../platforms/macos/README.md) to
use Launch Services and document restricted-launch diagnostics. No engine or
application behavior change was needed. If a normal Finder/Terminal launch
also aborts, collect its new stack and launch diagnostics before assigning the
failure to this same cause. A successful build or `open` exit code is not
runtime validation.

## Official sign-off and file retirement

A scoped milestone is accepted when its specified implementation, regression
checks, relevant CI and parity evidence have been reviewed and its PR merged.
Record the accepted commit/PR and evidence here, and update only the parity
rows/platforms that were actually validated. A passing test alone does not
make every file associated with a milestone obsolete.

After acceptance, remove disposable plans, superseded notes and generated
reports once useful decisions/evidence are captured in this status page or
architecture. Generated build/test fixtures and inventories stay under ignored
`build/`; keep the code that regenerates them. This cleanup consolidates the
initial validation note here and removes tracked generated inventories.

Retire legacy source, assets, build rules or dependencies only when:

1. The replacement covers each affected parity requirement on the platforms
   still using that code, including data compatibility and workflow checks.
2. No supported caller, build, test, documentation or packaging path needs it.
3. The cleanup removes stale references and relevant checks pass afterward.
4. The removal and acceptance evidence are recorded in a reviewed PR.

Keep regression tests after features are checked off. Keep current parity
requirements, architecture, supported user documentation and required license/
attribution files. Git history and linked PRs preserve removed implementation
and detailed review history; avoid a growing archive of duplicate work notes.

## Repeated macOS folder prompt: development workflow changed

The user confirmed that the repeated dialog asks Xiphos Native Preview for
Documents access. Documents-hosted GUI fixture tests can wait for this response
during engine configuration reads. A temporary app/fixture copy was responsive.
An opt-in `preview_fixture` build target now creates that isolated copy, verifies
the core fixture and launches it with an explicit root. Default builds and CI
do not launch a GUI. No privacy permissions, Full Disk Access or signing
identities are changed. This fixes the test workflow; release signing, chosen
user-library locations and persistent access still require separate validation.

Launcher validation: shell syntax passed; `preview_fixture` built successfully,
its core fixture test passed (1/1), and the newly copied app showed the original
John 3:16 fixture text in a fresh GUI inspection. This launch read temporary
fixture storage rather than the repository's protected Documents folder.
