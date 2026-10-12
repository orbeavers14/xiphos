# Modernization status and acceptance
## Local-only integration checkpoint — 2026-10-11

The repository owner authorized checks and merges without hosted build credits.
PR #1 foundation and PR #2 asynchronous macOS integration are merged into master.
PR #3 shared options is integrated with their combined source in this revision;
the build-file conflict was resolved by retaining all asynchronous UI/test targets
and linking/depending on CoreOptions in each Swift executable.

Exact isolated checks passed: #1 native build/tests 6/6 and native-disabled C++11
seams; #2 build/tests 8/8; #3 standalone 8/8 and combined native build/tests 10/10,
plus native-disabled rendering/history/options seams and strict C++11 options.
Local deployment target is 26 with the installed SWORD SDK, not a validation of
the intended macOS 13 minimum. Full GTK app/runtime, Windows/Linux, ICU-enabled
SDK and representative visual/module/profile checks remain deferred.

Both GitHub build workflows (builds and native-core) are disabled manually and
remain paused until hosted checks are requested. No hosted build was started
for this integration. This accepts experimental code integration only: 0/38
feature groups have full native parity and no legacy files may be retired on
this evidence. Later locally checked work remains in dependent draft branches;
their 49-test suite is distinct from master's combined 10-test suite.

The remaining document contains scoped implementation/history evidence; hosted
checks mentioned there describe the validation plan while workflows are paused.


Updated 2026-10-11. Goal: preserve Xiphos functionality, visual composition,
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
   Bind native option controls after shared preference defaults and module-scoped
   state are characterized; then expand decorated chapter options and
   characterize commentary/parallel panes.
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

## Swift option adapter integration: locally tested, native controls pending

Implemented 2026-10-10 on `modernization/macos-option-adapter`. This integration
branch combines the asynchronous macOS reader from PR #2 with the core option
policy/bridge and module metadata from PRs #3/#4. The local merge retained
Swift 6 compilation, actor/model tests and explicit CoreOptions static linking.
The combined foundation passed **11/11** local checks before adapter changes.

Swift now binds the 18 named C flags as a Sendable OptionSet, copies module
eligibility/headings/transliteration metadata from owned async catalogs and
wraps tab set/clear, configured chapter reads and history-free entry refresh.
No alias rules, filter defaults, variant precedence or rendering policy are
implemented in Swift. The synchronous characterization adapter does not expose
this new metadata; the native app uses the worker-backed catalog.

The resulting local build and CTest passed **12/12**, including the new Swift
option adapter test against the original OSIS fixture. Tests cover copied
metadata, all 16 red-letter/Strong's/morphology/note combinations, verse/chapter
agreement, visible red-letter/Strong's output changes, opposite tab settings,
history-free refresh, unknown flags, defaults after clear, closed tabs and
values surviving background shutdown. CTest explicitly prepares the isolated
option fixture before this Swift test. No GUI control change is claimed by
these adapter checks, and core sanitizer evidence belongs to the preceding
unchanged C++ behavior. No GitHub build was dispatched.

Next: wire module-aware native option controls and configured refreshes into
ReaderModel, then test GUI behavior, module preferences and visual/workflow
parity. Full native menus, wider markup/SDK validation and release acceptance
remain pending. The provisional estimate remains about **5%**, accepted
feature groups **0/38**, release-ready native platforms **0/3**.

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


## Shared engine preference defaults: locally characterized, pending review

Implemented 2026-10-10 on `modernization/shared-option-preferences`, based on
`modernization/macos-option-adapter`. `CoreOptions` now parses the engine subset
of Xiphos's `modops.conf` preferences. The legacy `main_new_globals` still owns
file loading and its display/layout settings, but calls the shared parser for
all 18 engine fields. Exact `On`/`Off` comparisons, default-on red-letter,
footnote, cross-reference, accent/vowel/cantillation and gloss options, and the
primary-reading fallback remain unchanged. Unknown preferences are ignored.
The parser accepts copied values, reads no files and saves no preferences.

Validation: the original initialization at `c9976a8` is frozen in a test oracle;
the actual current legacy initialization prefix is extracted at configure time.
Both reload generated SWORD configuration files and are compared with the
shared parser in 263 cases: empty defaults, each field with seven values, all
eight variant combinations, unknown keys and mixed settings. The existing
2,400-state engine setter characterization still passes. Full local macOS CTest
passed 13/13; focused AddressSanitizer/UndefinedBehaviorSanitizer checks passed
2/2 with leak detection disabled for the external SDK. The default C++11 legacy
seam build and warnings-as-errors core syntax check passed. Full legacy GTK GUI
compilation and interaction remain unvalidated on this machine.

Native controls are still pending: preserve separate module preferences and
history-free refreshes before exposing them. Native import/save of existing
`modops.conf`, display settings, original menus and wider SDK validation remain
separate gates. No GitHub build was dispatched, no legacy implementation was
retired and no feature-group acceptance changed. The provisional overall
estimate stays about 5% (3–7%); 0/38 groups accepted and 0/3 release-ready platforms.


## Module-scoped native preferences: locally checked, controls pending

Implemented on `modernization/module-scoped-preferences`, based on the shared
preference-default extraction. Study sessions now keep copied overrides by
installed module ID, with worker-serialized set/clear/query operations and an
explicit opt-in to the shared legacy defaults. Precedence is module override,
tab-wide override, enabled legacy defaults, then unchanged SDK state. Existing
unconfigured sessions retain their behavior. Settings are in memory; this
slice does not import or save `modops.conf` or establish persistent-preference
parity. The C bridge distinguishes configured flags from unconfigured SDK
state; Swift exposes thin async wrappers without its own default policy.

Local checks cover two distinct original OSIS modules, opposing settings,
chapter output, reference-history restoration, history-free refreshes,
independent tabs, copied queued settings, clear/fallback behavior, all 18 C flag
round trips, invalid modules/masks/request kinds, worker error recovery and
result lifetime after shutdown. The generated two-module fixture discards the
copied SWORD catalog cache before discovery. Local macOS CTest passed 14/14;
focused ASan/UBSan and ThreadSanitizer suites each passed 2/2 (including fixture
setup; the external SWORD SDK remains uninstrumented). C++17 warnings-as-errors
syntax checks passed for changed core and test sources. Swift module-option
wrappers also passed their expanded integration check.

Next: bind the validated reading-option controls to this state and ensure
chapter rendering uses the same tab configuration. GUI comparisons, preference
import/save, other settings and platform implementations still require their
own evidence. No GitHub build was dispatched or official parity row accepted.


## First native reading-option controls: locally tested, pending acceptance

Implemented on `modernization/macos-reading-controls`, based on the module
preference slice. The macOS menu binds Words of Christ in Red, Strong's Numbers,
Morphological Tags and Footnotes when declared by the displayed module. New
native tabs explicitly use shared legacy defaults; settings and default policy
remain in C++. Option changes refresh the committed reference without visiting
history or overwriting an unsubmitted draft. Chapter output now uses configured
tab rendering, and presentation queries the core's current module settings.
The red-letter SWORD class uses the existing Xiphos editor/template.h red style.
Chapter navigation is on a separate row to keep option labels readable.

Validation: full local CTest passed 15/15. After the final CSS/layout adjustment,
the affected reader model suites and fixture setup passed 5/5. New model checks
cover shared defaults, options in verse and chapter modes, retained drafts,
ignored overlapping actions, history-free refreshes, separate windows,
module-switch state, Back/Forward restoration, unsupported controls and all four
bindings. Tests explicitly retain legacy reference-only history deduplication.
Shell syntax and diff checks passed. No core runtime code changed in this slice;
previous module-settings sanitizer checks apply to that prerequisite.

A rebuilt temporary GUI at `/private/tmp/xiphos-preview.8aulUG` was inspected:
red-letter text visibly changed from red to ordinary text, Strong's 3056 and
morphology N-NSM links appeared, disabling footnotes removed the note marker,
and chapter rendering retained those settings with Back/Forward still disabled.
Switching to the second generated module showed its independent defaults;
switching back restored the first module's choices. The launcher now offers an
opt-in `preview_reading_options_fixture` target using only generated temporary
modules, without requesting Documents access or launching during ordinary CI.

These checks establish this limited reading-option slice, not complete option
or design parity. Existing `modops.conf` import/save, original note/Strong's/
morphology popups, remaining controls and themes, real-module comparisons,
GTK4/WinUI implementations and release checks remain pending. No GitHub build
was dispatched, no source retired and no feature group officially accepted.
Overall provisional completion remains about 5% (3–7%), 0/38 accepted groups,
and 0/3 release-ready native platforms.


## Read-only legacy preference import: locally checked, native picker pending

Implemented 2026-10-11 on `modernization/preference-file-import`, based on the
native reading-control slice. The C++11 preference seam decodes an explicitly
supplied existing regular file with SWORD's own `SWConfig` and the characterized
shared engine-default policy. It never discovers a home path or writes a file.
Worker imports replace one tab's engine preferences for installed modules;
missing sections use legacy defaults and unknown module sections are ignored.
Other tabs, display/layout settings and history remain unchanged. Replacement
state is fully prepared before commit, so file/decoding/allocation failures
leave prior settings intact. Imports reset that tab's earlier engine overrides.

Validation: all 263 original initialization cases now also compare the file
loader's decoded engine values. Expanded session/worker/C/Swift checks cover
imported output, absent sections, unknown sections and keys, UTF-8 values,
missing files and directory rejection, failure-state preservation, retained
history/window isolation and byte-for-byte unchanged source files. Full local
macOS CTest passed 15/15; focused ASan/UBSan passed 3/3 and ThreadSanitizer passed
2/2, including fixture setup (external SWORD SDK uninstrumented). The default
legacy seam build and C++11/C++17 warnings-as-errors syntax checks passed.
Windows-specific file handling remains unvalidated on a Windows toolchain.

Next: expose an explicit native import picker and then compatible preference
export/save with round-trip and failure checks. This is the 18-field engine
subset, not full preference migration or persistent setting parity. No GitHub
build was dispatched and no feature group was accepted or legacy file retired.


## Native reading-preference import: picker and refresh checked

Implemented 2026-10-11 on `modernization/macos-preference-import`, based on the
read-only core importer. Preferences → Import reading options opens a native
single-file picker. An explicit selection queues the core import and refreshes
the committed passage without submitting reference drafts or adding history.
An unreadable selection reports a contextual message and preserves current
settings/passage; cancellation queues no action. No preference path is silently
discovered, remembered or overwritten.

The expanded reader model suite passed with both windows and both generated
modules, imported settings, retained drafts/history, subsequent Back restoration
and failed import/source preservation. A local Swift 6 build and the affected
model/fixture suites passed 5/5. Previous full core/adapter regression baseline
was 15/15, with the importer sanitizer evidence recorded above; no core runtime
code changed here. Shell syntax/diff checks passed. The generated fixture
launcher copies its original `modops.conf` sample into the temporary GUI root.

End-to-end GUI inspection of `/private/tmp/xiphos-preview.XLtWN8` verified the
Preferences action, native picker, explicit generated-file selection and import:
Strong's 3056 appeared while red-letter styling and the footnote marker were
removed, and Back/Forward remained disabled. Reopening/canceling the picker
preserved that passage. The copied preference sample still matched the generated
source byte for byte. No Documents access prompt appeared in this fixture check.

Next is compatible explicit preference export/save and round-trip/failure
checks, then remaining options and study popups. Full display/preference
migration, automatic restore, other platforms and full parity are still pending.
No GitHub build was dispatched, no official acceptance was recorded and no
legacy file retired. The overall estimate remains approximately 5% (3–7%).


## Compatible engine-preference save: locally checked, native action pending

Implemented 2026-10-11 on `modernization/preference-file-save`, based on the
native importer. The shared C++ encoder emits all 18 legacy engine keys; the
native core saves through SWORD's parser/writer. Existing unrelated display
keys, unknown values, duplicate unknown keys and uninstalled-module sections
are retained semantically. Formatting/comment spacing follows SWORD's legacy
writer; exact source bytes are not promised for a save. Import remains read-only.

Saving requires configured effective options for every installed module and
never guesses SDK defaults. Session/worker/C/Swift APIs accept an explicit path;
there is no automatic persistence or home discovery. A same-directory temporary
configuration is written and reloaded to verify all actual sections/values,
excluding only SWORD's reserved ordering/comment metadata. Existing permissions
are preserved, new files are private, symlinks resolve to their existing target,
and detected destination changes/read-only files fail before replacement.
Temporary files are cleaned after success/failure. Replacement uses filesystem
rename; power-loss durability and coordination with external writers are not
claimed by this initial save layer. Windows replacement/path behavior remains
an integration gate.

Validation: 263 original configuration cases now include encoding followed by
reload through the original initializer. New save tests cover preserved display/
UTF-8/unknown values and duplicate keys, new files, all eight variant selections,
permissions, existing symlinks, verification failure, read-only files, directory/
missing-parent rejection and temporary cleanup. Session/worker/C/Swift save/load
checks preserve history and reject unconfigured state without creating a file.
The original no-variant fallback is retained: all variant flags off reloads as
primary rather than representing a persistent no-selection override. Full local
macOS CTest passed 16/16; focused ASan/UBSan passed 4/4 and ThreadSanitizer 3/3,
including fixture setup (external SDK uninstrumented). The C++11 legacy seam
build and C++11/C++17 warnings-as-errors checks passed.

Next: native Save reading options picker and end-to-end new-file/overwrite/
cancel checks. Automatic restoration, full preference migration, remaining
options/popups and other native platforms remain pending. No GitHub build was
dispatched, no parity group accepted and no legacy file retired.

## Native preference save: locally checked, acceptance pending

Implemented 2026-10-11 on `modernization/macos-preference-save`, based on the
shared preference writer. Preferences → Save reading options uses AppKit's
Save panel and explicit user selection; the existing core performs all encoding,
verification and replacement. A successful save reports completion without
changing the rendered passage, reference draft, window settings or history.
Failures show a contextual message and retain the current pane. Canceling the
panel does not submit a save.

Local Swift model checks passed 5/5 including fixtures; the strengthened
non-default-setting save/reload scenario also passed its focused 3/3 run.
Visible AppKit checks used generated modules/configuration outside Documents:
new-file save, reimport restoring Strong's numbers, system-confirmed overwrite
and cancel all passed. Overwrite retained `Headings=Off`, a UTF-8 custom value
and an uninstalled-module section. Cancel left both test files' hashes unchanged.
The shared writer's preceding 16/16 local tests and sanitizer checks remain the
core validation baseline; no GitHub build was dispatched for this UI change.

Next: remaining module reading controls and established study interactions, then
full preference restoration and the other native frontends. This is explicit
engine-option persistence only; full user-data migration and visual acceptance
remain pending. No feature group accepted and no legacy file retired.

## Greek accents and cross-reference controls: locally checked

Implemented 2026-10-11 on `modernization/macos-language-reference-options`,
based on the native saver. The Reading options menu adds Greek Accents and
Cross-references only when the shared catalog declares them. Settings still
use the shared module/tab APIs and rendering policy; Swift does not parse or
strip accents or references. Unsupported declarations and multiple-bit inputs
are rejected by the model before queuing an update.

A generated Greek OSIS entry with an original cross-reference note exercises
all four option combinations against direct legacy SWORD setters. Model checks
verify actual accented/plain text and note-marker changes, configured chapter
output, unsubmitted input and history preservation, window isolation and save/
reload. Full local CTest passed 16/16; focused ASan/UBSan passed 2/2 including
fixture setup (external SDK uninstrumented). Visible temporary-fixture checks
confirmed accented Greek becomes plain Greek, the reference marker hides, and
both controls are absent for a module without those declarations.

Cross-reference popup interactions, remaining language/variant controls and
full study-pane/visual acceptance remain pending. No GitHub build dispatched,
no parity group accepted and no legacy files retired.
