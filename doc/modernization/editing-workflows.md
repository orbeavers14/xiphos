# Shared rich documents and SWORD module editing

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


The next owner priority after session restoration is editing. The retained
GtkTextView file path serializes its buffer and calls `g_file_replace_contents`
(`src/editor/gtktextview_editor.c`, `_save_file`). Module note/book writes take
separate SWORD paths. Graphify identified the file serializer and shared
same-directory replacement transaction; these boundaries were verified in source.

## Exact source and revision-checked saves

`EditableDocument` owns serialized UTF-8 source and its exact-byte SHA256 revision.
Explicit-path open/save is available through shared C++, the FIFO worker and C11
handles. Empty files, BOM, CRLF/LF, markup and literal percent sequences retain
their bytes. There is no implicit formatting, HTML rendering, percent decoding,
profile discovery, recent-file registration or module write. Non-UTF-8/NUL-bearing
sources and files over4MiB reject. Sources/destinations must be regular files;
existing symlinks retain their alias and update the selected target.

Creating requires an absent expected revision and a still-missing file. Replacing
requires the revision from the last open/save; missing/stale/external-change
mismatches reject before replacement. The owned output is prepared first, staged
bytes are verified, and the existing same-directory transaction commits atomically
while retaining permissions. These are observed content-change checks, not an
interprocess lock, collaborative merge or crash-durable fsync protocol.

Worker commands validate the calling tab before IO and leave selections, histories,
search and preferences alone. C arguments copy before return; observer/result
storage precedes acceptance. Successful polling only moves owned data; results are
take-once and survive source/request/worker teardown. Accepted dropped writes drain
at shutdown. Public C bounds validate source/revision structure before observer/copy
allocation. Paths are explicit and bounded to4096 bytes.

The disposable fixture checks exact source, independently known empty SHA256,
external/stale changes, create-versus-replace, invalid text/size/directory inputs,
the exact byte limit, permissions/symlinks where supported, closed tabs, C admission
allocation failures, copied inputs, no-allocation committed polling and ownership.

## Shared rich formatting and undo (2026-10-31)

`rich_document.hpp` owns typed UTF-8 text/style runs, parsing, canonical serialization
and formatting. C handles expose copied documents; native code adapts runs to
AppKit attributes without a second formatting engine. Supported fragment markup:
bold, italic, underline, strike, foreground/background colors, links, six heading
levels, address/preformatted blocks and flat bullet/decimal/roman/alphabetic lists.
Links accept sword/bible/http/https/mailto schemes; attributes and text are escaped.
Formatting selects UTF-8 byte boundaries; the Swift adapter converts UTF-16 ranges.
Serialization emits balanced final tags, correcting the retained GTK serializer's
end-iterator omission. It does not promise byte-identical canonical markup after
an explicit rich edit. Opening and saving unchanged source retains exact bytes.

Unsupported elements/attributes (including images, tables, nested lists, arbitrary
CSS and full-document metadata) reject rich conversion. The native editor keeps
such content in Source mode instead of silently flattening it. Plain-text modules
use Source mode. Rich documents bound aggregate text/style metadata and serialized
output to4MiB, runs to16384, traversal depth to64 and link length to4096 bytes.
`EditorBuffer` tracks exact source, saved baseline and bounded shared undo/redo
(100 revisions and16MiB per stack); undo after Save correctly becomes dirty.

## Revision-checked SWORD source editing

The FIFO worker reads/saves installed, unencrypted UTF-8 `RawFiles` personal
commentaries (supported KJV canon) and indexed `RawGenBook` general books.
RawFiles targets require one explicit book/chapter/verse; general-book targets
must be exact existing absolute tree paths for ordinary editing. Unsupported drivers, encodings,
encryption, canons and book key types reject. General-book Clear Entry removes
content while retaining the tree node/offset; personal-note deletion uses the SDK
`deleteEntry` contract. A separate prepare/create operation supports new absolute RawGenBook sections;
move/rename/tree deletion and module creation remain outside this slice. Linked entries follow the installed SDK's write/delete
semantics; this service does not detach links into independent entries.

Each module must own an isolated data directory below its configured module root.
The service resolves `DataPath` against `PrefixPath`; `AbsoluteDataPath` can be an
asset directory and is not a RawGenBook filename prefix. Overlapping catalog data,
symlinks/hardlinks/special files within the data tree, more than4096 entries or
more than64MiB of data reject. Revisions include target/config identity and a
SHA256 manifest of the whole data tree, including file/directory permissions.
A stale revision rejects before replacing installed data.

A same-parent private staging directory receives the complete data tree. A fresh
SDK driver writes/flushes it, then a fresh driver verifies exact raw source and
key identity. Two directory renames install the verified tree after a second
revision check. A new manager and replacement sessions are staged before commit;
errors restore the original data and SDK state. If filesystem rollback itself
fails, the original backup is retained for recovery. This is a serialized local
transaction, **not crash-atomic, an interprocess lock or an fsync durability
protocol**. External writers must not edit the module concurrently.

Live tab IDs, histories, selections and capabilities survive replacement; cached
book documents refresh. Search generations/scopes are deliberately invalidated
because they may own SDK keys into the previous manager. Generation serials remain
monotonic, so a stale handle cannot address a later result. The Mac adapter clears
visible results and asks the reader to choose a scope and search again.

## Native macOS workflow and evidence

The application menu opens new/existing study documents and the selected personal
commentary/general-book entry. One native text editor presents Rich/Source modes,
formatting/link controls, shared undo/redo, Save and Save As. Module Save As exports
the draft to a file without modifying the module. File and module saves use the
shared revision checks. Native typing uses typed runs; input-method marked text is
preserved during composition. Save/Discard/Cancel close handling waits for a
successful save; quit checks dirty editors before saving the reading session.
Normal saves refresh the reading/book presentation without adding history visits.

`core_rich_module` uses original-prose RawFiles/RawGenBook fixtures to check rich
round trips, Unicode selections, unsupported markup, saved undo baselines, stale
rejection, SDK reload, sibling-tab book refresh, node preservation, injected
post-install rollback and C ownership/admission/no-allocation publication.
`macos_rich_editor` exercises real offscreen AppKit text/windows, native typing,
formatting, source fallback, file/module save/reopen, stale saves, module export
and dirty-close decisions. The fixture also renders a heading/list preview for
local visual inspection. See the Mac handoff for actual build/test results.

## Remaining editing and compatibility work

Journal/prayer-list organization, general-book tree/module creation, linked-entry
workflow acceptance, editor-window/draft restart and crash recovery remain open.
Windows/GTK4 editor adapters and physical Windows/Fedora validation remain open.
These supported services and offscreen checks do not establish whole F25/F26
workflow or visual parity. Installation and sync are the next core priorities;
devotionals remain deferred.

## Internal links in rich documents

Rich formatting now retains validated `passagestudy.jsp` and `xiphos.url` study
commands alongside existing URI links. Syntax is checked by the shared contextual
router; activation supplies its live context later. Serialized query ampersands
and Unicode text round-trip without embedding a synthetic source module.
Unknown actions and malformed image commands reject; scripts and arbitrary local
file links remain outside the rich subset. Journal/prayer organization and full
legacy markup compatibility remain open.

## Staged general-book section creation and index freshness

Explicit prepare/create calls through C++/FIFO/C create a missing UTF-8 RawGenBook
path, including missing parents, inside the private staged tree. Prepare reads
without writing. Paths reject empty/dot segments, trailing slashes and controls;
existing entries reject instead of overwriting. The whole-module revision guards
intervening edits. Empty sections are valid and reread through the ordinary editor
API after commit. Sibling tab selections and existing offsets remain preserved.

Module writes now invalidate a recognized usable search index in the staged copy,
using a driver configured only for that private tree. RawGenBook writes otherwise
leave an old SDK index searchable. Fresh queries fall back to current content
until rebuild. Known cache artifacts that this SDK cannot use reject the edit
without deleting unknown assets or foreign/corrupt caches. Rollback restores the
original content, tree and index together. Actual CLucene checks require a usable
index and prove new content appears after creation and a rebuilt index survives
an injected rollback. This does not strengthen the transaction's crash durability
or authorize concurrent external writers.

File > New Journal or Prayer Section now lists general books and accepts an
explicit path. Create and Edit commits an empty section before opening the normal
rich/source editor; the form explains that closing the editor retains the section.
Preparation/creation failures close the temporary tab and retain the form for retry.
Busy creation prevents close/quit. Native checks cover duplicate/path rejection,
rich save/reopen and ordinary module-draft recovery. A disposable live app verified
creation, bold formatting, save and tree navigation to the rendered new content.
Broader journal/prayer organization, move/rename/delete and full workflow parity
remain open.

## Legacy font-color fragments — 2026-11-03

The shared parser now accepts legacy `<font color>` wrappers, maps their color
through the existing bounded color validator, and exposes the same typed runs
used by C/Swift/native formatting. Nested colors restore the outer style;
colorless wrappers inherit it. An explicit rich edit serializes canonical color
spans; opening/saving unchanged source still preserves exact original bytes.

Font faces/sizes, event attributes and invalid/injected color values continue to
reject rich conversion so their original source is retained. This adds no new
style flags, ABI fields, frontend parsing engine or dependency. Checks cover
nested style scope, Unicode, canonical round trips, rejected unsupported metadata
and the C/Swift editor adapter. Broader rich-markup compatibility remains open.

## Representable inline CSS font traits — 2026-11-03

Rich span parsing now accepts font-weight bold/700 and normal/400, plus font-style
italic/normal. They map to the existing typed bold/italic flags, including nested
resets and restoration of outer traits, case-insensitive keywords and last
same-element declaration precedence. Explicit rich edits emit the existing balanced
canonical markup; unchanged source still retains exact bytes. C/Swift consumes the
same runs, with no new styling engine, flags or dependency.

The subset rejects unrepresentable weights, relative weights, oblique, important
qualifiers and other unsupported CSS. A normal weight inside a heading or normal
style inside an address also rejects: their block-level implied traits cannot be
cancelled by the current run model/canonical serializer. Such documents stay in
Source, preserving their original markup instead of silently changing appearance.
Checks cover inherited trait resets, Unicode/color composition, round trips,
unsupported cases and the native C/Swift editor bridge.

### Inline CSS text decoration — 2026-11-03

Rich conversion accepts `text-decoration: underline`, `line-through`, their
combination and `none`. Decorations compose with the existing font/color runs.
As in HTML, `none` does not cancel decorations propagated from an ancestor;
later declarations on the same span replace that span's local decoration.
Canonical `<u>`/`<s>` markup retains the result. Unknown decoration styles,
colors, overline, duplicate tokens and priority qualifiers preserve Source by
rejecting rich conversion. No new flags, dependency or frontend formatter.

### Revision-checked leaf rename

`xc_worker_rename_book_leaf` renames an existing RawGenBook leaf under its
current parent. The staged save verifies the original content and tree offset
before publication. Open tabs pointing to that leaf follow the new path.
`xc_worker_rename_book_subtree` also accepts a parent section. Descendants
retain their offsets and content, and open descendant tabs follow the new path.
Existing session files are not rewritten. Move and node deletion are separate
operations.

### Move and deletion

SWORD's RawGenBook deletion unlinks the tree node but leaves its index slot.
SWORD's index builder walks those slots and also follows sibling links. In the
macOS test, the leaf disappeared from path lookup and its parent survived, but
the subsequent index rebuild hung even when the leaf body was cleared first.

The public tree API can append and remove nodes but has no reparent operation
that preserves an existing node offset. A move needs to preserve or explicitly
remap stored offsets and links. Both operations need a successful index rebuild
and a search check before they can ship.

See the [upstream bug report draft](rawgenbook-index-hang.md) for reproduction steps and the candidate cursor fix.

#### Move contract

A section move changes its parent and keeps its local name, tree offset, raw
body, and descendant offsets. The destination must be an existing section
outside the source subtree. Reject a same-name child, the root as a source,
and a stale revision before publishing any changes.

A copy followed by deletion allocates a different offset. Existing offset
links would still address the old slot, so that sequence cannot implement
this contract. A SWORD reparent operation would need to update the old and
new child chains and the source node's parent in the staged module. Before
publication, verify both paths, the unchanged offsets and bodies, and index
rebuild and search. Open tabs should follow the moved paths.

An isolated SWORD 1.9.0 build on macOS validated an
[offset-preserving reparent prototype](rawgenbook-move-candidate.patch)
with the [SDK-only move test](rawgenbook-move-reproducer.cpp). The test
rejected moving a section under its own child, moved it to a parent with an
existing child, and reopened it by its original offset. The descendant kept
its offset and text, an existing linked entry still resolved, and CLucene
returned the descendant under its new path. The old path failed lookup.
The build also included the [index cursor fix](rawgenbook-index-hang.md).
Xiphos exposes the staged operation through `xc_worker_move_book_subtree`
when built with the candidate SWORD headers and library. The macOS integration
test verifies that a moved descendant keeps its offset and text, an open tab
follows the move, and a rebuilt CLucene index returns the new path. Stock
SWORD builds reject move requests.

### New general books

`xc_worker_create_general_book` creates a writable UTF-8 RawGenBook with an
empty `/Notes` section. It stages the module through the local installer
with replacement disabled. The macOS test covers editing, restart, and
duplicate-ID rejection.

`xc_worker_delete_book_leaf` is available in builds configured with
`XIPHOS_SWORD_SAFE_BOOK_DELETE=ON` and the
[reviewed SWORD patches](../../dependencies/sword/README.md). It checks the
revision, removes the staged leaf, and clears open selections on that leaf.
The macOS patched-SDK test rebuilt CLucene and found the retained entry but
no hit for the deleted leaf. Sections with children are rejected.

`xc_worker_create_study_organizer` uses the same staged installer to create
a writable book with separate `/Journal` and `/Prayers` sections. Each accepts
the shared section creation and editing operations. The core test creates
entries in both and reopens them after restarting the worker. Native organizer
controls and journal-specific presentation are separate frontend work.
