# Search workflow characterization and seams

Updated 2026-10-17. This is scoped source evidence, not accepted F18–F20 parity.
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
