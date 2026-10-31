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
