# Modernization status and acceptance

Current checkpoint (2026-10-20): shared retained search generations, selected/all
text export and initial restricted passive HTML documents are implemented through
one C++ owner with copied C/Swift values. Native Mac text save/copy at `5d52a11`
consumes the shared services; HTML adapters through `e8a8ec8` have no native HTML
buttons yet. Source failure/cancellation, stale IDs, capacity/output caps,
order/duplicates and state neutrality stay explicit. See [retention](search-retention-workflows.md),
[text export](search-text-export-workflows.md) and [HTML export](html-export-workflows.md).

Shared verse image source bytes and copied C/Swift values are now checked; actual
SDK image commands and typed reading-pane dispatch keep URI/native directory
handling in C++. See [image contract](verse-image-workflows.md). Native reading
image consumption is active next; HTML image embedding remains unsupported.

Full Mac suite at `7e9ff93`: **154 passes + 1 explicit index skip** (155 registered,
131.12 s), with desktop access for the isolated named-pasteboard test. Full portable suite at the same source checkpoint:
**118 core/model passes + 1 skip on Mac** (119 registered, 89.70 s), GTK runtime OFF.
Focused normal/ASan/UBSan/TSan/C11 evidence lives in the contracts and handoff.
SDK dependencies are partly instrumented and ASan leaks off. Current Fedora/MSVC,
actual SDK index execution and full corpus acceptance remain open.

Configured HTML sources reuse live rendering without publishing learned display
choices. The real ThML counted-terminal-NUL convention is handled explicitly.
Restricted HTML documents share text selection, bound final bytes, scope known
core styles and reject unsupported assets/markup with no partial document.
Native HTML destinations, resource/style/semantic-action expansion, complete
advanced headers and browser/CSP/visual acceptance remain pending. The browser
blocked local-file preview by URL policy; no workaround or visual acceptance.

Next bounded work: characterize shared resource/link export policy before native
HTML consumption, then advanced search controls/settings and workflow acceptance.
Keep target-platform validation and other core services in view. No whole parity
group, release or legacy retirement accepted: **0/38 groups, 0/3 release-ready apps**.
Planning estimate remains **about 8% (6–10%)**; no reliable core-only percentage
follows from test counts. No hosted jobs or reset credit used.

## Overall completion estimate

The last weighted engineering estimate remains **about 8% (6–10%)** toward a
validated native release on all three platforms. It predates Linux catch-up and
recent search work, and is not a measured parity score. No reliable updated
core-only percentage is available. Reassess after accepted workflow milestones,
not by test counts, touched files or inherited GTK3 functionality.

The historical weighting was foundation 10%, shared services 35%, native UIs 30%,
data compatibility 15% and release acceptance 10%. Its approximate contributions
were 4.0%, 2.8%, 0.9%, 0.15% and 0%. These within-area estimates are historical,
not fresh measurements of the current source.

## Parity coverage snapshot

The current matrix has 38 feature groups. Mac/shared-core has 21 partial rows,
WinUI 3 has no frontend rows implemented, and GTK4 has 17 partial rows. These
counts overlap, and a partial group can contain substantial pending workflows.
**No group has complete accepted parity; no native app is release-ready.**
The matrix and target handoffs retain the actual criteria and independent evidence.

## Current locally checked scope

| Area | Implemented partial scope | Detailed contract / remaining gates |
|---|---|---|
| Engine and reading | FIFO SDK owner, copied snapshots, navigation/history, chapter/commentary and options | [Architecture](architecture.md); full corpus, visual composition, restore and target toolchains |
| Lexical rendering | Owned word layout/CSS, fallback, shared notes/headings and explicit preferences | [Word layout](word-block-workflows.md), [numbering](note-numbering-workflows.md), [spacing](double-space-workflows.md); arbitrary markup/corpus/visual gates |
| Dictionary and hover | Owned keys/history, Unicode casing, contextual destinations, explicit scoped profile import/save | [Hover](hover-workflows.md); complete families, locales, saved startup and pane workflows |
| General books | Tree/paging/navigation/DisplayLevel, scoped restore, notes, bounded image/style preparation | [Book workflows](book-tree-workflows.md); complete sizing/assets/formats/corpus and saved windows/tabs |
| Search | Typed bounds/last scope, attribute and word dialects, ordered batches, retained generations, progress/cancel | [Search](search-workflows.md), [retention](search-retention-workflows.md); actual indexed execution, mixed families and advanced native controls/settings |
| Export | Shared GTK item formatter, selected/all text, native Mac text save/copy, restricted HTML C++/C/Swift values | [Text](search-text-export-workflows.md), [HTML](html-export-workflows.md); advanced headers, resources/semantic actions, native HTML, passage, printing and visual workflows |
| Resource reuse | Shared private filesystem containment/bounded-copy helper extracted from book service | [Reuse audit](resource-export-design.md); no new search-image support |
| Linux frontend | Earlier Fedora GTK4/WebKitGTK reading/options/dictionary/preview/profile scope | [Fedora handoff](handoff-linux.md); current shared additions need target recheck, input/picker/accessibility/corpus/accelerated renderer gates |
| Windows frontend | Shared source/API available; no native acceptance | [Windows handoff](handoff-windows.md); native MSVC/dependencies and matching reading slice first |

Parallel reading, complete storage/session migration, bookmarks/annotations,
editors, installation/index/cipher management, BibleSync, full menus/accessibility,
translation and release packaging remain substantial work. The existing GTK3 app
and regression sources remain the behavior/design reference.

## Next steps in order

1. Characterize source-module image context and semantic links under the shared
   [resource/export plan](resource-export-design.md). Preserve complete-or-failure
   output; no implicit external fetching or loss of required styles/context.
2. Consume checked shared HTML values through native destinations once the
   intended format contract is ready; finish advanced search headers/controls,
   mixed families, saved choices and representative workflow comparisons.
3. Recheck current shared APIs/GTK consumers on Fedora and establish the small
   native MSVC/WinUI reading slice before hardening the ABI. WSL is Linux evidence.
4. Continue bounded shared services for parallel reading, compatible persistence,
   bookmarks/annotations, editors, installation and sync with consuming frontends.
5. Audit focused related-project components only when useful; finish corpus,
   visual/input/accessibility/translation and release acceptance on all targets.

## Validation evidence and limitations

Current full-suite checkpoints and subsequent focused source checks are recorded
above and in [the Mac handoff](handoff-macos.md). Older exact test counts,
comparisons, dependency assumptions and corrections remain in the linked contracts
and [the versioned prior ledger](https://github.com/orbeavers14/xiphos/blob/7e9ff93/doc/modernization/status.md).
This cleanup removes repeated chronological summaries and stale next-task claims;
it does not erase regression evidence or accept unvalidated functionality.

The local environment is macOS 27.0.1 ARM64, AppleClang 21, Swift 6.4 in Swift 6
mode, SWORD 1.9.0.3916 and GLib 2.86.5. Configured dependencies/deployment are
26.0: the requested macOS 13 minimum is unvalidated. Portable-model checks on Mac
with GTK OFF are not Fedora runtime or Windows/MSVC checks. External SDK/system
libraries are not fully instrumented; ASan leak checks are disabled. The SDK index
creation diagnostic did not produce a usable index; optional checks explicitly
skip and the required-index negative control fails as expected. Neither is an
indexed-search pass.

The browser blocked local-file HTML preview by URL policy. No workaround, visual
HTML acceptance or CSP runtime acceptance is claimed. Native text save/copy tests
used disposable files and an isolated named pasteboard; user clipboard content
was not read or changed. Full permitted-module/profile, visual/design, keyboard,
screen-reader, data migration and platform release gates remain pending.

## Development and CI budget policy

Keep routine development and checks local. `builds`/`native-core` workflows remain
administratively disabled; their YAML still has automatic triggers. `linux-core`
is active but manual-only. Do not enable or dispatch without owner authorization.
The latest remote audit still showed October 15 jobs; these checkpoints started
none. No allowance-reset credit was consumed. Skipped/disabled checks are not
passes, and current Mac evidence cannot certify another OS or minimum deployment.

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

Later reading/search/resource milestones are sequential checked master commits.
The [shared handoff](handoff.md) links current contracts and implementation hashes.
Historic integration/conflict details remain in Git; no source/asset retirement
is accepted by a documentation cleanup.

## Official sign-off and file retirement

A scoped milestone is accepted when its specified implementation, regression
checks, relevant validation and evidence have been reviewed and integrated at a
recorded commit (with a PR when used). The owner
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
4. The removal and acceptance evidence are recorded in the reviewed changeset.

Keep regression tests after features are checked off. Keep current parity
requirements, architecture, supported user documentation and required license/
attribution files. Git history and linked PRs preserve removed implementation
and detailed review history; avoid a growing archive of duplicate work notes.
