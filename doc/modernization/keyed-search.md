# Typed dictionary and general-book search

Checkpoint:2026-11-01. This is a shared service boundary, not complete advanced
multi-family search or native UI acceptance.

`Reader::search_keyed_module`, `StudyWorker::search_keyed_module_task` and
`xc_worker_search_keyed_module` reuse the existing bounded query preparation,
SDK search execution, copied result limits, progress and cooperative cancellation.
The worker still owns the only FIFO SDK context. A live tab is required by the
worker API, but no history, verse last-scope or retained generation is published.

Supported sources are SWLD dictionaries and SWGenBook sources with the existing
indexed TreeKeyIdx capability. Exact/catalog-resolved source identity is retained.
Supported modes are regex, phrase, words, explicit multiword/indexed and
lemma/morphology/footnote attributes. Verse bounds, standalone last-results scopes
and unsupported sources reject. Indexed mode uses the shared capability/fallback
policy. Attribute matches require metadata populated by the module filters.

`SearchResult::key_kind` and `xc_search_key_kind` distinguish verse(0), dictionary(1)
and general book(2); NULL C results return-1. Result keys are opaque SDK identities.
A scripture-shaped dictionary key must be opened as a dictionary key. A book path
must use the existing book-path resolver and its owned offset, never VerseKey.
C inputs copy before return; take-once results survive request/worker teardown.
Existing verse APIs and their rejection of non-verse batch groups are unchanged.

Original-prose fixtures check all four dictionary modes, exact book-path content,
scripture-shaped dictionary identities, copied prefix/total counts, unsupported
modes/scopes/families, pre-execution cancellation, unchanged verse scope/generation
and C ownership. Fixture copies restore their own erased book content before
searching; no source module or user library is edited.

The [mixed-search integration](mixed-search.md) now supplies opt-in typed batches,
exports and native consumers. Keyed retained scopes are supported by mixed batches; broad corpus/cross-host
acceptance remains open. Existing verse-only batch
failure tests remain intentional.

## Exact keyed previews

`preview_keyed_search` and `xc_worker_preview_keyed_search` accept an exact installed
source ID, canonical returned key (up to4096 UTF-8/XML bytes), explicit kind1/2 and
a nonzero HTML cap up to8MiB. They require the matching dictionary/indexed-tree
capability and exact resolution after rendering; missing dictionary keys cannot
fuzzy-snap to a nearby entry, and missing book paths cannot snap to a parent.
The SDK key is restored on success or failure. No navigation, history, verse
scope or result generation is published. Session/FIFO/C previews now apply the
module engine options and prepare bounded inline images; raw Reader previews
retain raw SDK output. Native typed controls and source-specific links are
documented in the [mixed-search contract](mixed-search.md#macos-integration).

The C request returns the existing owned `xc_dictionary_entry` payload for both
kinds: consume its module/key/HTML fields. Inputs copy before return; output
survives request/worker teardown. This previews current installed content, not
an immutable content revision captured at search time. The cap bounds copied
HTML rather than the SDK's internal rendering allocation. Tests cover dictionary
and book dispatch, wrong kinds, missing keys/paths, output limits and C lifetime.

## Mixed batches and source-specific export

[Opt-in mixed-family batches](mixed-search.md) now retain typed complete keys and
feed existing stale-safe visible/all text and passive HTML exporters. This
supersedes the earlier pending batch/export statements for this supported slice;
standalone keyed-query behavior remains unchanged. Native typed consumption,
preview images/links and opt-in keyed PNG export are checked. Complete assets and full compatibility acceptance remain open.

Indexed dictionary/book queries now use the existing capability/fallback path;
actual external-index OR/Words AND and keyed last-scope isolation are checked in
a separate disposable fixture. See [mixed indexed policy](mixed-search.md#actual-keyed-indexes).
Verse-range requests remain rejected. The standalone Reader query
still has no session-owned last-results scope.

## Keyed entry attributes — 2026-11-03

Lemma, morphology and footnote modes now reuse the shared SWORD entry-attribute
engine for supported dictionaries and indexed-tree general books. Lemma and
morphology keep exact whole-entry identifier matching; footnote body searches
keep substring/case behavior. No verse conversion or separate matching engine is
introduced. Plain sources without those attributes return empty results.

Original-prose RawLD4/RawGenBook fixtures with OSIS/ThML filters compare shared
results directly with SDK results for all three modes and both case settings.
They distinguish G140/G1401 and N-ASF/N-ASF1, retain Unicode/verse-shaped opaque
keys, check complete same-source last-result narrowing, prefix counts,
cancellation, copied C input/result lifetime and byte-identical source data.
This proves these drivers/filters, not all module families or third-party corpora.
Explicit keyed intervals remain undesigned; verse ranges continue to reject.

The compatibility fixture also covers RawLD and ZIP-compressed zLD dictionaries
with OSIS/ThML filters, alongside RawLD4 and RawGenBook (eight generated sources).
Three entries cross a two-entry compressed block boundary. All three modes and
case settings compare with actual SDK results, then exact previews, complete
same-source narrowing and byte-identical source data are checked. Swift mixed
adapters consume all eight sources. This does not prove every compressor, driver,
module-filter combination or licensed corpus.
