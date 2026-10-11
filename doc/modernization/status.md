# Modernization status and acceptance

Updated 2026-10-11. Goal: preserve Xiphos functionality, visual composition,
data and workflows through one shared C++ application core and separate
SwiftUI/AppKit, WinUI 3 and GTK4 frontends. Native enhancements supplement
that contract. The [parity matrix](parity-matrix.md) defines acceptance evidence;
[architecture.md](architecture.md) describes boundaries and dependencies.

## Overall completion estimate

Approximately **6% complete** toward a validated native release on all three
platforms (planning range **4–8%**, not a measured parity score). This counts
modernization work, not functionality inherited from the existing GTK app.
The estimate is deliberately provisional and should change as services and
platform workflows are characterized. Reassess after each accepted milestone.

| Work area | Share of total effort | Estimated work completed | Contribution |
|---|---:|---:|---:|
| Inventory, architecture, build/test foundation | 10% | 40% | 4.0% |
| Shared application services | 35% | 4% | 1.4% |
| Three native frontends and design/workflow adaptation | 30% | 2% | 0.6% |
| User-data compatibility and migration | 15% | 1% | 0.15% |
| Release packaging and comprehensive acceptance | 10% | 0% | 0% |

The revised estimate includes explicit engine-preference import/save and the
shared note/reference workflow; it does not imply complete preference or
preview parity. These weights and within-area percentages are engineering estimates, not
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

1. Review the stacked draft milestones and validate representative modules
   against the legacy application. Bundle remote builds at meaningful review
   checkpoints; keep routine development/testing local to conserve CI usage.
2. Complete reading decorations, remaining language/variant controls, expanded
   cross-references and Strong's/morphology interactions. Characterize commentary
   and parallel panes while retaining established study-pane composition.
3. Implement equivalent GTK4 and WinUI 3 reading panes with native toolchains
   and interaction checks; retain the legacy app throughout migration.
4. Expand compatible storage and restoration beyond the 18 engine fields, then
   extract search, editors, module management and BibleSync with regression evidence.
5. Reassess audited SwordKit/BibleUI reuse against the shared API when a concrete
   adapter/component saves work, then complete visual/platform release acceptance.

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

## Shared study-note retrieval: locally checked, native presentation pending

Implemented 2026-10-11 on `modernization/shared-study-notes`, based on the
language/reference controls. A C++11-compatible rendering helper reproduces
the original backend's UTF8HTML conversion followed by SWORD body rendering.
Reader/session/worker APIs accept module, verse and note identifier and return
owned body HTML plus reference, label, type and reference-list metadata. The
requested verse is rendered first to regenerate its attributes, preventing
stale-note results after whole-chapter rendering. Missing notes use lookup
without creating phantom SDK attributes.

Sessions apply their effective module settings; previewing a note leaves
history untouched. Worker requests copy arguments, reject closed tabs, recover
after errors and return data that outlives the worker. Four Strong's/footnote
combinations match direct original backend calls on an original Unicode/formatted
note fixture. Tests also cover module/verse context, cross-reference metadata,
failed-option restoration and separate-tab settings. Full local CTest passed
17/17, ASan/UBSan 3/3 and ThreadSanitizer 3/3 including fixture setup (external
SDK uninstrumented). The default legacy rendering seam still builds with
C++11; helper/test warnings-as-errors checks passed.

Next: expose note requests through the C/Swift adapters and handle generated
note links in the native study pane. Cross-reference verse-list expansion,
introductory notes, legacy hover/click distinctions and full popup/design parity
remain pending. No GitHub build dispatched, no accepted parity group and no
legacy code retired.

## Study-note C and Swift adapters: locally checked

Implemented 2026-10-11 on `modernization/study-note-adapters`, based on shared
note retrieval. The experimental C API accepts explicit module/verse/identifier
and a tab, queues a nonblocking note request and transfers an owned result once.
Separate note handles expose six copied strings and remain valid after the
request and worker close. Invalid/null inputs and wrong result types are checked;
missing notes/modules/closed tabs return the existing contextual statuses.
Swift adds a thin async worker method and Sendable result, with no independent
attribute parsing, rendering or option policy.

Full local CTest passed 17/17. C note tests cover argument copying, not-ready/
wrong-kind/double-take access, metadata, reference lists, missing context,
null inputs, queue recovery, unchanged history and result lifetime. Swift checks
retrieve a note after a chapter render, retain it after shutdown and reject a
missing verse note without changing history. Focused ASan/UBSan and
ThreadSanitizer each passed 3/3 including fixtures (external SDK uninstrumented);
C11/C++17 warnings-as-errors checks passed. No GitHub build dispatched.

Next: shared decoding of generated study-note links and native presentation;
expanded cross-reference workflows, hover behavior, introductory notes and full
popup parity remain pending. No accepted feature group or legacy retirement.

## Shared study-note links: locally checked, native binding pending

Implemented 2026-10-11 on `modernization/shared-note-links`, based on note
adapters. The core decodes local SWORD `passagestudy.jsp`/`xiphos.url` showNote
commands and the fixed native preview origin, resolving explicit module/verse/
identifier/type in one queued tab operation. Values decode separately, so an
encoded ampersand in a module name cannot become an extra query parameter.
External origins, malformed escapes, duplicate/unknown parameters, unknown
actions/types, controls and unsupported ranges reject. Note type is validated
against the requested entry's attributes. Swift calls this shared operation.

Full local CTest passed 17/17. Pure tests cover native/legacy local origins,
UTF-8/default modules/reordered parameters and invalid inputs; integration
checks consume an actual SWORD-generated note URI with an explicit module
different from the fallback. C and Swift verify resolved body/context and
external-link rejection. Focused ASan/UBSan passed 4/4 and ThreadSanitizer 3/3
including fixtures (external SDK uninstrumented). C11/C++17 warnings-as-errors
checks passed.

Next: native preview binding and visible clicks. Hover behavior, expanded
cross-reference lists, introductory notes and full popup/design parity remain
pending. No GitHub build dispatched, accepted group or legacy retirement.

## Native study-note preview: locally checked, full interaction parity pending

Implemented 2026-10-11 on `modernization/macos-study-note-preview`, based on
shared note links. Shared dispatch now returns a passage target or rendered note
in one worker operation; the C request exposes the corresponding owned result,
and Swift binds the copied result. Clicking a generated note/cross-reference
marker opens a body preview below the module list. The main passage, draft input
and history stay unchanged. Close dismisses the preview; a successful passage
refresh clears it. Errors retain the current preview and main pane, and closed
models cannot publish late note results.

Notes now carry their source module through C/Swift, so links inside an explicitly
different module's note use that module as their fallback. The preview labels
source module and passage. A trusted fixed HTML base resolves relative engine
commands into the already validated internal origin; browser navigation is
canceled before core dispatch, with module JavaScript/network content still
disabled. This is an initial preview pane, not accepted full Xiphos composition.

Full local CTest passed 17/17; strengthened note-module fallback model checks
passed 3/3 including fixtures. ASan/UBSan and ThreadSanitizer each passed 3/3
including fixtures (external SDK uninstrumented), and C++11/C++17/C11
warnings-as-errors checks passed. Visible checks on temporary generated fixtures
confirmed study-note text, whole-chapter note retrieval, cross-reference body
content, Close and source labels. An unsubmitted reference remained unchanged
while the note opened. Model checks also cover two-window isolation, error
preservation, normal passage dispatch and late-result suppression.

Next: reference links inside notes and expanded cross-reference workflows.
Hover/pin behavior, introductory notes, Strong's/morphology lookup, all language/
variant controls and full visual/platform acceptance remain pending. No GitHub
build dispatched, accepted parity group or legacy file retirement.

## Scripture-reference links inside notes: locally checked

Implemented 2026-10-11 on `modernization/shared-reference-links`, based on
native note preview. Shared command decoding now accepts the engine's local
`showRef&type=scripRef` links for a single reference. The parser shares strict
local-origin/query validation with note commands. Core dispatch validates the
module and returns the target; visits normalize through the module's own SWORD
key and existing history. The native note adapter supplies its source module
as fallback, without a Swift reference/URL parser.

Full local CTest passed 17/17, ASan/UBSan 4/4 and ThreadSanitizer 3/3 including
fixtures (external SDK uninstrumented); C++17 warnings-as-errors checks passed.
Pure tests cover encoded/Unicode/explicit/default modules, dispatch kinds and
invalid/range/ambiguous requests. C++/C tests use an actual reference URI from
a SWORD-rendered cross-reference note and verify normalized entry output. Native
model and visible temporary-fixture checks confirmed the reference click opens
John 3:16 in the note's source module, closes the stale note and preserves
original reference-deduplicated history (Forward remains available).

Next: expanded cross-reference lists/ranges and remaining study interactions.
Hover/pin behavior, introductory notes, Strong's/morphology lookup, complete
layout and other native platforms remain pending. Overall planning estimate is
now approximately 6% (4–8%), with 0/38 accepted feature groups and 0/3 release-ready
platforms. No GitHub build dispatched and no legacy file retired.

## Shared reference-list parser and legacy seam: locally checked

Implemented 2026-10-11 on `modernization/shared-reference-list`, based on note
reference links. The existing fork's verse-list algorithm moves into a shared
C++11 helper using the module's locale/versification. It retains list ordering,
duplicates, indexed invalid-element skipping, range expansion and the existing
2000-verse guard per range. Results own reference strings and explicitly report
guard truncation/skipped elements. The helper clones the key to preserve its
position/flags; the legacy adapter preserves its former current-key mutation
and returns the same GList interface. Missing/null input is handled safely.
Original attribution and license notices are retained.

Characterization compiles the actual adapter and a frozen copy from f4ff0ca,
using a minimal ownership-compatible list shim without adding GTK dependencies
to native tests. All 56 cases across KJV/KJVA match: single/OSIS references,
relative/list/cross-chapter ranges, duplicates, missing/invalid/empty references,
whole chapters/books and guarded ranges. KJVA deuterocanonical expansion also
passes. Full local CTest passed 18/18; focused ASan/UBSan passed 1/1 (external
SDK uninstrumented). The default CoreRender legacy seam builds in C++11, and
the helper plus actual adapter pass C++11 warnings-as-errors checks. Full GTK
application/toolchain testing remains an acceptance gate on this macOS host.

Next: shared rendered-list snapshots and C/Swift/native preview bindings. No
GitHub build dispatched, accepted feature group or legacy file retirement.

## Rendered reference-list snapshots: locally checked, host binding pending

Implemented 2026-10-11 on `modernization/rendered-reference-lists`, based on
shared parsing. Reader/session/worker requests return owned module, canonical
context, original expression, entry snapshots, HTML and guard/skipped metadata.
Shared rendering keeps reference order, duplicates and positions without text;
headers use escaped labels and core-generated passage links. The reader restores
the module key and SDK filter state after rendering. Sessions apply effective
module settings without visiting or moving history, and queued inputs/results
are copied.

Four Strong's/Greek-accent combinations match direct legacy SWORD entry calls
for a generated list containing Greek, notes and duplicate references. Tests
also cover relative and cross-chapter ranges, missing-text positions, empty
lists, 2000-verse guard propagation, two-tab settings/history, failed-module/
context handling, closed tabs and result lifetime beyond reader/worker shutdown.
Pure presentation tests cover escaping, retained engine markup and unique
positions for duplicates. Full local CTest passed 19/19, ASan/UBSan 4/4 and
ThreadSanitizer 3/3 including fixtures (external SDK uninstrumented).
C++11/C++17 warnings-as-errors checks passed.

Next: C/Swift request bindings and a native referenced-passage preview. Full
GTK, hover/pin/expanded link workflows and cross-platform acceptance remain
pending. No GitHub build dispatched, accepted parity group or legacy retirement.

## Reference-list C and Swift adapters: locally checked

Implemented 2026-10-11 on `modernization/reference-list-adapters`, based on
rendered snapshots. The C API queues explicit module/expression/context/tab
requests and transfers one owned reference-list result. It exposes copied
metadata, entries, HTML and guard/skipped information with safe null/index
handling. Empty expressions are valid; module/context are required. Swift adds
a thin async method and Sendable copied snapshot; parsing and rendering remain
in the shared core.

Full local CTest passed 19/19. C checks cover queued argument copying, result
kind/not-ready/double-take, actual legacy entry outputs, metadata/index bounds,
empty/guarded lists, null/missing/closed inputs, queue recovery and unchanged
history. A request can be observed and its result retained after worker
shutdown. Swift verifies normalized context, duplicate order, configured
output, empty/error handling and result lifetime after shutdown. Focused
ASan/UBSan and ThreadSanitizer each passed 3/3 including fixtures (external SDK
uninstrumented); C11/C++17 warnings-as-errors checks passed.

Next: native referenced-passage preview and visible cross-reference workflow.
No GitHub build dispatched, accepted parity group or legacy file retirement.

## macOS referenced-passage preview: locally checked

Implemented 2026-10-11 on `modernization/macos-reference-list-preview`, based on
the reference-list adapters. Show referenced passages renders the note's full
reference list through the shared worker and settings policy. The lower study
pane displays source context, count, empty results and range-limit/skipped
notices. Back to note restores the body; Close clears both previews. Passage
headings use existing shared navigation. No parser or SWORD policy moved into
Swift.

Validation: all 19 local tests passed. The native model checks single and
range/duplicate lists, configured Strong's/Greek output, unchanged main pane,
draft and history, separate windows, failed actions and late results after
closing a window. A temporary GUI fixture visibly displayed five passages in
order (including duplicate John 3:16 and Greek text), retained an unfinished
John 4:2 draft, returned to the note, and navigated from a list heading with
legacy history deduplication. No GitHub build was dispatched.

Next: range/list links inside note bodies and remaining study interactions.
Hover/pin, full visual parity and cross-platform acceptance remain pending.
No feature group is officially accepted and no legacy file is retired.

## Contextual reference-link core: locally checked, host binding pending

Implemented 2026-10-11 on `modernization/contextual-reference-links`, based on
the native list preview. Legacy `show_ref` clicks display verse lists in the
sidebar. An additive core handler now resolves single/ranged/list showRef
commands into configured reference-list snapshots with explicit source
passage context. Passage URI navigation and study-note results keep their own
types; earlier single-reference APIs retain their contracts. Query/origin
validation remains shared, and previews never visit history.

Validation: 19/19 local checks, 4/4 focused address/undefined-sanitizer checks
and 3/3 focused thread-sanitizer checks passed (external SWORD SDK remains
uninstrumented). Strict warning-as-error C++17 compilation passed. Tests use a
real SWORD-generated range link and cover relative lists/duplicates, tab
settings, source context, malformed commands, preserved backward/forward
history, closed tabs and accepted results after worker shutdown. No GitHub
build was dispatched.

Next: C/Swift bindings and native note-body dispatch. Unknown-module legacy
fallback, hover/pin and full preview/design parity remain acceptance work.
No legacy retirement or feature sign-off occurs here.

## Contextual C/Swift link adapters: locally checked

Implemented 2026-10-11 on `modernization/contextual-link-adapters`, based on
the contextual core. The additive C request reports passage, note or
reference-list handles through existing owned-result accessors. Source context
and URI must be nonempty; nullable module fallback works when the command
provides an explicit module. Swift copies the typed result into Sendable values
and releases the C handles. No frontend parser was introduced.

Validation: 19/19 local checks, focused address/undefined and thread checks
3/3 each, strict C++17 compilation and C11 header checks passed. C cases cover
all three result kinds, pending/wrong-kind/double-take, null arguments, external
origin rejection, configured five-entry lists and requests/results retained
after shutdown. Swift covers all three kinds, relative duplicate lists, invalid
context, history preservation and copied-value lifetime. No GitHub builds.

Next: native note-body clicks should open the sidebar list with their committed
source context. Full hover/pin and preview parity remain pending.

## macOS contextual note-body links: locally checked

Implemented 2026-10-11 on `modernization/macos-contextual-links`, based on the
contextual adapters. Native note-body scripture links now open the shared
sidebar reference-list preview, including single references and ranges. This
supersedes the earlier single-reference main-navigation preview behavior and
follows legacy clicked `show_ref` composition. Each web pane passes its own
committed source module/reference; relative lists cannot use unfinished input.
A standalone list can appear without a note, and Back to note appears only
when a note body is available. Passage headings continue shared navigation.

Validation: 19/19 local checks passed. Native model checks cover ranged,
relative duplicate and single reference links, standalone preview, correct
context despite a draft, configured output, unchanged main pane/history and
header navigation. Visible temporary GUI checks clicked SWORD's actual range
and single body links, confirmed four/one sidebar passages, retained main
John 3:20 plus draft John 4:2, returned to the note and navigated from a heading
with legacy history deduplication. No GitHub build was dispatched.

Next: remaining language/variant controls and Strong's/morphology interactions.
Hover/pin, unknown-module fallback, full visual/workflow and platform acceptance
remain pending. Estimated completion stays about 6% (4–8% provisional range);
0/38 feature groups accepted, 0/3 native platforms release-ready.

## macOS Hebrew reading controls: locally checked

Implemented 2026-10-11 on `modernization/macos-hebrew-options`, based on
contextual native links. Hebrew Vowel Points and Hebrew Cantillation controls
bind the existing shared flags/settings and appear only for declared module
filters. Refresh keeps the committed passage, unfinished draft and history.
The original generated Hebrew fixture supplies all four option combinations.

Validation: 19/19 local checks and 2/2 focused address/undefined checks passed;
strict C++17 fixture compilation passed. All four core renders equal direct
legacy SWORD setter/render calls. Native model checks undeclared rejection,
window isolation, chapter output, draft/history preservation, save/reload and
restoration. Temporary GUI checks confirmed original/cantillation-off/vowels-off
text, preserved draft during vowel refresh, chapter output and hidden Hebrew
controls for an undeclared module. The SDK retains the shin dot when vowel
points are off; expected output follows the actual engine behavior. No GitHub
build was dispatched.

Next: variant-reading selection and remaining reading/display controls, then
lexicon interactions. Full theme, hover/pin and platform acceptance remain
pending; no feature sign-off or legacy retirement.

## Shared variant-selection legacy seam: locally checked

Implemented 2026-10-11 on `modernization/shared-variant-selection`, based on
Hebrew controls. The primary/secondary/all mapping moves from GTK's existing
reading_selector into a C-callable CoreOptions helper. GTK delegates while
retaining option saves, dialog/main navigation and invalid-choice behavior.
Legacy code and copyright notices remain; native operation is pending.

Validation: 20/20 local checks; the actual adapted callback equals the frozen
2fb7387 original for 12 valid/invalid/dialog/main cases, including Unicode and
percent signs in module names. The focused address/undefined check passed.
The default C++11 legacy seam builds, and strict C11 actual-adapter/header and
C++11 helper checks pass. Full GTK application dependencies remain unavailable
on this Mac, so this is seam validation, not a full GTK runtime check. No
GitHub build dispatched or feature group accepted.

Next: shared tab-aware variant selection, C/Swift binding and native controls.

## Tab-aware variant selection: locally checked, native binding pending

Implemented 2026-10-11 on `modernization/tab-variant-selection`, based on
the legacy selection seam. Shared selection applies one characterized
primary/secondary/all choice to copied effective module settings, preserving
every unrelated flag. The session/worker requires configured settings and a
declared variant filter, commits one module override and never visits history.
Invalid, undeclared, unconfigured and closed-tab operations reject.

Validation: 21/21 local checks, focused address/undefined and thread checks
3/3 each, default legacy seam build and strict C++11/C++17 checks passed. A
generated original OSIS fixture follows CrossWire's
[OSISVariants filter documentation](https://www.crosswire.org/sword/apiref/classOSISVariants.html);
all three actual renders equal direct legacy SWORD setter/render calls. Tests
cover entry/chapter/sidebar output, all eight prior variant-flag combinations,
unrelated preferences, window settings isolation, backward/forward history,
failed operations, copied worker submission and save/reload. No GitHub builds.

Next: C/Swift operation bindings and native variant choices. No full variant
or platform parity is accepted; legacy operation remains available.

## Native textual-variant controls and adapters: locally checked

Implemented 2026-10-11 on `modernization/macos-variant-controls`, based on the
tab-aware variant service. An additive C request and typed Swift adapter invoke
the shared selection policy. Native Reading options exposes the legacy
Primary Reading, Secondary Reading and All Readings choices for declared
variant filters. The core updates all three flags; Swift retrieves the result
and refreshes the committed passage without changing the draft or history.

Validation: 21/21 local checks, focused address/undefined and thread checks
3/3 each, strict C++17 and C11 header checks passed. C checks actual output,
ambiguous prior flags, status-only result kind, invalid/null/missing/undeclared
and closed-tab requests. Swift verifies ABI values and undeclared rejection;
the native model verifies all three choices, unchanged unrelated flags,
chapter/entry output, window isolation and save/reload. Temporary GUI checks
selected all three outputs, preserved draft John 4:2 while John 3:22 remained
committed, and confirmed no variant menu for an undeclared module. No GitHub
build was dispatched.

Next: remaining word-annotation/display controls and lexicon interactions.
Full variant-module corpus, visual/workflow and platform acceptance remain
pending. Overall estimate remains about 6%; 0/38 accepted feature groups.

## Native word-annotation controls: locally checked

Implemented 2026-10-11 on `modernization/macos-word-annotation-options`, based
on variant controls. Native Reading options now binds declared Lemmas,
Transliterated Forms, Glosses and Morpheme Segmentation through existing shared
settings. No filter or rendering policy moved into Swift. Original generated
word markup follows CrossWire's filter/render implementation, including the
lemma.* namespace recognized by OSISLemma.

Validation: 21/21 local checks, 2/2 focused address/undefined checks and strict
C++17 fixture compilation passed. All 16 actual core renders equal direct
legacy SWORD setter/render calls and every individual filter changes output.
Native model covers undeclared rejection, individual enable/restore, window
isolation, draft/history preservation, chapter output and save/reload. Visible
temporary GUI checks confirmed lemma links, encoded transliteration, gloss
removal, preserved unfinished input and hidden controls on an undeclared
module. Morpheme span output is validated by model/core tests; full segmentation
color/style and lookup interactions remain pending. No GitHub builds.

Filter references: [OSISLemma source](https://www.crosswire.org/ftpmirror/pub/sword/source/sword-1.9.0/src/modules/filters/osislemma.cpp),
[OSISXlit](https://www.crosswire.org/sword/apiref/osisxlit_8cpp_source.html),
[OSISGlosses](https://www.crosswire.org/sword/apiref/classOSISGlosses.html),
[OSISMorphSegmentation](https://www.crosswire.org/sword/apiref/classOSISMorphSegmentation.html)
and [OSISXHTML](https://crosswire.org/sword/apiref/osisxhtml_8cpp_source.html).

Next: lexicon/dictionary service and interactions, remaining enumerations,
headings and global transliteration controls. Full visual/platform parity and
acceptance remain pending; no legacy file retirement.

## Shared dictionary reader: locally checked, native binding pending

Implemented 2026-10-11 on `modernization/shared-dictionary-reader`, based on
word annotation controls. The core renders an explicit installed SWLD dictionary
module/key through the existing legacy-compatible setKey/renderText seam.
Snapshots own module, post-render key and Unicode HTML. Sessions use module
settings; the worker serializes copied requests. Dictionary previews do not
visit Bible history. SWORD owns normalization/padding/nearest-key behavior;
this is not an independent exact-match index.

Validation: 22/22 local checks, focused address/undefined and thread checks
1/1 each and strict C++17 compilation passed. Two original RawLD dictionaries
and a Bible fixture compare actual output/key values with direct legacy SDK
calls for case, Unicode, padded and missing keys. Checks cover actual filter
output, separate tab/module context, empty/missing/wrong-type requests, no
history visits, closed tabs, copied arguments and owned results after teardown.
No installed user library or GitHub build was used.

Next: C/Swift binding, shared preferred-lexicon and Strong's/morphology dispatch,
then the established native dictionary pane. General books, dictionary browsing,
full module corpus and all platform acceptance remain pending. No legacy
retirement or full feature acceptance; estimate remains about 6%.

## Dictionary adapters: locally checked, pane/dispatch pending

Implemented 2026-10-11 on `modernization/dictionary-adapters`, based on the
shared dictionary reader. The C API provides asynchronous typed dictionary
requests and owned results; Swift copies module, post-render key and Unicode
HTML into Sendable values. Lookup/render/option logic stays in the C++ core.
Request and result lifetimes follow the existing worker contract; accepted
requests can be observed after shutdown and transferred results outlive both
request and worker. This adds bindings, not a completed dictionary UI.

Validation: 23/23 local checks passed, including a new Swift adapter check.
Focused address/undefined and thread checks passed 1/1 each; strict C++17
compilation and C11 public-header checks passed. Tests cover real dictionary
option output, separate tabs/modules, copied Unicode results, pending/wrong-type
and take-once ownership, empty/missing/wrong-module errors, closed tabs, continued
queue operation, unchanged history and results retained after shutdown. SWORD
itself remains an uninstrumented dependency in sanitizer checks. No GitHub
builds or installed user-data changes.

Next: characterize preferred dictionary and Strong's/morphology dispatch in the
legacy source, implement those shared policies, then add the native dictionary
pane. Full dictionary browsing, general books, visual parity and platform
acceptance remain pending. No legacy files retired; 0/38 feature groups fully
accepted and the provisional overall estimate remains about 6%.


## Shared lexical parser: locally checked, selection/dispatch pending

Implemented 2026-10-11 on `modernization/shared-lexical-links`, based on the
dictionary adapters. Typed Strong's/morphology commands preserve raw key/type,
explicit/fallback source module, optional passage and Unicode. Existing strict
origin/query parsing rejects ambiguous or malformed commands. Parsing does not
select a dictionary or change the native dispatcher.

The [lexical lookup seam inventory](lexical-lookup.md) records actual main versus
dialog selection differences, preference defaults, source testament mutation,
and clicked dictionary history/uppercase behavior versus raw-key hover previews.
These observations require characterization before policy extraction; a single
undocumented selection heuristic would lose existing behavior.

Validation: 23/23 local checks, focused address/undefined check 1/1 and strict
C++17 compilation passed. Cases cover generated commands, reordered parameters,
encoded delimiters/Unicode/custom lemma tags, missing morphology types,
external origins, duplicate parameters, control characters and malformed
escapes. The existing dispatcher still rejects lexical actions until selection
is integrated. No GitHub builds or user-data changes. Estimate about 6%; full
F16 parity and native dictionary interactions remain pending.


## Lexical selection characterization: locally checked, extraction next

Implemented 2026-10-11 on `modernization/lexical-selection-characterization`.
The test compiles actual legacy main/dialog selection prefixes, compares frozen
96068973 logic and verifies source module/key calls across 11,264 cases plus
explicit differing fallbacks/context. It covers both testaments, all installed
lexicon combinations, null morphology preferences, exact/case-sensitive tags,
custom lemma namespaces and overlapping Greek/Hebrew tags. Production behavior
is unchanged. Validation: 24/24 local checks and strict C++11 compilation passed.
No GitHub builds. GTK presentation effects/full lexical parity are not tested;
next extract explicit main/dialog selection policy through the shared core.
Overall estimate about 6%, 0/38 feature groups accepted; no file retirement.


The parity matrix now records scoped implementation evidence for F09, F15–F17,
F29 and F37 rather than leaving implemented subsets marked wholly pending.
The legacy C++11 seam libraries still build with the native core disabled.
Full legacy GTK dependency/GUI validation remains unavailable on this Mac.
These are evidence updates; no feature group or native platform is accepted.

The selection characterization also passes address/undefined instrumentation
on its GTK-free harness. A negative-control build deliberately replaced the
main NT fallback name in an ignored temporary copy; the comparison failed at
the expected morphology divergence. Production/generated source was not
modified by that control. This confirms the harness detects a real selection
regression; it does not extend GUI/module-corpus acceptance.


## Shared lexicon selection: locally checked, native integration pending

Implemented 2026-10-11 on `modernization/shared-lexicon-selection`, based on
lexical characterization. Main and detached-dialog legacy selectors delegate
to one C11-callable C++ helper in CoreOptions. Explicit origin retains their
different recognition/fallback branches, borrowed/null/empty preferences and
lazy synchronous source-testament/availability callbacks. Presentation and
history paths are preserved; no legacy files retired. Invalid null
morphology/dialog types now safely yield no selection.

Validation: 24/24 local checks passed, followed by final focused selector/C
boundary checks; 16,968 frozen/actual comparisons preserve selected names and
callback order/context. Strict C++11/C11 and focused address/undefined checks
pass; native-disabled C++11 seam libraries build. Full GTK GUI and
representative lexicon corpus remain unvalidated. No GitHub builds or installed
user-data changes. Next: copied native lexicon preferences and safe verse-key
context, shared lexical dispatch and native dictionary/hover presentation.
Estimate remains about 6%, with 0/38 groups fully accepted.


## Shared lexical reader: locally checked, adapters/UI pending

Implemented 2026-10-11 on `modernization/shared-lexical-reader`, based on shared
selection. Explicit copied per-tab lexicon preferences retain nullable/empty
values and legacy defaults; existing tabs remain unconfigured until set.
Lexical previews parse/select/render with the shared policy and selected
dictionary's engine settings, returning owned command/source/dictionary data.
The worker serializes copied requests. Greek source testament is read from a
validated cloned VerseKey, preserving the module's committed location. URI
passage stays metadata; caller reference remains authoritative like legacy.

Validation: 25/25 local checks, focused address/undefined and thread checks
2/2 each (including the original dictionary fixture), and strict C++17 checks
passed. Cases cover actual OT/NT source keys, main/dialog differences, explicit
module versus passage metadata, separate tab/module settings, opt-in defaults,
Unicode ownership, invalid/missing/non-verse contexts and selected modules,
NUL preference/context rejection, atomic failed settings, history/location
preservation, queue recovery, closed tabs and accepted results after shutdown.
SWORD itself remains uninstrumented. No GitHub builds or user-data changes.
Next: C/Swift bindings for these copied preferences/lookup values, native
preferences and dictionary/hover/click workflows. Full lexical/platform parity
is pending; no retirement or accepted groups. Estimate remains about 6%.


## Lexical adapters: locally checked, native controls pending

Implemented 2026-10-11 on `modernization/lexical-adapters`, based on the shared
lexical reader. C requests copy nullable preference names and transfer owned
configured/unconfigured snapshots and lexical results once. Swift copies
Sendable preferences/results, validates enum constants and rejects embedded
NULs before submission. Core default names and selection/render policies remain
shared; no Swift dictionary-selection implementation was introduced.

Validation: 26/26 local checks, focused address/undefined and thread checks
2/2 each and strict C++17/C11 checks passed. C tests cover copied submissions,
null versus empty names, default snapshots, pending/wrong-kind/take-once results,
metadata/source context, invalid URI/origin/output, error recovery, unchanged
history, closed tabs and snapshots/accepted requests after worker teardown.
Swift tests cover shared defaults/ABI, copied nullable preferences, isolated
tabs, actual OT selection, NUL/external failures and owned values after close.
SWORD itself remains uninstrumented. No GitHub builds or user-data changes.
Native controls, feature-filtered dictionary choices, preview/pane composition,
hover/click dictionary history and profile persistence remain pending. Overall
estimate about 6%; no accepted parity groups or legacy file retirement.


## Lexicon catalog metadata: locally checked, native selectors next

Implemented 2026-10-11 on `modernization/lexicon-catalog-metadata`, based on the
lexical adapters. The core catalog copies dictionary classification and exact
first-Feature lexicon roles for Greek/Hebrew definitions and morphology.
Devotionals are excluded from dictionary choices like the legacy list. C and
both synchronous/worker Swift adapters expose copied values and bounds-safe
queries; the frontend need not read SWORD metadata or implement role matching.

Validation: 26/26 local checks passed, followed by focused catalog/C/Swift
checks, strict C++17/C++11/C11 checks and an address/undefined dictionary check
1/1. Expanded original fixtures cover all roles, generic dictionaries, exact
case, multiple Feature entries, a devotional and a non-dictionary advertising
GreekDef. Metadata compares with direct SDK type/first-Feature legacy predicates;
synchronous and worker adapters agree. Native-disabled C++11 seam libraries
build. SWORD itself is uninstrumented. No GitHub builds or user-data changes.
Next: native lexicon preference selectors and dictionary presentation; full
module corpus, effective capability, persistence and platform parity pending.
No retirement/accepted groups; estimate remains about 6%.
