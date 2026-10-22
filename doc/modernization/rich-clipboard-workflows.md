# Shared rich clipboard search export

Updated 2026-10-21. HTML search Copy can now supply HTML and plain text from one
FIFO job and one resolved retained selection. This is a bounded addition to the
[HTML document service](html-export-workflows.md), not full clipboard/export parity.

## Shared contract

`SearchHtmlExportRequest::plain_text_max_bytes` opts into a companion UTF-8
representation. Absent keeps existing HTML-only behavior; a present limit must
be 1..8 MiB. HTML and plain text have independent final-byte limits, so the
maximum copied pair is 16 MiB. Existing row/image limits still apply. A companion
uses the same title, include-scripture choice, copied module/key order (including
duplicates), generation and partial-source decision as HTML. Selection is resolved
once, on the existing SWORD owner, before either representation is assembled.

The existing shared text formatter is reused for selected text, all retained
text and this companion. Scripture uses configured SDK stripped-text reads on
that same owner; it is not native HTML stripping, a second search or a second
selection. There is no intervening queued job. Reads preserve logical options,
module position, history and search identity under the existing source contract.
Plain text retains the existing text format and source-disposition metadata;
this change does not add advanced headers or image descriptions.

The pair is complete-or-failure. Text exhaustion, source errors or cancellation
discard staged HTML/text, row counts and image statistics together. Existing
HTML resource restrictions still apply even if plain text could succeed alone.
Only successful requested companions have `SearchHtmlExport::plain_text` set.
Progress uses the same job/Stop observer; text reads check cancellation between
rows and before publication, without resetting HTML progress. SDK call duration
and temporary allocations retain their existing limitations.

## C and native adapters

`xc_worker_export_search_rich_html` adds a companion byte limit without changing
existing request/info layouts. Zero means HTML-only; 1..8 MiB enables text.
Existing HTML entry points delegate with zero. The original owned result handle
has additive borrowed text/size getters: absent/failure/null returns NULL/0;
present strings survive worker shutdown until result release. Take-once ownership
and request cancellation remain unchanged.

Swift copies optional text and validates its UTF-8 length before releasing the C
handle. The native search model requests a 1 MiB companion only for Copy. Save
keeps its HTML-only request and atomic file behavior. One native pasteboard item
contains exact core HTML and plain-text bytes; both are prepared before clearing
the destination. No application policy, markup conversion or SDK reads moved to
Swift. Tests use isolated named pasteboards, never the general clipboard.

## Evidence and remaining work

Executed checks and implementation hash are recorded in the [Mac handoff](handoff-macos.md).
Core/C checks cover existing HTML defaults, exact/one-byte-short companion limits,
all-retained and duplicate visible selection, cancellation, partial source,
null/absent getters and owned lifetime. Swift checks compare the exact configured
SDK text, image-plus-text aggregate failure, invalid limits and copied lifetime.
Native checks cover both pasteboard types, failure retention, captured query and
options, stale/Stop/close suppression, and HTML-only Save.

Actual rich-client paste interoperability, browser/CSP/visual output, representative
corpora, advanced headers and GTK/MSVC destination acceptance remain pending. No
whole parity group or legacy retirement gate is accepted. No hosted workflow is
needed for these local checks.
