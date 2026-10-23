# macOS chat handoff

Updated 2026-10-22. Read [AGENTS.md](../../AGENTS.md), [shared handoff](handoff.md),
[status](status.md), [core scope](shared-core-roadmap.md) and relevant parity rows.
Integration branch: `master`, fork `orbeavers14/xiphos`. Sequential direct commits
and pushes are authorized; preserve unrelated changes, never force-push.

## Current checked source and active work

Latest checked/pushed implementation is `2b2f518`, following `d0b068c`, `f0778e0`
and `0dbfff9`: explicit native saved-list editing, shared verified first-label
removal, owned C/Swift transport and native prefill/removal. Final combined full
Mac173 passes+1 explicit index skip (174 registered,149.80s); portable core/Linux
models on Mac130 passes+1 explicit index skip (131 registered,99.11s), GTK runtime
OFF. Scoped sanitizer/transport evidence is below. No whole parity acceptance.


Implementation through core/C `4fecce8` and Swift `374147d`: retained search
generations; selected/all text exports and native Mac text controls; restricted
HTML with explicit module-owned static PNG embedding, aggregate limits and copied
image statistics; wrapper/traversal/resource guards and thin C/Swift adapters. One C++ FIFO SDK owner, copied values and original
GTK3 behavior/reference remain the architecture. Native HTML destination controls now consume the same shared documents at `f89c585`.
The only unrelated local file is `.DS_Store`; leave it untouched.

Latest full Mac checkpoint at search activation `3b5ff5c`: **168 passes + one
explicit index skip** (169 registered, 123.07 s), with desktop access for isolated
pasteboard tests. This includes advanced modes, ordered batches, explicit partial
exports, annotation reset and actual source-module activation. Earlier scoped
sanitizer/portable evidence remains in linked contracts/checkpoints. No current
Fedora/MSVC, whole parity or release acceptance.

Current implementation: shared rich clipboard `b85c440` adds an optional bounded
plain-text companion to complete HTML on the same FIFO job/resolved rows. Native
Copy consumes both copied values; Save stays HTML-only. See
[contract](rich-clipboard-workflows.md) and latest checks below. Optional committed headers now have C/Swift/native consumption at `5831417`.
Next bounded work: remaining resource/style/semantic and advanced-search workflows. Inline
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

### Native HTML destination checkpoint (2026-10-21)

Implementation `f89c585`, pushed to fork `master`. Native Plain text/HTML format,
scripture and explicit PNG controls use one model lifecycle and the existing core
export service. NSSavePanel/atomic UTF-8 files and native HTML pasteboard values
consume copied complete documents. No frontend image/resource/rendering policy.
See [native contract/evidence](native-search-html-export-workflows.md).

Preview/export/search model builds pass. Final scoped selection 10/10 (14.94 s),
including fixtures, native HTML/text destinations, existing search and Swift
HTML transport; initial selection 9/9 (13.83 s). Exact isolated named clipboard
bytes, temporary atomic files, invalid destination/payload, original draft/query/
format capture and stale/Stop/close suppression are checked. Disposable OSISText
window: PNG search John1:8, native HTML save with scripture/PNG, 791-byte saved
UTF-8 document with one 75-byte PNG by static inspection, embedding-off resource
failure before picker. App quit and its sole disposable directory removed.
No rendered export preview or browser/CSP acceptance; no user's general clipboard,
profile/library, hosted workflow, dependency or language-standard changes.

This slice has focused/window evidence, not a new full Mac/Fedora/MSVC suite.
SDK/older-macOS constraints remain as recorded above. No whole parity/retirement
gate passed. Owner requested finishing this slice, pushing and stopping. Stop at
this checkpoint. Next shared slice: rich clipboard plain-text fallback, preserving
one owner/document snapshot rather than frontend HTML stripping or a second
selection/render engine; then advanced headers and broader resource/link policy.

### Shared rich clipboard checkpoint (2026-10-21)

Implementation `b85c440` on `modernization/rich-clipboard-fallback`; handed off
for reviewed integration. HTML Copy requests a bounded optional plain-text
companion through the same resolved selection/FIFO owner. One native pasteboard
item receives both exact copied representations. Save remains HTML-only.
Additive C11 functions/getters preserve existing request/info layouts. See
[contract](rich-clipboard-workflows.md); GTK3/Linux sources are unchanged.

Final full local Mac build succeeds; CTest **165 passes + 1 explicit index skip**
(166 registered, 141.38 s). This includes native named-pasteboard/file/model and
Swift copied-companion checks. Full tests used desktop access, never the general
clipboard. Focused ASan/UBSan checks pass **11/11** (27.39 s); portable core plus
all seven GTK model checks pass **26/26** (28.18 s) on this Mac, GTK runtime OFF.
Strict C11 header syntax and whitespace checks pass. No new GUI/rich-client paste,
Fedora/MSVC, actual indexed execution, whole parity or retirement acceptance.

Environment: macOS27.0.1 ARM64, AppleClang21, Swift6.4/Swift6 mode,
SWORD1.9.0.3916 static SDK, GLib2.90.1, libpng1.6.59. This fresh checkout uses
installed SDKs, not another chat's ignored build caches. Explicit fallback SDK
selection disables PkgConfig. Deployment target27 matches this host/dependencies;
older macOS versions remain unvalidated. Final Mac uses Apple SDK libxml2.9.13.
Sanitizer/portable focused checks used installed libxml2.15.4. Dependencies are
not fully instrumented; ASan leak detection is disabled.

Exact configure/build/test choices (standard installed SDK locations; generated
logs/cache stay ignored under build):

```sh
cmake -S platforms/macos -B build/macos -DCMAKE_BUILD_TYPE=Debug -DCMAKE_OSX_DEPLOYMENT_TARGET=27.0 \
  -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE -DSWORD_INCLUDE_DIR=/usr/local/include/sword \
  -DSWORD_LIBRARY=/usr/local/lib/libsword.a -DXIPHOS_SWORD_EXTRA_LIBRARIES='z;bz2;/opt/homebrew/lib/liblzma.dylib' \
  -DGLIB_INCLUDE_DIR=/opt/homebrew/include/glib-2.0 -DGLIB_CONFIG_INCLUDE_DIR=/opt/homebrew/lib/glib-2.0/include \
  -DGLIB_LIBRARY=/opt/homebrew/lib/libglib-2.0.dylib \
  -DLIBXML2_INCLUDE_DIR="$(xcrun --show-sdk-path)/usr/include/libxml2" \
  -DLIBXML2_LIBRARY="$(xcrun --show-sdk-path)/usr/lib/libxml2.tbd"
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --output-on-failure
clang -std=c11 -pedantic-errors -fsyntax-only -I src/core/include src/core/tests/c_header_test.c
```

`build/sanitized` uses the same SWORD/GLib/deployment selection, `-S src/core`,
Debug C/C++ flags `-fsanitize=address,undefined -fno-omit-frame-pointer` and linker
flag `-fsanitize=address,undefined`; XML is the installed2.15.4 SDK.
Build the seven export targets plus `core_attribute_search_test`,
`core_lexical_snapshots_test`, `core_verse_image_source_test` and
`core_heading_collection_test`, then run:

```sh
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure \
  -R 'search_html_export|search_html_images|search_export|search_all_export'
```

`build/linux-models` uses `-S platforms/linux` with the same fallback SDK flags,
installed XML2.15.4 and `-DXIPHOS_BUILD_GTK4_PREVIEW=OFF`; full build passes.
The focused command is:

```sh
ctest --test-dir build/linux-models --output-on-failure \
  -R 'search_html_export|search_html_images|search_export|search_all_export|linux_.*model'
```

Initial full run with automatically discovered XML2.15.4: **164 passes, 1 skip,
1 failure** in unchanged `core_xml_profile`; frozen and actual legacy functions
still agree exactly, but hard-coded expected results differ. A disposable diagnostic
found nested-only values return empty text rather than NULL, and malformed
`Raw & broken` returns `Raw  broken` rather than NULL. Original source/tests were
not relaxed. Final Apple SDK full suite passes. Newer XML compatibility needs a
separate characterization/preservation decision; passing focused2.15.4 checks is
not full dependency acceptance. Initial Homebrew SWORD selection also lacked
new search constants; use the documented newer SDK until compatibility is addressed.

Remote audit before publication: master remains at `a8c2a8b`, no open PRs;
`builds`/`native-core` disabled_manually and `linux-core` manual-only. No hosted
jobs dispatched, dependencies installed/moved, private modules/profiles committed
or user clipboard modified. Owner now authorizes continuing validated work and
merging/pushing checkpoints. Library upgrades are owner-deferred for a later review. Next bounded work:
advanced export headers and remaining resource/semantic policies, followed by
actual target-platform and representative workflow acceptance.

### Optional core search-context headers (2026-10-21)

Implementation `66196e3` on `modernization/shared-export-context-headers`.
Shared C++ text/all-text/HTML requests append an opt-in committed-context flag;
default output and existing C/Swift/native behavior are unchanged. One owner
copies headers once per emitting group; all formats/companions share metadata,
escaping, row order and final-byte budgets. No first match is consumed as a
header. Repeated groups and visible group reentry remain explicit. See
[contract](export-context-workflows.md) for scope and unimplemented sections.

Final complete Mac preview/core build passes; full CTest **165 passes + 1 explicit
index skip**, 166 registered, 147.84 s. Focused final ASan/UBSan **11/11** (35.07 s),
portable core/all seven GTK models **26/26** on Mac (28.48 s, GTK runtime OFF).
Additional final exact default-empty-document assertion passes with its fixture
**2/2** (7.33 s). Strict C11 and whitespace checks pass. Same SDK/cache selection
as the preceding rich clipboard checkpoint: Apple XML2.9.13 for full Mac;
installed XML2.15.4 for focused sanitizer/portable checks. Libraries, deployment,
workflow configuration and language standards did not change. Dependency upgrades
are owner-deferred; retain the preceding compatibility gate and index skip.

Commands: `cmake --build build/macos --parallel 4`, then desktop-access
`ctest --test-dir build/macos --output-on-failure`. Rebuild the same seven export
sanitizer targets, then run the preceding ASAN_OPTIONS/CTest export selection.
Portable: full `cmake --build build/linux-models --parallel 4`, then preceding
26-test export/Linux-model selection. Logs are ignored
`build/context-{full,final-sanitized,portable}-*.log`.

No new native header controls, GUI/rich-client paste, browser/CSP/visual,
localization, empty/failure group sections, Fedora/MSVC, complete advanced-search
or whole-parity acceptance. Existing C/Swift options cannot enable headers yet;
their default paths and GTK models were rebuilt/checked. Next bounded slice:
additive C11 options, copied Swift consumption and a native opt-in through the
existing export model lifecycle. Application policy remains in C++.

### C/Swift/native export context checkpoint (2026-10-21)

Implementation `5831417` on `modernization/native-export-context-headers`.
Additive C11 entry points copy a strict context flag into the existing shared
export requests; old entry points pass zero. C request/info layouts, payload
ownership and default byte output remain unchanged. Swift defaults off and copies
complete results. Native Export now has Include search details for text/HTML,
selected/all matches, through one existing model lifecycle. The choice is captured
before queueing; subsequent toggle/query/format changes cannot alter the request.
HTML Copy's companion uses the same context; Save remains HTML-only. No native
header formatter, metadata engine, storage engine or library upgrade was added.

Complete Mac preview/adapters/core build passes. Focused final export checks
**16/16** (22.87 s) include fixture setup, C/Swift and native named-pasteboard/
atomic-file/model checks. Existing native search/Swift search regressions **3/3**
(7.26 s, including fixture). ASan/UBSan export selection **11/11** (25.54 s).
Portable core/all seven GTK models **26/26** on Mac, GTK runtime OFF (30.17 s).
Strict C11 header syntax, documentation target and whitespace checks pass.
The immediately preceding full165-pass+1-skip checkpoint belongs to `66196e3`;
this thin transport/control slice has full compilation and focused runtime
coverage, not another complete-suite run. SDK/deployment, index skip and external
library instrumentation limitations remain unchanged. No Fedora/MSVC or new
physical-menu/window/visual/rich-client interoperability acceptance.

Commands use the same build directories/SDK selection recorded above:

```sh
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --output-on-failure \
  -R 'search_html_export|search_html_images|search_export|search_all_export'
ctest --test-dir build/macos --output-on-failure \
  -R '^reader_model_search$|^swift_search_adapter$'
cmake --build build/sanitized --target core_search_html_export_c_test core_search_html_images_c_test core_search_export_c_test --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure \
  -R 'search_html_export|search_html_images|search_export|search_all_export'
cmake --build build/linux-models --parallel 4
ctest --test-dir build/linux-models --output-on-failure \
  -R 'search_html_export|search_html_images|search_export|search_all_export|linux_.*model'
```

Native destination tests use desktop access and isolated named pasteboards/temp
files; the general clipboard and user data remain untouched. Logs are ignored
`build/native-context-*.log`. GTK3/Linux source, dependencies and workflows are
unchanged. No hosted runs were dispatched. Library upgrades remain explicitly
owner-deferred. Next: representative export/resource/semantic policies, richer
empty/failure group sections/localization, advanced search controls/settings and
independent target-platform acceptance. No whole parity or retirement gate passes.

### Native advanced single-module search controls (2026-10-22)

Implementation `d26aa2d`, branch `modernization/native-advanced-search-modes`.
All eight existing core modes are exposed in Match; optional search annotation
inputs copy into the existing typed request. Core retains phrase/last/bounds/
filter/backend behavior. Native status reports copied explicit index fallback.
No core/C ABI/Swift adapter/GTK3/Linux/workflow changes. The selected mode and
annotation inputs are captured before queueing; per-window context stays in core.

Commands: `cmake --build build/macos --target macos_preview macos_reader_model_test
macos_search_export_test --parallel 4` succeeds. Focused final desktop-access
CTest selection `-R 'reader_model_advanced_search|reader_model_search$|swift_attribute_search_adapter|swift_search_text_options|swift_search_batch|reader_model_search.*export'`
passes10/10 (17.21 s); initial narrower selection5/5 (13.73 s). Existing Mac SDK/
compiler/configuration unchanged. Logs: ignored `build/advanced-modes-*.log`.
Whitespace checks pass. No new full-suite, sanitizer, GUI physical-input, actual
indexed-execution, Fedora/MSVC or whole-parity acceptance. Tests use original
attribute/search fixtures; native destinations use isolated named pasteboards.

Next bounded task: consume ordered shared batch search in native module selection
and grouped results, retaining actual module/group/row identity through preview
and export. Do not loop independent searches or recreate scope chaining in Swift.

### Native ordered batch search checkpoint (2026-10-22)

Implementation `c17206e`, branch `modernization/native-batch-search`.
An explicit multiple-module mode offers ordered add/remove/move controls and
retains duplicates. One existing shared batch call handles all requests. Copied
results are grouped by original request, including failures/empty/unattempted
status. Hits retain exact module/group/row; preview and selected export consume
those identities. All export uses the complete retained generation, captured
query and existing incomplete-source refusal. Single-mode defaults remain.

Build `macos_preview macos_reader_model_test macos_search_export_test` in
`build/macos` with `--parallel 4`: pass. Initial selection
`ctest --test-dir build/macos --output-on-failure -R '^reader_model_batch_search$|^reader_model_advanced_search$|^reader_model_search$'`
passes5/5 (13.90 s, including fixtures). Final desktop-access complete
`ctest --test-dir build/macos --output-on-failure` passes **167 +1 index skip**,
168 registered, 142.55 s. Whitespace/link checks pass. Existing SDK/compiler/
configuration unchanged. Logs: ignored `build/batch-{model,full}-*.log`.
No shared core/ABI/adapter/GTK3/Linux/workflow source changes; no hosted jobs.

Checks cover order/duplicates, failed-group gaps, original drafts, actual-source
preview/selected rich export, complete returned export, stale refusal, core last
scope chaining/bounds, all-failed replacement, single-mode return and close guards.
Only original fixture content, temporary files and isolated named pasteboards are
used. No new sanitizer, physical native keyboard/menu/accessibility, actual index,
corpus/Fedora/MSVC or complete parity acceptance. Next: explicit partial-source
export choice and remaining advanced setup/saved workflows/resource policies.

### Disclosed incomplete-source export checkpoint (2026-10-22)

Implementation `667f9b7`, branch `modernization/native-partial-search-export`.
Native all-result export still refuses incomplete sources by default. An explicit
conditional toggle requires Include search details so existing shared text/HTML
warnings travel with output. The captured flag passes to the existing core API;
new committed single/batch results reset the choice. Selection export, source
identity and all stale/capacity/resource/output/cancellation checks remain shared.
No core/ABI/adapter or GTK3/Linux source changes.

Preview/model/export builds pass. Final desktop-access
`ctest --test-dir build/macos --output-on-failure -R 'reader_model_batch_search|reader_model_advanced_search|reader_model_search$|swift_attribute_search_adapter|swift_search_text_options|swift_search_batch|reader_model_search.*export'`
passes11/11 (19.88 s, fixtures included). Whitespace/link checks pass. Logs:
ignored `build/partial-export-{final-build,final-tests}.log`. Original-prose model
checks cover explicit paired/complete text acceptance and source warnings, exact
named-pasteboard/temp-file bytes, missing-details refusal, changed-choice capture,
new-generation reset, stale refusal and retention-capacity refusal. Earlier narrow
non-destination selection5/5 (14.90 s) is preliminary, not final destination evidence.

The preceding full167-pass+1-index-skip suite belongs to `c17206e`; this focused
native choice slice has no new full-suite/sanitizer, physical-input, actual-index,
corpus or Fedora/MSVC acceptance. Shared SDK/compiler/configuration unchanged.
No hosted jobs or user-data/general-clipboard changes. Next bounded work:
explicit search-annotation-context reset, complete saved/advanced setup and
remaining resource/semantic policies. No whole parity or retirement acceptance.

### Search annotation context reset checkpoint (2026-10-22)

Implementation `119e943`, branch `modernization/native-search-context-reset`.
Reset search annotations submits the existing per-tab clear command through the
shared FIFO worker. Native override inputs reset only on success. Scope, visible
and retained results, previews, original export provenance, current reading
preferences and other tabs' search context remain unchanged. Failed/closed
operations retain input/publication guards; the UI stays nonblocking. No shared
core/ABI/adapter/GTK3/Linux or workflow changes.

Preview/model/export builds pass. Final desktop-access
`ctest --test-dir build/macos --output-on-failure -R 'reader_model_batch_search|reader_model_advanced_search|reader_model_search$|swift_attribute_search_adapter|swift_search_text_options|swift_search_batch|reader_model_search.*export'`
passes11/11 (20.64 s). Whitespace/link checks pass. Logs: ignored
`build/search-reset-{build,tests}.log`. Tests seed an independent tab's context
and active reading preferences, verify old snapshot/header/scope/draft retention,
check the next phrase's cleared context, and exercise missing-tab failure and
pending-close suppression. Destinations remain isolated named pasteboards/temp
files; user data/general clipboard are untouched.

The latest complete167-pass+1-index-skip suite remains the batch checkpoint
`c17206e`. This focused native reset slice is not a new full-suite/sanitizer,
physical-control, actual-index, corpus/Fedora/MSVC or whole-parity claim.
Next bounded work: complete saved/advanced search setup, resource/semantic policy,
and independent target-platform/physical-input acceptance. Owner post-parity
priorities are in the shared handoff. No hosted builds were dispatched.

### Source-preserving search activation / full integration (2026-10-22)

Implementation `3b5ff5c`, branch `modernization/search-result-activation`.
Inspected legacy activation sends source module+key through its sword URI and
routes Bible/commentary panes by source. Native SearchPane formerly sent only a
reference to `chooseReference`, reusing the displayed translation. Both explicit
Read and double-click callbacks now pass the owned SearchHit. ReaderModel uses
existing shared paired routing, or the established standalone read path. No new
URI parser, module family engine or core/C ABI/adapter/GTK3/Linux changes.

Preview/model/export builds pass. Focused desktop-access
`ctest --test-dir build/macos --output-on-failure -R 'reader_model_search_activation|reader_model_batch_search|reader_model_advanced_search|reader_model_search$|reader_model_commentary|reader_model_search.*export'`
passes11/11 (24.43 s, fixtures included). Final complete desktop-access
`ctest --test-dir build/macos --output-on-failure` passes **168 +1 explicit index
skip**,169 registered,123.07 s. Link/whitespace checks pass. Logs: ignored
`build/search-activation-{build,tests}.log` and
`build/search-integration-full-tests.log`. SDK/compiler/configuration unchanged;
no new sanitizer, actual-index, physical input, standalone-only corpus,
alternate-versification or Fedora/MSVC acceptance.

Original two-Bible/two-commentary fixtures verify the exact searched translation
and commentary selections at one reference, paired Bible retention, another
window's unchanged content, retained search generation/query draft, missing-module
failure retention and closed-host guards. Existing reference-only history
deduplication is unchanged. Native destination tests in the full suite use only
named pasteboards/temp files. User data/general clipboard and workflows remain
untouched; no hosted jobs. Next: complete saved/advanced search setup and remaining
resource/semantic/corpus/platform acceptance. Owner enhancement priorities stay
after parity; no whole parity or legacy retirement gate passes.

### Shared export source counts (2026-10-22)

Implementation `d54aec4d`, branch `modernization/search-export-source-counts`.
Shared opt-in headers now distinguish captured SDK result elements, visible
reference count and preview limitation from exported row counts. Complete output
can include groups whose original preview had zero rows; selected duplicates
retain original source counts. HTML/plain companions use the same copied header.
No public API/layout or frontend formatter changes; defaults retain exact bytes.

Existing macOS27 ARM64 / AppleClang21 / Swift6.4 / SWORD1.9.0.3916 configuration.
`cmake --build build/macos --parallel 4` and sanitized build pass.
`ctest --test-dir build/macos --output-on-failure -R 'search_html_export|search_html_images|search_export|search_all_export'`
passes14/16 under restricted access; two named-pasteboard destination tests fail
with the known desktop clipboard restriction. Desktop-access rerun
`ctest --test-dir build/macos --output-on-failure -R 'reader_model_search_html_export|reader_model_search_export'`
passes4/4 including fixtures (6.56 s). The remaining14 normal checks passed.
`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure -R 'search_html_export|search_html_images|search_export|search_all_export'`
passes11/11 (28.37 s). Whitespace checks pass. Logs are ignored
`build/source-counts-*.log`. Dependencies remain partly instrumented; leaks off.
No new full suite, actual index, Fedora/MSVC, corpus, physical UI or parity claim.
Next: source disposition for empty/failed/unattempted groups in shared context
exports, then saved search setup and remaining resource policies. No hosted jobs.

### Shared source-group dispositions (2026-10-22)

Implementation `e2e53ee1`, branch `modernization/search-source-dispositions`.
Context-enabled exports now prepare one copied source summary with every original
request index/module/query and its completed, stopped, empty-scope, typed-failure
or unattempted disposition. Successful empty groups report zero SDK elements.
The summary survives zero-row exports, shares one resolved document between
HTML/plain and counts against final byte caps. HTML escapes all query metadata.
Defaults, result-row identity/counts, source acceptance and public layouts stay
unchanged. C/Swift/native consumers receive it through existing opt-in APIs.

Existing Mac27 ARM64 / AppleClang21 / Swift6.4 / SWORD1.9.0.3916 setup.
`cmake --build build/macos --parallel 4` passes; full desktop-access
`ctest --test-dir build/macos --output-on-failure` passes **168 +1 explicit
index skip**,169 registered,150.14 s. Sanitized build plus
`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure -R 'search_html_export|search_html_images|search_export|search_all_export|core_search_generation$'`
passes12/12 (52.22 s). Portable build plus
`ctest --test-dir build/linux-models --output-on-failure -R 'search_html_export|search_html_images|search_export|search_all_export|core_search_generation$|linux_.*model'`
passes27/27 (33.04 s), including all GTK models on Mac with GTK runtime OFF.
Strict C11 syntax and whitespace checks pass. Logs: ignored
`build/source-dispositions-*.log`. Named clipboard fixtures remain isolated;
no user profiles/general clipboard or hosted workflows changed.

Cases cover every no-row disposition and original ordered identity without SDK
reads, real missing/empty groups, escaped failed-query metadata, zero-row rich
output and exact/short final byte budgets, default formatting and native consumers.
No current Fedora/MSVC, real index, corpus, browser/physical-input or parity claim.
Next: read-only shared legacy named search-list import and saved setup, then
remaining resource/semantic policies and independent target-platform acceptance.

### Shared legacy named search-list import (2026-10-22)

Implementation `4ca44370`, branch `modernization/saved-search-lists-core`.
Read-only C++ profile/worker APIs own raw module lists/ranges, preserve ordered
first-section/first-label behavior and missing/empty attributes, and implement
bounded legacy comma splitting using the shared module-input validator. No
profile mutation, implicit module resolution, second worker or frontend engine.
See [contract and next consumers](saved-search-lists-workflows.md).

Mac27 ARM64 / AppleClang21 / Swift6.4 / SWORD1.9.0.3916 configuration unchanged.
`cmake -S platforms/macos -B build/macos` and complete integrated build pass.
Focused rebuilt targets plus
`ctest --test-dir build/macos --output-on-failure -R '^core_(search_lists_profile|profile_reader|book_profile|preview_profile|profile_save|search_query|search_batch|xml_profile)$'`
pass9/9 (12.59 s, includes attribute fixture). Same focused rebuilt targets in
`build/sanitized`, with `ASAN_OPTIONS=detect_leaks=0`, pass9/9 (28.34 s).
`cmake -S platforms/linux -B build/linux-models`, matching focused targets and
same CTest selection pass9/9 (13.64 s), portable on Mac/GTK runtime OFF.
Whitespace checks pass. Logs: ignored `build/search-lists-*.log`.
An initial new test expected out-of-range for a closed tab; it now checks the
existing invalid-argument contract and all final runs above pass. Leaks remain
off because dependencies/retained characterization helpers are partly uninstrumented.
No new full-suite, native UI, real-index, corpus or Fedora/MSVC acceptance.
Next: copied C/Swift transport and native named-list/range consumption through
these shared services; profile writes and complete advanced setup stay separate.
No hosted builds or private profiles/modules used.

### Saved search-list C/Swift transport (2026-10-22)

Implementation `80e8a3c4`, branch `modernization/saved-search-list-adapters`.
Additive queued C read/tokenization operations transfer owned typed handles,
borrow optional attributes safely and expose shared first-label effective indices.
Swift6 copies Sendable/Equatable arrays and uses the shared tokenizer. No C
layout changes or frontend parser; no profile/search/history mutation.
Module-list text now has an explicit16-MiB cap, including pre-copy C validation.

Same Mac/SDK/compiler environment. Complete integrated Mac build and focused
builds pass.
`ctest --test-dir build/macos --output-on-failure -R '^(core_search_lists_.*|swift_profile_adapter|core_dictionary)$'`
pass4/4 (1.58 s, dictionary fixture included). Sanitized target builds plus
`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure -R '^core_search_lists_.*'`
pass2/2 (1.11 s). Portable target builds and same CTest selection pass2/2
(0.49 s), on Mac/GTK runtime OFF. Strict C11 header syntax and whitespace pass.
Logs: ignored `build/search-lists-adapters-*.log`. ASan leaks remain off.

C checks include pre-poll/wrong-kind/take-once, absent/empty fields, invalid
kind/row/null clearing, duplicate shadowing, empty/malformed/oversized names,
missing/bad files, copied submission strings, closed tabs and shutdown draining.
Swift checks copied UTF-8/optional/effective values, shared token order/duplicates,
invalid text/file/closed-tab failures and post-shutdown values alongside existing
lexicon save/import isolation checks. No new full suite, UI/physical input,
Fedora/MSVC, real index or corpus/parity acceptance. Next: native explicit profile
picker and saved module/range selection using these services; writes stay separate.
No hosted jobs or private modules/profiles used.

### Native saved search-list/range consumption (2026-10-22)

Implementation `8f6bacc1`, branch `modernization/native-saved-search-lists`.
Search now explicitly loads named choices from a selected profile without applying
any draft automatically. Menu selections use copied core effective indices and
queued shared module parsing. Module order/duplicates/unavailable names survive;
empty lists cannot become all-module searches. Saved ranges retain the chosen
bounds provider and use existing core parsing when Search runs. No new XML,
first-label, comma, search or persistence engine in Swift.

Preview/model builds pass (existing onChange deprecation warnings remain).
Focused8 checks initially pass7 with the known restricted named-pasteboard failure
in reader_model_batch_search. Final complete desktop-access
`ctest --test-dir build/macos --output-on-failure` passes **171 +1 explicit
index skip**,172 registered,145.28 s. Logs: ignored
`build/native-search-lists-{configure,build,tests,full-tests}.log`.
Same Mac27 ARM64/AppleClang21/Swift6.4/SWORD1.9.0.3916 environment. No new core
source/ABI or sanitizer changes in this native-only slice; preceding normal,
sanitizer and portable evidence remains separately recorded.

New model checks cover no automatic application, draft/result/preview/scope and
other-window retention, malformed replacement, core duplicate-label shadows,
overflow/missing/empty/literal-space choices, explicit bounds provider, actual
ordered partial batch execution, source-file byte preservation, busy selection
exclusion and pending-close/missing-tab suppression. Generated temp profiles only.
A fixture-only disposable app was opened via launch-fixture.sh in search mode.
AX UI automation opened the Saved search lists menu and native file chooser,
loaded an original generated profile, chose its second duplicate label, verified
Text/Commentary/Text order, applied John1:2 bounds with Text provider, and searched
Original:3 passages across3 requests, each John1:2, with the source preview shown.
A prior café query correctly returned zero within that bound. Tested window was
closed; other existing windows were left alone. This is scoped menu/picker/action
checking, not whole graphical, physical-input/accessibility/corpus or parity
acceptance. Named test clipboards remain isolated; no installed/private modules,
user profiles, general clipboard or hosted workflows changed.

Next shared work: explicit saved-list editing/atomic persistence with preserved
unknown XML, remaining advanced setup/resource semantics and independent
Fedora/MSVC/index/corpus acceptance. No whole parity or retirement gate passes.

### Shared saved-list editing / verified persistence (2026-10-22)

Implementation `2e599e78`, branch `modernization/saved-search-list-persistence`.
Typed C++ edits update/create the first named module/range item and preserve
unrelated XML, attributes/children/comments and duplicate shadows. Selected-file
saves reuse bounded staging/readback/permission/replacement policy and return the
copied stage snapshot prepared before replacement. Worker copies all inputs,
checks the tab before mutation and drains accepted saves. No automatic profile
write, second SDK owner or frontend persistence policy.

Mac/SDK/compiler configuration unchanged. Complete integrated Mac build passes.
Focused rebuilt targets and
`ctest --test-dir build/macos --output-on-failure -R '^core_(search_list.*|profile_save|xml_profile)$'`
pass5/5 (1.09 s). Final matching ASan/UBSan targets and selection with
`ASAN_OPTIONS=detect_leaks=0` pass5/5 (1.76 s); portable-on-Mac targets/selection
pass5/5 (0.81 s), GTK runtime OFF. Whitespace checks pass. Logs: ignored
`build/search-list-save-*.log`. The last full suite remains171 passes+1 skip
through native setup; this slice adds one registered core save test, not a new
full-suite claim. Dependencies/legacy characterization remain partly instrumented;
ASan leaks off.

Tests compile the actual retained writer as C11, compare valid edits exactly,
exercise empty/duplicate/Unicode/escaped attributes, unknown XML/children/comments,
new/private files, source encoding, existing permissions/symlinks, malformed and
unrepresentable data, invalid paths/read-only files, collection overflow,
prepared snapshot identity, closed-tab retention, dropped observers/shutdown and
stage cleanup. No C ABI, native write UI, current Fedora/MSVC, broad corpus,
external-writer locking or whole parity acceptance. Next: additive C/Swift save
transport and explicit native editing over this same shared service. No hosted
builds or real user profiles were used.

### Verified saved-list C/Swift save transport (2026-10-22)

Implementation `c11b15b`, pushed sequentially to fork `master`. Additive C typed
edits and Swift6 copied values reuse the existing verified core writer. Both C
observer and result handle allocate before acceptance; poll moves the staged
snapshot into the owned result without allocation. No second XML/duplicate-label
policy, automatic profile write or new core owner. See
[saved-list transport contract](saved-search-lists-workflows.md).

Local normal selection6/6 including Swift profile/dictionary fixture; ASan/UBSan,
TSan and portable core-on-Mac selections each6/6. C11 header syntax and whitespace
pass. Existing Apple XML2.9.13/GLib2.90.1/SWORD1.9.0.3916 setup retained; SDKs
partly instrumented, ASan leaks off. Exact copied inputs, Unicode/empty values,
duplicate shadows, unrelated XML, strict input caps, rejected file preservation,
closed tabs, detached observers/cancelled Swift observations and drained shutdown
are checked. Source/results remain isolated original fixtures. No new full suite,
Fedora/MSVC/native write UI or whole parity acceptance.

The owner then requested integrating `linux/native-parity`; combined core/Mac/
portable checks are recorded in the following integration checkpoint. Explicit
native editing over this transport remains the next saved-list slice.


## Linux parity integration (2026-10-22)

Owner requested merging `linux/native-parity` into the integration branch,
`master`. Inputs: master `c11b15b` (verified saved-list C/Swift write transport)
and Linux tip `f0b6e57` (implementation `3f9a068`). The source merge is clean.
One build compatibility correction was required: newer SWORD class search
constants have out-of-line definitions, so the private adapter must use `const`
rather than `constexpr`. The released-SDK macro/numeric path stays unchanged.
No public API or language-standard change is introduced by that correction.

Checks actually run on the combined tree, sequentially:

```sh
cmake --build build/macos --parallel 2
ctest --test-dir build/macos --output-on-failure
cmake -S platforms/linux -B build/linux-models -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --output-on-failure
```

Full Mac suite: **173 passes + 1 explicit index skip**, 174 registered, 147.56 s.
Full portable core/Linux models on Mac: **130 passes + 1 explicit index skip**,
131 registered, 98.12 s. Both commands exit0; GTK desktop is OFF in the latter.
Environment: macOS27.0.1 ARM64, AppleClang21, Swift6.4/Swift6 mode,
SWORD1.9.0.3916, GLib2.90.1, Apple XML2.9.13 headers and source-pinned PNG1.6.59.
Logs: ignored `build/linux-parity-merge-{macos,portable}-{build,tests}.log` and
`build/linux-parity-merge-portable-configure.log`. Diff/Markdown link checks pass.

Fedora's recorded **130/130** normal checks include indexed execution with its
released SDK. This Mac run does not repeat that acceptance. Fedora's installed
SWORD sanitizer overlap and lack of an accessible GTK display remain open; see
[the Linux handoff](handoff-linux.md). No new GTK/WinUI desktop, visual, corpus,
accessibility or whole-feature acceptance follows. No hosted builds, workflow
changes, branch deletion or legacy retirement. Keep unrelated local files.
Next bounded core-consuming slice: explicit native saved-list editing through
`c11b15b`, with chooser cancellation, returned-snapshot and closed-window checks.


## Native saved-list editor (2026-10-22)

Implementation `d0b068c` (pushed fork `master`). Task/branch: sequential `master`, consuming the existing verified shared save
transport. Native sheet accepts explicit kind, label and raw value; a native save
chooser selects an existing profile or a new destination. Only the returned
verified snapshot replaces available choices. Search draft, results, preview,
scope and other windows remain unchanged; no automatic choice application.

Local build passes with desktop/compiler-plugin access (restricted Swift macro
execution initially failed before compilation). Focused selection:
`ctest --test-dir build/macos --output-on-failure -R 'reader_model_saved_search_lists|core_search_list|swift_profile|profile_adapter'`
passes8/8,6.97s including generated fixtures. Model checks add successful/failed
save isolation, preserved duplicate shadows, busy exclusion and closed-window/
missing-tab rejection. Full regression `ctest --test-dir build/macos --output-on-failure` passes173
plus1 explicit index skip,174 registered,125.66s with isolated desktop pasteboards.
Final test-only extension adds range-save isolation, byte preservation during
application and pending-close publication suppression; rebuilt model/focused8/8
passes. Logs: ignored `build/native-list-edit-{build,tests,full-tests,final-build,final-tests}.log`.
Environment remains Mac27 ARM64/AppleClang21/Swift6.4/SWORD1.9.0.3916,
Apple XML2.9.13/GLib2.90.1 and pinned PNG1.6.59. No shared source/ABI changes
require a new sanitizer or portable build for this frontend-only slice.

Disposable original-prose search window: native editor and chooser opened;
chooser Cancel returned with unchanged empty draft/results/status. A Unicode
label `Original café` with `Text,Commentary,Text` saved successfully and appeared
in the menu without applying. Static bytes match those literal values. Save-panel
name-field automation converted the requested absolute name to a colon-delimited
filename in the already selected disposable fixture directory; no user profile
was accessed. Tested window closed; other existing windows left alone.
No browser export preview, hosted workflow, new ABI/core policy, Fedora/MSVC,
whole visual/workflow parity or retirement acceptance. No completion score change.

Next bounded work: existing-choice prefill/list management over shared effective
indices, then remaining saved setup and shared advanced search/resource policies.


### Shared saved-list removal (2026-10-22)

Implementation `f0778e0` (pushed fork `master`). Sequential `master`: extracting retained `xml_remove_node` first-section/first-
label behavior into explicit C++ serialization, verified staged file replacement
and the existing FIFO worker. Later duplicate values become effective; unrelated
XML survives. Missing items are no-ops, missing profiles reject without creation.
No implicit search application. Existing C/Swift callers remain compatible;
additive removal transport/native action is subsequent work. C++ normal, ASan/UBSan and portable-on-Mac selections pass4/4 each (0.37s,
1.38s,0.66s respectively); GTK runtime OFF, sanitizer leaks OFF, dependencies
partly instrumented. Commands: build `core_search_list_save_test`,
`core_search_lists_profile_test`, `core_profile_save_test`, `core_xml_profile_test`
in `build/macos`, `build/sanitized`, `build/linux-models`, then
`ctest --test-dir <dir> --output-on-failure -R '^core_(search_list_save|search_lists_profile|profile_save|xml_profile)$'`
with `ASAN_OPTIONS=detect_leaks=0` on sanitized. The retained removal helper is
compiled as C11; no language/API layout increase. Logs: ignored
`build/search-list-removal-{build,tests,asan-build,asan-tests,portable-build,portable-tests}.log`.
Same documented SDK environment; no new full suite or desktop/corpus/Fedora/MSVC
acceptance. Next: additive preallocated C/Swift transport and explicit native
removal/prefill; preserve current draft/results and duplicate-shadow semantics.


### Saved-list removal C/Swift transport (2026-10-22)

Implementation `0dbfff9` (pushed fork `master`). Additive `xc_worker_remove_search_list` and Swift `CoreWorker.removeSearchList`
reuse the same preallocated observer/result and owned snapshot channel as saves.
Inputs copy before submission, kinds/path/labels are bounded, and poll moves the
verified core snapshot without allocation. Accepted removals drain through
observer cancellation and shutdown; no frontend XML or duplicate policy.
Complete Mac build passes. Focused normal8/8 (3.19s, Swift profile adapter and
original dictionary fixture included); ASan/UBSan6/6 (2.42s), TSan6/6 (3.55s),
portable-on-Mac6/6. Commands: build the six search-list/profile/XML core targets
used above, including `core_search_list_save_c_test` and `core_search_lists_c_test`;
run `ctest --test-dir <dir> --output-on-failure -R '^core_(search_list.*|profile_save|xml_profile)$'`,
adding `|^swift_profile_adapter$` for normal Mac. ASan leaks OFF, GTK runtime OFF,
SDKs partly instrumented. Strict C11 header and whitespace checks pass.
Logs: ignored `build/search-list-removal-adapters-*`. No new full suite,
Fedora/MSVC/desktop removal or whole-feature acceptance. Next: native explicit
removal and existing-choice prefill, retaining drafts/results/closed-window guards.


### Native saved-list management (2026-10-22)

Implementation `2b2f518` (pushed fork `master`). Sequential `master`: Manage named lists sheet now prefills through core-provided
effective indices and offers explicit first-label removal from a selected existing
profile. Successful snapshots alone replace choices; no current draft/results/
preview/scope application. Missing-value choices cannot silently prefill empty.
Model checks cover duplicate prefill, removal exposure, busy exclusion, unchanged
search/other windows, malformed-file retention and pending/closed-window guards.
Local preview/model build and focused8/8 (8.19s) pass. Full Mac regression `ctest --test-dir build/macos --output-on-failure` passes
173 +1 explicit index skip,174 registered,149.80s with isolated desktop pasteboard
access. This is the combined removal-core/transport/native tree. Logs: ignored
`build/native-list-management-{build,tests,full-tests}.log`. Same documented
Mac27 ARM64/AppleClang21/Swift6.4/SWORD1.9.0.3916 environment.

Disposable fixture UI: loaded original named profile, selected second duplicate
in prefill menu, verified first value `Text,Commentary,Text` and Unicode label in
fields. Opened Remove chooser and cancelled; current draft/results/status and
profile bytes unchanged. Also cancelled an incidental Save chooser. Tested window
closed, other windows left alone. Actual removal file mutation is covered in
model/core/transport tests; no GUI removal completion, full physical input,
accessibility, corpus or whole parity acceptance is claimed.


### Continuation boundary and next work (2026-10-22)

Four checked implementation checkpoints are pushed sequentially to fork `master`:
`d0b068c` native editing; `f0778e0` core removal; `0dbfff9` owned C/Swift removal;
`2b2f518` native prefill/removal. Current work has no second SDK owner or frontend
application-policy engine. Unrelated `.DS_Store` stays untracked. Hosted workflows,
credits/reset credits, user profiles and installed module contents were untouched.

Remaining usage budget is being reserved for final checks and a clean pushed
handoff. Next bounded shared work is module abbreviation registration/resolution:
see the [inspected-source audit](saved-search-lists-workflows.md#next-shared-seam-module-abbreviation-resolution-source-audit-2026-10-22).
Installed-ID collision exclusion, parenthesis filtering, duplicate registration,
SDK order/reload and post-range-setup resolution must be checked together. Current
raw saved tokens and requested-source metadata must remain intact until explicit
execution resolution. Full saved-query/setup, corpus/locale/index and independent
Fedora/MSVC/desktop acceptance remain open. No complete parity/retirement gate
or evidence-based completion-percentage change follows.


Final integrated portable check at `2b2f518`:
`cmake --build build/linux-models --parallel 2` and
`ctest --test-dir build/linux-models --output-on-failure` pass130 +1 explicit index
skip,131 registered,99.11s. Logs: ignored
`build/native-list-management-portable-{build,tests}.log`. This is Mac-hosted
portable/GTK-model evidence with GTK runtime OFF; it does not replace Fedora's
recorded indexed execution or validate its outstanding installed-SWORD sanitizer
failure/desktop gates. The two disposable preview processes launched in this
continuation were stopped after their tested windows closed; older instances
were left alone. No build/test process remains intentionally running.

Stopping at96% of the5-hour window (78% weekly) to retain a clean handoff reserve;
no reset/purchase used. All four implementation commits are already pushed.
Final documentation checks cover whitespace and local Markdown link targets.


## Abbreviation policy continuation (2026-10-22)

Implementation `9c2153a` (pushed fork `master`). Sequential fork `master`, after fresh fetch at `a112980`; unrelated `.DS_Store`
preserved. Extract exact map registration/lookup into C++11 shared value policy,
with retained GTK wrappers consuming it. Frozen baseline routines and actual
wrapper extraction compile in a strict C++11 test. Native catalog registration,
installed-ID collision exclusion and search resolution will follow separately;
no native behavior or corpus/platform parity is claimed by the pure-map slice.
Local checks pass1/1 each: strict C++11 normal Mac0.10s, ASan/UBSan0.49s,
portable-on-Mac0.06s, native-disabled C++11 0.05s. Commands: configure each
existing build (`platforms/macos`, standalone `src/core` sanitized, `platforms/linux`
GTK runtime OFF); build `core_module_abbreviations_test`; run
`ctest --test-dir <dir> --output-on-failure -R '^core_module_abbreviations$'`.
Native-disabled configuration explicitly enables BUILD_TESTING and C++11.
ASan uses `ASAN_OPTIONS=detect_leaks=0`; the test has no SDK/GLib dependency.
Logs: ignored `build/module-abbreviations-policy-*`. Whitespace passes. No new
full-suite/native behavior, full GTK3 application/desktop, Fedora/MSVC or parity
acceptance. Same documented Mac/AppleClang environment. See
[registry contract](module-abbreviations-workflows.md). Next: staged native
catalog registration and copied resolution on the same owner.
No hosted workflow/reset/purchase.


### Owned native module resolution (2026-10-22)

Implementation `3f88cfc` (pushed fork `master`). Sequential `master`: read-only Reader/worker resolution lazily stages bounded
copied SDK alias catalog metadata, preserves exact installed IDs and original
requested tokens, and publishes only a complete registry. Existing Reader owns
all SDK access; no current search/history/scope/preferences/profile mutation.
Exact IDs/ordinary reads remain usable when unrelated alias metadata rejects.
Normal Mac, ASan/UBSan and portable-on-Mac checks each pass2/2 (normal0.83s,
ASan1.74s; portable result in ignored log). Targets:
`core_module_resolution_test core_module_abbreviations_test`; selection:
`ctest --test-dir <dir> --output-on-failure -R '^core_module_(resolution|abbreviations)$'`.
Existing `build/macos`, `build/sanitized`, `build/linux-models` configurations;
ASan leaks OFF, GTK runtime OFF, SDKs partly instrumented. Logs: ignored
`build/module-resolution-*`. Same documented Mac/SDK environment. Exact budget
and overflow, actual retained catalog block/frozen registry, SDK order, duplicate
aliases, Unicode/empty/absent metadata, copied submission/teardown, closed tabs,
shutdown and ordinary read isolation pass. No new full suite, C ABI/Swift/native
consumer, Fedora/MSVC/corpus or whole parity acceptance. Next: copied C/Swift
resolution transport, then search execution with requested/resolved provenance.


### Module resolution C/Swift transport (2026-10-22)

Implementation `bd10e92` (pushed fork `master`). Sequential `master`: additive owned C request/result and Swift6 copied values
reuse the explicit resolver. Unknown token remains successful absent identity,
not a failed operation. Required bounded inputs copy before queueing; successful
poll/take-once and teardown ownership use existing request semantics. No new
search application, SDK owner, ABI layout/version or language change.

Normal4/4 (1.46s), ASan/UBSan3/3 (2.48s), TSan3/3 (5.39s), portable-on-Mac3/3;
strict C11 header/whitespace pass. Build targets are the three core module
resolution/registry tests plus `macos_module_resolution_test` on Mac. Selection:
`ctest --test-dir <dir> --output-on-failure -R '^core_module_(resolution.*|abbreviations)$'`,
adding `|^swift_module_resolution$` for Mac. Existing SDK/compiler environment,
ASan leaks OFF, GTK runtime OFF, SDKs partly instrumented. Logs: ignored
`build/module-resolution-adapters-*`. Tests cover pre-poll/wrong-kind/take-once,
null/empty/over-limit/invalid UTF-8, optional/Unicode values, copied inputs,
closed tabs, accepted cancellation/shutdown and value lifetime. No new full
suite/UI/Fedora/MSVC/corpus/whole-parity acceptance. Next: search execution with
requested/resolved identity retained through preview/export and all consumers.


### Alias search execution and source provenance (2026-10-22)

Implementation `50e8d71` (pushed fork `master`). Sequential `master`: only search execution targets resolve aliases on the same
Reader owner. Raw requested module tokens and explicit range providers stay
unchanged. Results additionally own installed source IDs; retention, visible/all
text/HTML/rich export selection and Mac/Linux hit adapters consume those actual
IDs. Optional shared context headers disclose resolution; exact-ID header bytes
remain unchanged. Existing requested C getter stays compatible; additive source
getter feeds preview/activation. No frontend resolver or automatic draft change.
Distinct original prose per module makes wrong-source export detectable.

Full Mac build and focused13/13 (22.95s) pass including native model alias saved
list application, preview, activation and text/HTML export; existing batch/text/
HTML regression and generated fixtures included. Full Mac179 passes+1 explicit index skip,180 registered,154.45s.
Final full portable core/Linux models on Mac135 passes+1 explicit index skip,
136 registered,79.07s, GTK runtime OFF; the Linux alias model is included.
ASan/UBSan11/11 (41.24s), TSan11/11 (39.72s). Same documented Mac/compiler/SDK
configuration, dependencies partly instrumented, ASan leaks OFF. Commands:

```sh
cmake --build build/macos --parallel 2
ctest --test-dir build/macos --output-on-failure
cmake -S platforms/linux -B build/linux-models -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --output-on-failure
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure -R '^core_(search_aliases|module_resolution.*|module_abbreviations|search_batch|search_export|search_all_export|search_html_export)$'
ctest --test-dir build/thread-sanitized --output-on-failure -R '^core_(search_aliases|module_resolution.*|module_abbreviations|search_batch|search_export|search_all_export|search_html_export)$'
```

Sanitizer targets rebuilt for the selected eleven cases/fixtures; ignored logs
`build/search-aliases-*` retain configure/build/exact result selections. After
moving the Linux model test from the GTK-only block, its explicit target and
fixture were rebuilt before the final full136-case portable run. Earlier135-case
portable run is superseded for final combined-tree evidence. Header comments/
formatting after full Mac run change no executable statements; strict C11 header,
shell syntax and whitespace checks pass. No current Fedora/MSVC/runtime index,
whole feature or release acceptance follows.


Active alias-search validation update: complete Mac regression passes179 +1
explicit index skip,180 registered,154.45s; ASan/UBSan focused11/11,41.24s.
The generated `search-aliases` disposable native fixture loaded literal saved
choices `Shared,AFirst,文字 café,Shared`, returned4 groups/4 matches, previewed
BSecond for first Shared, and Read in main pane showed distinct original BSecond
prose. Draft module tokens stayed literal. Tested window closed and only this
continuation's disposable preview process was stopped; older previews untouched.
No generated export HTML/browser workaround was attempted. This is scoped actual
menu/import/search/preview/activation evidence, not whole visual/accessibility/
export-browser/corpus acceptance. A Linux model test was initially registered
under the GTK runtime switch; corrected to the headless test block. Its explicit
build and final full136-case portable run passed, as did TSan11/11.


### Typed single-passage aliases (2026-10-22)

Sequential `master` after `50e8d71`: reuse the same owner registry at Reader's
single-passage resolution boundary. Pure parsing keeps literal module tokens;
validated owned targets return actual installed IDs. Existing source/navigation
pane fallback and explicit-module precedence remain intact. No navigation occurs
until the host applies the returned target; unsupported non-verse modules/ranges/
fragments/commands retain their separate gates. Existing C handles/Swift adapters
carry actual IDs, with Swift resolveLink NUL guards before conversion.

Final normal12/12 (8.68s), ASan/UBSan10/10 (15.40s), TSan10/10
(17.21s), and portable/Linux model11/11 pass. Includes owned C/Swift, native
Mac/Linux model URI navigation with unchanged search metadata, standalone RawLD
non-verse rejection and exact-ID bypass of unrelated bad metadata. Retained
show_ref checks installed IDs then falls back to settings.MainWindowModule; it
does not resolve aliases. An explicit command guard prevents the URI change
from leaking into that separate behavior. Existing note_dispatch already freezes
and compares the complete callback; command fallback is the next bounded gap.
No new full suite/UI URI click/corpus/current Fedora/MSVC or whole-feature
acceptance. Same environment and sanitizer instrumentation limits as above.


### Shared showRef module dispatch (2026-10-22)

Sequential local master after typed URI commit `e0e9a86`. Extract the retained
show_ref callback policy into a header-only C++11 shared dispatcher: hover does
no lookup/presentation; clicks select an exact installed requested ID or the
main-module fallback, then invoke the existing sidebar renderer with context and
expression unchanged. GTK uses this dispatcher; no legacy source is removed.
The frozen/actual full note/reference oracle now covers null/empty/unknown/Unicode
main and requested module values. Actual adapter compiles strict C++11.

Native contextual dispatch consumes the same decision on the sole FIFO owner.
For pane calls, omitted showRef modules default to navigation_module rather than
source_module; unknown/abbreviation tokens fall back to that same main module.
Source reference stays separate for relative list parsing. Other command families
retain source context; sword/bible URI aliases keep their own route. The older
two-kind single-reference API gains exact main fallback without enabling aliases
for commands. Invalid fallback/non-verse/type/parser failures publish no preview
or history. Native NUL context guards precede borrowed C-string policy calls.

Normal11/11 (6.39s), including C take-once/copied owner teardown, Swift and native
Mac model preview with unchanged reading/search state. ASan/UBSan9/9 (11.42s);
TSan9/9 (14.46s), portable/Linux model10/10 (7.26s). Full local Mac build
succeeds. Complete Mac181-case selection (155.58s) initially had three sandbox
clipboard failures and two old heading-preview expectations that defaulted a
module-less command to its commentary source. Retained main_url_handler defaults
missing modules to MainWindowModule, so the two expectations were corrected to
Text (including direct main-source bytes). Desktop-access recheck9/9 (17.03s)
passes all five affected tests plus prerequisites. Combined coverage:180 passes
and1 explicit index skip; this is full selection plus focused recheck, not a fresh
single clean full run. Complete Mac-hosted portable137-case selection (101.04s)
had the same old core heading expectation; its rebuilt focused recheck2/2 passes,
for combined136 passes+1 index skip. No production change was needed after these
runs. Native isolated Mac/Linux fallback model checks remain passing.
Initial nested-sandbox SwiftUI macro build failed; authorized local build outside
that nested sandbox succeeds. Same compiler/SDK/instrumentation limitations as
prior checkpoint. No new GUI/current Fedora/MSVC/corpus or whole-feature claim.

Publication note: e0e9a86 is committed locally. Automatic approval review rejected
the attempted push because trusted current messages did not explicitly authorize
the public fork destination/master. User authorization was requested; no alternate
push path or remote modification was attempted. Continue local work pending reply.


### Complete retained URI characterization (2026-10-22)

Local master after `4a71891`. Freeze complete preview/URI callbacks from
`e0e9a86`; compile current callbacks extracted by CMake and compare18,436
deterministic callback/state traces in strict C++11. No production route changed.
New target is available even with native core OFF. See
[the URI contract](passage-uri-workflows.md) for exact cases, native gaps and the
observed retained range split-buffer leak (reclaimed by oracle after tracing).
Focused1/1 passes in every configuration: normal0.40s, ASan/UBSan0.97s,
TSan1.84s, portable/Linux0.25s, native-disabled C++11 0.24s.
Full prior reference-command evidence is above; no full-suite repeat for this
test-only addition. Next bounded production gap: unknown single-passage URI
fallback to the exact installed main module, keeping range/keyed cases separate.
Publication approval remains pending; e0e9a86 and4a71891 are local commits.


### Unknown single-passage URI fallback (2026-10-22)

Local master after complete callback oracle `d82bb6b`. Reader.resolve_link now
uses exact/alias target resolution first, then ordinary unknown-target absence
falls back to exact installed current/main module as retained sword_uri does.
Invalid metadata, malformed inputs, unavailable fallback and non-verse targets
retain errors. A fallback alias is not a second resolution pass. Resolution alone
does not navigate/history/search; native hosts apply the copied actual target.
Worker contextual and older two-kind URI dispatch retain the original URI and
main context, preserving pane fallback separately from offered/source modules.
C/Swift and Mac/Linux models exercise unknown-target behavior with distinct
original prose and unchanged search generation. Normal11/11 (7.02s),
ASan/UBSan9/9 (11.57s), TSan9/9 (13.97s), portable/Linux10/10 pass.
C11 header syntax, whitespace and local Markdown targets checked. Same
environment/instrumentation limits as prior evidence. No full regression repeat
for this bounded change; complete prior selections/rechecks are recorded above.
No range/anchor/keyed-family expansion or GUI/full-platform/corpus acceptance.


### Aggregate native reference preview bound (2026-10-22)

Sequential master after published `d4097b5`. The owner explicitly authorized
checked pushes to the public fork's master; the previously blocked commits
e0e9a86/4a71891/d82bb6b/d4097b5 were published normally after ancestor validation.
No hosted builds or reset credits were used.

Add an explicit aggregate-cap overload to the shared C++11 reference parser,
preserving the existing three-argument symbol and retained per-range-only policy.
Native rendered reference snapshots pass a2000 total cap across all ranges and
singles; ordered duplicates/prefix, original expression/context and limited flag
are preserved. skipped counts cover the inspected prefix. Exact cap does not
claim truncation unless another reference would be appended. Existing unrendered
reference_targets retains its original policy, avoiding silent truncation in the
link-only hover HTML path that lacks a guard transport. That separate path is
not claimed bounded by this change. Native UI limit labels now cover aggregate
previews; no frontend parsing/selection engine is introduced.

Normal5/5 (2.55s) including existing C and Swift copied limited metadata; parser
checks compare bounded duplicate/order prefixes against original complete output
and two individually sub2000 ranges totaling more than2000. Snapshot tests verify
actual content/source prefix, per-tab history isolation and original unrendered
targets. ASan/UBSan7/7 (7.47s), TSan7/7 (8.94s), portable7/7
(3.28s), C11 header syntax and whitespace checks pass. New URI range routing is next,
not enabled by this cap. Same platform/instrumentation/full-acceptance limits.


### Bounded contextual Bible URI ranges (2026-10-22)

Sequential master after published aggregate bound `3458b0a`. Extract retained
Bible/range punctuation decision into a C++11 header used by GTK sword_uri and
FIFO native contextual dispatch. Complete18,436 frozen/current callback traces
preserve exact lookup order and state behavior. Pure contextual parser stages
literal module/expression with a32768-byte URI bound; owner resolves aliases and
requires an installed Biblical Texts module before using the existing capped
reference preview service. Source context/main fallback remain separate. Older
Reader/two-kind URI APIs stay single-only. Unknown and non-Bible range URI routes
remain errors rather than guessing retained keyed/fallback navigation behavior.
showRef commands retain their separate exact-ID/main-fallback policy.

Normal6/6 (4.25s): actual alias prose, range/duplicate/source context, module-less
main fallback, Unicode, input limit, aggregate2000/limited metadata, copied C
take-once/shutdown, Swift and native Mac model non-navigation/search isolation.
An expanded type fixture initially added commentary after opening Reader; SDK
modules-conf.cache hid that later addition. Corrected fixture stages every module
before first open. Dictionary/commentary URI ranges reject while commentary
showRef list remains supported. ASan/UBSan11/11 (12.33s), TSan11/11
(15.15s), portable/Linux12/12 (7.55s) pass.
No new GUI URI click/current Fedora/MSVC/corpus or whole-feature acceptance.


### Bounded native target/link-only previews (2026-10-22)

Sequential master after URI ranges `fbfea5b`. Native Reader.reference_targets
now uses the same2000-reference aggregate prefix cap as rendered snapshots.
Shared display_reference_links discloses limited with an accessible status
paragraph, preserving the committed-context Back link. This closes the explicit
unrendered-path gap left at3458b0a without changing the retained three-argument
parser/GTK callback policy. Existing non-limited HTML stays byte unchanged.
Skipped counts cover the inspected prefix. Tests compare target/snapshot metadata
and reference order across multiple individually sub-limit ranges and check
disclosure/Back link; hover/normal/C/Swift transport checks remain scoped.
Focused normal7/7 (3.49s), ASan/UBSan7/7 (7.27s), TSan7/7 and
portable7/7 pass. Final complete regressions are in progress.
No new GUI/corpus/current Fedora/MSVC or whole-feature acceptance.


### Native URI escape validation and final checkpoint (2026-10-22)

Sequential master after link-only bounds `9ce2756`. Native decode rejects
incomplete/non-hex percent sequences before module resolution/fallback. Valid
encoded percent/literal plus/Unicode decode exactly once. Pure single/contextual,
Reader/C and worker checks cover rejection; source retainer callback behavior is
unchanged. Normal4/4 (3.80s), C11 header and whitespace checks pass. Final
ASan/UBSan11/11 (16.98s) and TSan11/11 (20.22s) pass. Complete
Mac and Mac-hosted portable/Linux regressions are in progress. Native-disabled strict C++11 registry/complete URI oracle2/2 pass
(0.43s), including the newly shared retained range decision.

The latest link-only focused TSan7/7 took8.62s, portable7/7 3.18s. That slice
initially missed the complete reference-list type include in content_test; added
the include before any passing checks/commit. No production repair was needed.
Same environment/instrumentation limitations as prior entries; no reset credit,
hosted build or acceptance score increase.


Final review/next bounded work: the32768 contextual URI check runs in the core
parser after the C/worker entry points have copied their string arguments.
`xc_worker_follow_pane_link_at` and the other contextual entry points currently
check non-null/nonempty input then submit copied strings; this continuation
claims a parser acceptance bound, not a pre-admission allocation bound. Audit
and add bounded pre-copy admission consistently before expanding keyed/unknown
range URI routes. Preserve copied FIFO ownership and current status/empty-output
contracts. Then use the complete retained callback oracle for unknown/non-Bible
range navigation, anchors and dictionary/keyed hover. Do not infer that all
passage hover renders a Bible preview. Parallel/BibleSync/corpus/platform/whole
parity work remains independent.


Final published implementation checkpoint: `c70cca9` (with `9ce2756`,
`fbfea5b`, `3458b0a`, `d4097b5`, `d82bb6b`, `4a71891`, `e0e9a86`). All local
tracked implementation changes are committed and pushed normally to the
authorized fork master. Preserve unrelated untracked .DS_Store.

Final complete Mac build succeeds; fresh full182-case desktop-access selection
passes181 with1 explicit index skip (153.04s). Fresh full Mac-hosted portable/
Linux138-case selection passes137 with1 explicit index skip (106.42s), GTK
runtime OFF. Final focused ASan/UBSan11/11 (16.98s), TSan11/11 (20.22s).
Native-disabled C++11 render/policy libraries build; registry/complete URI
oracles2/2 (0.43s). C11 public header syntax, diff whitespace, local shell syntax
and continuation Markdown link targets pass. Earlier failed/rechecked full
selections above are historical; these fresh final complete runs supersede them.

Environment remains Mac ARM64/macOS27.0.1, AppleClang21/Swift6.4, SWORD1.9.0.3916,
GLib2.90.1, Apple XML2.9.13 and pinned PNG1.6.59. Sanitizer SDK/framework coverage
is partial and ASan leak detection is OFF, as previously documented. No actual
indexed search on this Mac SDK, current Fedora/MSVC, broad corpus, new GUI URI
click or whole-workflow acceptance is claimed. Scoped native alias-search window
evidence earlier in this continuation remains separate from URI/model checks.
No hosted build, reset credit or parity/retirement score change. Stopping near
the current usage window limit with checked/published work and explicit next-step
pre-copy admission audit above.


### Study-link admission continuation (2026-10-22)

Sequential master after fetched `f8ba862`; tree had only unrelated .DS_Store.
The owner asked to work through the usage window, which refreshed to3% used.
Implement public C byte constants with core static assertions, bounded C scanning
before request/string allocation, owner-independent ready exceptional C++ futures
before FIFO admission, synchronous Reader bounds, and Swift guards before C-string
conversion. Seven existing main study-link worker routes share32768 URI/4096 per
module/source-reference bounds; nullable/empty rules and valid-request ownership
stay as before. C callers get immediate INVALID_ARGUMENT/null output on oversized
input. C++ callers already own the by-value arguments; no claim about allocations
while constructing those arguments. No frontend parsing or extra SDK worker.

Normal focused7/7 (3.63s); combined core/Swift3/3 (2.68s); ASan/UBSan7/7
(9.00s), TSan7/7 (10.93s), Mac-hosted portable7/7 (4.74s). Dedicated allocator
interposition verifies no C++ observer/string allocation on submitting-thread C
rejection; nonterminated max+1 buffers exercise bounded scans under sanitizers.
Exact UTF-8 byte caps, each oversized field, seven routes, ready futures, live/
closed tabs, state isolation, valid argument copying and shutdown result lifetime
are covered. An initial expected reference length used a wrong hardcoded prefix
length; fixed the test to derive the actual prefix size before passing checks.
C11 header syntax and whitespace pass. Same Mac/SDK/instrumentation limits as
prior checkpoint, ASan leaks OFF; no new GUI/corpus/Fedora/MSVC/full parity claim.

Next source seam: unknown/non-Bible URI range navigation depends on retained
BackEnd::get_valid_key, called by main_update_nav_controls. Characterize/extract
its module-key mutation, conditional auto-normalization and exact en short-label
choice before claiming those routes match legacy. Dictionary URI hover also
needs a separate typed raw-key route rather than pretending it is a lexical
command or a verse reference.


### Shared navigation-key normalization (2026-10-22)

After `69db801`, freeze complete BackEnd::get_valid_key and compile its current
source-extracted adapter against real RawText/RawCom/RawFiles keys. Extract the
mutation/label operation into C++11 CoreRender navigation_key_text. Retained
wrapper still returns strdup-owned bytes and missing-module Gen.1.1, forces
auto-normalization only for Bible/regular commentary, retains other flags, and
uses a short label only for exact locale en (null/empty/EN/other use full text).
No legacy source removed or frontend normalization added.

13,446 frozen/current comparisons cover KJV/KJVA, starting contexts, relative/
range/list/invalid/out-of-canon references, initial auto/intros flags, type modes,
label bytes, final key/error/flags and callback order; owned copied labels survive
key mutation. Normal1/1 (1.37s), native-disabled strict C++11 1/1 (1.56s);
ASan/UBSan1/1 (3.03s), TSan1/1 (2.82s), portable1/1 (1.24s). Initial test enum/int
initializer-list deduction failed strict compilation; explicit vector<int> fixed
the test before passing checks. This characterizes get_valid_key itself, not full
navbar/XML/IPC or corpus/locale workflow acceptance.

Next: native read-only cloned navigation targets consuming this operation, then
contextual unknown/regular/personal-commentary range URI dispatch. Preserve the
older single-target API's decoded-reference contract and exact fallback module
rules. Dictionary/book targets remain separate typed paths.


### Cloned canonical URI navigation targets (2026-10-22, active)

After shared normalization `8fcfa4c`, add Reader.navigation_target for exact
installed Bible/commentary IDs. It clones the live VerseKey, applies the retained
type-dependent normalization and returns copied full canonical text, rejecting
invalid SDK output. No module position/flags, read rendering, history or frontend
state is published. Existing Reader.resolve_link and older two-kind URI parser
stay single-only and keep their decoded-reference contract. Contextual unknown
range URIs use exact main fallback; regular/personal-commentary range aliases use
their actual target module. Bible ranges still render bounded sidebar snapshots;
dictionary/book keyed routes remain separate. Existing C link handles transport
the target and hosts apply it through visit_linked_passage. No frontend normalizer.

Core normal5/5 (4.65s) including complete URI and normalization oracles; native
target test compares72 frozen SDK normalization/classification cases, cloned key
isolation, canonical outputs, exact fallback, explicit host application, state
isolation, C take-once/copied shutdown results, closed tabs and FIFO drains.
ASan/UBSan10/10 (14.48s), TSan10/10 (17.04s), portable/core/model11/11 (8.62s)
pass. Model seed changed to a populated reference; the Mac history assertion
initially expected Back after returning to the startup reference. The retained
reference-only deduplication correctly moves to the existing first entry (Forward
available, Back unavailable); corrected the test rather than changing history.
The initial suspicion about an empty personal-commentary fixture was not the
cause of the history result. Also derived relative-key expectations from actual
SDK behavior instead of assuming a bare number always means verse24.

Final Mac/core selection passed 6/6 (5.89s); portable model recheck passed 2/2
(1.94s). Same environment/SDK/sanitizer limits as above;
no GUI URI interaction, broad corpus/locales/current Fedora/MSVC or whole parity
acceptance. Next typed URI family remains dictionary/keyed hover/click plus anchors.

### Shared retained URI anchor split (2026-10-22)

After `7c116dc`, extracted the retained in-place click anchor split into the
C++11 passage URI helper. Hash takes precedence over bang regardless of order;
null versus empty anchors and caller-owned buffer offsets remain distinct.
Complete frozen/current callback comparison and 19,562 exhaustive short/Unicode
prefix splits pass native-disabled C++11 (0.31s), ASan/UBSan (0.94s), and TSan
(2.14s), one test per configuration. The sanitizer library/SDK limits above
still apply. No native anchor UI or whole workflow acceptance is implied.

### Full continuation checkpoint and dictionary URI hover (2026-10-22)

Full checkpoint through `7c116dc`: Mac 185 passes plus one explicit index skip
(186 registered, 162.04s); Mac-hosted portable/Linux models 141 passes plus one
index skip (142 registered, 107.04s), GTK runtime OFF. These supersede the prior
full 182/138 registered runs. They include admission, shared normalization and
cloned navigation targets, before dictionary URI hover additions.

Dictionary hover now uses copied keyed parsing and actual owner alias/type
resolution, preserving whole decoded paths and the retained short declared
module scan. It accepts SDK dictionary-family DailyDevotion modules separately
from lexical choice eligibility. Uses existing DictionaryEntry C handles, typed
Swift consumption and Linux information presentation; no navigation/history visit.
Original-prose fixture adds punctuation/slash/anchor/query keys, Unicode, encoded
percent/plus, daily dictionaries, 19/20-byte module IDs and short aliases to long
IDs. Complete callback oracle now compares 18,484 traces plus 19,562 anchor splits.

Core normal 3/3 (1.87s); native Mac model plus fixture 2/2 (2.88s); portable model
and core 3/3 (2.75s); ASan/UBSan 3/3 (4.58s); native-disabled C++11 complete URI
oracle 1/1 (0.30s). TSan 3/3 (5.15s) passes. Final existing/new hover and admission selection 13/13
(8.27s). Same SDK and sanitizer coverage
limits apply. No physical GUI hover, corpus/current Fedora/MSVC or whole parity
acceptance. Next URI work is keyed clicks, native anchor consumption and retained
status/bookmark behavior, each through shared core services.

### Ordinary dictionary URI click consumption (2026-10-22)

After hover `c6125e8`, contextual ordinary dictionary URI clicks bypass scripture
parsing, strip the shared retained anchor, apply shared Unicode uppercase and
visit existing dictionary history. Resolve-only reads do not visit. Requested
normalized history keys remain distinct from SDK matches. Existing C dictionary
handles and Mac/Linux dictionary presentation/choices/history consume the copied
result, without lexical metadata or a Bible/search visit. Omitted URI modules
use main context, explicit long IDs work on click, and short hover rules remain.

New core checks cover punctuation/slashes, hash-over-bang precedence, empty
anchors/keys, partial SDK matches, actual IDs, raw resolves, dictionary Back,
rejected input/history isolation, sibling tabs, closed tabs, FIFO draining and C
kind/take-once ownership. Model checks exercise click plus Back/Forward and
reading/draft/window isolation. Core normal 5/5 (4.23s); Mac model 2/2 (3.06s);
portable 4/4 (2.92s); ASan/UBSan 3/3 (3.27s); TSan 3/3 (5.10s). C11 header and
whitespace checks pass. Same platform/SDK/sanitizer limits apply.

Inspection found a separate DailyDevotion branch in main_display_dictionary:
switching module may replace a non-date key with today's date, update devotional
selection and change its notebook. Daily URI clicks explicitly reject until that
service/presentation is characterized; hover still reads the actual dictionary.
Native Bible/commentary anchor consumption, book/prayer routes, status/bookmark
behavior, physical GUI URI clicks and broad corpus/platform acceptance remain open.

### Owned contextual verse anchor targets (2026-10-22)

After dictionary clicks `498f08e`, contextual verse parsing now owns an optional
anchor separate from reference/module. Shared retained split preserves absent vs
empty, hash-over-bang precedence and literal decoded suffixes. Anchored single
Bible/commentary/unknown targets use cloned canonical navigation; commentary and
unknown range targets retain anchors; Bible ranges remain stripped sidebar
previews. Existing strict single APIs reject anchors. C getter and Swift copies
preserve lifetime without a view action or history mutation.

Core normal 6/6 (4.83s); Swift/model plus fixture 2/2 (2.82s); portable 2/2
(1.68s); ASan/UBSan 2/2 (2.66s); TSan 2/2 (4.19s). Includes literal quote,
backslash, closing-script text, Unicode, escaped percent, null/empty C pointers,
copied shutdown results, actual module/fallback/range identity and isolation.
The anchor string is data; no scripts/content were executed. Retained utilities.c
consumes special_anchor exactly once in the first rendered pane. Native explicit
target application, pane precedence and empty-anchor scrolling are next; current
models do not apply the transported anchor yet. Same platform/sanitizer/corpus
limits apply, with no new whole-group acceptance.

### Shared transient pane anchors and native rendering (2026-10-22)

After `65fa7e4`, shared explicit linked navigation returns an owned transient
anchor plus actual target module. Bible targets select main; regular/personal
commentary targets select commentary. Prepared before history publication; no
anchor stored in session/history or replayed by ordinary refresh/history. C
optional-anchor visits preserve null/empty/lifetime and bound every copied field;
both old/new linked C variants reject oversized/unterminated values before
observer/string allocation. Worker returns ready errors before FIFO admission.

Mac/Linux models consume that target identity, preserve the other pane's ordinary
selected-verse anchor and add repeat-event revisions. Empty means top. Hosts use
isolated trusted scrolling with module scripts disabled, Unicode-preserving
string encoding and ID/named-target lookup. Missing targets report failure; Mac
ignores obsolete async failures. Linux host source is adapted, but GTK runtime
and compilation remain OFF here; portable models/helper compile and pass.

Core/admission/fixture recheck 4/4 (2.97s); portable selection 5/5 (7.04s), final
helper/model recheck 2/2 (2.68s); ASan/UBSan 4/4 (7.51s); TSan 4/4 (9.80s).
Final native model + offscreen real WebKit + fixture 3/3 (4.04s), WebKit itself
0.44s. Real DOM checks cover UTF-8/quote/backslash/closing-script text, legacy
named anchors, top/repeat, missing targets, disabled module scripts and obsolete
completion suppression. No external content or module script executes. C11 and
whitespace checks pass.

Test corrections: reference-only history deduplication made Forward the valid
direction after returning to the first reference; no history behavior changed.
The Mac empty-anchor assertion initially expected no commentary anchor, but the
regular commentary's ordinary chapter anchor correctly remained selected. The
assertion now distinguishes transient URI overrides from default chapter anchors.
A missing CSS argument in the new WebKit fixture was corrected before compilation
and execution passed. No physical user click, full corpus, Fedora/MSVC or whole
parity acceptance is implied. Next typed URI routes: devotional and book/prayer
clicks, plus status/bookmark behavior.

### Full URI-pane checkpoint and direct book display policy (2026-10-22)

Through transient-pane implementation `db2f46e`, fresh full Mac 189 passes plus
one explicit index skip (190 registered, 161.83s); full Mac-hosted portable/Linux
144 passes plus one index skip (145 registered, 113.68s), GTK runtime OFF. Includes
real offscreen Mac WebKit scrolling; physical URI clicks, current Fedora/MSVC and
whole parity gates remain open. These supersede the full 186/142 registered runs.

Next, expose book_display_target from the existing saved-display policy and reuse
it for saved display restoration. Numeric-prefix/path, minimum-four and bounds
behavior stays equal to complete extracted main_display_book. Normal 1/1 (0.12s),
ASan/UBSan 1/1 (0.13s), TSan 1/1 (1.38s), portable 1/1 (0.22s) pass. No SDK
selection, rendering, persistence or native URI book acceptance in this step.

### Resolved owned book/prayer URI targets (2026-10-22)

After direct display policy `cc39c27`, contextual GenericBook/prayer URIs resolve
aliases, strip the shared anchor and return BookUriTarget with actual owned SDK
tree, raw display key and optional anchor. Reuses book_display_target and private
resolve_book_target queries; no book/document selection, read, history or profile
write. Distinct take-once C handle borrows owned tree/key/anchor with shutdown
lifetime. Native consumption/book-origin cross-link dispatch follows separately.

Original-prose RawGenBook/prayer/Text fixture covers numeric-prefix and minimum
four, relative/path/Unicode/list/query titles, hash-over-bang, percent-once, explicit
root versus numeric startup, unsafe VerseKey driver, missing paths, bounds, main
versus source, hover, history/search/sibling isolation, closed tabs, FIFO draining
and C kind/ownership/null getters. Core normal 1/1 (0.61s), combined book/URI
regression 7/7 (4.72s), ASan/UBSan 2/2 (1.07s), TSan 2/2 (1.34s), portable
2/2 (0.63s) pass. Initial test used mixed string-literal/std::string initializer
list inference; corrected to explicit vector<string> before compilation passed.
Same environment/SDK limits; no native book URI click, current Fedora/MSVC or
whole book/prayer acceptance is claimed.

### Native book URI selection and shared book-origin dispatch (2026-10-22)

After target transport `54aa353`, Mac and Linux models consume actual copied book
URI targets through existing shared display-key document restoration. Preserve
explicit root versus numeric minimum-four, prayer/list families, raw title
punctuation, reading/search/history/window isolation and transient anchors. Mac
BookPane uses existing isolated anchor rendering. GTK book host source uses the
same UTF-8 script helper, weak status lifetime and render-revision guard; GTK
compile/runtime remains OFF here.

New book-pane C++/C command requires a committed document. Delegates notes/images
to existing owned book-source handling; module URIs use contextual dispatch with
separate live main module/reference, so relative Bible lists never use tree paths.
Missing main context is allowed only for explicit installed modules. Admission
checks precede C copies/observer and worker FIFO. Swift copies target and nested
book/study variants and shares the main study-result presenter. Linux similarly
reuses its contextual handler and shared document pipeline. No new SDK owner.

Core/book-origin/admission selection 3/3 (2.56s); native new book model plus fixture
2/2 (1.42s), portable new model plus fixture 2/2 (1.13s); combined existing/new
book-note/document publication/native models 11/11 (9.65s). ASan/UBSan 3/3
(4.75s), TSan 3/3 (6.47s). C11 and whitespace checks pass. No physical URI click,
current Fedora/MSVC, broad corpus or whole book/prayer acceptance.

Next corrective audit: complete HtmlOutput in gtk/utilities.c consumes the global
special anchor once. Unknown URI fallback calls commentary before Bible when
commentary is visible; dictionary display calls HtmlOutput too, despite receiving
a stripped key. The current callback oracle logs dispatch but its display stubs
do not consume that global anchor. Extend the oracle through that complete
consumer, then correct native fallback-pane/dictionary-anchor handling. Do not
claim those cases match full retained rendering yet. Devotional date/pane rules
and status/bookmark behavior remain separate pending routes.

### Complete retained URI/HtmlOutput anchor oracle (2026-10-22)

After book integration `39ead32`, freeze complete HtmlOutput with GPL notice and
extract current function into a strict C++11 consumer oracle. Shared test host
keeps original callback matrix unchanged and optionally drives complete output
stream/jump seams. 432 frozen/current URI-plus-output traces prove first rendered
pane one-use, visible commentary precedence for unknown fallback, dictionary
global-anchor consumption, explicit empty anchors, notebook preservation and
Bible-sidebar early return/leak. No GTK DOM or whole rendering acceptance.

Original + new consumer tests 2/2 in each configuration: normal 0.57s, ASan/UBSan
1.02s, TSan 1.85s, portable 0.38s, native-disabled C++11 0.34s. Native corrections
for unknown fallback pane visibility and dictionary anchor transport are next.

### Unknown URI fallback anchor pane correction (2026-10-22)

Complete consumer oracle `ca50a33` proves commentary-before-Bible consumption on
unknown fallback. Native targets now copy anchor_main_fallback; shared explicit
application takes current commentary visibility and chooses commentary when
visible, otherwise main. C getter/new policy request and Swift/Linux copies
preserve this metadata; flags reject unless exactly 0/1 before copies/observer.
Older linked-visit C variants retain known-target semantics. Mac supplies hidden
commentary state while book display is active; known commentary links switch back
to that pane. Ordinary reads/history still clear transient anchors.

Core 5/5 (4.12s), native paired/book models plus fixtures 4/4 (5.84s), portable
7/7 (8.61s), ASan/UBSan 5/5 (8.70s), TSan 5/5 (11.54s) pass. Includes positive
C policy application, copied routing flags, invalid flags with zero submission
allocations, hidden/visible commentary and book-origin context. Same SDK/platform
limits; dictionary anchor transport is next, with no whole parity acceptance.

### Owned dictionary URI anchor consumption (2026-10-22)

Complete retained HtmlOutput evidence now drives dictionary URI anchor transport.
Contextual dispatch returns a DictionaryUriEntry wrapper containing the ordinary
entry and an optional owned anchor; lookup/history receive only the stripped,
normalized requested key. Raw/hover/history entries retain no anchor. The copied
C getter preserves absent versus explicit empty; Swift/Linux dictionary panes
apply transient anchors with revisions for repeated identical targets. Ordinary
reads and history navigation clear the metadata. Shared C++11 DictionaryEntry
content remains unchanged. Original ThML fixture prose includes a named target.

Core 5/5 (4.11s), native models plus actual offscreen WebKit and fixtures 5/5
(6.79s), portable 4/4 (4.46s), ASan/UBSan 3/3 (4.39s), TSan 3/3 (5.21s) pass.
Native-disabled C++11 full libraries build and 4/4 tests pass (2.01s). Strict C11
header and whitespace checks pass. Includes owner-close copies, repeated/empty
anchors and clearing on ordinary/history reads. GTK compile/runtime, physical
URI activation, devotional date/pane routing and broad acceptance remain open.

### Complete dictionary/devotional display characterization (2026-10-22)

Freeze complete main_display_dictionary from `0babbf7` with its GPL notice, and
extract the current complete dispatcher. Strict C++11 deterministic host seams
compare 2,880 traces across absent/same/different modules, installed/available
gates, features, keys, date shapes, invalid UTF-8 and missing devotional widgets.
The clock seam supplies July 9 independent of host time zone. Calendar-invalid
99.99 is retained because the legacy branch validates only NN.NN shape. A changed
DailyDevotion module redirects to its pane and returns before dictionary history;
an absent/current dictionary module follows ordinary dictionary handling even
with that feature. Null key becomes Grace. Invalid UTF-8 can follow module/pane
changes but returns before history/tab publication. Record these ordering rules
without silently correcting them or claiming native devotional pane support.

Normal 1/1 (0.07s), portable 1/1 (0.20s), ASan/UBSan 1/1 (0.24s), TSan 1/1
(0.71s) pass; native-disabled full C++11 libraries and all 5/5 tests pass (2.09s).
The oracle covers full dispatcher callbacks, not GTK rendering/date-picker UI.

### Shared C++11 dictionary/devotional route policy (2026-10-22)

The complete dispatcher oracle now guards dictionary_display_route in the
shared C++11 rendering seam. It returns ordinary dictionary, supplied devotional
key or host-local-today; it performs no SDK/UI/clock work or allocation. Retained
main_display_dictionary consumes the decision at its existing changed-module
branch. The host still supplies local today lazily and preserves callback order.
No new native devotional pane is claimed. Expanded complete-function comparisons
cover every 10,000 NN.NN numeric shape plus seven short/long prefixes: 12,887
frozen/current traces, including null module/feature/key helper guards.

C++11 all 5/5 (2.66s); expanded ASan/UBSan 1/1 (1.16s), TSan 1/1 (1.70s)
pass. Shared route plus exhaustive Unicode key checks ASan/UBSan 2/2 (12.76s),
TSan 2/2 (23.32s) passed before expanding the date-shape matrix. Full normal
regressions are recorded at the next final checkpoint.
