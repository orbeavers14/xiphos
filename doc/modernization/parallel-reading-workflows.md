# Parallel reading: retained contract and core boundary

Updated 2026-10-30. Owner priority shifted away from devotionals to parallel
reading, bookmarks/annotations, editing, module installation, sync, complete
session persistence and broader compatibility testing. Devotional work is deferred;
its checked service remains available without further expansion in this slice.

## Source audit

src/main/parallel_view.cc has two different paths. main_update_parallel_page
renders the selected verse as stacked module rows. main_update_parallel_page_detached
renders a chapter as a table, including when hosted in the parallel tab. Treat
these separately before creating native snapshots or navigation state.

The detached path requires an existing realized target and non-NULL parallel list.
It counts raw non-NULL string slots, including empty strings, duplicate modules
and missing modules. It resolves the first token's abbreviation and looks up that
module for display control. Missing first control warns and returns; it does not
substitute the main Bible or a later available module. A non-NULL empty list
attempts a NULL first-module lookup and warns in the characterized host.

Column widths use integer 100/count (zero count ->100); image analysis uses
integer count*2/3. Headers use reverse abbreviation lookup on the raw selected
token, while the control lookup uses the resolved first token. Heading style,
control CSS, sticky column headers and optional justified margins enter the full
HTML assembly. The selected-verse anchor is read after interpolation updates it.
Raw order/duplicates are retained. The inspected renderer, settings reader and
preferences add/reorder paths do not enforce eight modules; the parity matrix's
up-to-eight wording is not evidence of a current hard cap. Test eight and more
without silently dropping saved selections in future bounded APIs.

The interpolation source separately uses the first control's verse maximum and
normalized book/chapter/verse, then renders each module against that textual
reference. Its own comments acknowledge differing versifications and the
approximate alignment. It uses both the main and legacy parallel backends and
raw/resolved module identities at different seams. This source observation is
not accepted alternate-canon alignment or a tested native service. Preserve the
one SWORD-owning FIFO worker in new code; do not copy the legacy second-backend
architecture into frontends.

## Named-set storage audit

Complete get_parallel_set/save_parallel_set and name_to_key/key_to_name in
src/main/settings.c are characterized. Values live under modules/parallel_set_NAME
as unescaped comma-separated tokens. Order, duplicate tokens and internal/trailing
empty tokens survive splitting. Missing or empty stored values reload as NULL;
saving a non-NULL empty array writes empty text, which reload treats as absent.
Comma-bearing tokens are not lossless. Storage helpers do not themselves validate
installed modules or apply a count cap.

Name display conversion replaces spaces with underscores on save and underscores
with spaces on display. Thus Study Set and Study_Set collide and literal
underscores do not round-trip. Preserve existing raw keys and unrelated settings
when designing a repository; do not silently normalize, rename or rewrite user
profiles. Startup also migrates old int1..int5 selections and may load an active
named set; complete startup/rename/delete callbacks and actual XML transactions
still need their own characterization and round-trip fixtures.

## Implemented first shared seam

Commit `ca3a90a` introduces C++11 parallel_layout, consumed by the retained
detached renderer for width/image-scale arithmetic. It preserves integer
truncation, missing/duplicate slot counting and zero behavior; image-scale
calculation avoids intermediate multiplication overflow. It chooses no module,
performs no SDK/UI work and imposes no hidden module cap.

A GPL-preserving frozen complete detached orchestration function from `5e45425`
is compared to the current extracted complete function and macros across640
traces. Deterministic module/UI/alias/output seams record control choice, raw
labels/count/order, realized/null gates, styling, interpolation arguments, image
scale and post-interpolation anchor. The interpolation routine is stubbed in this
oracle: these checks do not prove verse content or versification alignment.
A second oracle compares the four complete named-set helpers across793 traces/
comparisons with owned XML read/write seams. It characterizes helper behavior,
not the full XML parser, preferences UI or migration workflow.

Actual checks: native-disabled C++11 full libraries build and8/8 tests pass
(3.35s); focused normal2/2 (0.08s), Mac-hosted portable2/2 (0.10s), ASan/UBSan
2/2 (0.16s), TSan2/2 (0.26s). Layout bounds additionally compare integer results
for counts0..256 and exact size_t maximum arithmetic. No full native-suite/GTK
application rerun, actual SDK alignment, C transport, native parallel pane or
whole-feature acceptance is claimed. Existing full198-case Mac/151-case portable
checkpoints predate these two additional tests. SDK/framework sanitizer coverage
remains partial and ASan leak detection OFF.

## Next slice

Characterize complete interpolate_parallel_display with actual SDK-generated
original-prose modules, including alternate canons, sparse/missing entries,
headings, aliases, duplicate columns, missing non-control modules and navigation
boundaries. Then add owned row/column snapshots on the existing FIFO worker with
explicit raw/resolved identities, scoped parallel options and bounded admission.
Preserve the control-module rule and separate shared navigation from frontend
presentation. Named-set repositories and complete session persistence should
reuse verified storage transactions and preserve unknown fields; they are not a
reason to duplicate history or introduce per-frontend storage engines.
