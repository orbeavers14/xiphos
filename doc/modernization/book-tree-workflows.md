# General-book tree source seam

The first shared F10 slices are read-only C++17 sources and configured entry
rendering, not a complete book reader. `Reader::book_tree(module, offset)` and `book_tree_path(module, path)`
return copied node data and optional parent, first child, previous sibling and
next sibling. They use the Reader's existing manager and private query keys.
No additional engine, frontend tree algorithm, history or persistence is added.

## Reference behavior and deliberate source contracts

- `src/main/module_dialogs.cc` builds lazy tree choices using SDK child/sibling
  order. `src/backend/sword_main.cc` copies TreeKeyIdx values, selects offsets,
  and exposes local names and relationships. The source API preserves that
  order, the SDK's canonical paths, and its root offset 0. Root's display caption
  is the module ID; its SDK path is empty. A top-level node can have root as its
  parent. Structural presence is separate from the presence of entry text.
- `src/main/navbar_book.cc` uses these same relationships for parent/sibling
  navigation. `src/main/sword.cc::main_display_book` clamps numeric reading
  offsets below 4 to 4. Consequently a tree-root query is distinct from reading
  startup. This slice does not change that existing GTK3 behavior or choose a new
  native reading-input policy. The legacy free-text numeric-prefix ambiguity also
  remains a later compatibility decision: typed offsets and canonical slash
  paths keep source discovery unambiguous.
- Only indexed-tree Generic Books are accepted. No unsafe cast of dictionary,
  verse, or unsupported general-book driver keys is permitted. Offset arguments
  are owned unsigned 64-bit values validated against the signed 32-bit, four-byte
  SDK index representation before conversion, including on Windows LLP64.
- Invalid SDK offsets can leave a stale local title/path. Pending path snaps can
  land on a nearby real node when a requested path is missing. Check SDK errors
  before copying and require exact canonical path identity. Failures never publish
  those misleading nodes. Empty path selects root; `/` is not its canonical path.
  Paths reject embedded NUL, relative input and lengths above 4096 bytes.
- Results own all strings and survive later queries and Reader destruction.
  Successful and rejected queries do not select a module key, render text, change
  entry attributes, create a missing path, or call any SDK edit/save operation.
  One-hop snapshots bound their own work; ancestor validation rejects cycles and
  chains of 256 or more ancestors before SDK path assembly. This is a query limit,
  not comprehensive validation of arbitrary corrupt SDK files.

## Local evidence and remaining work

`Reader::read_book(module, offset, options)` pairs exact raw SDK HTML and entry
presence with the validated source tree. It applies/restores the existing shared
engine-option scope and selects the requested live tree offset without replacing
its key object. It does not re-resolve the title/path, so linked entries retain
their selected alias identity. Root stays root; structural-only branches remain
empty rather than showing nearby prose. Source validation happens before live
selection. The common render boundary restores attribute-processing state on
exceptions; failed renders publish no result. Like ordinary entry reads, a
successful selection remains current. It adds no visit/history or preference
write. Unexpected key/source mutation during rendering rejects the result.

`core_book_entry` generates original OSIS/ThML prose and compares exact configured
HTML against frozen live SDK rendering across note/red-letter toggles, root,
branches, leaves and links. It checks real note visibility/red-letter changes,
source/HTML lifetime, missing/wrong-module/offset rejection, injected render and
source-mutation failures, filter/processing recovery and module-data byte identity.
This is raw SDK output: native book-note URI dispatch, display cleanup and image
resource resolution are not enabled or claimed by this API.

## Serialized native transport

StudyWorker queues tree/path queries on its existing engine thread. Book reads
use StudySession's existing module override, tab override, enabled legacy defaults,
then unconfigured Reader baseline, in that order. No separate book preference
engine or history is introduced. Source queries require no open tab; entry reads
reject closed/unknown tabs. Failed queries do not stop subsequent jobs, and close
drains accepted work.

The additive C API exposes `xc_worker_book_tree`, `xc_worker_book_tree_path` and
`xc_worker_read_book`, with separate take-once tree/entry handles. Strings copy
at submission. Returned entry trees are borrowed from their entry and must not
be released separately. Node getters publish borrowed strings with explicit
`present`: actual root offset 0 remains distinct from an absent relationship.
Null handles/outputs and unknown kinds have deterministic empty/error behavior.
Results outlive their request and worker; normal release functions own destruction.
Existing C layouts/version and all existing consumers remain compatible.

The existing `XC_MODULE_NOT_FOUND` status also represents a missing tree node or
path for these operations. Other malformed arguments map to `XC_INVALID_ARGUMENT`;
unexpected SDK failures map to `XC_ENGINE_ERROR`. Requests retain the common
nonblocking poll contract; worker close still belongs off the UI thread.

`core_book_worker` checks copied arguments, FIFO and tab/module option precedence,
unconfigured/default reads, history neutrality, typed errors/continued work,
take-once/wrong-kind behavior, root/absence distinctions, borrowed/owned lifetime,
initialization failures, shutdown draining and unchanged module data bytes. C11
header checks cover the new API without exposing C++/SWORD types.

## Bounded child enumeration

Reader/StudyWorker `book_children(module, parent, after, limit)` returns an owned
parent and up to 256 direct children in the same firstChild/nextSibling order as
`main_dialogs_add_children_to_tree`. Default page size is 64; valid sizes are
1 through 256. No recursive subtree walk, sorting or UI list indices are used.
The optional `after` cursor identifies the last returned child, and must still
be a direct child of this parent. A single validated lookahead produces
`next_after` only when another child exists. Exact final and empty pages need
no extra continuation. A leaf, root, and structural branch retain their actual
source meaning.

The query validates the cursor without rescanning all preceding siblings. Its
own traversal/copies are bounded by page size plus one lookahead, with bounded
ancestor checks for each node. It detects repeated sibling offsets within the
request, including a cycle back to its starting cursor. Pages do not freeze a
module revision or prove the absence of cycles spanning multiple pages in a
corrupt module. Invalid/deleted/reparented cursors reject; external module edits
between requests require refreshing the tree. As with other SDK queries, these
guards do not claim comprehensive arbitrary-file corruption safety.

The additive C page handle follows the same copied-request/take-once lifetime.
`has_after` is strict 0/1; a false value ignores the supplied offset. Borrowed
parent/child node views remain valid until page release. Bounds/null errors clear
stale node outputs; absence of continuation clears the cursor output. Requests
use the same FIFO worker and shutdown contract, and never change live selection,
entry attributes, history or module data bytes.

`core_book_children` generates a 600-child original-prose branch. Page sizes
1/64/127/256 reproduce the complete frozen SDK sequence without omissions,
duplication or sorting, including numeric/Unicode titles. Tests cover root,
leaf, exact/partial final pages, invalid limits/offsets/parents/cursors, state
preservation, Reader/worker/C lifetime, queued failures and shutdown draining.

## Shared module capability

Module snapshots carry `indexed_book`, exposed as `xc_catalog_has_book_tree` and
`xc_module_has_book_tree`. The shared predicate requires the SDK Generic Books
type and a runtime TreeKeyIdx key, matching the source-query gate. This is a
capability for these tree APIs, not a claim that every book driver or native book
workflow is supported. Frontends can use it rather than guessing from names,
file layout, or platform-specific casts.

An actual SDK `RawGenBook` configured with `KeyType=VerseKey` remains a Generic
Book but is ineligible for these indexed-tree APIs. Fixtures verify that it and
dictionaries are excluded, eligible/empty/deep books are included, unsupported
queries reject, synchronous/worker/C catalog copies agree, index/null getters are
safe, and copied capability metadata survives worker shutdown. No existing C
layout or legacy driver operation is changed.

## Typed ordinary book-note source

Reader/StudySession/StudyWorker `book_note` pairs a copied tree source with the
existing StudyNote value. It selects the validated exact offset, refreshes that
source's attributes and reuses the common UTF8HTML/SDK note-body renderer. It
does not re-resolve a title or feed a book path through a verse parser. Linked
aliases retain their selected offset/path; a later unrelated render cannot
supply stale notes. Missing IDs, empty branches and unsupported module/key types
reject without publishing a result. Unexpected source mutation rejects too.

The common C++11 note helper now has an already-positioned-key entry point used
by both typed books and existing passage wrappers. Attribute capture is enabled
temporarily even when a caller deliberately disabled it, then the original flag
is restored on success/failure. This prevents stale note attributes from another
source. Existing enabled-caller HTML/metadata and passage validation stay intact.
Metadata-only lookup refreshes source metadata without a note-body render.

The worker reuses existing session module/tab/default/unconfigured engine policy,
adds no history, and drains accepted note requests. The additive C book-note
handle owns both its tree and ordinary note; `xc_book_note_tree/content` borrow
views until release, and must not be released separately. `render_body` is strict
0/1. Request polling, error mapping, argument copying and take-once semantics
match the other book handles. No existing C layout or public verse parser changes.

`core_book_note` compares frozen pre-extraction SDK metadata/UTF8HTML/body output
for OSIS/ThML, note/red-letter options, source and linked aliases. Real colored
note bodies verify option precedence rather than merely testing unchanged prose.
It checks disabled-capture stale-state recovery, source/body filter failures,
metadata-only avoidance of a failing body, unexpected source mutation, state and
owned/borrowed lifetime, module data bytes, queued errors and shutdown draining.
Unconfigured SDK baseline is measured separately from explicit all-Off options.

This is ordinary note-source transport. Native hover/click routing,
cross-reference-list presentation, heading-note repair and book display cleanup
remain separate gates; no full note UI parity is claimed.

## Source-bound SDK book-note commands

The shared command decoder now separates common note fields from the existing
verse-reference guard. `book_note_link` requires the complete decoded command
module/path to match its captured tree context. It retains the same local/fixed
origin allowlist, percent/control/duplicate/unknown-parameter checks, action and
note-type checks. Existing verse parsing and short-passage fallback stay separate
and retain their prior rules. Book titles may contain Unicode, spaces, slashes,
and encoded verse delimiters; short book paths never become current verses.
Root requires an explicitly present empty passage parameter.

StudySession/worker/C `book_note_link_at` use the captured module and typed offset,
validate the command against an owned source tree, then reuse ordinary typed note
lookup and effective engine options. The returned source and note type must still
match. They do not retarget another module/node, execute external links, interpret
heading IDs, visit history or implement cross-reference-list presentation. The
result is the existing owned book-note kind, including metadata-only behavior.

`core_book_note_link` exercises real OSIS/ThML SDK markers, linked aliases, later
navigation, short/root/Unicode/reserved-character paths, strict unchanged verse
parsing, malformed/duplicate/unknown/external commands, wrong module/path/type/ID,
queued errors, copied strings, C take-once lifetime and shutdown draining. Native
book windows and hover/click controls still need separate integration/acceptance.

## Read-only saved selection

`parse_saved_book_selection` / `read_saved_book_selection` reuse the existing
bounded, encoding-aware XML profile parser. They copy optional `modules/book`,
`keys/book` and `keys/offset` text using the actual legacy getter's first matching
key and null/empty semantics. Raw strings stay unnormalized, including missing
modules, slash paths, numeric-looking titles, whitespace, numeric prefixes and
out-of-range text. No SDK discovery, numeric conversion, selection, locale change,
preference write or implicit profile-path access occurs.

`settings.c` reads these fields separately: a missing book key causes legacy
startup to add `0`, while a missing offset adds an empty value and uses zero.
`main_display_book` later chooses numeric-prefix vs path behavior, clamps numeric
reading offsets below 4 to 4, and only numeric selection updates `keys/offset`.
Thus an old offset may coexist with a newer path key. The raw import intentionally
preserves the evidence instead of inventing precedence or silently applying
startup writes. Typed startup resolution, per-tab book data and round-trip saving
require later characterization/acceptance; this import alone is not that workflow.

`core_book_profile` compares the new values with the extracted actual legacy XML
getter, including duplicate sections/keys, missing/empty/child-only values,
entities, numeric text and UTF-16. It verifies owned UTF-8 results, unchanged file
bytes/permissions, explicit-path validation, parser size/DTD/NUL guards and
independent concurrent parser contexts. Only original generated profiles are used.

`core_book_tree` generates original-prose RawGenBook fixtures, including empty,
structural-only and deep branches, numeric and Unicode titles. It compares all
one-hop relationships with the frozen SDK sequence used by the legacy backend,
checks typed errors/recovery, live key identity/position, attributes, result
lifetime, and byte-for-byte identity of every module data file and filename
before/after successful and rejected queries. Manager-generated configuration
caches are outside this module-data identity assertion.

## Typed restore candidates

`saved_book_target` makes restore origin explicit. `display_key` follows
`main_display_book`: missing key becomes `0`; only an ASCII digit in the first
byte selects decimal-prefix offset parsing; other strings, including relative
titles, signs, leading spaces and explicit empty paths, remain path candidates.
Numeric reading candidates below 4 become 4. `initial_window` instead follows
`settings.c` / `main_window.c`: decode the separate saved offset's decimal prefix,
then produce the numeric display candidate. Missing/non-numeric offsets become
4, and leading ASCII whitespace or `+` retains defined legacy prefix behavior.
A newer path key may coexist with an older saved offset; the two contexts do
not silently override each other. Missing/empty module produces no book target.

Candidates own their values and do not read the SDK, select a node, render,
publish tab state or write profiles. SDK existence/alignment and relative-path
resolution remain separate. The parser rejects embedded NUL, address strings
above 4096 bytes, offsets outside the signed 32-bit SDK index range, and negative
initial offsets. It avoids platform-dependent `atol` overflow and unsigned/signed
formatting behavior; these rejected cases are not claimed as legacy equivalence.
Explicit raw display key `-4` remains a path, as in the legacy first-byte branch.

`core_book_selection` compiles the extracted actual `main_display_book` against
headless adapters and compares its selected address with candidate decoding.
It also compares the defined settings-offset/format/display startup sequence,
checks conflicting saved fields, defaults, numeric/path distinctions, errors
and lifetime. The GTK3 function and startup writes remain untouched.

## Read-only SDK restore resolution

`Reader::resolve_book_target` resolves the typed candidate using private SDK keys.
Numeric candidates retain the existing indexed-offset checks. Display-path
candidates allow successful SDK normalization of relative paths and `/` root,
then return the SDK's actual canonical path and offset. Missing snaps reject
before stale node data can be published. The strict `book_tree_path` API still
requires exact canonical input and retains its previous behavior.

StudyWorker `resolve_saved_book_selection` copies raw values, decodes the explicit
restore context, and returns an optional owned tree on the single engine thread.
No module is successful absence; invalid modes, modules, addresses and missing
nodes remain typed errors. C submits borrowed optional strings in
`xc_saved_book_selection` and receives the same take-once tree kind. The presence
getter returns -1 for pending/failed/wrong-kind requests, 0 for no book, and 1 for
a resolved tree; it stays stable after transfer. Requests drain on close.

These operations do not render entries, change live selection/attributes/history,
publish a tab selection, auto-choose a module, or read/write any profile. The
saved-tab XML and visibility/fallback rules remain separate startup gates:
`tabbed_browser.c` can replace the effective initial-window offset with a restored
tab's raw `book_offset`, while later tab activation uses that raw value as a
display key. Callers must supply the effective fields for the explicit context;
global XML import alone is not a complete saved-tab restore implementation.

`core_book_restore` compares valid normalized paths with frozen SDK snaps,
including relative, root, numeric-title and Unicode paths. It checks conflicting
saved fields, strict-API compatibility, missing/error rejection, live key and
attribute preservation, complete module-data byte identity, worker/C argument
copies, successful absence, result ownership and shutdown draining.

## Explicit per-tab publication

StudySession `select_book` applies the shared numeric reading rule (offsets below
4 become 4, while explicit path roots remain root), validates the target, renders with existing
module/tab/default/unconfigured engine precedence, and verifies the resulting
source still matches. It stages owned request/source copies before publishing
the tab's committed book selection. A compile-time no-throw move requirement
protects that final publication step. Failed target/render/copy operations cannot
replace the prior committed source. The SDK cursor remains ephemeral; this is
application-state atomicity, not an SDK/file transaction.

`restore_book_selection` uses the same explicit restore contexts. No saved module
is a successful no-op and does not clear an existing selection, matching the
legacy null-module early return. Read-only book/notes/source queries do not
publish state. Book selections are independent per tab and do not add Bible or
dictionary history. The stored request is typed intent, not a guarantee of exact
original numeric-token round-trip. Profile writes and complete saved-tab restore
remain separate work.

Worker/C calls expose selection, restore and copied committed-source snapshots
through the existing FIFO owner. Explicit selection returns the owned book-entry
kind. Optional restore uses presence 0/1 and the entry kind; committed snapshots
use presence 0/1 and the tree kind. Presence stays stable after transfer. Dropping
observation does not cancel an accepted publication; close drains accepted work.
No existing C layouts or platform frontend behavior change.

`core_book_session` verifies actual configured HTML, option precedence, separate
tabs, state-neutral reads/notes, failed/absent restore retention, both restore
contexts, copied request/source lifetime, FIFO and closed-tab behavior, C payload
kinds/presence/take-once ownership and observer-drop/shutdown behavior. Existing
lower-level entry tests cover injected SDK render failures; this session test
directly exercises target/restore failures and successful publication.

## Explicit profile-file restore

StudyWorker/C `restore_book_profile` reads only an explicitly selected XML path
on the existing engine thread, then uses the same restore decoder and successful
per-tab publication operation. It validates tab lifetime before file access and
keeps current module/tab engine preferences. No profile discovery, file writes,
locale/lexicon application, saved-tab XML replacement or window-visibility change
occurs. Optional entry ownership/presence semantics match raw-value restore.

Read/parse/target failures leave the committed selection intact. A profile with
no book is a successful no-op. Input paths copy before submission returns; close
drains accepted work. Existing size, DTD/network, encoding and path guards come
from the shared profile reader.

`core_book_profile_restore` uses original UTF-16 profiles and Unicode filenames,
checks actual configured HTML, both restore contexts, other-tab neutrality,
missing/malformed/DTD/overflow rejection, absent-book retention, C argument/result
ownership and shutdown draining. Successful/failed imports retain source profile
bytes and permissions. No personal profiles or installed content are tested.

## Shared navigation and Swift transport

`StudySession::navigate_book` and queued/C counterparts resolve parent, first
child, previous sibling and next sibling from the committed tab source. They
capture fresh SDK relations, reject changed source identity and publish through
configured selection. Missing selection/relation is successful optional absence,
with no state change. Parent-to-root uses the same legacy numeric reading minimum
of four; explicit root paths and low-level source queries retain root zero.
FIFO commands use the last successfully committed source, including when an
observer was released. No Bible/dictionary history or preference writes occur.

`core_book_navigation` checks SDK order, entryless branches, linked entry source,
configured HTML, boundaries, invalid commands, tab isolation and C ownership/
shutdown. Alias HTML is compared at the alias source: its note links legitimately
carry a different path than the linked entry's original node.

Swift transport copies capability flags, node relationships, child pages,
configured selections and optional restores before releasing C results. Strings
reject embedded NUL; numeric prefix/default/path interpretation remains in C++.
`swift_book_adapter` checks both restore modes, explicit profile import, root
identity, paging, isolation, failure neutrality and copies surviving shutdown.
Mac native controls now route eligible modules into the lower-right book pane,
with core commands and bounded, read-only contents browsing. Direct input uses
the display-key decoder; failed reads retain presentation. Model and disposable
window checks cover this subset. Full visual/corpus/accessibility acceptance,
complete saved-tab restoration, visibility and
round-trip writes remain separate gates.

The next slices include native note clicks and saved-tab/visibility
integration, image/resource routing, links, history/persistent selection,
additional book windows and editors. Preserve the GTK3 app and all its write
paths. No complete F10, corpus, platform or legacy-retirement gate is accepted
by these scoped checks.

## Native ordinary book notes

Mac `bb6b2a7` copies the shared exact-source book-note result into the sidebar.
Book markers and note-body clicks use the book command, with no verse-parser
fallback. Mismatched/unsupported commands retain the previous preview and reading.
A successful new book read clears stale notes; browsing contents retains them.
Book and verse-reference sidebar previews replace each other. Model/adapter tests
and an original-prose disposable window verified marker → correct body and
navigation cleanup. Full Mac/core suite: 96/96; no new core policy or whole parity.

## Configured display-depth source and transport

Legacy `GTKEntryDisp::display` reads `DisplayLevel` (absent defaults to one).
For values above one, actual `_render_display_level` walks selected node and
children depth-first in SDK sibling order, adds `<br/><hr/>` between nonempty
rendered sections, and restores the selected cursor. Empty branches still lead
to descendants; linked entries retain their own tree source. This workflow is
not applied by the single-entry native read yet. Shared source `219d299` and
queued transport `799ab02` now return owned root/section sources and raw combined
HTML. The SDK traversal is iterative; limits are 64 relative levels, 4096 visited
nodes and 8 MiB combined HTML, with smaller limits available to direct C++ readers.
Bound failures reject the whole document. Selected cursor and processing flags
are restored across tested bounds and injected descendant-render failures; module
data bytes remain unchanged. The bounded DisplayLevel decoder preserves defined
atoi prefixes/defaults/signs and rejects overflow/NUL/oversized input.

`core_book_document` compares actual legacy rendering across depths, roots,
entryless branches, linked entries and engine options. `core_book_document_worker`
checks existing module-over-tab options, FIFO policy, read-only selection/history,
borrowed section getters, take-once C ownership and copies surviving shutdown.
At `799ab02`: 98/98 Mac/core, 80/80 portable Linux/core on Mac and focused
ASan/UBSan 4/4; C11 and strict C++17 decoder checks pass. These checks are local.
Atomic publication `6cb35cf` stages the complete owned document before replacing
tab state. Missing restore/navigation is a no-op; a traversal failure retains the
previous document. Raw entry selection invalidates old document context. The
common book parser resolves note sources only among rendered document sections,
rejecting skipped, outside, foreign-module or ambiguous paths. Worker/C `99056d7`
transports these operations, explicit read-only profile restore and optional owned
snapshots on the existing FIFO. No verse fallback or extra engine is introduced.

Mac `e52a497` copies/presents these documents and descendant notes. Window checks
verified combined text/separators, a descendant note with its own path while the
selected root stayed fixed, entryless-branch content and retained reading after
a bounds failure. Full Mac/core: 102/102; portable core: 82/82 on Mac; focused
ASan/UBSan 6/6 and C11 pass. View extraction `49e4561` then passed app/model builds
and six focused checks without changing control logic. Shared display cleanup and
annotation layout are the next seam; raw SDK sources remain available.

## Shared book display presentation and refresh

Core `86e4ad0` reuses the existing combined lexical/cleanup stage for application
book selection. A separate optional presentation owns visible HTML, matching CSS,
cleanup diagnostics and image/font/double-space states. Raw combined and section
HTML stay unchanged; read-only document calls do not add presentation. Unsupported
annotation grammar retains the whole-document fallback with spacing-only CSS.
Discovery is committed with successful selection after all owned copies are staged.
Refresh reuses the committed requested target, preserving explicit root paths.

C getters copy presentation metadata while borrowed strings remain owned by the
document. Mac `614a81d` copies the packet and uses native display controls; raw
HTML is not substituted for formatted HTML with different CSS. Controls refresh
through the worker and retain the unsubmitted key. The full local Mac/core suite
passed 105/105; the final compact menu adjustment passed app/model builds and nine
focused checks. Portable Linux/core on Mac passed 83/83; focused ASan/UBSan 7/7
and C11 pass. Disposable window checks cover menu state, image/font toggles,
spacing, root/draft refresh and compact header. Real resource loading, note
numbering and full font/RTL/corpus/platform acceptance remain pending.

The next seam characterizes legacy showImage commands and copied SDK resource
configuration before implementing resource I/O/native viewers. Legacy strips a
leading `file:` and accepts eight lowercase image suffixes; clicked images go to
a platform viewer and hover shows the filename. Relative/file-prefix/base-path
semantics must be recorded rather than guessed. No asset loading is claimed yet.

## Image command context and bounded local resources

`b3f1198` compares typed image filenames against the extracted actual legacy
showImage branch, for clicked/hover paths, file-prefix forms, Unicode and all eight
case-sensitive suffixes. Declared URI modules are retained as metadata; the legacy
image branch does not rebase the filename using them. Worker/C results own copied
SDK DataPath, AbsoluteDataPath, PrefixPath and PreferredCSSXHTML values, preserving
missing/empty distinctions and leaving selection/history/SDK attributes unchanged.

The SDK metadata test confirms that AbsoluteDataPath supplies the directory even
for a RawGenBook file-prefix DataPath. The [CrossWire SWMgr source](https://www.crosswire.org/sword/apiref/swmgr_8cpp_source.html)
describes this distinction; local tests validate the installed SDK behavior.

`bd487b4` reads local regular image files inside that canonical SDK directory,
with a 16 MiB cap and no truncation. Relative and contained absolute/file forms
are supported; outside paths, escaping symlinks, authorities/UNC, drive-relative
forms and nonregular files reject. Contained symlinks work. The shared bounded
binary reader also backs preferences with their existing error behavior retained.
Observed path/size/time changes reject; files are not locked/frozen across requests.
MIME is a filename hint, not an image decoder result. Empty/binary/NUL data and
owned C copies survive worker shutdown; module/config/profile bytes stay unchanged.

Local checks: 107/107 Mac/core, 85/85 portable Linux/core on Mac, focused
ASan/UBSan 9/9 and C11. Native separate-image preview is next. Inline resource
routing, image sizing, CSS files, other formats/corpus, Windows/GTK runtime and
remaining full book workflows stay pending. No complete F10 acceptance.
