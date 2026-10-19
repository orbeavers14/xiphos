# Search workflow characterization and seams

Updated 2026-10-18. This is scoped source evidence, not accepted F18–F20 parity.
Retain the GTK search interfaces/backend until complete replacements pass gates.

## Existing workflows

The sidebar chooses Bible/commentary, regex/phrase/word, case sensitivity,
unbounded/range/last-search scope. Search text is saved in a 255-byte setting
while the engine receives the complete entry text. Word search selects external
index mode only when the module reports a usable search framework; otherwise
it remains multiword. Indexed sidebar searches prefix + per ASCII-space word
and append a trailing-space workaround, preserving repeated/leading/trailing
spaces and tabs exactly. Query text then passes through the module's strip
filters. Before search, Greek accents, Hebrew vowel points and Arabic vowel
points are disabled in its dedicated legacy manager.

BackEnd::do_module_search clears current results, short-circuits an empty
last-search scope, invokes SDK search with progress callbacks, copies results
to the next last-search scope, and sorts external-index results. The legacy
result iterator copies keys to a 128-byte buffer before advancing. A native
owned result should preserve complete key text with explicit output bounds.
The advanced dialog handles multiple modules, abbreviations, attribute queries,
explicit index fallback messages, and result/preview/export interactions.
Cancellation currently changes the legacy search module's terminateSearch flag.
Native cancellation must respect one engine owner and avoid an unsynchronized
UI write to SDK state. No second search manager belongs in a native frontend.

| Requirement | Source | Native status |
|---|---|---|
| Sidebar mode/case/bounds/last scope | src/main/search_sidebar.cc | Shared policy/modes/case/bounds checked; last-scope characterization checked, implementation pending |
| Backend results/index/scopes/cancel | src/backend/sword_main.cc | Owned results/explicit bounds/cooperative cancel checked; complete scopes/index workflows pending |
| Multi-module/index/attribute/result handling | src/main/search_dialog.cc | Pending |
| Progress/events/native preview/export | src/gtk/search_dialog.c; src/main/search_sidebar.cc | Core progress/cancel checked; native controls/preview/export pending |

## First shared policy

`sidebar_search_text` is C++11, used by the retained sidebar and available to
the native core. It matches a frozen copy of the actual pre-extraction transform
for Unicode/punctuation/space cases and a deterministic 1,000-input byte corpus.
Non-indexed text remains unchanged. Inputs are NUL-free; this helper does not
validate search modes, execute SDK work, tokenize language, open files or manage
history. Removing the old early GString allocation also avoids retaining it on
the sidebar's empty-input return. Focused normal, sanitizer and native-disabled
legacy-library evidence is recorded with the implementation checkpoint.

The initial FIFO execution seam copies bounded results and query context, restores
temporary settings and leaves reading/history/session publication independent.
Bounds on copied output do not bound SDK execution or internal result memory.
Complete last-search, advanced/indexed execution, all module families, corpus/
locale and native workflow acceptance remain gates.

The [official SDK source](https://www.crosswire.org/sword/apiref/swmodule_8cpp_source.html)
helps identify search ownership and backend-dependent case flags, but is an older
revision than the installed 1.9.0.3916 SDK. Validate actual behavior with local
original-prose fixtures; do not infer platform acceptance from that source page.

## Initial owned Reader execution

`Reader::search_module` now executes whole-module Bible and regular-commentary
phrase/regex/word searches. It copies requested/effective query, actual SDK mode,
SDK result count and complete ordered references. Indexed word selection uses
the legacy capability probe and shared AND preparation; external results sort
as the backend does. Actual indexed execution remains unvalidated here. Output
is at most 10,000 copied references/8 MiB, with smaller defaults (1,000/1 MiB).
When a bound is reached, only complete prefix references are returned and the
limited flag is set; the SDK's own execution/internal results are not bounded.
Queries are nonempty UTF-8 without NUL, at most 32 KiB. Optional bounds and
cooperative controls are described below. Last-search, index creation and native
UI remain pending.

The three temporary accent/vowel settings restore per-filter values, including
strip failures. Attribute processing and skip-link flags restore as well. The
checked SDK restores the key value but can replace its internal key object;
SDK pointers must never escape or be held across core operations. Search results
are owned values. Original-prose tests compare modes, case, ordered keys and
queries with an extraction of actual BackEnd::do_module_search, check copied
limits/no matches and unchanged data, and verify settings/recovery.

SWORD's case-insensitive flag depends on its compiled regex backend and is not
exported as a public constant. Native-core CMake exposes
`XIPHOS_SWORD_SEARCH_ICASE_FLAG` (GNU/ICU 2, C++11-regex 1). The local SDK is
checked with 2 and explicit upper/lower phrase-hit expectations. Run core_search
against each actual SDK before accepting case behavior; do not guess another
OS's dependency build settings. The initial failed fixture lacked its RawText
data directories; fixture reads now verify stored text before comparing search
results. This was corrected before source acceptance.

## Queued result transport

The same read-only search now runs through StudySession/StudyWorker and owned C
results. Query/options are copied before return; pending requests expose no
payload, successful payload transfer is once, and borrowed metadata/reference
strings outlive later jobs and worker shutdown until result release. Invalid
mode/case/limits/NULL inputs clear the output; accepted SDK/source failures have
no partial payload. Search leaves reading, book selection and Bible/dictionary
histories independent across tabs. Swift actor adapters copy the same result
without parsing, scanning modules or managing a separate search engine. Native
search controls remain pending; core cancellation/progress is described below.

Current consolidated commands, checked revisions and counts are in the
[Mac handoff](handoff-macos.md). Dependency instrumentation is partial; leak
checks are off. Mac-hosted portable tests do not validate Fedora/Windows.

## Cooperative search control (current implementation)

Each queued search owns an atomic-only control: queued/running/complete/cancelled/
failed phase, monotonic 0–100 progress, and a cancellation request. Reader scopes
mark success/failure, including source validation; worker initialization/unknown
tab failures cannot strand a queued control. Search cancellation is observed
before execution and at SDK progress callbacks. Only callbacks on the engine
owner may change the SDK's non-atomic stop flag; foreign callbacks may update
atomic progress. Result cancelled/executed fields distinguish a skipped search
from interrupted execution. A late completion can win; results remain owned and
state-neutral, with a possibly partial bounded reference prefix.

This is cooperative interruption, not a hard deadline: an SDK backend or a long
individual operation may not issue callbacks promptly. A percent of 100 alone
does not establish successful completion; inspect phase and request status. C
request controls remain available through pending/completed observation, with
explicit owner lifetime/serialization. Swift search task cancellation signals
the core and drains the accepted request before discarding its data; other
accepted operations retain their original cancellation behavior (including
observing created tab IDs). No native search controls are claimed here.

Deterministic controlled-SDK tests isolate cross-thread requests from owner-only
interruption, pre-execution stop, failed initialization/tab lookup, completion
races and successful search after cancellation. Existing whole-module results
and history/source neutrality remain covered. Final counts/checkpoints are in
the rolling handoff; dependency instrumentation is partial, and multiple SDK
regex/index backends, Windows/Fedora, last-search, attributes, full progress
UI/preview/export and corpus/locale workflows still need acceptance.

A controlled foreign-thread callback verifies atomic progress without SDK stop
writes, followed by owner-thread interruption. See the handoff for normal,
address/undefined and thread sanitizer evidence.

## Explicit search bounds

`SearchQuery.range` optionally owns a source module and a UTF-8 bounds expression.
The source module supplies its VerseKey/versification, independently of the
searched Bible or regular commentary. The core clones that key and uses the
legacy `parseVerseList(expression, "", true)` contract. Bounds stay as interval
keys: Gen–Rev is not expanded through the preview's 2,000-verse limit. Expressions
are capped at 4,096 bytes and 64 parsed intervals; SDK empty/unrecognized-range
semantics remain unchanged. These checks do not bound SDK execution time.

The C range request copies both inputs before acceptance. Range getters borrow
from the owned search result, return NULL for unscoped results, and survive worker
shutdown until result release. Swift copies the optional range and otherwise uses
the same cancellation/draining path. A later unscoped search never inherits bounds.
Search does not publish reading selection or history.

Tests extract the actual retained legacy range setup and compare ordered matches
for Bible/commentary targets, different source versifications, disjoint/whole-Bible
and unrecognized bounds. An isolated fixture avoids mutating another test's SDK
catalog. Cases also cover output truncation, pre-execution cancellation, invalid
inputs and C/Swift ownership. ASan found the legacy setup assigning a key from its
own borrowed text buffer; copying that text first fixes the observed use-after-free
in the retained GTK path. The extracted actual routine is exercised with sanitizers;
a full GTK3 build/runtime is still pending on its dependency environment.

Next: implement per-tab last-search scope after the characterization below. It
must retain complete owned SDK
keys independently of the bounded public result prefix, and publish atomically
with an explicit failure/cancellation policy. Do not implement it by reusing the
visible truncated reference array. Native search controls, advanced/multiple-module/
attribute/index management, preview/navigation/export and full corpus remain open.

## Last-search scope characterization (implementation pending)

The range test now extracts the actual `clear_search_list`, `set_range`,
`set_scope2range`, `set_scope2last_search` and backend execution routines.
Original-prose tests establish this sequence on the installed SDK:

| Trigger | Current visible results | Next legacy last-search scope |
|---|---|---|
| Successful unscoped or bounded search | Returned SDK keys | Full copied SDK result keys |
| Search within last results | Matches within previous keys, including another verse module | Newly narrowed full keys |
| Executed search with no matches | Empty | Empty |
| Last-search requested when scope empty | Empty; SDK search skipped | Remains empty |
| Missing module | Empty; error | Previous keys retained |
| SDK throws before returning | Empty; exception | Previous keys retained |
| SDK returns partial keys with stop flag set | Partial keys | Those partial keys replace previous scope |
| Explicit clear | Existing results unaffected by clear itself | Empty |

Repeated narrowing and independent copied lists are checked. The deterministic
partial-return/exception cases inject a test-only SDK module on the engine owner;
they establish backend assignment policy, not actual indexed cancellation latency.
No native last-search request exists yet. Legacy storage is backend-global;
native per-tab ownership is a design boundary that must be explicit and tested.

Sidebar setup clears the selected scope pointer before choosing bounds/last,
clears stored last results for explicit bounds, and uses the main Bible as bounds
source even for a commentary target. Advanced setup uses its first selected verse
module for custom bounds; unbounded mode clears stored keys and mixed non-verse
selections disable custom ranges. The two UIs must not be treated as identical
reset policies. A native atomic command may improve failure publication, but any
deliberate difference from the table needs documented rationale and tests. In
particular, do not silently keep the previous scope on every cancelled execution
and claim that reproduces the legacy partial-return policy.

## Owned result-copy boundary

The production search path uses `copy_search_references` on the engine owner.
Its separate owned value contains the SDK element count, complete copied prefix
strings and a limited flag. Traversal advances the SDK list; no borrowed key or
string escapes. Limits are validated independently at this internal boundary.
This extraction does not create or persist last-search scope.

Dedicated tests retain 300-byte and Unicode keys beyond the legacy iterator's
128-byte buffer, enforce byte limits without splitting UTF-8/reference strings,
and distinguish exact budgets from actual truncation. An interval SDK element
can expand to three returned verses: `total_results` remains the SDK element
count (one), not a guaranteed expanded match count. A frontend must not present
that field as an exact expanded verse total. Empty lists and empty-key failures
are checked, and copied strings survive SDK-list destruction.

## Next implementation boundaries

Keep stored last-search data private to the shared engine/session, not in Swift
or a C result handle. Session ownership must be independent across tabs, with
all SDK key traversal/copy/destruction on the serialized owner. Preserve full key
semantics/versification and interval bounds; reparsing the visible prefix loses
both omitted matches and potentially source-key metadata. Never retain a pointer
into a module's reusable SDK search-result list.

Implement and check in bounded steps:

1. Reader execution returns an internal complete owned scope alongside the bounded
   public result. Stage all allocations/copies before replacing committed scope.
   Define limits for retained scope separately from public display limits; report
   overflow explicitly rather than silently reducing future searches.
2. Session owns the committed scope and selects unbounded, explicit range or last
   scope mutually exclusively. Add independent-tab, zero-match, reset, source-
   module/versification and following-search tests. Keep reading/history untouched.
3. Distinguish a request cancelled before SDK execution from a partial returned
   execution. Match characterized legacy partial publication where applicable;
   document deliberate improvements for failed native result preparation. Test
   successful recovery and late-completion races before exposing the command.
4. Add copied C request/scope metadata and Swift adaptation with take-once,
   shutdown and cancellation tests, then native scope controls. Advanced dialog
   multi-module state has its own reset/aggregation contract and remains separate.

This sequence is a proposed implementation plan. The stored scope and commands
are not implemented or accepted at this checkpoint; no public API is promised by
this design note. Review the actual SDK key representation before choosing the
private storage type, and retain existing unscoped/range requests compatibly.

## Complete private session scope (2026-10-18)

Sessions now retain complete SDK key snapshots independently of displayed result
limits. `SearchQuery.within_last_results` is session-only and mutually exclusive
with explicit bounds. Standalone Reader calls remain unscoped/stateless and
reject this flag. The shared C++ worker transports the copied flag automatically;
C/Swift scope commands and native controls are the next transport step.

A private immutable snapshot clones each SDK key, including type, interval bounds
and versification, before indexed display sorting. Each execution uses its own
scope-key copy. No SDK object travels in a public result. Session copies share only
an immutable snapshot; replacing/clearing one does not replace another's state.
Calls/lifetimes remain serialized on the engine owner, including scope destruction.
Reading, book selection, dictionary state and histories remain independent.

Normal and explicit-bound searches reseed this tab's scope. Subsequent searches
within it use all stored matches even when the earlier public prefix held only
one. Returned zero matches commit a present empty scope; absent/empty last scopes
skip SDK search and cannot fall back to whole-module execution. Pre-execution
cancellation preserves the committed scope. Returned execution, including partial
keys after cooperative cancellation, replaces it as characterized in legacy.
Validation, SDK failure, retention overflow or failed owned output preparation
retain the previous scope. The last case is a deliberate atomic-publication
improvement over legacy clearing visible results before preparation.

`SearchResult.scope` is copied current presence/SDK element count;
`scope_updated` marks replacement by this request. Neither is an expanded verse
total. Explicit `clear_search_scope` resets to absence; it does not navigate.
Private retention is capped at 100,000 SDK elements and 16 MiB of accounted key
text/range-text/versification bytes. These are logical capacity guards, not an
exact heap/time bound. Overflow throws `length_error` without publishing a partial
scope; the current C bridge maps such engine failures to XC_ENGINE_ERROR. Public
reference-prefix limits remain separate. Larger/indexed/module-family acceptance
and restoration/persistence of scopes remain pending.

Tests cover full-prefix independence, cross-module/repeated narrowing, independent
tabs, empty/clear/reset, rejected mixed scopes, source ownership/intervals/Vulg,
pre-cancel and deterministic returned partial/exception/overflow behavior. Initial
focused checks: three Mac, three portable-on-Mac, five ASan/UBSan and three TSan
passed; the added alternate-versification assertion is checked separately before
commit. Dependency instrumentation and existing platform acceptance limits apply.

## Queued scope commands and copied C metadata

Worker `last_search_scope` and `clear_search_scope` run on the existing FIFO
owner. The additive C `xc_worker_search_module_in_last_results` uses the same
copied query/options, cancellation control and take-once result path. Existing
options/result-info layouts stay unchanged. `xc_search_get_scope`,
`xc_search_scope_updated` and `xc_search_within_last_results` expose owned scalar
metadata from the search result. Query/clear requests use non-consuming
`xc_request_search_scope` after successful poll; pending, failed and wrong-kind
requests reject and clear output. Counts are SDK elements, not expanded totals.

Queued tests check truncated-display/full-scope independence, copied mutable input,
separate tabs, clear/empty behavior, wrong-kind/NULL/unknown/closed-tab handling,
FIFO ordering and result/metadata ownership after shutdown. Four focused Mac and
four ASan/UBSan checks pass; strict C11 header compilation passes. C commands are
available to all hosts; current Fedora/MSVC/WinUI validation is still pending.

## Shared search capability

Module catalogs now copy `verse_search` (C synchronous/queued getters and Swift
`hasVerseSearch`). Catalog discovery and actual execution call the same shared
SDK predicate: VerseKey plus Biblical Texts or a regular commentary driver.
Native pickers consume this flag instead of duplicating driver/type rules. This
means initial verse-search eligibility, not indexed/advanced/attribute readiness.
Dictionary and runtime Generic Books remain excluded. The installed SDK reports
a RawGenBook configured with VerseKey as Biblical Texts, so it is eligible for
this search gate while remaining ineligible for indexed book-tree APIs. Source
capability is checked at execution too. Driver names alone do not decide it.

Four focused core/Swift checks and three ASan/UBSan checks pass. The book-tree
fixture separately verifies the SDK runtime classification of VerseKey RawGenBook. Strict
C11 passes. Owned C catalogs retain capability values after worker destruction.
The first capability test compile used incorrect synchronous C function names;
correcting them to the existing xc_open/xc_close API resolved it before checks.

Capability correction: the initial book-tree assertion incorrectly assumed that
RawGenBook plus VerseKey remained runtime Generic Books. It failed because this
SDK reports Biblical Texts. That failing test was pushed before its result was
inspected; the correction tests actual runtime classification and leaves the
already matching shared execution predicate unchanged. The book workflow's older
unverified type description is corrected too. Subsequent checks supersede the
failed assertion; no full module/corpus acceptance is implied by capability.

## Swift progress and interactive Stop

Swift search optionally delivers changed-only `CoreSearchProgress` values with
phase, monotonic percent and cancellation-request state. Callback values are copied
scalars; request handles stay actor-owned and the SDK stays on its C++ owner.
Callbacks may suspend without blocking the UI. Terminal phase can be observed
before payload transfer; request polling remains authoritative for transfer/error.

Existing task-cancellation behavior still drains then throws. Interactive controls
can opt into `returnCancelledResults`: cancelling the Swift task signals the C
request, drains it, and returns the authoritative owned result instead of discarding
partial results. This keeps visible partial matches consistent with the committed
next scope. Pre-execution stop and late successful completion remain distinct.
SDK failures still throw. Two focused core/Swift checks pass, covering copied
progress, changed-only delivery, terminal disposition, unchanged default throwing
behavior, returned cancellation metadata and agreement with queried scope state.

## Initial native search controls (2026-10-18)

The Mac sidebar has native Modules/Search tabs. Search consumes shared capability,
query/mode/case, explicit bounds/last-results scope, copied progress/Stop, complete
scope metadata and bounded results. Its separate presentation model contains no
SDK manager, range parser, query preparation, scope storage or search engine.
Selection previews the searched module through configured history-neutral core
reading; first results preview automatically. Read/double-click uses the existing
main-pane navigation, matching sidebar's module-less verse navigation. Link clicks
reuse shared semantic routing. Clear last resets scope while retaining visible
matches, and closed windows suppress late results. Rapid preview changes invalidate
old presentation without interrupting or dropping accepted core reads.

Two focused native/core model checks pass after refining rapid-preview handling.
Full local suites pass 126/126 Mac/core and 98/98 portable core on Mac; eight
focused ASan/UBSan and four TSan checks pass, with partial SDK instrumentation and
leaks off. The actual SDK case bit remains 2 here. GUI original-prose checks cover
picker eligibility, ordered results/automatic and changed-row preview, explicit
bounds, last-results narrowing, clear then empty search, case-sensitive 1 versus
insensitive 2, searched commentary identity, Read and physical double-click to a
new main reference. The refined short Clear last label fits the sidebar. AX row
activation alone did not synthesize the double-click gesture; coordinate mouse
checks verified it. The generated app/fixture was quit and removed.

The initial sandboxed launch failed in Launch Services; launching the same test
copy outside the shell sandbox succeeded. The fixture copy stayed in temporary
storage, avoiding Documents access prompts. Model/control tests verify Stop and
late-completion handling; actual slow-corpus GUI interruption, full keyboard/
VoiceOver, saved defaults/settings, advanced/attribute/index/multi-module/export,
full visual/corpus and other-platform controls remain gates. This is scoped
functionality, not complete F18/F20 or 1:1 search acceptance.

## Shared attribute query policy

`attribute_search_text` is an owned C++11 policy used by the retained advanced
dialog. Lemma builds `Word//Lemma./`, morphology `Word//Morph/`, and footnotes
`Footnote//body/` before unchanged raw query bytes. Lemma/morphology require SDK
whole-entry matching; footnote bodies retain substring/regex semantics. This
prevents a lemma identifier such as G140 from unintentionally matching G1401.
No trimming, escaping, case handling, UTF-8 interpretation, SDK search or UI state
is implemented in this formatting rule. NUL/unknown kind reject at the owned API.

A frozen verbatim conditional at `0d2e73f` is compared with the extracted actual
retained adapter over explicit long/Unicode/slash/regex/whitespace cases plus 200
byte mutations, every radio combination, case-flag baseline and nonattribute
mode. The original strongs/morphology/footnote precedence and no-selection path
are retained. Normal and ASan/UBSan policy checks and strict C++11 syntax pass.
This extracts policy only; owned attribute execution/transport and complete
advanced-dialog behavior remain the next boundaries. GTK adapter compilation is
characterized with stubs; full GTK3 runtime requires its desktop dependencies.

The real parent-project legacy route also builds CoreRender with native core
absent and strict C++11 enabled. A first standalone configuration's OFF argument
was overridden by its existing unconditional native-ON setting; it still built
only the requested C++11 render target, and the separate parent wrapper verified
the actual native-disabled path. No full GTK desktop build is claimed.

## Owned attribute execution and transport

`SearchMode::lemma`, `morphology`, `footnote` (C modes 3/4/5) now use the same
owned policy, SDK entry-attribute search, proper whole-entry/case flags and exact
module stripText output. Requested raw text stays separate from effective SDK
query. All existing limits, bounds, complete last scope, cancellation and ownership
paths are reused. The current family gate remains runtime Bible/regular-commentary
VerseKey modules; dictionaries/books/advanced module families are not added here.

Original OSIS Bible and commentary fixtures compare all three modes/case settings
with the extracted actual backend. They prove G140 excludes G1401/G1404 yet matches
one component of a multi-lemma word, N-ASF excludes N-ASF1, footnote Alpha has
sensitive/insensitive counts 1/2, and UTF-8 note text works. The cases cover caller
annotation filters On/Off, exact position/processing/link/filter restoration,
source-data byte identity, display-prefix versus full scope, cross-module last
search, explicit bounds, copied mutable C input/options and shutdown lifetime.
Swift adapters copy the same modes and metadata without preparing queries.

Five focused core/Swift checks, four portable-on-Mac and five ASan/UBSan checks
pass; strict C11 passes. Native basic sidebar keeps its original three match
controls; advanced attribute controls, include-annotation checkbox workflows,
multiple modules, explicit index fallback, dictionary families and full corpus/
locale/backend/visual acceptance remain pending. Source compatibility retains
existing options/info layouts; previously unsupported C mode values 3–5 are now
valid and the invalid-enum regression checks moved beyond that range.

## Shared search-text option policy

The retained advanced dialog now uses a C++11 table for the exact ordered names
Strong's Numbers, Morphological Tags and Footnotes with copied checkbox booleans.
Its exact-phrase branch still performs no mutation: it preserves the previous
search-manager state rather than forcing visible checkboxes to apply. A complete
frozen function from `0d0f37a` and extracted actual replacement compare all eight
checkbox combinations and phrase/nonphrase cases, including this sticky-state
transition. Two policy checks pass normal and ASan/UBSan; strict C++11 syntax and
helper-only CoreRender build pass. Native session materialization of that state
is the next step; no native advanced-dialog acceptance is claimed.

## Private per-session search-text context

C++ queries can now supply optional `SearchTextOptions`. A nonphrase explicit
choice updates that session's logical search-only context after successful
preparation; exact phrase ignores changed checkbox input and uses the previous
context. An absent input preserves that context too, matching sidebar reuse of
the legacy search manager. No prior context means the SDK baseline, not an
assumed all-Off setting. Ordinary standalone Reader calls remain stateless.

For each job, the shared owner materializes that context into Strong's Numbers,
Morphological Tags and Footnotes filters, then restores each actual filter/alias
baseline. The context is not a reading/display preference and cannot leak into
another tab or pane. `effective_text_options` records a prepared named override
policy; `text_options_updated` reports a logical-context update independently of
scope replacement and SDK execution. An empty last scope still prepares/commits
nonphrase options before the legacy-style execution short-circuit. Cancellation
before preparation leaves context unchanged; cancellation after preparation may
update it even when SDK execution was skipped. Validation/failed preparation
retain prior logical context atomically, a deliberate improvement over legacy
mutating globals before subsequent failure. Returned partial execution follows
normal prepared-context publication. Clear results does not clear text context;
explicit context reset does not clear results. Nothing is saved to disk here.

Frozen setup plus actual backend comparisons cover all eight flag combinations
followed by phrase searches with opposite checkbox values. A controlled SDK
search directly inspects all three real filter values during execution, including
phrase use of retained values; tests also verify another session's baseline,
empty/clear/reset, pre-cancel, missing source and per-filter restoration after an
injected strip failure. Two final focused normal checks and four ASan/UBSan checks
pass. A missing test-only VerseKey include was corrected before checks. C/Swift
composable request transport is the next step; no frontend search engine is added.
