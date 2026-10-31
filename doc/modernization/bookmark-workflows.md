# Bookmark repository contracts

The owner prioritized bookmarks/annotations alongside parallel reading, editing,
installation, sync and complete session persistence. Devotionals remain deferred.
The retained format is `SwordBookmarks` XML with optional `syntaxVersion`, ordered
`Folder`/`Bookmark` children and nested folders. Folder caption/color and bookmark
module/key/module-description/description are raw optional attributes. Missing and
empty remain distinct; duplicate names and reference/module associations remain
ordered rather than becoming map keys. No installed module is required to inspect
a file, including `studypad` and unavailable module identities.

## Checked owned read service

`parse_bookmarks`/`read_bookmarks` return depth-first copied items with a preceding
parent index and recognized-sibling ordinal. These indices describe one snapshot,
not persistent identities. The exact source bytes have a SHA-256 revision for
future stale-edit detection; it is not an authentication or sync protocol.
Reads use explicit selected files and the shared bounded secure XML parser. No
home lookup, default bookmark generation, module resolution, URI dispatch, CSS,
highlight application or file mutation occurs.

Folder labels use caption. Bookmark labels use a nonempty description, otherwise
`key, module` for an explicit empty description, or key when description is absent.
The retained empty-description formatter has a500-byte stack buffer and undefined
NULL-argument behavior; the new reader uses bounded owned strings and leaves the
label absent when either identity required by that formatter is missing. Long
inputs are tested only in the safe native reader, not executed through that unsafe
retained branch. Folder colors are copied without interpreting them; bookmark
leaf colors are ignored as in the retained loader.

Named folders with missing captions remain typed metadata. Unknown element
subtrees are counted/skipped rather than executing the retained top-level
catch-all folder path, which can attach children through an uninitialized iterator.
Unknown XML must remain preserved when mutation support is introduced. No full
compatibility claim is made for malformed/unknown legacy GUI input.

Limits are10000 recognized items,64 hierarchy levels,32768 bytes per copied text
(including derived labels),16MiB owned aggregate including labels/revision, and
16MiB source XML. A larger derived label or duplicate-copy expansion rejects;
there is no silent truncation. The existing shared XML parser rejects malformed,
DTD-bearing and hidden-NUL documents while retaining declared encoding support.

The host compares18 complete frozen/current recursive tree loads across label,
color and missing-description cases, plus the complete folder/colored-folder/
bookmark XML helper encoding. Raw parent/order/label/module/key/color data agree
on this supported domain. GTK storage/rendering is stubbed; original legacy
allocation leaks are not repaired in the oracle and sanitizer leak detection is
OFF. No native bookmark presentation, create/edit/delete/move, import/export,
colored-tag precedence, annotation editor or whole-parity acceptance is implied.

## Revision-checked edits and owned transport

The repository now supports insert/update/remove/move through explicit core/FIFO/C
commands. Existing-file edits require the exact revision from a prior read. A
NULL revision is accepted only for insert into a missing file; a deleted file is
not silently recreated under an old revision. Indices and recognized-sibling
positions are scoped to that revision. Move positions count destination siblings
after removing the moved target. A parent must be a folder; moving a folder into
itself or its descendants rejects. Removing a folder removes its complete subtree.

Updates replace the known optional attributes for the existing kind and retain
all other attributes/children/comments. Insert/update do not convert node kind.
Folder writes require a nonempty caption and optional color; bookmark writes
require a nonempty key and leaf fields. An explicit empty description requires a
present module and combined key/module length <=497 bytes, so a new edit does not
create the retained fixed-buffer/NULL-label hazard. Missing descriptions and
module-optional generic Bible bookmarks remain supported. No module lookup or URI
execution occurs. Raw imported data remains inspectable within native read bounds.

The shared transaction verifies exact staged bytes and prepares the owned
snapshot/revision before replacing the selected file. Existing permissions and
unrelated XML survive; formatting/encoding may normalize. Source revisions catch
stale positional edits and the file helper catches observed changes during a
transaction, but neither is an interprocess lock or a sync merge algorithm.

Worker paths are bounded and semantic edits are validated before FIFO acceptance;
tab validity is checked before file IO. C structural bounds/flags precede
allocation/copying. Result envelopes are allocated before accepting persistent
writes. Tests inject those allocation failures and prove no write occurs; polling
a committed result allocates nothing. Dropped accepted writes drain on shutdown.
Owned C values survive later edits, source deletion and worker teardown, and take
only once. These operations leave reading selection, histories and search state
alone. Native presentation/drag-drop, whole import/export workflows and annotation
highlight precedence remain separate work.
