# macOS chat handoff

Updated 2026-10-21. Read [AGENTS.md](../../AGENTS.md), [shared handoff](handoff.md),
[status](status.md), [core scope](shared-core-roadmap.md) and relevant parity rows.
Integration branch: `master`, fork `orbeavers14/xiphos`. Sequential direct commits
and pushes are authorized; preserve unrelated changes, never force-push.

## Current checked source and active work

Implementation through core/C `4fecce8` and Swift `374147d`: retained search
generations; selected/all text exports and native Mac text controls; restricted
HTML with explicit module-owned static PNG embedding, aggregate limits and copied
image statistics; wrapper/traversal/resource guards and thin C/Swift adapters. One C++ FIFO SDK owner, copied values and original
GTK3 behavior/reference remain the architecture. No HTML controls were added.
The only unrelated local file is `.DS_Store`; leave it untouched.

Latest full Mac checkpoint: **164 passes + one explicit index skip** (165
registered, 137.38 s), with desktop access for isolated pasteboard checks. The
shared HTML image/resource selection passes 11/11 each normal, ASan/UBSan, TSan
and portable core on Mac, GTK runtime OFF. Swift image/default HTML selection
passes 8/8 including fixtures; C11 header syntax passes. Earlier full portable
model evidence remains 121 passes + one skip (122 registered, 92.46 s at
`b335db5`). No current Fedora/MSVC, whole parity or release acceptance.

Current task: the complete-or-failure HTML static PNG policy is implemented and
checked across C++/C/Swift. Default calls still reject resources. Next bounded
task: native HTML save/copy controls that consume the complete shared document,
then remaining resource/style/semantic and advanced-search workflows. Inline
reading resources, hover filename status, other formats/corpus, browser/CSP
runtime and GTK/Windows/native release gates remain.

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

## PNG preparation and dependency checkpoint

Native image controls are pushed at `b335db5`; its full160/122 registered suites
are the latest complete Mac/portable checks. New private static-PNG validator now
passes normal3/3 (codec/Swift image/source fixture), ASan/UBSan1/1, TSan1/1 and
portable-on-Mac1/1. Native-OFF C++11 configure/build passes with PNG discovery
explicitly disabled; shell syntax/C11 pass. No new full-suite claim here.
Use the [PNG contract](png-export-workflows.md) for exact local static SDK source,
SHA/license/deployment/provenance, limits and build commands. All native caches
select that SDK via `PNG_PNG_INCLUDE_DIR` / `PNG_LIBRARY_RELEASE`; source/static
archive live only under ignored build. PNG/zlib are new explicit native-core
requirements; target Linux/MSVC acceptance is pending. Next shared document
resource/cache policy, preserving current default rejection and no partial output.
No hosted jobs/reset credit or browser workaround used.

### Dependency inventory checkpoint (2026-10-20)

Added the tracked [dependency directory](../../dependencies/README.md) with
shared-core requirements, platform/reference-app dependencies, inspected local
SDK locations and a convention for future ignored source builds. Existing SDKs
were not moved. libpng implementation remains `5d540fc`; this checkpoint changes
documentation only. Checked local Markdown links and whitespace; no builds or
hosted jobs needed. This is an inventory, not a complete transitive lockfile or
release license audit. Continue shared HTML resource policy/cache work next.

### Private HTML image resource checkpoint (2026-10-20)

Implementation: `63ce703` on the fork's `master`. Added private per-document
module/URI image caching, bounded aggregate occurrence/raw/pixel/encoded limits,
typed file-byte exhaustion and optional complete-fragment PNG resolution. Full
passive DOM validation precedes image reads; repeated resource identities reuse
the first validated copy. The public search export request, CSP and C/Swift
transport remain unchanged and still reject images. See
[resource contract/evidence](resource-export-design.md).

Local Mac normal, ASan/UBSan, TSan and portable core/model configurations each
pass 8/8 focused checks. Existing C++/C search HTML export paths plus fixture
setup each pass 5/5 normal, ASan/UBSan and TSan. AppleClang21, macOS27 ARM64,
Swift6 toolchain, C++17 core; checked native SDK target26, SWORD1.9.0.3916,
GLib2.86.5, Apple libxml2/zlib and official-source libpng1.6.59 static SDK.
Portable tests ran on Mac with GTK runtime OFF, not Fedora/MSVC. Dependencies
are partly instrumented, ASan leaks off. No new whole parity/retirement or
release acceptance; no browser/CSP/visual export acceptance. No hosted jobs.

The owner requested finishing this workflow, pushing and stopping; stopped at
this checkpoint. Next: connect an explicit owner-side HTML document image policy
to the existing Reader source service, aggregate across rows, change CSP only
for opted-in documents, publish statistics only on complete success, then add
ABI-safe C/Swift options and native consumption. Keep default reject behavior
and the documented browser-policy restriction. Unrelated `.DS_Store` left alone.

### Shared HTML static PNG export checkpoint (2026-10-21)

Core/C implementation `4fecce8`; Swift adapter `374147d`; both pushed to fork
`master` after local checks. See the [contract](html-image-export-workflows.md)
for explicit policy/defaults, aggregate limits, actual row-module ownership,
successful-only counters and cancellation semantics. No existing C request/info
layout changed; C11 syntax and older C/Swift default paths remain checked.

Full local Mac build and tests pass: 164 passes + 1 explicit index skip, 165
registered, 137.38 s (`build/search-html-images-full-{build,tests}.log`). Existing
index framework remains unavailable; skip is not execution acceptance. Normal,
ASan/UBSan, TSan and portable core configurations each pass the focused 11/11
selection; Swift image/default HTML selection passes 8/8 including fixtures.
These are not new Fedora/MSVC or browser/CSP/visual checks. SDK dependencies
remain partly instrumented, ASan leaks off. Native HTML controls are not added.

Actual environment: macOS27.0.1 ARM64, AppleClang21/Swift6 mode, SWORD1.9.0.3916,
libpng1.6.59 official-source static SDK, Apple libxml2/zlib. Resumed pkg-config
discovery selects installed GLib2.90.1, superseding the older 2.86.5 fallback;
all four checked configurations now use2.90.1. Homebrew GLib/liblzma report
macOS27 minima despite the configured target26. Tests on this host establish
neither macOS26 nor13 compatibility. See the [dependency inventory](../../dependencies/README.md).
No package installation/move, hosted build, usage reset, user profile/module or
general clipboard modification occurred. Unrelated `.DS_Store` is untouched.

Next: add native HTML destinations through the existing shared export workflow,
without frontend rendering/resource logic or a workaround for the recorded
browser-policy block. Keep unsupported styles/formats/semantic actions explicit
and retain all whole-parity/legacy-retirement gates.
