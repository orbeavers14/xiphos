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

The estimate includes explicit engine-preference import/save, shared note/reference
workflows and partial dictionary/lexical services and native controls; it does
not imply complete preference, preview or dictionary parity. These weights and within-area percentages are engineering
estimates, not completed issue counts. Foundation still needs a representative module corpus,
Windows native tooling and regression infrastructure. **0 of 38 feature groups
have full accepted parity across the native platforms; 0 of 3 native apps are
release-ready.** The stacked draft milestones are implemented and locally checked
within their documented scope, pending review, platform checks and merge.
No milestone has official acceptance yet.

## Current locally checked scope

- Shared C++ module/verse/chapter reading, Bible history, rendering options,
  engine-preference import/save, note and reference-list previews.
- Shared lexical command parsing, characterized Greek/Hebrew selection and
  copied dictionary reading/key navigation; separate dictionary history state
  and explicit visit/navigation/snapshot commands.
- macOS reading controls, note/reference previews, five lexicon choices,
  Strong's/morphology click dictionary results, direct lookup/key steps and
  separate dictionary Back/Forward/Clear controls and nine-key chooser.
  Explicit XML lexicon import/save uses native file pickers while retaining
  unavailable choices, other settings and reading/dictionary state.
  Strong's/morphology and footnote hover results use a retained information sidebar.
  Cross-reference hovers follow a core-owned per-window list/information setting.
  Reading input and histories stay unchanged; full hover/pane parity is pending.
- Legacy app delegates rendering, selected policy/parser/selection seams,
  dictionary key stepping, dictionary history and Unicode click casing to shared
  C++ helpers. The native host initializes locale before starting the worker.

Current local suite: **46 checks**. The shared Unicode click-key utility reuses
GLib and agrees with the frozen legacy call over **4,448,324 comparisons in
four local locales** including the actual adapted legacy casing block. Main
native click commands use the same helper; hover/dialog reads keep raw keys.
The dictionary history value/legacy adapters also agree over **10,287 tested transitions**. Sanitizer and strict-language
checks are scoped in the evidence table below; they do not instrument SWORD or GLib.
GTK full-app runtime and native Windows/Linux interaction checks remain pending.
Routine milestones use local builds; no new GitHub builds were dispatched.

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
2. Expand hover behavior across notes/references/dialogs and compatible lexicon
   persistence. The [profile seam audit](hover-workflows.md#lexicon-profile-persistence-seam)
   identifies XML keys, startup defaults, duplicate-node behavior and the locale
   startup boundary; 296 XML helper comparisons/assertions now establish the local profile seam.
   The shared owned reader now matches the characterized getter/default rules;
   shared safe literal-text saving, worker/C/Swift commands and native selected-file
   controls are implemented. Saved-locale startup application and other hover
   families remain next.
   Complete remaining reading controls and
   characterize commentary/parallel panes while retaining study-pane composition.
3. Implement equivalent GTK4 and WinUI 3 reading panes with native toolchains
   and interaction checks; retain the legacy app throughout migration.
4. Expand compatible storage and restoration beyond the 18 engine fields, then
   extract search, editors, module management and BibleSync with regression evidence.
5. Reassess audited SwordKit/BibleUI reuse against the shared API when a concrete
   adapter/component saves work, then complete visual/platform release acceptance.

## Validation evidence and limitations

| Area | Completed local evidence | Remaining acceptance |
|---|---|---|
| Inventory and architecture | Source/build/dependency/license audit; 38 feature groups; generated UI/manual inventory; shared ownership/thread boundary | Representative licensed modules, full workflow/visual baselines and native platform tooling |
| Reading and navigation | Owned module/verse/chapter snapshots; legacy SDK comparisons; copied C/Swift adapters; nonblocking worker; per-tab Bible history | Full chapter composition, commentary/parallel panes, restoration and all platform interactions |
| Reading settings | Shared 18-field policy/defaults/scoped options; native toggles/variants; explicit import/save preserving unknown/uninstalled settings | Remaining controls, complete preferences/profile compatibility and real-module corpus |
| Study links | Notes, contextual links and ordered reference-list snapshots; legacy parser comparisons; native copied previews | Hover presentation, detached dialogs and complete link/pane workflows |
| Dictionary and lexicons | Characterized selection, owned reading/key steps, 9-key chooser; separate history agrees over 10,287 transitions; native controls | Persistence, hover presentation, pane composition and full GTK4/WinUI interactions |
| Unicode click policy | Shared/actual legacy casing agrees over 4,448,324 comparisons in four locales; native main clicks normalize/record; raw dialog/hover semantics retained | Unicode database/version comparisons, real-module workflows and forced-locale preference import |
| Native environment locale | 31/31 suite then 3/3 focused fixture/Turkish/invalid-locale checks; 32 checks now defined | Saved locale selection, translation resources and other native host startup |

The full local suite now passes **46/46**. Complete main note/reference callbacks
also agree over 96,792 frozen/actual ordered comparisons, including preference
routing, heading setup and early guards. The legacy handler now avoids its
early-return GString allocation without changing callback traces. This harness
uses backend/presentation shims and is not full GTK/SWORD interaction acceptance.
The legacy main handler now delegates destination/precedence decisions to a
C++11 CoreOptions helper with a C11 header. The same 96,792 traces agree after
adoption, twenty direct C boundary cases pass, and the native-disabled seam
build passes. The owned main-hover worker/C API now implements regular-verse body notes,
ordered cross-reference list/information-link modes, raw lexical snapshots and
explicit no-presentation markers. The per-tab list preference defaults enabled.
Generated real-module checks cover both destinations, copied/scalar lifetimes,
history isolation and errors. The typed Swift adapter now copies all four result
kinds and the core-owned preference, with fixture/lifetime/failure checks.
Native main/information pointer transport now publishes note bodies, ordered
sidebar references or information links with a guarded preference control;
heading/short-passage/fallback/status/dialog behavior and click routing remain
separate acceptance gates. Complete main lexical click/hover
handlers agree over **135,258 frozen/actual callback comparisons**; strict
C++11 and focused address/undefined checks cover that harness. The
[hover contract inventory](hover-workflows.md) records destinations, retention
and outstanding link families. Its source audit corrects the initial unsupported
preview-pin requirement; linked/pinned tabs remain a separate parity workflow.

The owned C++ profile reader passes 367 checks, including actual XML helper
comparisons, UTF-16 decoding, unavailable names, concurrent parser isolation,
explicit file preservation and invalid-input guards. Focused address/undefined
and thread checks pass; libxml2 itself is not instrumented. The reader never
applies saved locale. The separate explicit writer passes 39 preservation,
round-trip and failure checks, retains unrelated XML/locale/duplicate keys and
uses the same staged replacement helper as engine preferences. Null/empty
choices reject because legacy defaults cannot round trip them. Native XML
full-profile storage, interprocess locking, power-loss durability and Windows runtime
replacement validation remain pending. Worker/C commands now check tab-scoped
publication, failed-import retention, history/engine-setting isolation, copied
paths/results and accepted-job drain after observer release/shutdown. The Swift
adapter additionally validates typed save/import round trips, escaping, missing/
NUL paths, closed-tab errors, source preservation and owned results after shutdown.

Native lexical-hover checks cover raw values, committed source context, window
isolation, latest-result ownership, failed-hover retention and closed-window
protection. Temporary GUI checks confirmed Strong's/morphology definitions,
pointer-leave retention, independent dictionary click/close behavior, unchanged
John3:16 with an unfinished John4:2 draft and disabled history navigation.
The source parser/selection/rendering stays in the existing C++ worker. Full
link-family, keyboard, real-module and platform hover parity remains pending.

Native profile model checks additionally cover busy-action rejection, unchanged
committed reading/draft/panes, friendly failures, stale hover invalidation and
closed-window publication. A disposable GUI run confirmed import of all five
choices (including unavailable dictionaries), native save to a new file and a
visible malformed-file error while John3:16, the John4:2 draft and disabled
history controls stayed intact. No installed user profile was accessed.

Native note-hover model checks cover both destinations, no-op/failed retention,
latest-result ownership, per-window preference/busy guards, pending-result
invalidation, return-link navigation and closed publication. A disposable GUI
run confirmed footnote body retention, five ordered/repeated references in the
sidebar, the alternate link-only information mode and Back to the committed
verse. Reading drafts/panes/history controls stayed intact until explicit
return-link navigation. No installed profile or privacy setting changed.

Information-link mode now obtains owned reference targets from the existing
shared parser without rendering verse bodies. The target-only API agrees with
existing full snapshots across 64 module/context/expression cases, including
order/duplicates and the 2,000-verse per-range guard. It retains parser metadata
and copied lifetime semantics. Cross-reference hover additionally retrieves
owned exact-source note metadata without rendering a discarded note body.
Sixteen passage/filter cases match the existing full metadata, with scoped
settings, missing/invalid inputs and lifetime checks. Required values are copied
before a body render can replace SDK attributes. No timing or resource-savings
percentage is claimed.

Latest code milestones: [shared Unicode helper #54](https://github.com/orbeavers14/xiphos/pull/54),
[legacy adoption #55](https://github.com/orbeavers14/xiphos/pull/55),
[native click/origin policy #56](https://github.com/orbeavers14/xiphos/pull/56),
[macOS locale startup #57](https://github.com/orbeavers14/xiphos/pull/57),
[lexical hover characterization #59](https://github.com/orbeavers14/xiphos/pull/59),
[native lexical hover #60](https://github.com/orbeavers14/xiphos/pull/60),
[XML helper characterization #62](https://github.com/orbeavers14/xiphos/pull/62),
[owned profile reader #63](https://github.com/orbeavers14/xiphos/pull/63),
[verified profile writer #64](https://github.com/orbeavers14/xiphos/pull/64),
[serialized commands #65](https://github.com/orbeavers14/xiphos/pull/65),
[typed Swift adapter #66](https://github.com/orbeavers14/xiphos/pull/66),
[native profile controls #67](https://github.com/orbeavers14/xiphos/pull/67),
[note callback characterization #68](https://github.com/orbeavers14/xiphos/pull/68),
[shared note policy #69](https://github.com/orbeavers14/xiphos/pull/69),
[owned main-hover service #70](https://github.com/orbeavers14/xiphos/pull/70),
[typed hover adapter #71](https://github.com/orbeavers14/xiphos/pull/71),
[native study hover #72](https://github.com/orbeavers14/xiphos/pull/72),
[reference targets #73](https://github.com/orbeavers14/xiphos/pull/73).
Scoped address/undefined and thread checks cover shared ownership/state changes;
strict C++11/C++17/C11 checks cover the corresponding seams. These are not full
SDK instrumentation or a leak audit. See each PR for its exact checks and scope.

The [historical implementation record](https://github.com/orbeavers14/xiphos/blob/bdade21e849cc543221f33adb9c16f82e1f30c9f/doc/modernization/status.md)
retains the complete chronological milestones, test counts, source comparisons,
GUI checks, earlier CI snapshots and constraints at an immutable checked commit.
Its intermediate next-step statements describe those earlier checkpoints;
use this page and the parity matrix for current status. Consolidating that
record here removes duplicate work notes from the current status page without
removing test code, license notices or acceptance requirements.

Local runtime checks use Apple Silicon, Swift 6 and SWORD 1.9.0.3916. The
installed SWORD SDK requires macOS 26.0; the current local GLib 2.86.5 build
also targets 26.0. The preview's requested macOS 13 minimum is not runtime
validated. GTK3/WebKitGTK are unavailable locally, so full legacy runtime
validation remains pending. GTK4 and WinUI 3 frontends are not implemented.
No newer GitHub runner build has been dispatched for these routine milestones.
Old CI successes do not validate the current stacked revisions.

Generated GUI tests copy only the disposable app/fixture under `/private/tmp`
to avoid repeated Documents access prompts. Installed libraries, user data and
macOS privacy settings are untouched. See the [macOS development instructions](../../platforms/macos/README.md).

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


## Official sign-off and file retirement

A scoped milestone is accepted when its specified implementation, regression
checks, relevant CI and parity evidence have been reviewed and its PR merged.
Record the accepted commit/PR and evidence here, and update only the parity
rows/platforms that were actually validated. A passing test alone does not
make every file associated with a milestone obsolete.

After acceptance, remove disposable plans, superseded notes and generated
reports once useful decisions/evidence are captured in this status page or
architecture. Generated build/test fixtures and inventories stay under ignored
`build/`; keep the code that regenerates them. Detailed historical evidence
remains in Git history and linked PRs; the current
page retains the compact evidence and acceptance contract.

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
