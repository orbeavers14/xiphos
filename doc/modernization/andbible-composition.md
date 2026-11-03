# AndBible import, bookmark interchange and verse composition

Checkpoint: 2026-11-03. Shared C++17 core, owned C11/Swift transport, explicit
native import review and reading composition are implemented in scoped slices.
Representative real-backup and whole-workflow acceptance remain open.

## Import and interchange

`read_andbible_bookmarks` reads an explicitly selected standalone SQLite backup
(up to64MiB). It rejects nonempty WAL/journal sidecars, copies the bytes into a
private temporary directory, and uses a read-only, bounded transaction with
extensions disabled, defensive/trusted-schema settings, an authorizer and an
execution budget. It never opens the original database for writing. This is a
backup importer, not a live AndBible database migration or an interprocess lock.

Supported ordinary tables/columns match the retained import contract:

- Required `BibleBookmark(id,v11n,ordinalStart,ordinalEnd,book)`.
- Optional `Label(id,name,color)`, `BibleBookmarkToLabel(bookmarkId,labelId,orderNumber)`
  and `BibleBookmarkNotes(bookmarkId,notes)`.

UUIDs must be16-byte blobs. Labels retain ARGB-derived RGB colors and per-bookmark
ordering. Each label membership becomes a bookmark occurrence; unfiled entries
remain beneath Uncategorized. Internal speech/unlabeled labels do not become
user folders. Plain notes remain descriptions and exact `andbible-note` metadata,
matching the retained import semantics; they do not silently overwrite personal
verse annotations. The XML also records stable entry/membership identities,
source checksum and versification. Unknown versification, invalid ordinal and
invalid bookmark identity rows have explicit separate skip counts. Other invalid
schema/linkage/duplicate note data fails the preview rather than discarding it.

Bounds include5000 source rows,1000 labels,64 memberships per row, bounded UTF-8
fields and the existing10000-item/16MiB XML limits. Unknown SQLite schema versions,
ZIP/container backups and AndBible export are not implemented. No real-user
backup corpus acceptance is claimed from generated fixtures.

`append_bookmark_document` imports supported SwordBookmarks XML beneath an
explicit folder in one verified same-directory replacement. Existing exact-byte
revision and existence guards apply. Unknown XML/comments and import metadata
survive. The existing append API still rejects stable identity collisions. Explicit
non-destructive review/merge is now available separately, as described below. Generic XML has no invented
persistent identity and follows the explicit append request.

`export_bookmark_document` copies the complete valid XML, byte-for-byte, to a
separate destination. Source revision is required; a missing destination requires
no destination revision, and replacement requires its exact current revision.
No lossy reconstruction from displayed rows is used. Export supports
SwordBookmarks XML, not arbitrary bookmark formats.

## Shared presentation and source identity

`compose_verse_decoration` combines an explicit canonical target, its ordered
bookmark tags and escaped personal-note cache. It retains the existing inner
annotation/current-verse color priority and all bounded ordered tag occurrences;
the first outer tag color wins. Copied tag metadata is capped at16MiB and partial
source/output state is exposed. Colors are raw metadata, not injectable HTML/CSS.
Native consumers must validate styling and consume the owned plan with the
matching source. This does not establish native visual parity.

Imported versification provenance is checked before applying a tag. An unmatched
canon is exposed as unsupported rather than silently reinterpreted. Fresh OSIS
normalization now uses the actual control module canon, fixing KJVA Tobit keys
that previously passed through default KJV normalization.

Worker jobs use the existing single FIFO SDK owner. C inputs copy before return;
results are take-once, own their strings and survive request/worker teardown.
Import/export storage jobs do not visit navigation history or discover profiles.

## Search and maintenance evidence

The alternate installed SWORD1.9.0/CLucene2.3.3.4 SDK passes required actual-index
creation/execution, advanced OR versus sidebar AND, bounded results/ranges and
last-results scopes. Primary SWORD1.9.0.3916 still has no usable index; its skip
must not be reported as successful indexed execution. Maintenance now uses the
shared usability probe: `hasSearchFramework()` alone reports backend availability
on the CLucene SDK even after index deletion.

An original-prose SWCipher/RawText fixture proves actual ciphertext does not
contain the prose, blank/wrong keys do not reveal it, the correct key unlocks it,
clearing the key invalidates cached content, and fresh workers reopen it without
rewriting ciphertext. This is not licensed-provider integration or a commercial
module compatibility claim.

Generated integration fixtures cover labels, UUIDs, notes/escaping, counts,
source preservation, duplicate/stale rejection, exact XML export, C result
lifetimes, ordered overlapping composition, output limits, KJVA/canon mismatch,
Hebrew/Arabic phrase search and passage links. Mixed-family batches preserve
Bible/commentary results and explicit dictionary/general-book failures. Generic
family search, additional semantic-link/rendering cases, broader versification
and RTL visual/corpus acceptance remain open.

## Checked personal annotation writes

Annotation snapshots now own the SHA256 revision of the exact source XML. New
checked C++/FIFO/C save/remove calls require that loaded revision for an existing
file, or a null revision for a still-missing file. Both raw stored-label operations
and SDK-canonical verse-target operations use this guard. The original entry
points retain their legacy update semantics. Unknown XML, duplicate/first-write
behavior and the retained last-match cache policy stay unchanged.

Revision mismatches, changed whitespace/comments and missing-versus-existing
conflicts reject before replacement. Reload/retry uses the fresh snapshot; this
remains an observed-change check, not an interprocess lock or automatic merge.
Revision metadata counts toward bounded snapshots. New C revision validation
rejects malformed digests without allocating; copied inputs and preallocated
owned results retain take-once and no-allocation committed polling behavior.
Generated tests cover stale source preservation, copied revision lifetime,
canonical KJVA generic targets, removal, unknown metadata and explicit reload.
File > Personal Verse Notes now consumes copied targets/profiles for checked
plain-text save/remove, module-specific or generic targets, explicit existing-file
selection and conflict reload/retry. Target changes require Load before Save;
unknown fields and later duplicates remain. Legacy-hidden/duplicate records are
disclosed. Closing, reloading or quitting with a dirty draft asks before discard;
a failed save retains the draft and blocks close. Note entry uses the visible
main verse even while a book pane is open, independently of BibleSync policy.

Choose Bookmark XML and Preview Saved Composition consume the shared owned plan:
ordered tag-color metadata, escaped saved-note HTML, highlight precedence and
incomplete-source diagnostics. Raw color strings are displayed as text, never
injected into HTML/CSS. The preview explicitly excludes unsaved editor text. This
is not yet in-place reading highlighting or full native color styling. Validated note-form drafts now participate in periodic recovery with an explicit
annotation target: notes-file path, control module/reference, exact stored label,
source revision and saved/current plain text. Recovery never treats a verse note
as a whole-file editor save. Version2 snapshots carry annotation labels; ordinary
snapshots retain version1 and old files remain readable. The C input struct stays
unchanged; a distinct transactional append operation adds annotation drafts.

Native restore opens the note form without reading over the recovered text or
refreshing its original revision. Stale saves retain the recovered draft. Multiple
recovered note windows participate in autosave and quit review. Unvalidated note
text recovers as an untitled document, with no invented source-write intent.
Core bounds, checksum, previous-valid fallback and conflict rules remain shared.
Native periodic/fresh-process and live startup-prompt checks pass; caret position
and in-reading highlight/style integration, plus full cross-platform/corpus
acceptance, remain open.

## Non-destructive collision review and merge — 2026-11-03

`review_bookmark_import` compares an explicit incoming XML snapshot with the
expected destination revision/existence. One stable AndBible entry/tag-membership
identity can be new, identical, conflicting or ambiguous in the destination.
Unidentified generic XML bookmarks remain new append entries; the service does
not invent persistent identities. Duplicate incoming identities reject preview;
ambiguous destination identities are disclosed and prevent application.

The comparison includes all leaf attributes/children and inherited folder
attributes, order and unknown metadata. Attribute ordering and outer import-folder
captions do not cause false conflicts; import-source checksums are provenance,
not entry content. Canonical namespace URIs, text and unknown leaf content remain
significant. Comparison work is bounded to64MiB and depth64, and copied collision
strings to16MiB. Unsupported/oversized metadata rejects instead of being ignored.

Explicit policies are reject collisions (default), skip identical (any conflicting
metadata still stops the whole transaction), and keep existing (skip every
unambiguous collision and import only new entries). None overwrites a note,
updates existing tag metadata or removes old memberships. There is no destructive
replace policy or two-way synchronization. A bookmark filed under two labels is
two reviewed entries. Exact repeats with no new entries preserve destination
bytes/revision and add no import folder, though verified replacement may still
replace the file. Generic unidentified entries remain explicit append operations.

Application recomputes the comparison inside the existing verified replacement
and exact revision guard. New-entry folder/root metadata and
existing unknown XML remains intact. Source/limit/stale/conflict failures leave
the destination unchanged. The original SQLite backup is never changed.
Owned C review counts/collision rows and revisions survive request/worker close;
existing C structs and append semantics remain unchanged. Native chooser/review
shows counts, existing/incoming descriptions and an explicit policy selector.
Changing the destination requires review against that destination. Preview pins
the copied imported XML, not a live link to a subsequently changed backup.

Generated original-prose checks cover actual backup note changes/new entries,
repeated source UUID rejection, unknown metadata, attribute order, inherited
color conflicts, UTF-16 destination XML, mixed new/duplicate imports, byte-idempotent
repeats, ambiguous IDs, stale guards, C ownership and native policy/export flows.
Real-user backup/schema migration and destructive conflict resolution remain open.

## Import root metadata preservation — 2026-11-03

Append and merge now retain an original root caption separately from the chosen
import-folder caption, using the stable protocol namespace
`urn:xiphos:bookmarks:import:1`. The wrapper records its original root type and,
when present, the original unqualified caption. This is storage metadata, not a
product-branding choice. Namespace-prefix collisions get a distinct prefix;
a default namespace cannot accidentally make marker attributes unqualified.
Conflicting reserved metadata rejects the transaction instead of overwriting it.

Review recognizes these wrappers for generic stable identities as well as legacy
AndBible wrappers. It compares the original caption while ignoring the chosen
outer folder name. Unknown namespaced captions/attributes remain distinct and
preserved; namespace prefix spelling does not invent a conflict. Import may
serialize to UTF-8, while exact XML export still copies the resulting bytes.
Original root captions, empty versus absent values, unknown namespaces and
reserved-metadata collisions are covered by generated fixtures. No new dependency
or public ABI change is introduced by this metadata correction.
