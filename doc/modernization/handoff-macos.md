# macOS chat handoff

Updated 2026-10-20. Read [AGENTS.md](../../AGENTS.md), [shared handoff](handoff.md),
[status](status.md), [core scope](shared-core-roadmap.md) and relevant parity rows.
Integration branch: `master`, fork `orbeavers14/xiphos`. Sequential direct commits
and pushes are authorized; preserve unrelated changes, never force-push.

## Current checked source and active work

Implementation through `7e9ff93`: retained search generations; selected/all text
exports and native Mac text controls; initial restricted HTML documents with C/
Swift adapters; wrapper-context/traversal guards; extracted shared resource file
policy and NUL-path validation. One C++ FIFO SDK owner, copied values and original
GTK3 behavior/reference remain the architecture. No HTML controls were added.
The only unrelated local file is `.DS_Store`; leave it untouched.

Latest integrated native image checkpoint: full Mac **159 passes + one explicit
index skip** (160 registered, 134.30 s), with desktop access for the isolated
pasteboard test. Portable models on Mac **121 passes + one skip** (122 registered,
92.46 s), GTK runtime OFF. Both builds/tests exited0. Source work is sequential;
preserve unrelated `.DS_Store`. No whole parity group or release is accepted.

Current task: shared verse image sources, C/Swift owned values, actual SDK command
resolution and contextual dispatch are implemented; native Mac image preview is
now checked in models and a disposable window. Next bounded task: complete-or-
failure HTML resource policy with format/aggregate bounds, keeping current default
unsupported behavior until checked. Inline reading resources, hover filename
status text, full formats/corpus and GTK/Windows/native release gates remain.

## Implementation and scoped evidence

| Scope | Commits / contract | Local evidence |
|---|---|---|
| Complete retained generations | C++ `37d9004`, C `155e0cb`, Swift `d1a15df`; [retention](search-retention-workflows.md) | copied/stale/capacity/order/lifetime; complete means returned keys, not every module succeeded |
| Selected/all text | `3faadea` through `80589a4`; [text export](search-text-export-workflows.md) | same shared selection/SDK text; output limits, duplicates, explicit partial-source choice and state preservation |
| Native Mac text actions | `5d52a11` | selected/all save/copy, scripture choice, progress/Stop/stale/close model tests; original-prose window/save checked; isolated named pasteboard |
| Reference/source correctness | `c3218e6`, `0fffdbd`, `2666599`; [HTML sources](html-source-workflows.md) | malformed SDK UTF-8 rejects; real ThML counted-terminal-NUL correction; configured bounded body/CSS without publishing learned preferences |
| Restricted HTML service/adapters | C++ `1b327d9`, C `e7c141e`, Swift `e8a8ec8`; [HTML export](html-export-workflows.md) | normal and scoped ASan/UBSan/TSan, C11 and Swift6 ownership/cancel/stale/caps; unsupported assets/styles/markup refuse |
| Wrapper/traversal policy | `395c834` | 7 normal Mac / 6 ASan/UBSan / 6 TSan focused checks; depth128/node100,000 exact and one-beyond policy checks normal/sanitized; portable 6/6 |
| Resource helper reuse | `ff46f9b`; [reuse audit](resource-export-design.md) | normal/ASan/UBSan/TSan/portable focused 4/4 each, including fixtures and owned C transport; no new search-image support |
| Resource NUL paths | `7e9ff93` | normal 4/4, ASan/UBSan 3/3 and TSan 3/3 image/stylesheet checks, including prerequisite fixture; exact path and malformed directory/filename cases |

Native-disabled C++11 helpers and pedantic C11 header checks passed after the
HTML wrapper change. No whole parity group, release or legacy retirement is
accepted: 0/38 groups, 0/3 native apps. Overall historical estimate about 8%
(6–10%); no reliable core-only percentage follows from these counts.

## Actual local environment and commands

macOS 27.0.1 (26A434), ARM64; AppleClang21; Swift6.4 in Swift6 mode;
SWORD1.9.0.3916; GLib2.86.5; configured libxml2 SDK. Dependency/deployment minimum
26.0 is not validation of requested macOS13. See [build guide](../../platforms/macos/README.md).
Machine-specific ignored SDK seeds/caches are not portable configuration.

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel 2
ctest --test-dir build/macos --output-on-failure
cmake -S platforms/linux -B build/linux-models -C build/linux-model-sdk.cmake -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --output-on-failure
cmake -S src/core -B build/legacy-search-policy -DXIPHOS_BUILD_NATIVE_CORE=OFF -DCMAKE_CXX_STANDARD=11 -DCMAKE_CXX_STANDARD_REQUIRED=ON
cmake --build build/legacy-search-policy --parallel 2
clang -std=c11 -pedantic-errors -fsyntax-only -I src/core/include src/core/tests/c_header_test.c
```

Full Mac tests require desktop access for the isolated named-pasteboard check;
sandbox-only execution failed that check, and desktop-access execution passed.
This never reads or changes the user's general clipboard. Disposable native GUI
app/fixtures are copied under `/private/tmp` to avoid repeated Documents prompts;
quit them before deleting only those disposable copies. User libraries/privacy
settings remain untouched. Avoid concurrent fixture-mutating test suites.

Existing `build/sanitized` uses Debug C/C++ address/undefined instrumentation and
`build/thread-sanitized` uses separate thread instrumentation. Do not mix them.
SDK dependencies are partly instrumented; ASan leak checks are off. Example:

```sh
cmake --build build/sanitized --target core_book_image_resource_test core_book_stylesheet_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure -R 'core_book_image_resource|core_book_stylesheet'
cmake --build build/thread-sanitized --target core_book_image_resource_test core_book_stylesheet_test --parallel 2
ctest --test-dir build/thread-sanitized --output-on-failure -R 'core_book_image_resource|core_book_stylesheet'
```

HTML-focused checks build `core_html_document_policy_test`,
`core_search_html_export_test` and `core_search_html_export_c_test`, then run
`-R 'html_document_policy|search_html_export'`. Traversal-only checks use
`-R '^core_html_document_policy$'`. Fixtures are included in the recorded counts.

## Acceptance limits and correction record

The in-app browser rejected local-file HTML preview by URL protocol policy.
Do not work around the rejection through other URLs/surfaces. Static/core checks
are not visual HTML or CSP runtime acceptance. Native HTML destinations,
resources/module CSS, semantic actions, printing, full advanced/corpus workflows
and GTK/MSVC/current target runtime gates remain pending.

Indexed diagnostic `a3dcae8`: SDK creation status0 produced no usable framework.
Optional normal/ASan probes skip; required-index negative control exits1 as
expected. Configure `XIPHOS_REQUIRE_INDEXED_SEARCH_TEST=ON` on a capable SDK and
require a non-skipped pass. Controlled fake-framework tests validate context/
restoration, not actual indexed execution. Validate SDK ICASE flag2 per target.

The prematurely pushed capability assertion assumed VerseKey RawGenBook stayed
Generic Books; `76895f4` corrects it to observed Biblical Texts. Real ThML plain
filters count one terminal NUL inside SWBuf size; source/query/text guards now
exclude exactly that one terminator and reject remaining embedded NUL/invalid
UTF-8. Do not restore strict size-as-content or CString truncation assumptions.

Native search window checks covered eligibility, preview/main navigation, bounds,
last/case/commentary and double-click. Slow-corpus GUI Stop timing is unvalidated.
Earlier book PNG/click/Close/image-off, preferred CSS/fallback, cross-section note
numbering and selected-root preservation were scoped window checks; full input/
VoiceOver/corpus/native sizing and saved workflows remain pending. Detailed
chronology/commands remain in the contracts and [prior handoff](https://github.com/orbeavers14/xiphos/blob/7e9ff93/doc/modernization/handoff-macos.md).

No hosted jobs or reset credit used. Remote audit shows builds/native-core paused,
linux-core manual-only and latest jobs October15. Current Mac portable-model
checks are not new Fedora or native Windows acceptance. No legacy source retires.

## Verse image-source characterization

New `core_verse_image_source` creates isolated original RawText/RawCom ThML/OSIS
modules and checks SDK directory/source behavior, image-off/settings neutrality
and history. Normal/ASan/UBSan/TSan pass 1/1 each. Configure the existing three
build directories, build `core_verse_image_source_test`, then
`ctest --test-dir <build> --output-on-failure -R '^core_verse_image_source$'`
(with `ASAN_OPTIONS=detect_leaks=0` for address/undefined). This is characterization,
not new HTML-image or native/browser acceptance. Source policy details live in
[resource export design](resource-export-design.md).

## Owned verse image resource checkpoint

Owner-side C++ Reader/FIFO worker commands now return copied source identity,
MIME hints and bounded binary bytes. Existing book slash/path/file helpers are
reused; book family guard stays unchanged. Original fixtures now include a
Unicode/literal-percent/space native library root, regular files and contained/
escaping symlinks. Normal Mac, ASan/UBSan, TSan and portable-on-Mac focused suites
pass 5/5 each (source/resource + book image/style + fixture). Configure/build
`core_verse_image_source_test core_verse_image_resource_test core_book_image_resource_test core_book_stylesheet_test`,
then run `-R '^core_verse_image|^core_book_image_resource$|^core_book_stylesheet$'`.
This is source bytes, not HTML-image embedding or visual acceptance. See
[verse image contract](verse-image-workflows.md). No hosted jobs/reset credit used.

## C verse image transport

C++ source pushed at `2492d64`. Additive owned C11 transport passes normal,
ASan/UBSan and TSan 4/4 focused checks plus strict C11 syntax. Build targets
`core_verse_image_c_test core_verse_image_source_test core_verse_image_resource_test core_reader_test`
and run `-R '^core_verse_image|^core_reader$'` in each configured directory.
Input length guards precede copying/enqueue; owned results have take-once/null/
empty/binary/lifetime checks. See [contract](verse-image-workflows.md). No native
UI/HTML-image or whole-group acceptance. Swift copied values are next.

## Swift verse image values

C transport pushed at `4b3220f`. Swift6 adapter passes 7/7 focused checks including
existing book-image adapter and C/C++ source/resource cases plus fixtures. Build
`macos_verse_image_adapter_test macos_book_image_adapter_test core_verse_image_source_test`,
then run `-R '^swift_verse_image_adapter$|^swift_book_image_adapter$|^core_verse_image'`.
Data/source are copied before C release, with signed/NUL guards and authoritative
drain semantics. No URI/file policy or native image controls added in Swift.

## SDK verse image command sources

Swift source values pushed at `840faec`. New C++ Reader/FIFO command source shares
bounded image reads while separating query and URI decoding and retaining the
matched OSIS native directory. Actual generated links, declared-module neutrality
and copied lifetime pass normal/ASan/UBSan/TSan/portable focused 4/4 each. Build
`core_verse_image_source_test core_verse_image_resource_test core_book_image_resource_test`
and run `-R '^core_verse_image_source$|^core_verse_image_resource$|^core_book_image_resource$'`.
This is new SDK-command behavior; old book filename semantics remain unchanged.
Next copied command transport and typed reading-pane dispatch/native controls.

## Typed reading-pane image dispatch

Command source pushed at `3a1eeec`. Shared contextual classifier/follow/resolve
now returns a distinct image source result; existing C requests expose
`xc_request_take_verse_image` without a new frontend parser. Image hover is
no-presentation with no file read; legacy filename status text remains pending.
Normal, ASan/UBSan, TSan and portable-on-Mac focused suites pass 10/10 each. Build
`core_verse_image_resource_test core_verse_image_c_test core_content_test core_study_note_test core_hover_test core_study_passage_test`
and run `-R '^core_verse_image|^core_content$|^core_study_notes$|^core_hover$|^core_study_passage$'`.
Next Swift typed result and native macOS image viewer consumption with model/window
checks. Current Linux model safely reports unsupported image presentation; GTK
image UI/runtime and Windows remain pending. No whole parity group is accepted.

## Native reading image controls

Shared dispatch source pushed at `c4336f6`. Swift typed image result and native
Bible/commentary image sheet pass 9 focused checks plus the full suites above.
The book/native thumbnail/view code is shared in renamed `ImagePreview.swift`.
Model command: build `macos_reader_model_test macos_preview macos_verse_image_adapter_test macos_book_image_adapter_test core_verse_image_source_test`,
then run `-R '^reader_model_verse_images$|^reader_model_book_images$|^swift_verse_image_adapter$|^swift_book_image_adapter$|^core_verse_image'`.
Window check uses `sh platforms/macos/launch-fixture.sh build/macos verse-images`;
select ThMLText, Read John1:8, click image, Close, leave draft John1:7, choose
OSISNotes and click its John1:8 image, Escape, then click the invalid John1:1 image.
Labels, original PNG, retained source/draft and failure message checked. App quit
and only the disposable temp copy removed. No export-HTML browser workarounds.
