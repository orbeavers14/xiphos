# Selected visible search text export

Updated 2026-10-20. Initial checked C++ session/worker service, not a complete
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
before acceptance. No hosted job/reset credit or legacy retirement. Native
C/Swift transport and actual save/copy UI, representative corpus/locales and
Fedora/MSVC runtime validation remain next gates.
