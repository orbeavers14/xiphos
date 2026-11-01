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
Supported modes are regex, phrase, words and explicit multiword. Verse bounds,
last-results scopes, raw indexed/attribute modes and unsupported sources reject.
Words may use the existing SDK index optimization when available; raw advanced
indexed syntax is not enabled by this service.

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

Next: typed multi-family batch generations, source-specific preview/activation,
text/HTML export dispatch, keyed retained scopes, raw indexed/attribute coverage,
native controls and broad corpus/cross-host acceptance. Do not enable these result
families in existing verse-only UI/export paths until those consumers dispatch
by source capability. Existing verse-only batch failure tests remain intentional.
