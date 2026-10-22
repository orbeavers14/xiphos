# Optional committed search-context export headers

Updated 2026-10-21. The shared C++ header policy now has additive C/Swift options
and a native Mac opt-in. Existing C/Swift/GTK callers keep their current output
because the option defaults off.

## Contract

`SearchTextExportRequest`, `SearchAllTextExportRequest` and
`SearchHtmlExportRequest` append `include_search_context=false`. Opting in inserts
plain-text or escaped passive HTML headers immediately before each run of rows
from a retained search group. The header uses committed request/result metadata,
never a frontend draft or a rerun search. HTML and its requested plain-text
companion share the same copied header strings on the existing owner/FIFO job.

Each header carries one-based originating group index, module, original query,
requested mode, case choice, requested entire-module/range/previous-result scope,
changed effective query, captured search annotation policy, selected backend and
explicit indexed-to-multiword fallback or stopped-group status. It also records
the original SDK result-element count, original visible-reference count and
whether that preview was limited. These are source counts, independent of the
exported row count: duplicate selected rows do not increase them, and an
all-retained export can include groups with no visible preview references. SDK
elements are not a promise about the number of verses in expanded range keys. Named annotation
policy is not a claim that every module implements every SDK filter. With no
captured policy the header reports unchanged SDK state, not invented defaults.
These are provenance labels, not a complete portable saved-search serializer;
previous-result keys and ambient SDK filter values are not reconstructed.

Distinct batch groups remain distinct even when their module names match.
Visible selections preserve caller row order and duplicates; reentering a group
repeats its header. All-retained selection preserves original group/key order.
Headers never consume a result row or omit the first match (unlike the retained
legacy advanced-export orchestration). A header emits only for a group with
selected rows; empty/failed/unattempted groups get no empty header. Source
metadata retains those dispositions. Context-enabled plain text also labels an
incomplete/stopped aggregate source, including accepted sources with no rows;
HTML retains its existing incomplete-source warning.

Headers are copied once per emitting group, shared by internal rows and limited
by existing bounded search requests/batch count. No SDK key traversal, rendering,
search, filter mutation or history visit occurs while building header strings.
Actual module/key rows, output row counts and source metadata keep their existing
meaning. Existing text/scripture formatting and rendering remain unchanged.

All header/warning bytes count against the existing final UTF-8 document limits.
Exact limits succeed; one-byte-short limits return empty failed results. A rich
HTML/text pair still succeeds or fails together. Every caller-supplied metadata
field is escaped for HTML; plain text preserves original UTF-8 bytes. Labels are
currently English, as in the existing initial document policy. Localization,
empty/failure group sections and full advanced/native workflow acceptance remain
open.

## C, Swift and native consumption

Additive `xc_worker_export_search_visible_text_with_context`,
`xc_worker_export_search_all_text_with_context` and
`xc_worker_export_search_rich_html_with_context` accept a strict 0/1 flag. Inputs
copy before queueing; invalid flags clear the observer and return invalid argument.
Existing entry points pass zero to the same implementation. No request/info layout,
owned result, cancellation, lifetime or byte-limit contract changes.

Swift defaults `includeSearchContext` to false and copies the existing complete
payloads. Native Export has an unchecked-by-default Include search details toggle
for both text and HTML, selected and all returned matches. The model captures the
choice before queueing with format/scripture/PNG/selection. Later edits cannot
change the document. Copy uses the same details in HTML and its shared text
companion; Save still requests no companion. There is no frontend header formatter
or metadata lookup, and no new persistence engine.

## Evidence and next slice

Executed checks/hash are in the [Mac handoff](handoff-macos.md). Regression cases
cover default byte preservation, complete retained groups beyond visible caps,
first/duplicate rows and duplicate module groups, reordered visible group reentry,
HTML metacharacters, exact/short text/HTML limits, captured range/case/annotations
after later setting changes, previous-result scope and actual recorded index
fallback. The unavailable real SDK index remains an explicit skip, not acceptance.

C/Swift/native option and destination/model checks now exercise default and
opted-in paths, strict C flags and captured-choice preservation. No new physical
menu/window/visual acceptance is claimed. Do not rebuild historical headers in a
frontend or silently enable the new format for existing callers. Next are richer
empty/failure group sections, localization and remaining resource/semantic policy. Actual corpus,
HTML/CSP/visual, rich-client paste, Fedora/MSVC and whole-parity gates remain.
