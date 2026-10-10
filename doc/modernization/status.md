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
2. Extract per-tab reference/navigation/history and full chapter rendering
   with options into C++; characterize existing behavior first. Screen and
   workflow comparisons must retain the established study-pane composition.
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
