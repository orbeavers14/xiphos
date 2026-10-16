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

`core_book_tree` generates original-prose RawGenBook fixtures, including empty,
structural-only and deep branches, numeric and Unicode titles. It compares all
one-hop relationships with the frozen SDK sequence used by the legacy backend,
checks typed errors/recovery, live key identity/position, attributes, result
lifetime, and byte-for-byte identity of every module data file and filename
before/after successful and rejected queries. Manager-generated configuration
caches are outside this module-data identity assertion.

The next slices are saved book-selection characterization and native tree/navigation controls,
then image/resource routing,
links, history/persistent selection, additional book windows and editors. Preserve
the GTK3 app and all its write paths. No F10 group, native GUI, corpus, platform or
legacy-retirement gate is accepted by source queries alone.
