# Mixed-family search and export

Checkpoint:2026-11-02. Opt-in shared service; no whole advanced-search or native
interface acceptance is claimed.

## Execution and retained identities

`StudySession::search_mixed_batch`, the FIFO worker task and
`xc_worker_search_mixed_modules` reuse the existing ordered batch engine. The
original `search_batch`/`xc_worker_search_modules` remain verse-only so existing
consumers cannot accidentally navigate opaque keys as scripture.

Mixed batches accept Bibles/regular verse commentaries, SWLD dictionaries and
supported SWGenBook indexed-tree books. Requests retain their raw module tokens;
actual resolved IDs and `SearchKeyKind`/`xc_search_key_kind` identify the result
source. Query, copied-result, aggregate-reference and complete-retention limits
remain shared. Failures stay attached to their requested group; later groups
continue. Repeated sources remain distinct groups. C currently supplies one
common query/options across its ordered module list; C++ supports per-group
queries as before.

Only verse groups update the tab's complete verse last-results scope and saved
search annotation choices. Keyed groups reject verse ranges. Inclusive exact entry intervals are available
through the [typed interval API](keyed-search.md#inclusive-keyed-intervals--2026-11-03). Lemma/morphology/footnote modes reuse SDK
entry attributes (see [keyed contract](keyed-search.md#keyed-entry-attributes--2026-11-03)). Indexed mode now uses the
existing SDK capability probe and explicit multiword-fallback metadata.
Last-results searches now use complete retained keys from one matching actual
source in the preceding successful, non-cancelled generation. Missing, repeated,
partial or capacity-exceeded sources reject instead of broadening. Empty sets
stay empty across repeated narrowing. All groups in a batch read the preceding
generation, so earlier groups in the same request cannot change later scopes.
Clear last invalidates both keyed and verse scopes while keeping exports intact.
Standalone keyed Reader queries still reject session-only last scopes. Their basic text modes use the same
filter preparation and cooperative progress/cancellation as standalone keyed
search. A cancelled job before any execution keeps the previous generation;
late cancellation retains the existing completed-prefix rules.

Complete SDK keys stay private on the one engine owner. Owned generation reads
preserve key kind, actual source, visible prefixes and complete returned keys.
Indexed-tree keys may expose no range text; retention now accounts that optional
metadata without weakening verse/other-key validation. Generated tests verify
that cloned complete book paths do not expand into unrequested sections.

## Content and export

Existing visible/all-retained text and passive HTML exports now carry committed
key kind through row resolution. Verse reads keep their established configured
source/CSS path. Dictionary and book reads require the exact installed type and
canonical key/path, apply the tab's module engine options and restore the SDK key.
No dictionary fuzzy fallback or verse parsing is used for keyed content. Generic
HTML is bounded SDK body output; complete native font/decorations are not claimed.

The legacy `include_scripture` flag means include the matching entry body for all
supported families. Plain companions and HTML share the same resolved rows.
Explicit visible ordinals preserve order/duplicates; all-retained export requires
complete retention and explicit acceptance of failed/stopped source groups.
Stale generations, bad ordinals, capacity/output failures and unsupported markup
publish no partial document. These are current installed contents, not immutable
search-time content revisions; external module changes are not interprocess locked.

Keyed HTML uses the existing passive allowlist. The explicit static-PNG option
now resolves dictionary/indexed-tree images from the actual source module using
the shared resource resolver and per-document cache. Count, original-byte, pixel
and encoded-output budgets aggregate across all families. Default resource
rejection and all-or-nothing publication remain unchanged. Active formats are
not accepted by static-PNG export. Complete assets, richer formats and broader
semantic-link/corpus acceptance remain open.

## macOS integration

The Search pane now offers an explicit “Include dictionaries and books” option.
The default remains verse-only. Bounds choices remain verse modules; unsupported
keyed scopes/modes retain the shared core's per-source rejection. Single keyed
queries use the same retained mixed generation as multiple-source queries.

Copied hits retain their source kind through selection, preview, activation and
export. Dictionary activation validates the exact canonical key and records its
existing dictionary history within one FIFO job. Book activation selects the
exact canonical path through the existing shared session service. Missing keys
cannot fuzzy-open a different entry. Both open their existing native panes and
preserve the main reading pane and unsubmitted reference draft. Keyed preview links now use source-specific routing. Dictionary links revalidate
the exact captured entry before invoking the shared contextual link service.
Book links first select/show the exact source document, then use its existing
book-pane handler; relative scripture links retain the main reading context.
Image links open the existing native viewers. Opaque source keys never become
verse context for preview rendering. Explicit supported scripture/dictionary/book
links retain the existing core URI semantics; arbitrary web navigation rejects.
The export control now says “Include entry text” for all supported families.

Native search drafts persist the opt-in flag before restoring module choices.
Older version1 drafts without that optional field retain verse-only behavior.
No parallel search, navigation or persistence engine was introduced.

Session/FIFO keyed previews now apply the session's module engine options and
reuse bounded book inline-image preparation. Their HTML retains semantic links;
the native preview allows only prepared data-image loads and disables module
scripts/network/filesystem subresources. Missing or unsupported preview assets
retain the existing book preparation fallback; export failures still publish no
partial document. Raw Reader keyed previews remain raw SDK HTML. The shared
resource resolver handles OSIS directories with and without trailing separators
without decoding a native directory prefix or accepting a sibling-prefix match.

Generated compatibility fixtures add RawLD4 dictionaries in ThML and OSIS,
Unicode image names, Hebrew/Arabic keys/text and nested book paths. Phrase,
words, regex and multiword searches, exact previews, source-owned image commands,
static PNG export and aggregate-limit failure are checked. Live native checks
confirm compact controls, OSIS dictionary and book preview images/viewers,
preserved Bible reading and mixed-option/module/query restoration after restart.
These scoped checks do not establish broad commercial-module or cross-platform
visual acceptance.

## Evidence and next steps

Original-prose fixtures exercise all supported families, one missing source,
repeated dictionary groups, hidden result prefixes, retained tree paths, final
verse-scope isolation, annotation-choice preservation, complete/visible exports,
exact/short byte caps, stale IDs, explicit partial sources, retention capacity,
keyed resource rejection, cancellation and owned C results after worker teardown.
Existing verse-only batch/export characterization remains intentional.

Next: broader keyed interval/corpus acceptance,
keyed assets and broader SDK/module/canon/RTL/physical-platform acceptance.

## Actual keyed indexes

Original-prose RawLD4 and RawGenBook fixtures create and execute actual SDK
indexes in the alternate SWORD/CLucene build. Raw advanced indexed syntax keeps
SDK OR behavior; Words keeps sidebar AND preparation. Primary builds without a
usable index skip the positive gate explicitly and retain reported fallback.

The SDK's indexed general-book search can ignore an arbitrary-key range. Shared
execution now intersects keyed last-results output with the exact authoritative
key set before copying counts, sorting, complete retention or export. This
applies to automatic indexed Words too. Filtering keeps source order, removes
out-of-scope paths and cannot expand tree parents into unrequested children.
No verse parser or second search engine is introduced.

## Broader original asset corpus — 2026-11-03

Generated OSIS/ThML dictionary and tree-book fixtures now contain original PNG,
JPEG, GIF, BMP and static SVG images, with percent-encoded Unicode filenames.
Shared resource preparation owns all15 preview images. Native production WebKit
actually decodes all15 under the existing data-only image/content-script policy.
This validates rendered images, not just MIME hints or string substitution.
Fixtures contain original tiny artwork, with no downloaded module content.

The existing passive-export policy remains explicitly static PNG only. Exporting
the mixed-format corpus fails atomically, with no partial document. Existing
path traversal/remote/file URI and aggregate image-limit rejection remains checked.
This is broader generated-corpus evidence, not acceptance of every format,
compressor, module filter, real-world corpus or physical platform.
