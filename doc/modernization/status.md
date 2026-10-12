# Modernization status and acceptance
Updated 2026-10-12. Goal: preserve Xiphos functionality, visual composition,
data and workflows through one shared C++ application core and separate
SwiftUI/AppKit, WinUI 3 and GTK4 frontends. Native enhancements supplement
that contract. The [parity matrix](parity-matrix.md) defines acceptance evidence;
[architecture.md](architecture.md) describes boundaries and dependencies.

## Overall completion estimate

Approximately **7% complete** toward a validated native release on all three
platforms (planning range **5–9%**, not a measured parity score). This counts
modernization work, not functionality inherited from the existing GTK app.
The estimate is deliberately provisional and should change as services and
platform workflows are characterized. Reassess after each accepted milestone.

| Work area | Share of total effort | Estimated work completed | Contribution |
|---|---:|---:|---:|
| Inventory, architecture, build/test foundation | 10% | 40% | 4.0% |
| Shared application services | 35% | 6% | 2.1% |
| Three native frontends and design/workflow adaptation | 30% | 3% | 0.9% |
| User-data compatibility and migration | 15% | 1% | 0.15% |
| Release packaging and comprehensive acceptance | 10% | 0% | 0% |

The estimate includes explicit engine/presentation preference import/save, shared
note/reference workflows, paired/sparse commentary services and partial
dictionary/lexical services and native controls; it does
not imply complete preference, preview or dictionary parity. These weights and within-area percentages are engineering
estimates, not completed issue counts. Foundation still needs a representative module corpus,
Windows native tooling and regression infrastructure. **0 of 38 feature groups
have full accepted parity across the native platforms; 0 of 3 native apps are
release-ready.** PRs #1–80 are merged into `master` after scoped local review
and integration checks. The owner authorized local-only acceptance with hosted
platform checks deferred to conserve build credits. This accepts experimental
implementation integration, not full native feature parity or legacy retirement.

## Parity coverage snapshot

The ledger currently has **15 of 38 groups with some initial shared-core/macOS
implementation evidence**, and **23 pending groups**. None of the 15 is fully
accepted: groups bundle many capabilities, and reading a verse does not complete
whole-book rendering, commentary synchronization, tabs/session restore or visual
composition. This is a count of groups touched, not a parity percentage and not
a replacement for the weighted engineering estimate above.

| New frontend | Initial evidence in ledger | Fully accepted groups | Main gaps |
|---|---:|---:|---|
| macOS/shared-core slice | 15 partial, 23 pending | 0/38 | Full pane layout, commentary, parallel views, search, bookmarks/annotations, editors, installation, complete settings/data migration, accessibility/translation and release packaging |
| WinUI 3 | 0 initial frontend entries, 38 pending | 0/38 | Native frontend/tooling, shared-core integration, full workflows and validation |
| GTK4 | 0 initial frontend entries, 38 pending | 0/38 | Native frontend, shared-core integration, full workflows and validation |

The existing GTK3 app is retained as the behavioral oracle; its inherited
functionality does not count as implemented parity in a new frontend. Fixture
comparisons and local tests support individual seams, while representative
modules/profiles and legacy/native visual, keyboard and runtime comparisons
remain required. Prioritize the integration checkpoint below, then the main
study-pane composition/commentary workflow before broader native feature claims.

## Current locally checked scope

An initial [preverse heading collector](heading-workflows.md) now agrees with
complete frozen/current collection functions when block/cleanup postprocessors
are disabled/shimmed. Normal and address/undefined checks pass; native heading
display and the remaining transformation/cache/presentation work are pending.


The [commentary synchronization slice](commentary-workflows.md) includes shared
paired visits/history, typed C/Swift adapters and native upper-right commentary
above dictionary content. Sparse/linked chapter traversal and selected/missing
anchors have frozen/current legacy characterization. Independent native chapter
controls and passage-link pane routing are checked, including unsent input,
note source context and selected-row theme/link contrast.

Shared per-module chapter preferences now match the legacy regular versus personal
classification and default/Off decoding. A combined single-decode import stages
engine and presentation settings together; verified save preserves unrelated,
personal and uninstalled settings. Native adapters apply these defaults and
restore each module setting; explicit import/save round trips preserve drafts
and history. The full suite passes **58/58**; focused address/undefined and thread
adapter checks pass **1/1** each, with the external
SDK instrumentation limits described below. Initial core commentary heading
snapshots, typed C/Swift adapters and native verse/chapter controls use shared
Headings import/save and pass 58/58 local checks, with focused address/undefined
2/2 and thread 2/2. Draft/history/window isolation and a generated-fixture GUI
check pass. Complete heading
transformations, detached windows,
companion/personal editing, saved pane startup and broader corpus/visual acceptance
remain pending. The engineering estimate is now about 7% (range 5–9%);
the fully accepted parity count remains zero.

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

Current local suite: **58 checks**. The shared Unicode click-key utility reuses
GLib and agrees with the frozen legacy call over **4,448,324 comparisons in
four local locales** including the actual adapted legacy casing block. Main
native click commands use the same helper; hover/dialog reads keep raw keys.
The committed `core_tab_history_adapter` harness now compares complete frozen/current
add/fake tab-history functions over 49,560 bounded state/callback comparisons,
including all counts through the 50-entry limit, duplicate positions, cursor and
first-back state, absent/empty/Unicode modules, maximum-sized keys and sequential
eviction visits. It uses the actual layout/limits header with explicit GUI/type
shims; a negative control detects changed first-back behavior. The adapted
functions/helper are unchanged from #1, but this reproducible test is a later
milestone and does not certify full GTK runtime. The dictionary history value/legacy adapters also agree over **10,287 tested transitions**. Sanitizer and strict-language
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

1. Validate representative modules and profiles against the legacy application,
   then the main study-pane composition/commentary workflow. Keep routine
   development/testing local; resume hosted platform checks only when authorized.
2. Expand hover behavior across notes/references/dialogs and compatible lexicon
   persistence. The [profile seam audit](hover-workflows.md#lexicon-xml-profile)
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

The full local suite now passes **49/49**. Complete main note/reference callbacks
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
heading/status/dialog behavior and complete context/click routing remain
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

Main-hover source parsing now adds the characterized decoded-byte short
passage fallback (<5 bytes) and unavailable-module fallback to the supplied
committed source. Twelve generated combinations match the expected source
note, while explicit installed modules/full passages and independent strict
raw/click parsers remain intact. Main, information, reference-list, note-body
and dictionary-definition surfaces now share semantic hover dispatch using
the live committed main source. An older information preview cannot supply
the fallback context; explicit URI targets remain authoritative. Model checks
cover retained old information and nested reference-list results without
changing draft/main HTML/history. A temporary GUI run verified a note inside
the five-passage sidebar opened retained information for John 3:16 while the
main stayed at John 3:20 and the draft at John 4:2. Detached dialogs, generic
dictionary URI hover, heading, aliases and representative modules remain
acceptance gates; dictionary semantic callback wiring is not full keyed-module
hover validation.

A separate owned preview-profile reader now reads `misc/xrefsinverselist`
without changing tabs/files or lexicon import. Seventy-seven checks compare the
actual XML getter with the observed legacy bounded atoi/default rule, including
missing/empty/duplicate nodes, signed/zero/non-numeric prefixes, UTF-16, file
preservation and concurrent copied results. Out-of-int-range prefixes reject
instead of inheriting integer overflow. This does not execute the complete
legacy startup or implement preview XML saving/native restoration.

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
[reference targets #73](https://github.com/orbeavers14/xiphos/pull/73),
[metadata-only notes #74](https://github.com/orbeavers14/xiphos/pull/74),
[note source fallback #75](https://github.com/orbeavers14/xiphos/pull/75),
[development guide cleanup #76](https://github.com/orbeavers14/xiphos/pull/76),
[nested study hover #77](https://github.com/orbeavers14/xiphos/pull/77),
[workflow guide cleanup #78](https://github.com/orbeavers14/xiphos/pull/78),
[preview profile reader #79](https://github.com/orbeavers14/xiphos/pull/79),
[committed history adapter characterization #80](https://github.com/orbeavers14/xiphos/pull/80).
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
Old CI successes do not validate the current integrated revision.

Generated GUI tests copy only the disposable app/fixture under `/private/tmp`
to avoid repeated Documents access prompts. Installed libraries, user data and
macOS privacy settings are untouched. See the [macOS development instructions](../../platforms/macos/README.md).

## Development and CI budget policy

Develop, build and run focused regressions locally. Both `builds` and
`native-core` GitHub workflows are **disabled manually** at the owner's request;
keep them paused until hosted checks are authorized. Workflow-state and recent-run
API checks before, during and after this integration confirmed both were paused
and no new hosted runs started. Merges used reviewed expected head SHAs and
preserved dependency ancestry; no force-push or release tag was used.

The workflow files retain draft/path filters, concurrency cancellation, deliberate
manual dispatch and integration/release triggers for future authorized use.
Disabled/skipped jobs are not passes. Local macOS testing cannot establish
Windows/Linux or the minimum macOS runtime. Relevant platform checks remain
release/retirement gates; do not run the entire matrix for routine small changes.

## Completed default-branch integration

**PRs #1–80 are merged into `master`.** The 77 PRs remaining after #1–3 were
retargeted and merged in dependency order on 2026-10-12. Each reviewed head/base
was verified before merging. A clean local projection established the combined
source; actual master trees were compared with it at batch boundaries and after
#80. The final #80 integration commit is
`00dd335d95354dcc97bbd84dc50c8891e03bd918`; its executable/configuration source is
identical to the checked final snapshot. Subsequent status reconciliation changes
only documentation. PR discussions retain scoped original evidence and the
local-only integration review; Git history retains all implementation commits.

| Integration checkpoint | Fresh local evidence |
|---|---|
| [#1 foundation](https://github.com/orbeavers14/xiphos/pull/1), [#2 async UI](https://github.com/orbeavers14/xiphos/pull/2), [#3 options](https://github.com/orbeavers14/xiphos/pull/3) | Exact native snapshots passed 6/6, 8/8 and 8/8; combined #2/#3 passed 10/10 and native-disabled seams |
| [#13 preferences/reading controls](https://github.com/orbeavers14/xiphos/pull/13) | Combined native build and 16/16 tests |
| [#30 study links/rendering controls](https://github.com/orbeavers14/xiphos/pull/30) | Combined native build and 21/21 tests |
| [#57 dictionaries/locale](https://github.com/orbeavers14/xiphos/pull/57) | Combined native build and 32/32 tests |
| [#80 final stack](https://github.com/orbeavers14/xiphos/pull/80) | Combined native build and 49/49 tests; core address/undefined 35/35 and thread 35/35; native-disabled rendering/history/options seams |

The #3 build conflict retained asynchronous targets while linking CoreOptions.
GitHub reported an ancestry conflict for #5 despite a clean local merge; an
explicit merge of checked master ancestry into its head retained the exact
projected tree and existing commits. No remaining source conflicts required
resolution. Metadata timeouts were reconciled by reading the applied state;
no unexpected head/base changes were accepted.

Strict C++11 helper and C11 public-API checks passed. Local deployment target is
26 with the installed SWORD/GLib dependencies; the requested macOS 13 minimum is
not validated. External SWORD, GLib and system libxml2 are not fully instrumented;
leak detection was disabled for address checks. Full legacy GTK runtime,
Windows/Linux, ICU-enabled SDK, representative modules/profiles, keyboard,
translation and visual/design checks remain deferred. No release is certified.

For future milestones, keep small reviewed PRs, appropriate local regression
checks, coherent bases and explicit acceptance limits. Recheck changed heads/bases,
retain tests/licenses, and update only parity rows actually validated. Source/data
compatibility and full platform checks must precede affected legacy retirement.

## Official sign-off and file retirement

A scoped milestone is accepted when its specified implementation, regression
checks, relevant validation and evidence have been reviewed and its PR merged. The owner
explicitly authorized local-only integration for #1–80 with hosted/platform checks
deferred; record that limitation rather than treating skipped CI as a pass.
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
