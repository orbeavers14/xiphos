# Selected visible search text export

Updated 2026-10-20. Checked C++ session/worker service and additive C transport, not a complete
native export workflow. See [retention](search-retention-workflows.md),
[shared item formatting](export-workflows.md) and [remaining design](search-export-design.md).

## Selection, ownership and output

`export_search_visible_text` accepts the originating live tab/generation ID and
1–10,000 explicit visible `(group,row)` ordinals. It resolves every ordinal from
the retained generation before reading scripture. Order and duplicates survive;
failed/unattempted/omitted rows reject rather than becoming all-results exports.
Capacity-exceeded complete retention still permits explicit visible-row export.
Scope/context reset does not discard the generation. Stale IDs never use a newer
search. IDs are ephemeral within a live tab; do not persist or move them between
workers/tabs.

The owned UTF-8 payload uses a caller-localizable plain title followed by two
newlines, the checked legacy item template for each selected row, and one final
newline. Item descriptions use legacy advanced export's `KEY MODULE` spelling.
Every selected row, including the first, is emitted. This corrects the
characterized advanced first-row omission. The single title/explicit selection
layout is an initial service contract, not byte-identical complete legacy
advanced headers/group sections. HTML, full retained results, persisted settings,
bookmarks, clipboard, destination writing, native save/print and pagination remain
separate gates. No file or clipboard is touched by this service.

Optional scripture comes from SWORD `stripText` with the tab's current configured
module engine options, not search-only annotation choices or approximate HTML
stripping. Physical engine filters restore with existing scoped options. One FIFO
job prevents application commands interleaving the reads. Externally modified
module files are not a transactional snapshot. Only existing supported Bible and
regular commentary search families are accepted. Reading and dictionary histories,
book selection, search generation, last scope and search context remain unchanged.
SDK internal key positions may move as with existing read-only Reader operations.

Positive output caps default to 1 MiB, ceiling 8 MiB. Title is NUL-free valid UTF-8
up to 32 KiB; selected module/key copies also have an aggregate 8 MiB guard.
Exact item overhead and final document newline are budgeted before scripture is
copied. Scripture checks its sized SDK buffer for byte overflow, invalid UTF-8
and embedded NUL. Output limits bound copied bytes, not SDK temporary allocations,
render duration or total heap capacity.

Stale generation, invalid selection, output limit, cooperative cancellation,
missing module and engine error have explicit dispositions. Failures contain no
partial document and row count zero; an optional failed request-row ordinal gives
context. Current generation metadata retains partial/cancelled search disposition.
Worker requests copy inputs; cancellation uses the existing atomic job control,
with completion allowed to win a late stop. Acceptance precedes owner-side work;
worker destruction drains accepted exports and owned payloads outlive shutdown.

## Evidence and remaining gates

Original-prose OSIS Bible/commentary tests cover ordered duplicate selection,
first-row inclusion, exact/one-byte-short header/item/scripture budgets, zero
partial payloads, failed/omitted/out-of-bounds groups, stale/origin tab, capacity
fallback, configured SDK text, physical filter restoration, state neutrality,
pre-cancel/completion race, invalid input and FIFO/shutdown lifetime.

Local normal export+fixture pass 2/2, ASan/UBSan export/generation/shared formatters
and fixture pass 5/5, TSan export/generation+fixture pass 3/3. Dependency sanitizer
coverage remains partial; ASan leaks off. Initial compile checks caught SWBuf's
actual return type and a test-only nonexistent shutdown call; both were corrected
before acceptance. No hosted job/reset credit or legacy retirement. Swift transport and actual save/copy UI, representative corpus/locales and
Fedora/MSVC runtime validation remain next gates.

## C11 export transport checkpoint

The additive C request copies bounded row ordinals, title, scripture choice,
generation ID and byte cap before acceptance. A dedicated take-once owned export
handle carries disposition, source generation, failed-row ordinal and the complete
UTF-8 payload/byte count. It survives observer and worker destruction. Failure
handles expose empty text. Existing search progress/cancel controls observe the
export job; the operation never reruns a search or writes a destination.

Original-prose C tests cover input mutation after submission, typed output/selection/
stale failures, invalid/NULL/wrong-kind/pending access, queued invalid UTF-8/unknown
tab, exact owned size, cooperative cancellation, detached observer and shutdown.
Normal export+fixture pass 2/2; C export/generation+fixture pass 3/3 ASan/UBSan and
3/3 TSan. Strict C11/pedantic public-header compilation passes. Dependencies remain
partly instrumented and leaks off. This does not accept a native dialog workflow.

## Swift export adapter checkpoint

Swift 6 accepts copied visible-row values and returns a copied typed disposition,
source metadata, failed ordinal and complete text. It validates negative ordinals,
NUL titles and positive byte limits before conversion to unsigned C values.
The adapter copies all borrowed fields before release and assembles no document.
Default cancelled tasks drain then throw; interactive callers may request the
authoritative cancelled/completed payload disposition and copied progress.

Five focused local checks pass: Swift text-export/generation, C/C++ export and
attribute fixture. Cases cover original text/Unicode title, exact UTF-8 byte cap,
no partial failure document, stale/invalid identity, negative/NUL input,
progress/cancellation and ownership after worker shutdown. Actual native
save/copy, complete-result/HTML and platform/corpus gates remain pending.

## Complete returned text checkpoint (C++ only)

`export_search_all_text` resolves complete retained groups on the same FIFO owner,
using typed post-sort keys independently of displayed prefixes and final scope.
It shares the selected-text document/SDK-reading implementation, preserving module
order, duplicate groups and every returned row. An explicit result selection kind
distinguishes visible selection from all retained output, including failures.

The generation must still match and complete retention must be available.
Default requests reject a source with cancellation, failed modules or unattempted
groups (`source_incomplete`). Explicit `allow_partial_source` accepts the returned
successful groups, with original cancellation/failure/count metadata preserved;
it never claims all requested modules succeeded. Complete-retention capacity
failure is distinct from output limits. No source rerun or visible-prefix fallback.

A positive row cap defaults to 10,000 and cannot exceed it. If any group is clipped,
or more references exist after the aggregate cap, the entire export fails with
empty text. Later empty groups at the exact cap are valid. A successfully empty
source can produce a title-only document. Explicitly accepted all-failed sources
also retain their failure metadata with zero rows. Copied module/key bytes share
an 8 MiB guard; final output has the same independent 8 MiB ceiling. Full retained
SDK key/traversal allocations remain outside those logical output guards.

Original fixtures exercise omitted display groups, duplicates, exact/overflow row
and document caps, shared scripture reading, default rejection/explicit partial
source, all-error/empty groups, stale/capacity/visible fallback, state neutrality,
pre-cancel, copied options and worker shutdown. Normal all/visible/C-export+fixture
pass 4/4. Final ASan/UBSan all+visible+fixture pass 3/3; TSan pass 3/3. Sanitizer
coverage remains partial/leaks off. C/Swift complete-result transport, HTML,
native save/copy/print and representative corpus/platform acceptance are pending.

## C11 complete-result transport checkpoint

An additive all-results descriptor copies generation, plain title, scripture,
positive aggregate rows/bytes and strict partial-source opt-in before acceptance.
It uses the same take-once owned text-export handle and progress/cancel observer.
A separate selection getter returns visible versus all-retained without changing
existing C struct layouts. Capacity-unavailable and incomplete-source dispositions
are distinct from stale/invalid/output limits. Source failure counts remain owned
metadata after partial-source acceptance; no implicit query retry/fallback occurs.

Extended original-fixture C tests recover all keys beyond a one-row prefix,
validate copied title/row policy, clipped output, default partial-source refusal,
explicit acceptance, selection kind, invalid/null options and complete export drain.
Normal C+fixture pass 2/2; C/all-C+++fixture pass 3/3 ASan/UBSan and 3/3 TSan.
Strict C11/pedantic header check passes. Partial dependency instrumentation and
ASan leaks-off limits remain. Native UI/HTML/corpus acceptance is still open.

## Swift complete-result transport checkpoint

Swift 6 now adapts complete-result requests through the same copied-result helper
as visible export. Selection kind and capacity/incomplete-source dispositions are
typed values. No selection expansion, partial-source policy or formatting is
implemented in Swift. Positive row caps/NUL-free titles guard signed-to-C inputs;
default cancellation drains then throws, with interactive authoritative outcomes
available through the existing option.

Six focused checks pass: Swift all/visible/generation, C all/visible export, C++
all export and fixture setup. Tests recover omitted groups/duplicates, establish
exact Unicode bytes and row caps, reject incomplete source by default, accept it
explicitly while keeping failure metadata, distinguish unavailable versus stale,
validate invalid bounds, and preserve copied values after shutdown. Native
save/copy actions, HTML/printing and broad corpus/platform gates remain open.
