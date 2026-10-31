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

## Characterized annotation candidate plan

3811bd5 adds a bounded owned candidate plan in the retained breadth-first order.
The GUI's synthetic Bookmarks root contains all top-level XML items; its children
seed the traversal. Nonempty folder colors override inherited colors and empty
colors inherit. Raw bookmark descriptions, source item/color provenance, module
identities and preprocessed keys remain copied metadata. Foreground selection
matches the retained hexadecimal-prefix/luminance policy, including malformed
input behavior; colors are not applied as CSS by this service.

The complete frozen/current tag-map builder, cache clear and lookup are compared
in54 original-fixture cases with real SDK reference expansion. Tests cover nested
inheritance, generic/text/regular-commentary/dictionary/personal/missing identity
classes, ordered duplicate overlaps, key restoration, escaped descriptions,
note-marker preprocessing and disabled cache behavior. Classification in this
host is controlled; production installed-catalog eligibility remains the next
service layer. The candidate plan does not itself filter modules, expand SDK
references, group overlaps or render annotations.

Marker cleanup deliberately reproduces the retained broad behavior: after `#` or
`!`, later ASCII `n` and digits are replaced by spaces through the remainder of the
key, even across later references. Tests characterize this rather than silently
inventing a new parser rule. A first test expectation missed one replacement
space and was corrected after the complete map/call-order comparisons passed.
Native planning validates parent/kind/depth/sibling order and aggregate copied
text bounds, including caller-constructed snapshots. Physical highlight rendering,
personal-note precedence and native annotations remain unaccepted.

## Exact-catalog owned annotation maps

`Reader::bookmark_tags` and the explicit FIFO/C selected-file command now expand
eligible candidates into owned OSIS verse groups with ordered candidate-index
occurrences. Generic absent/empty module identities qualify. Installed exact Bible
IDs and regular commentaries qualify regardless of the current control Bible;
personal RawFiles commentary, dictionary/devotional, missing IDs and abbreviations
do not. This preserves the retained exact-catalog policy rather than importing the
search/parallel abbreviation policy. Every candidate retains eligibility state,
including candidates after a limited inspected prefix. No source mutation occurs.

The parser uses the control module's cloned reference-list key. OSIS grouping then
uses a fresh VerseKey with that locale, as retained code does, rather than promising
accurate cross-versification equivalence. Twenty-seven complete map comparisons
cover installed KJV/KJVA Bible and regular-commentary controls and nine source-module
classes. They verify keys remain stable and grouping/duplicates agree. Module
filtering is now production core behavior, unlike the earlier controlled-class host.

Default bounds are8192 distinct verses,32768 occurrences and16MiB copied string
bytes. Maximum configurable counts are65536 verses and262144 occurrences; source
and owned string caps remain16MiB. Reference strings are <=32768 bytes and control
module IDs <=4096. Metadata that cannot fit fails; encountering another verse,
occurrence or returned-reference byte beyond a bound discloses `limited` with the
complete ordered prefix. Duplicates on an existing verse consume occurrences, not
distinct-verse slots. Exact complete limits do not claim truncation. Retained
per-range SDK parser limits also disclose partial results. Skipped module/reference
counts and ignored source subtrees remain visible. These caps do not bound SDK
internal allocation. No CSS, DOM, user-note rendering or annotation precedence is
applied by the map service.

The C interface copies all inputs, supports default/explicit limits, preallocates
owned result storage and publishes successful results without allocation at poll.
Tests cover invalid admission without allocation, injected observer/envelope
failure, copied inputs, exact-limit/prefix semantics, malformed UTF-8, unsupported
controls, take-once ownership and shutdown drain. Owned groups survive source and
worker teardown. A test initially counted its own path temporary in admission
allocation measurements; the path is now prepared outside the measured region.
