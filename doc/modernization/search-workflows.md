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
| Sidebar mode/case/bounds/last scope | src/main/search_sidebar.cc | Indexed text policy extracted; service work active |
| Backend results/index/scopes/cancel | src/backend/sword_main.cc | Extraction pending |
| Multi-module/index/attribute/result handling | src/main/search_dialog.cc | Pending |
| Progress/events/native preview/export | src/gtk/search_dialog.c; src/main/search_sidebar.cc | Pending |

## First shared policy

`sidebar_search_text` is C++11, used by the retained sidebar and available to
the native core. It matches a frozen copy of the actual pre-extraction transform
for Unicode/punctuation/space cases and a deterministic 1,000-input byte corpus.
Non-indexed text remains unchanged. Inputs are NUL-free; this helper does not
validate search modes, execute SDK work, tokenize language, open files or manage
history. Removing the old early GString allocation also avoids retaining it on
the sidebar's empty-input return. Focused normal, sanitizer and native-disabled
legacy-library evidence is recorded with the implementation checkpoint.

The first execution seam should copy bounded results under the existing FIFO
worker, retain requested/effective query/mode/case context, restore temporary
filter settings, and keep reading/history/session state independent. Result
limits alone do not bound SDK search execution or internal result memory.
Bounds/last-search, complete cancellation/progress, advanced/indexed execution,
all module families, corpus/locale and native workflow acceptance remain gates.

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
Queries are nonempty UTF-8 without NUL, at most 32 KiB. No range/last-search,
active cancellation, progress exposure, index creation or UI is added yet.

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
without parsing, scanning modules or managing a separate search engine. No
native search controls or active cancellation/progress are introduced yet.

At this checkpoint: 120/120 Mac/core, 93/93 portable Linux/core checks on Mac,
three focused ASan/UBSan checks, and strict C11 pass locally. Three focused
core/Swift adapter checks also pass. SDK dependencies are not fully instrumented;
leak detection is off. Current Fedora/Windows acceptance remains pending.

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

Control checkpoint evidence: 121/121 Mac/core and 94/94 portable core on Mac
passed before the final owner-thread callback guard. After that guard, four
focused core/Swift checks, four ASan/UBSan and two TSan checks passed. A controlled
foreign-thread SDK callback verified it can report progress but cannot set the
stop flag; the subsequent owner callback performs interruption. All tests are
local, with SDK dependency instrumentation limits retained.

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

Next: characterize per-tab last-search scope. It must retain complete owned SDK
keys independently of the bounded public result prefix, and publish atomically
with an explicit failure/cancellation policy. Do not implement it by reusing the
visible truncated reference array. Native search controls, advanced/multiple-module/
attribute/index management, preview/navigation/export and full corpus remain open.
