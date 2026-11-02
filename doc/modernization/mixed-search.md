# Mixed-family search and export

Checkpoint:2026-11-01. Opt-in shared service; no whole advanced-search or native
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
search annotation choices. Keyed groups reject verse ranges, last-results scope,
attribute modes and raw indexed syntax. Their basic text modes use the same
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

Keyed HTML uses the existing passive allowlist but has no image loader yet.
Resources reject even when verse PNG embedding is requested; a keyed path never
enters the verse asset resolver. Verse PNG behavior is unchanged. Complete keyed
assets, richer formats and semantic-link/corpus acceptance remain open.

## Evidence and next steps

Original-prose fixtures exercise all supported families, one missing source,
repeated dictionary groups, hidden result prefixes, retained tree paths, final
verse-scope isolation, annotation-choice preservation, complete/visible exports,
exact/short byte caps, stale IDs, explicit partial sources, retention capacity,
keyed resource rejection, cancellation and owned C results after worker teardown.
Existing verse-only batch/export characterization remains intentional.

Next: native typed result selection/preview/activation and controls, keyed
last-results/range policy, keyed raw-index/attribute coverage where supported,
keyed assets and broader SDK/module/canon/RTL/physical-platform acceptance.
