# Shared core: initial reader slice

Build/test independently of the GTK application:

```sh
cmake -S src/core -B build/core -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core --parallel
ctest --test-dir build/core --output-on-failure
```

Requires CMake >=3.16, C++17 and SWORD >=1.8.1. The legacy rendering seam uses
C++11. pkg-config discovery is preferred; SDKs can instead supply
`SWORD_INCLUDE_DIR` and `SWORD_LIBRARY`, with
`XIPHOS_SWORD_EXTRA_LIBRARIES` for static SDK transitive libraries. The fallback
SDK version and architecture must be checked by the SDK provider. Windows
SDK/MSVC linking has not yet been tested.

Tests generate a small on-disk RawText OSIS module with original prose.
They validate discovery isolation, UTF-8 XHTML, legacy setKey/renderText
comparison, C header consumption, handle/result ownership, missing modules,
empty libraries and invalid arguments. Fixtures remain in the build directory
for the Swift smoke test and manual preview; no live user data is modified.

The default legacy application still builds normally; its get_render_text
calls the same rendering function. The standalone core build avoids all GTK,
WebKit, help and packaging dependencies. There is no search/history/storage
implementation here yet. The C ABI is experimental and documented in c_api.h.
Follow the [architecture](../../doc/modernization/architecture.md) and
[parity ledger](../../doc/modernization/parity-matrix.md) before expanding it.

## Study-session history

`StudySession`/`xc_session` adds one history per tab or window. The C++ session
borrows its Reader; C ABI sessions retain the reader and remain valid after
`xc_close`. Visit and navigate return independently owned entries; release each
once. Serialize calls and teardown across all handles sharing a reader.

Navigation direction is -1/+1. An unavailable destination returns invalid
argument without moving the cursor. Failed visits do not add history. Clear
History leaves displayed content to the frontend. Legacy behavior uses
reference-only identity, retains forward entries on new visits and keeps the
first visit's module metadata on a duplicate reference. Capacity is 50.
Commentary restoration, synthetic entries, history export and persistence are
later slices. The shared C++11 helper is also used by legacy tab_history.c.

## Chapter snapshots and navigation

`Reader::read_chapter` / `xc_read_chapter` returns a copied title, normalized
anchor and ordered rendered verse entries, including empty text. Enumeration
uses the module's VerseKey/versification. Release C snapshots exactly once;
borrowed snapshot strings survive reader close until chapter release.
`StudySession::navigate_chapter` / `xc_session_navigate_chapter` resolves verse 1
of the preceding/following chapter using SWORD, then records a successful visit.
Only -1/+1 are accepted; Bible ends do not wrap. Failed destinations leave
session history unchanged. This is not full decorated Xiphos chapter output;
headings, options, semantic actions and synchronized study panes are pending.

## Study-note retrieval

`Reader::study_note`, `StudySession::study_note` and `StudyWorker::study_note`
accept an explicit module, verse and SWORD note identifier. They regenerate
that verse's entry attributes and return owned reference/type/label/body HTML
and cross-reference-list metadata. Body conversion/rendering matches the
existing backend; sessions apply their effective module options without
visiting or moving history. Missing notes reject rather than inserting empty
SDK attributes or reusing attributes from the last chapter verse. The helper
is C++11-compatible in CoreRender; native host lifetime/thread rules still
apply. `xc_worker_study_note` returns a polled request whose result transfers once
through `xc_request_take_note`; note strings survive worker/request close until
`xc_note_release`. Swift `CoreWorker.studyNote` copies the result strings
into a Sendable value. Native link handling, hover presentation and expanded
reference lists remain separate pending work.

`study_note_link` decodes local SWORD `showNote` commands, including the fixed
`xiphos-preview://study/` native origin. Query values decode individually;
unknown actions/parameters, duplicates, malformed escapes, control characters,
external origins and passage ranges reject. `StudyWorker::study_note_link` and
`xc_worker_study_note_link` resolve and render in one engine job with tab
settings, validating the link's note/cross-reference type against entry metadata.
Swift exposes the same operation without a frontend URL parser.

`resolve_study_link` also accepts passage URIs, returning a passage target or an
owned note while preserving history. `xc_worker_resolve_study_link` stores the
corresponding result kind; `CoreWorker.resolveStudyLink` exposes a typed Swift
enum. Notes carry their source module so nested links have the correct fallback.

Shared `study_link` decoding also accepts local `showRef&type=scripRef`
commands for one scripture reference. Dispatch validates the module with the
existing reader, and the eventual visit uses that module's own SWORD key for
normalization. Native hosts pass the note's source module as the fallback.
Ranges and multiple-reference lists remain separate pending work.

## Reference-list parser seam

`parse_reference_list` is a C++11-compatible shared helper in CoreRender. It
uses a clone of the module's own SWORD key to expand lists/ranges, retaining
the fork's ordering, duplicates, invalid-element skipping and 2000-verse guard
per range. Owned results report the guard (`limited`) and skipped elements.
The legacy `BackEnd::parse_verse_list` now delegates to it while preserving its
current-key mutation and GList result. Characterization compiles both a frozen
pre-extraction adapter and the actual current adapter without GTK, comparing
56 cases across KJV/KJVA; full GTK application validation remains a gate.
`Reader::read_reference_list`, `StudySession::read_reference_list` and
`StudyWorker::read_reference_list` now return owned module/context/expression,
entry snapshots, shared HTML and limit/skipped metadata. They use effective
module options without visiting history and restore the module key after
rendering. Missing text positions and duplicate entries remain present.
Native C/Swift requests and presentation are pending.
