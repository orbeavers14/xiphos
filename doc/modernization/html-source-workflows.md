# Configured HTML source snapshots

Updated 2026-10-20. Initial C++ session/worker source seam for future HTML output,
not an HTML export document or native HTML save workflow. See the
[remaining export plan](search-export-design.md) and [text contract](search-text-export-workflows.md).

## Shared source, separate publication

`read_entry_snapshot` and `read_commentary_entry_snapshot` return owned Entry
values through the existing configured Reader/presentation pipeline. The latter
uses the tab's commentary heading choice; the former retains the body-only path
used by legacy item export. HTML, heading fragments, note source metadata and
matching core lexical/spacing CSS travel together. Unsupported lexical grammar
retains the same raw/spacing-only fallback as live reading.

The snapshot path deliberately does not publish automatically discovered image/
font preferences into the live tab. It resolves current per-module engine and
presentation settings, runs the same renderer/cleanup/numbering, then discards
local learned options. Explicit preferences, histories, book selection, search
scope/generation/context and independent tabs remain unchanged. Existing live
reads still publish discovery exactly as before; both paths share one private
implementation. No second renderer or frontend presentation engine was introduced.

Worker requests copy module/reference inputs and execute on the same FIFO owner.
Accepted snapshots drain on close and copied strings/CSS/metadata outlive the
worker. SDK key positions and internal entry attributes may change as in existing
read-only operations; existing physical filter/label scopes restore their baseline.
External module files are not transactional snapshots.

Entry is a source fragment, not a complete page, sanitized document or bundled
asset tree. Relative images/semantic links/font declarations can remain in module
HTML. Host theme/module assets, isolated CSS across differently configured rows,
aggregate output caps, document sections/escaping, HTML transport/save, printing
and corpus acceptance remain next gates. No source-size/SDK-time bound is added
by this preliminary API.

## ThML SDK text compatibility correction

The first source test exposed a real flaw in the earlier strict query/scripture
buffer guard: this SWORD build's ThMLPlain filter reports one terminal NUL inside
SWBuf::size(). A plain `Original` query reports nine bytes, including that storage
terminator; OSIS reports eight content bytes. Legacy C-string calls used eight.
The earlier guard incorrectly rejected normal ThML queries and scripture export.

One private shared SDK text copier now excludes exactly one counted terminal NUL,
then checks the full remaining declared content and byte cap. Embedded NUL/hidden
tails, additional NULs and malformed UTF-8 still reject. Exact 64 KiB prepared text
and stripped-scripture caps count content bytes, not a counted terminator. No
replacement characters, implicit string truncation or search rerun is used.

Pure sized-buffer cases cover empty/Unicode/exact-cap, counted terminators,
one-byte-short budgets, interior/double NUL and invalid/truncated Unicode. Real
original ThML fixtures now execute a query with exact effective text and export
scripture successfully. This corrects the compatibility error before HTML work
advances; the previous failing source test is not a passing checkpoint.

## Evidence

Original lexical/note fixtures compare snapshot output with live configured reads,
while checking the deliberate discovery-publication difference, independent SDK
filter baseline, source CSS/notes/fallback, retained keys/context/history neutrality,
missing-module failure and worker lifetime. Existing frozen note/lexical/spacing/
option tests remain the independent rendering oracles; snapshot/live equality
alone is not full rendering parity.

Focused ASan/UBSan and TSan suites pass 10/10 each, including fixture setup and
scoped-option/spacing regressions. The first ASan invocation missed a prerequisite
fixture executable: three dependent tests did not run; building that prerequisite
and rerunning passed. Dependencies remain partly instrumented and ASan leaks off.
Full normal Mac passes 150 + 1 explicit index skip (151 registered), 127.53 s,
with desktop access for the isolated clipboard test. Portable core/models on Mac pass 115 + 1 index skip (116 registered), 86.92 s,
GTK runtime OFF; full environment evidence belongs in the Mac handoff. No new native
HTML, Fedora/MSVC, whole parity group, hosted build or retirement acceptance.

## Bounded body consumer

`capture_entry_html` returns a smaller owned body-only `EntryHtmlSource`: module,
normalized reference, HTML, matching core CSS, layout flags and accounted copied
bytes. Its positive limit defaults to 1 MiB and cannot exceed 8 MiB. The sum of
returned string bytes must fit exactly; one-byte-short output fails without a
partial source or learned preference publication. Inputs require nonempty
NUL-free UTF-8 module/reference identifiers up to 4096 bytes.

The bounded Reader branch copies the sized returned SDK HTML buffer through the
same terminal-NUL/UTF-8 guard before any C-string truncation. It also guards that
intermediate rendered body, then shares existing presentation without publishing
learned choices. Live/legacy entry paths retain their existing behavior. Final
HTML/CSS metadata is validated and counted before returning the compact source.
Raw SDK note metadata/headings are not exported by this body-only API. SDK/parser/
presentation temporary allocations and render time are not total heap/time bounds.

Seven focused checks pass on normal Mac, portable-on-Mac (GTK OFF), ASan/UBSan and
TSan respectively, covering source, shared SDK copier, scoped-option regression
and prerequisite fixtures. New cases establish exact aggregate bytes, one-byte-
short and early SDK-body failure, invalid bounds/UTF-8/NUL/oversized reference,
unchanged preferences/filter baseline/search keys, and queued close/lifetime.
Existing uncapped Entry snapshot methods remain available; this bounded source is
the intended consumer for document assembly. HTML document/CSS isolation and
assets remain explicit next work, with no new whole feature acceptance.
