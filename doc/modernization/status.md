# Modernization status and acceptance
Updated 2026-10-16. Goal: preserve Xiphos functionality, visual composition,
data and workflows through one shared C++ application core and separate
SwiftUI/AppKit, WinUI 3 and GTK4 frontends. Native enhancements supplement
that contract. The [parity matrix](parity-matrix.md) defines acceptance evidence;
[architecture.md](architecture.md) describes boundaries and dependencies.

For work across computers/chats, start with [the shared handoff](handoff.md).
The Linux GTK4/WebKitGTK preview now exposes the existing macOS preview's
implemented reading/options/dictionary/contextual-preview/profile/window scope.
Fedora ARM64 validation passes 51/51 integrated tests, four sanitized model
executables and six software-rendered desktop smoke modes. See the
[Linux handoff](handoff-linux.md) for tested revisions, commands and limits.
Manual physical-input, picker, screen-reader, visual/corpus and accelerated
rendering acceptance remains pending; no complete parity row is accepted.
Hosted Linux checks remain manual-only (`cf9c853`); no catch-up runner jobs ran.
The weighted planning estimate below predates Linux catch-up and has not been
recomputed. It must not be read as a new measured parity score.

The following earlier checkpoints retain their original test counts and limits;
the latest session checkpoint follows them.

Earlier scoped presentation checkpoint: [shared double spacing](double-space-workflows.md)
adds exact legacy defaults, per-module persistence and queued C/native commands
(`25cf291`, `68562af`, `420ae13`). Mac regression checks pass 70/70; portable
Linux/core checks pass 54/54 on Mac. Focused ASan/UBSan core and Linux model runs
pass 4/4 and 3/3 respectively. A real Mac window check confirms CSS-only chapter
spacing refresh on/off. Current GTK controls still need native Fedora validation.
No full parity row or release gate changes status.

Earlier stages of the [note-numbering seam](note-numbering-workflows.md) added owned explicit
counters reused by legacy cleanup (`c39b024`) and a shared SDK label switch
(`8572594`). Frozen/current comparisons and 40 OSIS/ThML/GBF/TEI filter cases pass;
that checkpoint passed 71/71 Mac and 55/55 portable Linux/core on Mac. Native
numbering adoption remains pending metadata/fragment capture and owned filter
restoration. The tested TEI direct-filter limitation is documented explicitly.

Owned attribute/heading metadata follow-up: `858ea63` shares exact UTF8HTML
attribute reads, and `e3fcdb2` captures SDK heading fragments at the frozen
cleanup point. Owned metadata/counters reproduce heading-then-body numbering.
Current suites pass 72/72 Mac and 56/56 portable Linux/core on Mac; focused
ASan/UBSan 2/2, C++11 and native-disabled helper checks pass. Reader adoption and
owned filter restoration remain the next boundaries; no native numbering or
whole parity group is accepted.

Latest implementation `c499324` establishes owned SDK note-renderer baselines,
restores scoped values across real module aliases/nesting/exceptions, and copies
opt-in C++ Reader body/heading sources. Current suites remain 72/72 Mac and 56/56
portable Linux/core on Mac; focused note/Reader ASan/UBSan passes 3/3. Automatic
session numbering, saved settings and native controls are still pending. See the
[rolling Mac handoff](handoff-macos.md) for current commands and bounded next work.

Follow-ups `d1175af` / `5da4a7a` share exact saved note-setting decoding, protect
unhandled native target values and avoid an extra marker-free text copy. Full
local suites pass 72/72 Mac and 56/56 portable Linux/core on Mac; focused
ASan/UBSan 2/2 and counter TSan 1/1 pass. Final extra dense/linked coverage
`084fdde` passes normal/sanitized 2/2 each with production source unchanged.
Session implementation `1bd924c` adds opted-in C++ session numbering,
per-module settings and selective persistence with owned Reader fragment metadata.
Full local suites pass 73/73 Mac and 57/57 portable Linux/core on Mac; focused
session ASan/UBSan 3/3, strict C++11 and native-disabled helper checks pass. Dense/
sparse counter ordering, paired-pane resets, complete frozen cleanup comparisons,
SDK restoration, history, fallback, imports/saves and ownership are checked.
Existing worker/C/native consumers remain compatible; their numbering commands,
adapters and controls are the next boundary. Unsupported WORD lexical note markup
still falls back. No whole parity row or release gate is accepted. The owner
requested push and stop; shared ownership is released after this checkpoint.

Latest implementation `1d3612f`, with queued API `aff5f59`, adds native Bible/
commentary note-number controls through shared session policy and copied typed
settings. Mac/core passes 75/75; portable Linux/core passes 59/59 on this Mac;
focused ASan/UBSan passes 5/5 and strict C11 header syntax passes. A disposable Mac
window checked entry On/Off/On, chapter continuation, a numbered-marker note-pane
open and separate commentary Off while Bible numbering stayed On. Current GTK
controls require Fedora runtime checks; Windows remains unvalidated. Unsupported
WORD lexical markup and SDK heading/TEI note identifiers retain their stated
limits. No whole parity group or completion estimate is changed. Shared ownership
is released; the requested control checkpoint is complete.

Latest shared layout `1f899ca` keeps top-level SDK paragraph/heading wrappers
outside inline word spans, including multiple headings per fragment, with frozen
inline comparisons and raw fallback for malformed/nested supported blocks. Local
suites remain 75/75 Mac and 59/59 portable Linux/core on Mac; ASan/UBSan 4/4,
strict C++11 and native-disabled helper checks pass. A disposable Mac window
checked combined headings and note labels with lexical layout enabled. Ordinary
SDK note markers already work in the checked grammar; arbitrary WORD/corpus and
empty heading-note identifiers remain separate follow-ups. No full parity score
changes. The Mac chat continues the bounded heading-note investigation.

Internal source checkpoint `91f83c9` captures heading notes without overwriting
source/body attributes, handles both SDK filter orders, and restores SDK processing
state across injected failures. Full suites pass 76/76 Mac/core and 60/60 portable
Linux/core on Mac; focused ASan/UBSan 3/3, C++11 and native-disabled helper checks
pass. Fragment-local IDs are not yet published in native snapshots; shared scoped
identity/lookup is the next task. No new whole parity or GUI acceptance is claimed.

Scoped identity/lookup checkpoint `7c98ebf` gives each heading fragment a reserved
canonical note namespace and resolves it through configured Reader/session/worker/
C note calls. Suites pass 78/78 Mac/core, 62/62 portable Linux/core on Mac and
ASan/UBSan 5/5; strict C++11 and native-disabled helper checks pass. Existing native
snapshots do not publish these IDs yet; opt-in appearance-preserving snapshot
integration is next. No whole parity or native GUI gate is accepted by this step.

Heading snapshot gate/capability `3411f45` and native adoption `2390568` repair
supported heading-note URIs only when all presentation outside IDs is unchanged.
Raw metadata, known local identity, labels/counters and history are preserved.
Native Mac clicks verified heading/body separation and distinct ThML fragments.
Full suites pass 81/81 Mac/core and 65/65 portable Linux/core on Mac; focused
ASan/UBSan passes 5/5. GTK runtime/Fedora, MSVC/WinUI and broad corpus/visual gates
remain pending. This accepts no whole parity row or revised percentage. Next
shared work is SDK processing-state recovery during render exceptions.

SDK recovery checkpoint `2b756bf` centralizes shared text rendering behind a
C++11 processing-state guard. Healthy output/source-attribute dataflow stays the
same; injected entry/attribute/heading/note failures no longer leave later reads
with attribute processing disabled. Suites pass 82/82 Mac/core and 66/66 portable
Linux/core on Mac; ASan/UBSan 5/5, strict C++11 and native-disabled helper builds
pass. No new full parity/GUI claim is implied. Next is read-only general-book
source characterization and a bounded shared slice.

General-book source/render/transport/paging checkpoints `6f61997`, `6b2399c`,
`b9c0a34`, `f35b6dd`, `b49c823`, `9888bf0`, `045f27e`, `1a19c1d` add copied tree discovery, exact configured SDK HTML/node
identity and queued C ownership. Bounded pages reproduce a 600-child frozen SDK
sequence without frontend navigation logic. Current suites pass **89/89 Mac/core**,
**73/73 portable Linux/core on Mac** and profile **ASan/UBSan 3/3**; prior
books/common parser passed **ASan/UBSan 7/7**;
C11 passes. Prior note/recovery checks passed ASan/UBSan 8/8 plus C++11 and
native-disabled helpers.
Tests cover option precedence/isolation, linked/empty entries, cursor/error guards,
source/render state, handle lifetime/shutdown and unchanged module data bytes.
Shared catalog eligibility excludes unsupported verse-key Generic Books. See
[the book contract](book-tree-workflows.md). Typed ordinary notes reuse common
rendering, with fresh source capture even when its initial flag was disabled.
SDK book-note URI commands now validate exact captured module/path/type while
verse parsing stays strict. Read-only saved selection now matches actual legacy XML getters without
normalizing, applying startup defaults or writing files. Typed startup resolution
is next;
native tree/UI, images/links, history, corpus and full F10 remain pending.
No percentage or whole-parity gate is changed and no hosted builds ran.

Restore-candidate decoder `929c8d8` preserves distinct initial-window offset and
tab/display key inputs, matching the extracted legacy display adapter for defined
prefix/default/path cases. It rejects unsafe overflow/NUL/oversized inputs without
SDK selection or writes. Suites pass 90/90 Mac/core, 74/74 portable Linux/core on
Mac and decoder ASan/UBSan 1/1. SDK resolution/queued ownership is next; native
startup and whole F10 acceptance remain pending.

SDK candidate resolution `eb33f2a` adds read-only relative/root path normalization
and optional queued/C ownership, retaining strict canonical source queries.
Portable Linux/core on Mac passed 75/75. Mac full run was 90/91 with one timing
failure; isolated paging rerun passed 1/1 in 3.6 s. Sanitizers passed 3/3 on isolated
rerun after an initial fixture timeout; C11 passes. These are rerun results, not
clean initial runs. Per-tab selection publication is next; no full parity accepted.

Per-tab publication `0af4593` commits owned source state after successful target
validation/rendering, keeping failed/absent restore and read-only calls neutral.
Suites pass 92/92 Mac/core, 76/76 portable Linux/core on Mac and focused
publication/restore/decoder ASan/UBSan 5/5; C11 passes. This later full run is clean.
Explicit profile-file restore `47fc462` now queues bounded XML import through the
same per-tab publication, preserving source bytes/permissions and prior state on
failure. Latest `106577b` enforces the shared numeric reading minimum of 4 while
preserving explicit root paths and raw source queries. Latest local checks:
**93/93 Mac/core**, **77/77 portable Linux/core on Mac**, focused selection/entry/
profile-restore/session **ASan/UBSan 4/4**, strict C11 and C++17. Native book
controls and complete saved-tab XML/visibility/fallback/write integration remain
pending. No hosted builds or new full parity acceptance. This checkpoint is
handed off near the usage ceiling with no active shared task claim.

Shared navigation `0b81ec5` resolves fresh parent/child/sibling relationships from
committed tab state and reuses configured publication, including legacy numeric
root handling and optional no-op boundaries. Swift transport `aa3b307` copies
book capability, nodes, pages, optional selections and profile restores without
replicating core policy. Local navigation checkpoint: **95/95 Mac/core**,
**78/78 portable Linux/core on Mac**, focused **ASan/UBSan 3/3** and C11 pass.
Native controls are in progress. The usage window refreshed automatically; no
reset credit was used. No new whole parity gate or percentage is accepted.

Native general-book controls `d65aabc` use the shared navigation/selection APIs
in the lower-right dictionary/book area. The model preserves main Bible content,
window isolation, rejected-key reading and stale-result guards; browsing is
read-only. A disposable original-prose Mac window verified child/parent/sibling,
entryless branches, accessible contents actions and rejected-input retention.
Full local Mac/core suite passes **96/96**. F10 now has scoped partial Mac evidence;
**0/38** whole groups and **0/3** releases remain accepted. Native note clicks are
the active next slice; image routing, full saved-tab restore/writes and corpus/
visual/keyboard/VoiceOver acceptance remain pending. No hosted builds ran.

Native ordinary book notes `bb6b2a7` reuse shared exact-source validation, copied
body ownership and the sidebar. Model/adapter checks cover mismatched-command
retention, reading/history neutrality, verse/book preview replacement and stale
note removal; a disposable window confirmed marker/body and navigation cleanup.
Mac/core passes **96/96**, all locally. Active next seam: legacy configured
`DisplayLevel` descendant rendering, characterized against the extracted helper.
No whole parity or completion-percentage increase is accepted.

Configured DisplayLevel source `219d299` matches extracted legacy traversal and
retains owned section identities through empty/linked nodes. Read-only worker/C
transport `799ab02` keeps module/tab options and selection/history neutrality.
Local checks: **98/98 Mac/core**, **80/80 portable Linux/core on Mac**, focused
**ASan/UBSan 4/4**, strict C11 and C++17 decoder. Explicit bounds reject whole
results; no partial document is accepted. Atomic publication and rendered-section
note routing are active next; native multi-section consumption, display cleanup,
resources and full parity remain pending.

Atomic document publication `6cb35cf`, queued/C operations `99056d7` and Mac
consumption `e52a497` now preserve complete configured documents and resolve notes
only to exact rendered sections. Traversal/restore failure retains prior state.
Window checks verified descendant text/separators, descendant note/root separation,
entryless-branch content and bounds failure retention. Full Mac/core **102/102**;
portable Linux/core on Mac **82/82**; focused **ASan/UBSan 6/6** and C11 pass.
Native view extraction `49e4561` then passed app/model builds and six focused checks.
Next: shared document cleanup/annotation layout. Remaining corpus/visual/platform,
resource, generalized-link and saved-tab gates keep whole parity and estimates unchanged.

Shared book presentation `86e4ad0` reuses combined annotation layout and image/font
cleanup while preserving raw SDK documents/sections. Atomic selection carries
matching visible HTML/CSS, discovery states and diagnostics; refresh keeps typed
root intent. Mac `614a81d` consumes the packet and offers native display controls
with draft/history retention. Full local Mac/core **105/105**, portable Linux/core
on Mac **83/83**, focused **ASan/UBSan 7/7**, C11 pass; final compact menu change
then passed app/model builds and nine focused checks. Window controls/spacing/root
refresh/header were checked; actual asset loading and full font/corpus/RTL were
not. Next: image-command/resource-context characterization. Whole parity unchanged.

## Overall completion estimate

Last pre-Linux-catch-up estimate: approximately **8% complete** toward a validated native release on all three
platforms (planning range **6–10%**, not a measured parity score). This counts
modernization work, not functionality inherited from the existing GTK app.
The estimate is deliberately provisional and should change as services and
platform workflows are characterized. Reassess after each accepted milestone.

| Work area | Share of total effort | Estimated work completed | Contribution |
|---|---:|---:|---:|
| Inventory, architecture, build/test foundation | 10% | 40% | 4.0% |
| Shared application services | 35% | 8% | 2.8% |
| Three native frontends and design/workflow adaptation | 30% | 3% | 0.9% |
| User-data compatibility and migration | 15% | 1% | 0.15% |
| Release packaging and comprehensive acceptance | 10% | 0% | 0% |

The weighted contribution is approximately 7.85%, rounded to 8%. The service
estimate now includes owned commentary headings, shared image/font cleanup and
preferences, typed snapshot/state/diagnostic adapters, and initial owned lexical
rendering/styles. These are scoped engineering milestones, not accepted whole
feature rows or proof that the core is complete.

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
| macOS/shared-core slice | 16 partial, 22 pending | 0/38 | Full pane layout, commentary, parallel views, search, bookmarks/annotations, editors, installation, complete settings/data migration, accessibility/translation and release packaging |
| WinUI 3 | 0 initial frontend entries, 38 pending | 0/38 | Native frontend/tooling, shared-core integration, full workflows and validation |
| GTK4 | Scoped existing macOS-preview capability set; see Linux handoff | 0/38 | Manual visual/input/accessibility/corpus validation, accelerated rendering and full workflows beyond both previews |

The existing GTK3 app is retained as the behavioral oracle; its inherited
functionality does not count as implemented parity in a new frontend. Fixture
comparisons and local tests support individual seams, while representative
modules/profiles and legacy/native visual, keyboard and runtime comparisons
remain required. Prioritize the integration checkpoint below, then the main
study-pane composition/commentary workflow before broader native feature claims.

## Current locally checked scope

[Word block characterization](word-block-workflows.md) compares complete frozen/
current functions over 1,638 valid cases, including original Greek/Hebrew SDK
output. It records inherited input-buffer mutation and recursive static-tag defects;
an initial owned/bounded C++11 block renderer now preserves supported layout,
fixes local recursion/input ownership and retains original HTML on unsupported
markup. It now preserves opaque SWORD footnote markers while formatting surrounding
words, checks OSIS/ThML paragraphs and multiple lexical anchors, and safely falls
back on uppercase lexical annotations before unsafe inherited searches. Checked
owned-string annotation edits and scanner state keep legacy spacing and link values
while removing manual temporary allocation/free pairs from the new helper.
Tests add 8,000 noise inputs, 3,039 structured mutations and 400 concurrent renders.
Opt-in session/worker snapshots now stage lexical heading/body layout before cleanup,
carry owned CSS/status and roll unsupported documents back as a whole. The new
private-fixture snapshot test covers chapters, paired history, tab isolation,
save/import and worker lifetime. Typed C metadata and the queued consumer opt-in
now expose the same owned snapshot contract. Swift copies matching HTML/CSS/status
into immutable values, with a new async ownership/fallback check; full macOS suite
68/68 after native view integration (`d953adc`). Main/commentary entry/chapter
views consume the core HTML/CSS together; CSS-only changes reload. A window check
found inherited cross-word font nesting that displaced annotations. The shared
layout now rejects structurally broken output, retains raw content without
annotation CSS, and preserves captured h1–h6 wrappers around formatted words
(`1393edd`). Low-level frozen-oracle behavior remains available. Mac GUI checks
covered positioned Strong's, chapter selection and readable raw fallback.
The Linux adapter (`0b40574`) copies the same metadata; portable/core checks pass
53/53 on this Mac and focused ASan/UBSan checks pass 4/4. Its GTK source has not
been compiled or run here: native Fedora rebuild/desktop checks remain required.
Broader corpus/visual acceptance remains pending. Shared owned lexical CSS
and HTML are now available through one layout decision, including explicit
unsupported-content CSS fallback (48 policy cases and 400 concurrent combined calls).
Worker note/ranged-reference fixtures verify that formatted URLs still resolve
through the existing shared services (normal/ASan/UBSan/TSan focused runs 3/3 each).
The CSS matches frozen selection and three actual legacy call sites over 768 cases,
plus 2,048 concurrent selections. Strict C++11/native-disabled builds pass. Native
broader CSS/parser and pixel acceptance remain pending. Focused normal/address/
undefined/thread checks pass, with sequential scope and SDK instrumentation limits.

The [image/font cleanup slice](html-cleanup-workflows.md) shares owned C++11
cleanup/discovery and preference decoding with legacy rendering. Core sessions/
workers apply per-module settings to Bible entry/chapter bodies and commentary
headings/bodies, retaining copied SDK positions and owned diagnostics. Typed C/Swift
adapters and native Reading/Commentary options carry these results and discovered
states. Unknown eligibility, busy/closed guards, draft/history/window isolation,
explicit save/import and temporary-preview menu states/pane composition are checked.

Evidence includes 27,648 primitive byte/diagnostic comparisons, 200 complete
initializer profile combinations, 60,750 complete cleanup combinations, 36
numbering cases and 144 cases across four local locales. Full local suite: 65/65;
focused address/undefined and thread snapshot/adapter checks: 2/2 each. C++11 and
native-disabled helper builds pass. Image resources/link handling, installed-font
appearance, other pane cleanup, remaining transformations and full GTK/platform/
corpus acceptance remain pending. SDK instrumentation limits still apply.

An initial [preverse heading collector](heading-workflows.md) now agrees with
complete frozen/current collection functions when block/cleanup postprocessors
are disabled/shimmed, including original OSIS and ThML multi-heading/linked data.
Owned core/macOS commentary headings, shared preferences and reference-preview
source context pass local checks. Complete transformations/cache/presentation,
Bible headings and broader SDK/corpus/visual acceptance remain pending.


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
and history. The full suite passes **65/65**; focused address/undefined and thread
adapter checks pass **1/1** each, with the external
SDK instrumentation limits described below. Initial core commentary heading
snapshots, typed C/Swift adapters and native verse/chapter controls use shared
Headings import/save and pass 65/65 local checks, with focused address/undefined
2/2 and thread 2/2. Draft/history/window isolation and a generated-fixture GUI
check pass. Complete heading
transformations, detached windows,
companion/personal editing, saved pane startup and broader corpus/visual acceptance
remain pending. The engineering estimate is now about 8% (range 6–10%);
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

Current local suite: **65 checks**. The shared Unicode click-key utility reuses
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
3. Bring up small equivalent GTK4 and WinUI 3 reading panes before declaring the
   core complete. Exercise module loading, passage/commentary rendering and
   history through the same shared API on each target, with native toolchain and
   interaction checks. Most frontend expansion can follow the core services, but
   this early checkpoint must expose dependency, compiler, threading, ownership
   and embedded-web-view differences before the API hardens. Retain the legacy
   app throughout migration. Windows can host Linux development through WSL2 or
   a VM; see the [development host plan](architecture.md#development-hosts-and-platform-acceptance).
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
validation remains pending. GTK4 now has the scoped Linux implementation documented above; WinUI 3 remains unimplemented.
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
