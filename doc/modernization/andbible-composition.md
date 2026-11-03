# AndBible import, bookmark interchange and verse composition

Checkpoint: 2026-11-01. Shared C++17 core, C11 ownership bridge; native chooser,
merge review and renderer integration remain separate acceptance work.

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
survive. Stable AndBible identity collisions reject the whole operation; explicit
conflict-review/update semantics remain future work. Generic XML has no invented
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
is not yet in-place reading highlighting or full native color styling. The new
note-form drafts are not yet part of autosave/crash recovery; normal quit is
protected by Save/Discard/Cancel. Complete annotation display integration, draft
recovery and cross-platform/full-corpus acceptance remain open.
