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

## Current next slice

Build the compatible named-set repository and complete-session persistence
boundary on the checked raw storage contracts. Preserve ordered tokens, unknown
fields and existing name keys; add verified round trips before any profile writes.
Parallel rows and per-tab selection now exist as described below; native command/
presentation consumption and full multi-canon/corpus acceptance remain open.
Bookmark/annotation repositories, editing, installation and sync remain the next
owner priorities. Devotionals stay deferred.

## Complete real-SDK interpolation characterization (2026-10-30)

Complete get_heading/interpolate_parallel_display from `c973bc7` are now frozen
with GPL notices and compared with current extracted complete routines. Actual
generated original-prose OSIS modules use KJV, KJVA and Synodal plus sparse RawCom
notes; two private SDK managers provide independent baseline/current state in the
test process only. 576 full traces compare output, SDK matched keys, callbacks
and selected verse across control order, aliases, missing/duplicate modules,
headings, alternation, realized gates, boundary/invalid input and short/full labels.
Control coordinate readers and frozen complete get_valid_key use real VerseKeys.
Font/number/URL/UI seams remain deterministic; this is not visual/localized UI
acceptance or proof of accurate cross-versification equivalence.

The first actual control determines row count for differing canons. A missing
non-control column emits an unmatched closing cell in retained HTML; raw alias
heading rendering can lose heading text although the resolved body renders.
Explicit regression checks record both bugs without blessing them as native
markup requirements. Future owned snapshots should preserve missing slots and
source identities instead of copying malformed table assembly.

Checks: normal1/1 (3.50s), Mac-hosted portable1/1 (3.50s), ASan/UBSan1/1
(8.78s), TSan1/1 (10.00s); native-disabled C++11 all9/9 (7.40s). Initial
harness const qualifiers were corrected to match retained mutable settings before
these runs passed. No production service or behavior change in this test slice.

## Owned row planning and chapter service

`d5018ac` adds cloned C++11 row planning: 36 valid owned plans match full frozen
interpolation references/selection, with12 invalid SDK inputs rejected. Plans own
canonical references/title/selected verse and preserve live module keys. Count
the normalized control chapter before enumerating up to1024 rows. Bare numeric
inputs follow SDK shorthand, not contextual verse-list parsing: a test assumption
that 24 meant John3:24 was disproved and corrected, not shipped as a new rule.
Callers wanting contextual partial references must resolve them with the existing
reference services before requesting a chapter.

`4bc24ae` adds owned ParallelChapter matrices through Reader/session/FIFO worker
and C ABI. At most32 raw columns retain order/duplicates/empty/missing slots.
The first resolved Bible/regular verse commentary controls the plan; absent or
unsupported control fails without selecting another module. Later missing or
unsupported columns have explicit status and empty cells (NULL C cell data).
Raw tokens, actual installed IDs, alias use, Bible/RTL capability, control row
reference and each SDK matched cell reference remain distinct. Body/headings
use the actual column module, intentionally correcting the characterized alias
heading loss. No malformed retained table markup is copied into native results.

Requests bound module tokens to4096 bytes and reference text to32768 bytes, with
1..16MiB aggregate returned string bytes (caller may request a smaller budget).
The aggregate includes metadata, rows, matched references, headings and bodies;
overflow fails the complete request. SDK internal rendering allocations and
container overhead are not covered by that string bound. More than32 columns
reject explicitly; saved sets must never be silently truncated. Options/headings
are explicit parallel context and restore after rendering; unique module keys
and flags restore on success or failure. Read-only requests do not establish
selection or visit main/dictionary history or change search generation.

## Atomic per-tab selection and navigation

`bcb159e` adds visit/selection/clear and next/previous chapter commands. Each tab
commits a lightweight owned intent only after a complete render succeeds: ordered
raw modules, actual control/reference, explicit parallel options/headings and
byte budget. Read-only previews and failed visits preserve the prior selection.
Chapter navigation reuses the existing control-aware adjacent-chapter policy,
resetting to the destination chapter's first verse. Boundary failures preserve
state. Clearing is independent of main/dictionary/search and sibling tabs.
No parallel history stack or file persistence is claimed.

C chapter/selection results are take-once owned handles surviving request/worker
close. Selection is an envelope with explicit presence. Parallel result storage
is allocated before accepting a state-changing job; no-throw moves publish into
that envelope when polling, avoiding allocation after committed visits. Tests
interpose submitting-thread allocation to prove invalid input allocates no
observer/argument copies and successful committed-result polling allocates none.

The reusable real-SDK oracle host also compares18 native matrices against the
complete frozen renderer's body and matched-reference output. Tests cover32
columns exactly, aliases and heading correction, missing/unsupported/duplicate/
empty columns, aggregate exact/overflow limits, SDK key preservation, history/
options/sibling isolation, queued copies, selection rollback/clear/navigation,
chapter boundary failures, accepted-work draining and C ownership/bounds/nulls.
The disposable fixture explicitly removes only its own modules-conf.cache when
regenerating metadata; stale cache initially hid newly configured abbreviations.
Production module-cache refresh remains part of installation/compatibility work.

Full host regression checkpoint: Mac201 passes+1 explicit index skip (202
registered,170.72s); Mac-hosted portable core/Linux models154 passes+1 skip
(155 registered,140.47s; GTK runtime OFF). Those complete runs preceded the final
normalized-control planning clarification and bare-number assertions; focused
rechecks after those edits cover the final source: normal6/6 (4.89s),
ASan/UBSan6/6 (13.73s),
TSan6/6 (15.99s), portable6/6 (4.88s), strict C++11 all9/9 (7.37s). Strict
C11 public-header syntax and whitespace checks pass. Native importer/WebKit
checks rebuilt/reran in the complete host suite, but no native parallel UI was
added. No current Fedora/MSVC/corpus/visual acceptance or whole-parity increase;
SDK/framework sanitizer coverage partial and ASan leak detection OFF.

### Explicit saved parallel profile repository

The core now reads the bounded legacy `modules/parallel_set_*` subset without
opening an implicit home profile. Startup selection follows the retained order:
configured `parallels`, otherwise the nonempty `int1`–`int5` values, then a nonempty
current named definition. Raw names, duplicates, blank tokens, orphan current
keys and missing/empty definitions remain distinguishable in the owned snapshot.
Storage admits up to1024 listed sets and4096 module tokens, with4096-byte keys
and tokens and a16MiB aggregate copied snapshot. These are import/storage limits;
rendering still has its separate32-column limit. No list is silently truncated.

Explicit save/rename/remove/activate edits use raw stored keys. Display labels
replace underscores with spaces, a lossy mapping; the retained deletion callback
incorrectly compares that label with raw keys. Native callers must keep the raw
identity. New edits reject empty/reserved `names`/`current` keys, invalid XML local
names, unescaped commas and invalid XML text. Save upserts the first effective
scalar definition and adds a label only when unlisted; existing duplicate labels
remain. A missing modules section can be created by an explicit save.

Rename preserves listed order/duplicates and follows the current key, rejecting
any existing destination definition, including an orphan. Only the effective
source definition is renamed; shadowed duplicates remain. Delete removes matching
listed identities using the complete retained callback's empty-token fold, removes
only the first definition in the first modules section and clears matching current
metadata. An unlisted removal is a no-op. Activation writes the current key and a
nonempty definition into `parallels`; missing/empty definitions retain the previous
active list, as in legacy. These operations do not change a reading tab.

Unrelated XML nodes, attributes, comments and shadowed definitions are preserved
semantically; serialization may normalize formatting/encoding to UTF-8. Edits use
the shared verified same-directory replacement helper, prepare the owned result
before replacing the destination and preserve existing permissions. Missing files
can be created only by save. Concurrent observed changes reject; this is not an
interprocess locking protocol. Full session persistence, worker/C transport and
native named-set presentation remain separate work.

The repository now has FIFO worker commands and owned C snapshots. Input paths
are explicit and bounded4096 bytes; tab validity is checked before file access.
C input arrays/flags are bounded before copying, and committed snapshot publication
uses a preallocated result envelope and no-throw move. Accepted writes drain on
worker shutdown even if the observer is dropped. A repository edit leaves tab
selection, histories, search generations and sibling state untouched.

`visit_parallel_profile` explicitly imports either the effective startup list
(NULL key in C) or a first listed raw key. It renders the complete bounded matrix
before replacing one tab's parallel selection. Missing/empty/over32-column lists,
missing control modules and render limits fail without replacing the old selection.
It uses the existing alias, options and missing-column policies, never silently
truncates, and does not edit the selected profile. Storage activation and tab
application are intentionally separate commands; neither implies the other.
