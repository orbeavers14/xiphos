# Search workflows and shared contracts

Updated 2026-10-18. This is scoped F18–F20 implementation evidence, not accepted
1:1 search parity. Keep the GTK3 interfaces/backend and their licenses. Current
commands, environment, counts and next work live in the [Mac handoff](handoff-macos.md)
and [shared handoff](handoff.md). This consolidated contract replaces superseded
“implementation pending” passages; the full chronological evidence is retained
[in Git at cf80541](https://github.com/orbeavers14/xiphos/blob/cf80541/doc/modernization/search-workflows.md).

## Source workflows and seams

| Workflow | Retained source | Shared/native status |
|---|---|---|
| Sidebar mode/case/bounds/last | src/main/search_sidebar.cc | Shared execution and basic Mac controls checked; full saved/corpus acceptance pending |
| SDK results/index/scopes/cancel | src/backend/sword_main.cc | Shared capability, owned results/scope and cooperative control checked; actual indexed execution pending |
| Attribute/include options/advanced word dialects | src/main/search_dialog.cc | Shared C++11 policy reused by GTK, owned C/C++/Swift execution and Mac single-module advanced mode/annotation controls checked; multi-module controls pending |
| Ordered module search | src/main/search_dialog.cc | Bounded shared worker/C/Swift service checked; module abbreviations/mixed families and dialog setup pending |
| Progress/preview/navigation/export | src/gtk/search_dialog.c; src/main/search_sidebar.cc; src/main/search_dialog.cc | Basic Mac progress/Stop/preview/navigation checked; advanced grouping/export pending |

Sidebar settings retain a 255-byte search-text field while execution receives the
complete entry text. Its bounds provider is the main Bible even for commentary.
Advanced custom bounds use the first selected verse module; selecting non-verse
modules disables custom range setup. Advanced unbounded/bounds setup clears stored
keys before execution, while last-results setup reuses them. The new atomic core
preserves prior committed state on failed preparation rather than reproducing
that destructive setup order. This deliberate difference is tested; complete
legacy saved setup/restoration is still pending.

Advanced execution visits selected modules in order, resolves abbreviations and
warns/skips missing modules. Each returned SDK search replaces stored last keys
before indexed display sorting. Thus last-results searches chain across modules;
the next scope is the final successful module's result, not a union. Complete
module-selection/range setup and supported dictionary/book families remain gates.
Shared item formatting now serves the retained export helpers; complete export/
bookmark orchestration remains outside the core. Actual range/export/bookmark
routines are characterized, including first-result/header omission and destructive
bookmark-list traversal. See [export evidence](export-workflows.md) and the
[proposed retained-result/export slice](search-export-design.md).

## One owner, owned values and module capability

One `StudyWorker` owns Reader/sessions and all SDK lifetimes on a FIFO engine
thread. SDK key creation, cloning, traversal and destruction stay there. A tab
owns private search scope and annotation context independently of reading/book
selection and Bible/dictionary histories. Standalone Reader searches are
stateless and reject session-only last scope. Native adapters never create a
second SDK manager, parse search ranges or maintain another search engine.

Catalog `verse_search` and execution share the SDK predicate: VerseKey plus
runtime Biblical Texts or a regular commentary driver. C catalogs and Swift
`hasVerseSearch` copy it. Dictionary/runtime Generic Books remain excluded.
SWORD 1.9.0.3916 reports RawGenBook configured with VerseKey as Biblical Texts;
that is eligible here while ineligible for indexed book-tree APIs. Driver names
alone are insufficient. This gate does not establish every module's search/index
or attribute capability. Full family/corpus validation remains pending.

Queries own raw module/text, mode/case, optional bounds/last flag and optional
annotation policy. Search text is nonempty NUL-free UTF-8, at most 32 KiB.
Requested text remains distinct from exact copied module `stripText` output.
Results own ordered complete reference strings, selected SDK type, SDK element
count, limited/cancelled/executed flags and copied scope/context metadata. SDK
search can replace its internal key object while restoring position; no borrowed
SDK pointer may survive an operation.

## Modes, shared policy and case behavior

| C/C++/Swift value | Meaning | Preparation / SDK selection |
|---|---|---|
| 0 | Regular expression | SDK regex |
| 1 | Exact phrase | SDK phrase; retains prior annotation context |
| 2 | Sidebar words | Automatic indexed selection, otherwise SDK multiword |
| 3 | Lemma | `Word//Lemma./` + raw text, entry-attribute + whole-entry flag |
| 4 | Morphology | `Word//Morph/` + raw text, entry-attribute + whole-entry flag |
| 5 | Footnote body | `Footnote//body/` + raw text, entry-attribute without whole-entry flag |
| 6 | Advanced multiword | Always SDK multiword; no framework probe |
| 7 | Advanced indexed | Raw advanced syntax; fallback to multiword with `index_fallback` |

`sidebar_search_text` prefixes + per ASCII-space word and appends the legacy
trailing-space workaround only for indexed sidebar words. Repeated/leading/trailing
spaces, tabs, punctuation and Unicode bytes remain exact; unindexed input is
unchanged. Advanced indexed syntax receives neither sidebar prefixes nor its
trailing-space hack. Attribute policy preserves bytes without trimming/escaping;
whole-entry flags prevent G140 from matching G1401. Query stripping remains SDK
work under the effective search context.

C++11 CoreRender policy serves both retained GTK and the native engine. Frozen
pre-extraction conditionals/functions plus configure-time actual-source extraction
compare sidebar text (including 1,000 deterministic byte cases), all attribute radio
combinations/precedence, case flags and annotation-setting names/order. Actual GTK
adapter paths are compiled with stubs, not claimed as a full desktop GTK3 build.
Strict C++11, the actual parent helper route and genuine standalone native-OFF
helper builds are checked. Standalone `XIPHOS_BUILD_NATIVE_CORE` now honors explicit
OFF instead of overwriting it; default native ON and parent defaults are preserved.

The exact shared indexed capability probe is `hasSearchFramework` followed by
`isSearchOptimallySupported("God", -4, 0, nullptr)`. It runs inside scoped effective
filters and module-flag restoration. Controlled SDK tests inspect those arguments,
filter values, automatic AND versus raw OR dialects, forced unindexed mode,
missing/unsupported framework fallback and throwing probes. These tests do **not**
validate actual CLucene/index execution or index creation/maintenance.

Configure `XIPHOS_SWORD_SEARCH_ICASE_FLAG` for the actual SDK regex backend:
GNU/ICU-style 2 is checked here; C++11-regex uses 1. SWORD does not export that
build choice. Each target SDK needs explicit upper/lower hit assertions; successful
compilation is insufficient. Words cancelled before preparation report multiword
fallback because no capability probe ran. Check `executed` before treating a
selected type as a backend that actually executed.

## Bounds, output and complete private scope

Bounds own a provider module and NUL-free UTF-8 expression up to 4,096 bytes/64
parsed SDK intervals. A cloned provider VerseKey supplies versification using
`parseVerseList(expression, "", true)` independently of searched module. Keys stay
intervals; Gen–Rev does not pass through the preview's 2,000-verse expansion cap.
SDK empty/unrecognized-range semantics remain. Later unbounded requests do not
inherit bounds; explicit bounds and last scope are mutually exclusive.

Public reference output defaults to 1,000 keys/1 MiB, maxima 10,000/8 MiB. Only a
complete ordered prefix is copied: UTF-8/key bytes are never split. `limited`
requires actual omitted data; filling a budget exactly is not itself truncation.
SDK interval elements may expand into several copied verses. `total_results` and
scope counts are SDK element counts, not exact expanded verse totals. Long keys
beyond the old iterator's 128-byte buffer, byte boundaries, empty/malformed keys
and lifetime after SDK list destruction are tested. These guards bound copied
output, not SDK execution time or internal results/allocations.

Private immutable scope clones every complete SDK key with type, interval bounds
and versification **before** indexed display sorting. Later searches clone the
input keys; no module reusable-result pointer is retained. Scope never derives
from a truncated displayed prefix. Session copies may share an immutable old
snapshot, but replacing/clearing a session does not change another's state.

| Disposition | Committed next scope |
|---|---|
| Returned ordinary/range/last execution | All returned SDK keys, regardless of display cap |
| Executed zero-match search | Present empty scope |
| Last requested with absent/empty scope | Skip SDK; retain absent/empty scope |
| Cancel before SDK execution | Previous scope retained |
| SDK returns partial keys after cancellation | Those complete returned keys replace previous scope |
| Validation/SDK/retention/owned preparation failure | Previous scope retained |
| Explicit clear | Absent; visible matches/history remain independent |

Scope replacement stages all owned preparation before publication. Retention caps
are 100,000 SDK elements and 16 MiB accounted key/range-text/versification bytes,
logical guards rather than an exact heap/time cap. Overflow throws `length_error`
(current C mapping: XC_ENGINE_ERROR); no silently shortened next scope. Empty SDK
key text rejects even when a batch has no remaining output budget. Scope presence,
count and `scope_updated` are copied scalars. Persistence/restoration is pending.

## Private annotation context and restoration

Optional `SearchTextOptions` covers Strong's Numbers, Morphological Tags and
Footnotes using the exact retained ordered names. Explicit nonphrase choices
update a tab's logical search context after successful preparation. Phrase ignores
changed checkbox input and reuses prior context. Absent input also preserves
context. No prior context means SDK baseline, not assumed all-Off. Stateless
Reader calls cannot retain a prior logical context.

Each job temporarily applies that context plus Greek Accents/Hebrew Vowel Points/
Arabic Vowel Points Off, then restores every actual filter/alias baseline. Module
attribute-processing/skip-link flags and stop state restore too, including errors.
Logical context is distinct from reading preferences and cannot leak to another
tab. `effective_text_options` describes the prepared named override policy, not a
promise the SDK executed. `text_options_updated` is independent of scope/execution.

Empty last scope still prepares/commits nonphrase options. Cancellation before
preparation preserves context; cancellation after preparation can update it even
without SDK execution. Returned partial execution publishes prepared context.
Validation/failed preparation retain prior context atomically, improving on legacy
global mutation before subsequent failure. Scope clear does not reset context;
context reset does not clear scope. Neither writes a settings file here.

Original OSIS Bible/commentary fixtures compare all three attribute modes/cases
and all eight annotation choices followed by changed-checkbox phrase searches
against actual extracted backend/setup. They prove exact G140 versus G1401/G1404,
component lemma matching, N-ASF versus N-ASF1, note Alpha counts 1/2 and café text.
Tests inspect actual filter values during search/probe, source-data identity,
position/flags/alias restoration, injected failure, tabs, bounds, scope and lifetime.

## Cooperative control and copied transport

Each job owns atomic queued/running/complete/cancelled/failed phase, monotonic
0–100 progress and cancellation request. Initialization/tab/source failures cannot
strand queued controls. Only SDK callbacks on the engine owner write non-atomic
`terminateSearch`; foreign callbacks may update atomic progress only. Cancellation
is observed before execution and at callbacks, with no hard deadline/latency bound.
Late completion may win. Percent 100 alone is not success: inspect phase/poll and
authoritative result flags. Callback/exception/pre-cancel/recovery cases are tested.

`xc_search_query` composes copied raw input, base options, all/bounds/last scope and
strict annotation booleans. Initialize defaults; bounds require both range inputs
and other scopes reject stray range fields. Existing C functions/layouts remain
compatible and use the shared input factory. Results transfer once after OK poll.
Strings borrow their owned result through release and survive worker shutdown.
Scope/context query/reset requests expose repeatable copied metadata, rejecting
pending/failed/wrong-kind/NULL access and clearing outputs.

Swift copies Sendable values; raw handles remain actor-owned. Changed-only progress
callbacks carry copied scalars. Default task cancellation signals/drains then throws
CancellationError. Interactive `returnCancelledResults` drains and returns the
stopped/late-completed owned result, preserving its relation to committed next
scope/context. Terminal progress can precede C payload transfer: poll remains
required. Engine publication can precede later C/Swift presentation allocation
failure; re-query scope/context to resynchronize. Release of an observer does not
cancel accepted work; worker shutdown drains it.

## Ordered bounded multi-module service

C++ session/worker batch accepts 1–32 explicit queries; C/Swift multi-module forms
apply one common query to 1–32 ordered module IDs. Order and duplicates remain.
One FIFO job/control covers all groups. Last scope chains through each successful
group; final scope belongs to the final execution, never an output union. All
query shapes/UTF-8/modes/limits validate before execution. Missing providers/modules
and recoverable module errors are owned statuses at original indices; later
requests continue. OK overall poll can contain failed groups: inspect statuses.

Per-module output defaults remain 1,000/1 MiB. Whole-batch defaults/maxima are
10,000/8 MiB. Later modules still execute and retain complete scopes when the
copied display budget fills. Only private batch execution permits zero remaining
output; public single-search zero limits still reject. Scope/text context stage
privately across the batch; completed owned output publishes once. Recoverable
group failures retain preceding successful stage. Fatal input/allocation failure
publishes no batch state. Returned cancellation commits the completed/SDK-partial
prefix; pre-cancel preserves state. Requested modules retain all inputs, while
groups contain the attempted prefix. Equal module progress slices are not an ETA.

C `xc_worker_search_modules` requires query.module NULL; its array supplies IDs.
Options set per-module limits; optional total limits span all groups. Input is
copied before return. Take batch once; each successful group's const `xc_search`
is borrowed until batch release and must never be released separately. Existing
const search getters apply. Swift copies all groups before releasing batch;
bounded CString scopes never cross await and add no scope/context algorithm.

Tests compare ordered typed chaining with the actual backend, duplicates/all 32
requests, total count/byte limits and continued execution after output exhaustion,
missing modules/providers, whole-input rejection, context/history/tab neutrality,
pre-cancel/reuse, mapped callback cancellation/restoration, FIFO, mutated buffers,
wrong-kind/NULL/take-once/borrowed lifetime, both Swift cancellation modes and
shutdown. Batch API is not complete advanced module selection or UI acceptance.

## Native controls and remaining acceptance

Mac sidebar Modules/Search tabs use shared capability and basic phrase/words/regex,
case, all/bounds/last, Search/Stop/clear, progress and bounded result list. First
and changed selections preview the searched module through history-neutral shared
reading. Read/physical double-click uses existing current-main-module navigation,
as the sidebar's module-less verse link does. Semantic links reuse core routing.
Clear last retains visible matches/preview. Stale preview/close guards retain
accepted reads while suppressing late presentation; the list remains selectable
while preview loads so double-click works.

At `0ca0bab`, original-prose window checks verified eligibility, ordered automatic/
changed preview, bounds, last narrowing, clear then empty, case 1/2, commentary
identity, Read and physical double-click to another verse. AX row activation alone
did not synthesize double-click; physical coordinates verified it. The generated
temporary app/fixture was quit/removed. Launch Services initially rejected the
sandboxed launch; the same temporary copy launched outside that shell sandbox.
No Documents prompt was required. Model/control tests cover Stop; actual slow-
corpus GUI interruption, full keyboard/VoiceOver and representative visual parity
remain open. Advanced modes/annotation controls/grouping and export are still UI
gates despite available shared services.

Other gates: actual indexed execution/maintenance, mixed dictionary/book families,
module abbreviation and custom-range setup, saved defaults/history/settings,
full locales/versifications/corpus, SDK backends, GTK4/MSVC/WinUI/native packaging.
No whole parity group or source-retirement gate is accepted. Local sanitizer
instrumentation is partial and leaks are off; portable suites on Mac are not
current Fedora desktop evidence. See handoffs for exact counts and revisions.

## Retained result groups (2026-10-19)

C++ session/worker generations now retain complete ordered result groups separately
from last scope, with aggregate limits, explicit unavailable capability and stale-
safe copied reads. See [the current retention contract](search-retention-workflows.md).
Initial C/Swift generation transport and complete export remain the next boundaries;
existing search APIs keep working. No additional whole parity group is accepted.

## Actual index diagnostic and required gate

`core_search_index` copies only the generated original-prose fixture to a separate
index directory, checks its resolved SDK data path and creates a text-only index
there. Source bytes remain unchanged. If the SDK supplies a usable framework,
the test exercises actual raw OR/sidebar AND execution, ordered prefix versus
complete scope, explicit bounds and last scope. This executed branch remains
unvalidated here: the installed SDK returned creation status 0 but no usable
framework. Normal and ASan/UBSan runs explicitly skip (return 77), not pass index
acceptance. No cause is inferred from a success-looking creation status alone.

Set `-DXIPHOS_REQUIRE_INDEXED_SEARCH_TEST=ON` when accepting an indexed target SDK.
That adds `--require-index`; unavailable framework then fails rather than skips.
The required configuration argument and expected failing local negative control
are checked. A passing required run is still needed on a capable SDK, followed by
annotation/index creation options, cancellation, module-family/corpus/backend and
native workflow gates. Default optional skips never certify indexed execution.

## Corrections retained in the evidence record

- The initial fixture lacked RawText directories; reads now verify stored text
  before search comparisons. Corrected before source acceptance.
- ASan found retained range setup assigning a key from its own borrowed text.
  Copying first fixed the observed use-after-free; extracted actual code is tested.
- The first capability assertion wrongly assumed VerseKey RawGenBook runtime type
  remained Generic Books. That failing assertion was prematurely pushed;
  `76895f4` corrects it to observed Biblical Texts and passes normal/sanitized
  checks. Execution logic already matched the SDK. No type guess is accepted.
- The old standalone build overwrote explicit native-OFF. `0d0f37a` fixes it;
  actual parent/default/helper-only configurations were checked independently.
- Swift 6 rejected a batch observer captured across nested helper closures.
  Returning the locally created observer/status resolved isolation without
  unchecked Sendable pointers. An argument-order test typo was fixed too.
  No failed Swift adapter build was published.

History retains intermediate counts, test-only compile corrections, focused
commands and source provenance. Current full checks supersede historical totals;
none proves complete feature parity or a reliable core-only percentage.

## Native advanced single-module controls (2026-10-22)

Implementation `d26aa2d`: Match exposes all eight declared modes. Optional Strong's,
morphology and footnote annotation inputs are copied with mode/query/case/scope
before enqueue. Disabling the override retains core search context. Phrase controls
explain and defer the core's prior-context preservation; no native query rewriting
or engine selection is introduced. Explicit index fallback is reported from copied
result metadata. Existing preview/export/Stop/close handling is reused.

Native preview/model/export builds and focused regressions pass10/10; detailed
commands and limitations are in the Mac handoff. Physical controls, real indexed
execution, multi-module setup, saved preferences and complete corpus/platform
acceptance remain pending. No whole parity group passes.
