# Linux / Fedora / WSL2 chat handoff

## Integration into master (2026-10-22)

At the owner's request, `linux/native-parity` through `f0b6e57` is integrated
with master `c11b15b`. The Mac integration check corrected newer SDK class search
constants from `constexpr` to `const`; the Fedora released-SDK path is unchanged.
Actual combined-tree Mac checks: **173 passes +1 index skip** (174 registered);
portable core/Linux models on Mac, GTK OFF: **130 passes +1 index skip** (131
registered). See [commands and environment](handoff-macos.md#linux-parity-integration-2026-10-22).
These do not replace the recorded Fedora results below or close the SDK sanitizer
and desktop-display gates. The Linux branch is retained; fetch master before the
next Linux slice. No hosted build was dispatched or enabled.

## Local catch-up checkpoint — 2026-10-22

Task/branch: `linux/native-parity`, based on `bbc12ee0`; implementation commit
`3f9a068a`. Owner requested current Mac preview implementation-scope catch-up.
The owner explicitly approved publication on 2026-10-22 after initial automatic
review blocked the upload. Implementation is published on `linux/native-parity`
through the GitHub connector. Its tree is byte-identical to the locally checked
implementation (`28cec0d1`, tree `bd571d2575cf2360a03fc99255059b8771a32247`).
The connector assigns a different commit identity; use the published hash above.
No hosted workflow was dispatched or enabled.

Environment: Fedora 44 aarch64, GCC/G++ 16.2.1, CMake 4.3.0, Ninja 1.13.2,
SWORD 1.9.0-38.fc44, GTK 4.22.5, WebKitGTK 2.54.1, GLib 2.88.4,
libxml2 2.12.10, libpng 1.6.58. The owner installed system development packages.
A temporary workspace SDK was abandoned in favor of those system packages.

Changed behavior:

- General-book tab consumes shared DisplayLevel/resource HTML/CSS, lazy contents
  pages, root/up browsing, key/path reads, parent/child/sibling navigation, explicit
  XML restoration, image/font/spacing/note controls and typed note/image dispatch.
- Actual-source verse/book image bytes feed a bounded native PNG viewer; scripts
  and remote/file subresources stay blocked. Only prepared book HTML allows data
  images. Image decoding/desktop presentation needs a display check.
- Search tab consumes all eight modes, ordered module requests (including literal
  spaces, duplicates and failures), bounds/provider/complete last scope, annotation
  choices/reset, progress/Stop, selected previews and source-preserving activation.
- Explicit saved-list import/application consumes copied optional values, shared
  first-label indices and tokenization. No native XML/list parsing or list saving.
- Retained generation/group/row identities feed selected/all text/HTML export,
  optional scripture/context/PNG and explicit partial-source disclosure. Native
  HTML Copy supplies both shared representations; selected-file Save uses async
  replacement. Desktop destination acceptance remains pending.
- Released-SDK search constants and sized stripped buffers are adapted privately.
  The old pointer API hid embedded NUL tails; the equivalent sized strip operation
  restores existing validation without discarding tests. Installed SDK disassembly
  confirms its pointer wrapper calls `renderText(text, -1, false)`. Newer SDKs keep
  their sized `stripText` path. Existing search/legacy comparisons remain.
- Explicit libxml include paths fix standalone test compilation. The Linux helper
  defaults to one job. An earlier overlapping parallel build exhausted this 4-GiB
  VM; it was stopped, generated Ninja dependency records repaired, and subsequent
  builds/tests ran serially with debugger symbols omitted. Source was preserved.

Checks actually run:

```sh
cmake -S platforms/linux -B build/native-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS_DEBUG='-O0 -g0' \
  -DCMAKE_CXX_FLAGS_DEBUG='-O0 -g0'
cmake --build build/native-linux --parallel 1
ctest --test-dir build/native-linux --output-on-failure --parallel 1 \
  --timeout 120 --no-tests=error
ctest --test-dir build/native-linux -R '^linux_study_model$' \
  --output-on-failure --parallel 1
```

Full normal suite: **130/130 pass**, including usable Fedora SDK indexed search
(no index skip). Latest expanded model run: **4/4 pass** with book, attribute and
image prerequisites; checks include retained failures, group identity, routing,
exports, optional saved values, exact binary ownership and shared-window shutdown.
The full current GTK executable and opt-in desktop smoke compile. `bash -n
platforms/linux/core.sh` and `git diff --check` pass.

Sanitizer configuration: `build/linux-sanitized`, GTK OFF, same Debug flags, plus
`-fsanitize=address,undefined -fno-omit-frame-pointer` in C/C++ and sanitizer linker
flags. Built serially the `linux_study_model_test`, `core_attribute_search_test`,
`core_book_document_presentation_test`, `core_verse_image_source_test`,
`core_search_generation_test`, `core_sdk_text_test` and `core_search_test` targets.
With `ASAN_OPTIONS=detect_leaks=0` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, focused CTest regex
`^(linux_study_model|core_search_generation|core_sdk_text|core_search)$` records
**4 passes, 1 failure, 2 dependency-blocked** (7 including fixtures).
`core_attribute_search` reports `memcpy-param-overlap` inside the uninstrumented
Fedora `libsword.so.1.9` `SWModule::search` (offset `0x8a510`). Direct sanitized
runs of generation and Linux model also encounter that dependency diagnostic.
No suppression, system-library patch or sanitizer acceptance is claimed.

GUI interactions checked: **none**. `GSK_RENDERER=cairo
build/native-linux/linux_study_ui_smoke build/native-linux/ui-smoke-fixture`
returns **77**, `No accessible GTK display`. The smoke now checks display
availability before creating worker/UI objects; no headless cleanup dereference.
Its fixture root combines generated book-presentation and attribute-search
`mods.d`/`data` directories only. The smoke does not access the user's clipboard.
The six older desktop smokes, actual key/pointer delivery, picker cancellation,
rich-client clipboard, screen-reader, corpus/visual and accelerated rendering
checks remain open. Do not count compiled controls as desktop acceptance.

Shared API changes: none in public C/C++ signatures; private SDK compatibility
and test discovery fixes only. Swift/WinUI and legacy GTK source are retained.
Mac/newer-SDK rechecks remain separate; no whole parity group is accepted.

Next: run the opt-in new and existing desktop smokes
from an accessible Fedora session, validate actual input/pickers/clipboard and
resolve the packaged SWORD sanitizer defect in a separate dependency checkpoint.

Shared API notice (2026-10-18): complete per-tab last-search scope is now private
to the shared core (`c1b0a88`); worker/C query/clear and scalar metadata commands
are in `c38cd8f`. Existing normal/range searches reseed this tab's complete scope,
independent of display limits. Search capabilities copy from the shared runtime
predicate (`4e3402c`, SDK classification correction `76895f4`). Basic Mac controls
`0ca0bab` use copied scope/progress; other hosts should adapt the same APIs.
Validate `XIPHOS_SWORD_SEARCH_ICASE_FLAG` against this platform's SDK (tested Mac
uses 2; C++11 regex uses 1). Mac-hosted portable 98/98 is not new native-platform
acceptance; preserve the actual results below. See [search contracts](search-workflows.md).


Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the [shared handoff](handoff.md),
[status](status.md), [architecture](architecture.md) and [parity ledger](parity-matrix.md).

## Earlier incoming changes — current local checks recorded above

Fetch current master. Shared SDK render recovery `2b756bf` restores the caller's
attribute-processing flag on stored/buffer render failures without changing
healthy output. Full suites pass **82/82 Mac/core**, **66/66 portable Linux/core
on Mac**; focused SDK-recovery/snapshot **ASan/UBSan 5/5**, strict C++11 and
native-disabled helper checks pass. These are portable checks on this Mac, not a
current Fedora GTK build; the OFF configuration builds no GTK executable.

Native models `2390568` opt into shared heading links (`3411f45`), repairing only
exact presentation matches outside validated IDs. Mac window clicks checked OSIS
heading/body separation and distinct ThML fragments. Unsupported/mixed shapes
retain the original rendering. The new Linux model check covers click/hover,
retained-source links, linked entries, label toggles, fallback and shutdown.
Earlier lexical (`0b40574`) and spacing (`420ae13`) GTK changes also still need
current native validation. See [layout](word-block-workflows.md),
[spacing](double-space-workflows.md) and [notes](note-numbering-workflows.md).

Next Fedora task: default-ON build and all integrated tests (66 at this
checkpoint), then six desktop smokes and actual keyboard/pointer checks of
heading/body lookup, linked chapters, lexical/spacing/number controls, profiles
and history. Retain picker, accessibility, RTL/font/corpus and accelerated-renderer
gates. No hosted checks were used. The Mac chat now claims read-only general-book
tree/source characterization and a bounded shared slice; coordinate before
editing those APIs. Do not overwrite current Windows platform work.

## Recorded native Fedora checkpoint and authority

The owner authorized implementing and automatically merging locally checked Linux
milestones until matching the **existing macOS preview's implemented scope**.
This is not permission to bypass branch protection or spend hosted runner minutes.
`linux-core` remains manual-only; older workflows remain administratively paused.

Previous native implementation tested: `a80409b7d23d49a57a0a571e86ac09f15db9e8e6` on
`linux/macos-scope-audit`, following merged [#89](https://github.com/orbeavers14/xiphos/pull/89).
The subsequent handoff commit is documentation-only. The comparison target is
`platforms/macos/ReaderApp.swift` and its adapters at `8f12841`; macOS and shared
core sources have not changed during this Linux catch-up.

## Implemented comparison with macOS

| Existing macOS preview capability | Linux implementation and evidence |
|---|---|
| Module discovery, metadata and first-Bible startup | Reading/dictionary selectors, description/type/language, automatic John 3:16; startup smoke |
| Direct reading, normalization, paired commentary and history | Shared asynchronous snapshots, core Back/Forward/Clear; model and desktop checks |
| Chapters and book/chapter/verse choices | Core labels/targets, chapter navigation, verse numbers/highlight, sparse commentary modes/defaults |
| Commentary/standalone selection | Paired changes without history visits; standalone reading/chapter/link routing |
| Reading filters/variants | Same twelve declared toggles and variant choices; real SDK HTML and desktop checks |
| Heading/image/font presentation settings | Core getters, per-module updates and unknown/capability guards |
| Reading preferences import/save | Explicit native selected-file paths, core transactions, preservation/failure checks |
| Dictionary lookup, nearby keys and history | Core nine-key order, steps, separate recorded-click history, failures/close and retained source context |
| Lexicon preferences and XML import/save | Five nullable values, role-filtered installed choices/unavailable names, explicit XML transactions |
| Note/reference/lexical click results | Owned sidebar/dictionary snapshots, expansion/back-to-note, core ordering/limits/skipped metadata |
| Retained semantic hover information | Native hit testing, separate latest observer, live committed hover context, failed/late-result retention |
| Cross-reference list/information choice | Core-owned per-window setter/getter and both destinations |
| Multiple windows on one worker | Ctrl+N/New window, separate core tabs/settings/history/lexicons; close/pending-work isolation |
| Selected-verse scrolling | Copied escaped anchors in an isolated host world, verified successful Bible lookup/scroll |
| Nonpersistent embedded web content | Ephemeral WebKit network sessions, module scripts/resources blocked, semantic host CSS |
| Native desktop actions | Ctrl+L, Alt+Left/Right, Ctrl+N/W/Q, accessible labels and explicit VM software rendering |

These rows establish **implementation-scope equivalence**, not certified 1:1
visual, keyboard, accessibility, data/corpus or release parity. The legacy GTK3
app is retained; no one of the 38 full feature groups is accepted on this basis.

## Reproducible local environment

- Fedora 44 Workstation VM, aarch64.
- GCC 16.2.1; CMake 4.3.0; Ninja 1.13.2; native core/frontend C++17.
- pkg-config: SWORD 1.9.0, GLib 2.88.4, libxml2 2.12.10,
  GTK4 4.22.5, `webkitgtk-6.0` 2.54.1.
- Fedora prerequisites: `gtk4-devel webkitgtk6.0-devel` in addition to the core
  script's dependencies. The owner installed system packages in the terminal.
- Existing core release/ASan/UBSan checks each passed 47/47 at `8f12841`.
  No core API, GTK3, macOS or hosted-workflow source changed in catch-up PRs.
  All fixtures contain original prose; no installed module/profile was modified.

## Exact checks and results

From the repository root:

```sh
cmake -S platforms/linux -B build/gtk4 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gtk4 --parallel 2
ctest --test-dir build/gtk4 --parallel 2 --timeout 120 --output-on-failure
```

Final integrated result: **51/51 passed** (47 shared core + four Linux model tests).
The models check real SWORD render changes, chapter/commentary/selector behavior,
separate histories, busy/closed guards, note/ordered reference/lexical ownership,
hover invalidation/retention, profile read-only import and unrelated-data
preservation, nullable lexicons, dictionary source retention and shared-worker
window isolation. Shared legacy history deliberately retains forward entries;
the Linux host does not substitute browser-style history.

Final sanitizer result: **all four Linux model executables passed** with current
model source linked against the previously sanitized core. External SWORD/GLib/
libxml2/WebKit/GTK libraries are not instrumented; leak detection is off. To
reproduce, first run `platforms/linux/core.sh --sanitize --jobs 2`, then compile
each `platforms/linux/tests/{reader,options,preview,window}_model_test.cpp` with
`reader_model.cpp`, `-std=c++17 -g -fsanitize=address,undefined
-fno-omit-frame-pointer -Iplatforms/linux -Isrc/core/include`, the four
`build/linux-core-sanitized/libxiphos_core{,_render,_history,_options}.a` libraries,
`$(pkg-config --libs sword glib-2.0 libxml-2.0)` and `-pthread`.
Run with `ASAN_OPTIONS=detect_leaks=0
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` and these arguments:

| Executable | Fixture arguments (relative to repository root) |
|---|---|
| reader_model_test | `build/gtk4/core/study-passage-fixture build/linux-adapter-sanitized/empty-library` |
| options_model_test | `build/gtk4/core/module-preferences-fixture build/linux-adapter-sanitized/profile-fixture` |
| preview_model_test | `build/gtk4/core/module-preferences-fixture build/gtk4/core/dictionary-fixture build/linux-adapter-sanitized/lexicon-profile-fixture` |
| window_model_test | `build/gtk4/core/module-preferences-fixture` |

All six final-source desktop smoke modes passed in the Fedora session:

```sh
build/gtk4/xiphos-linux-preview --smoke-startup-test build/gtk4/core/study-passage-fixture
build/gtk4/xiphos-linux-preview --smoke-test build/gtk4/core/study-passage-fixture
build/gtk4/xiphos-linux-preview --smoke-options-test build/gtk4/core/module-preferences-fixture
build/gtk4/xiphos-linux-preview --smoke-preview-test build/gtk4/core/module-preferences-fixture
build/gtk4/xiphos-linux-preview --smoke-lexical-test build/gtk4/core/dictionary-fixture
build/gtk4/xiphos-linux-preview --smoke-window-test build/gtk4/core/study-passage-fixture
```

Smoke modes use software rendering. They exercise real windows, entry/button/
action signals and widget publication; startup additionally verifies an ephemeral
WebKit session, and the window smoke requires successful Bible anchor scrolling.
No physical key/pointer events or manual file-picker responses were synthesized.
Diff, documentation-link checks and the unchanged shared-core/macOS diff pass.

## Limits and next acceptance work

- No manual native visual comparison, physical keyboard/pointer-hover flows,
  file-picker selection/cancellation, screen reader, translated UI, RTL/font or
  representative licensed module/profile acceptance is claimed.
- The initial accelerated VM smoke timed out with EGL/DRI warnings. Software
  rendering passes; accelerated rendering/performance remains unaccepted.
  WebKit subprocesses sometimes emit D-Bus name-release warnings during successful
  shutdown. GTK compatibility file chooser APIs warn as deprecated on GTK 4.22;
  they retain the declared GTK 4.6 minimum.
- Chapter PR #85 had one initial undiagnosed smoke assertion followed by passing
  diagnostic runs; final expanded smoke modes pass. Preserve historical evidence
  in its PR rather than silently claiming manual GUI stability certification.
- Complete parity with the legacy app, user-data/session restoration, search,
  editors, installation/sync and release packaging remain beyond both previews.
- Next: review the above desktop/corpus acceptance gates on the native hosts;
  compare any future macOS changes before claiming ongoing scope equivalence.
  Keep new core contracts shared and coordinate ownership before editing them.

## Integrated milestones

[#82](https://github.com/orbeavers14/xiphos/pull/82): Linux core automation;
[#83](https://github.com/orbeavers14/xiphos/pull/83): first reading slice;
[#84](https://github.com/orbeavers14/xiphos/pull/84): native actions/software rendering;
[#85](https://github.com/orbeavers14/xiphos/pull/85): chapters/selectors;
[#86](https://github.com/orbeavers14/xiphos/pull/86): dictionary;
[#87](https://github.com/orbeavers14/xiphos/pull/87): reading options/profiles;
[#88](https://github.com/orbeavers14/xiphos/pull/88): contextual previews/lexicons;
[#89](https://github.com/orbeavers14/xiphos/pull/89): shared-worker windows/scrolling.
The final scope-audit implementation is identified above; Git history/PRs retain
per-checkpoint commands, intermediate counts and acceptance limitations.

## Shared API checkpoint to consume (2026-10-18)

Pull checked shared work through `cf80541`: attribute modes 3–5, explicit multiword
6/raw indexed 7, canonical `xc_search_query` with optional annotation context,
complete per-tab last scope and ordered bounded multi-module requests. Core batches
`f86293b`, C `ad28750`, Swift `cf80541` use one worker; do not duplicate scope/context
or query preparation in the frontend. `xc_worker_search_modules` takes one common
query (module NULL), ordered module IDs and optional total output limits. It returns
owned batch/per-group status; successful child searches are borrowed const until
batch release and must not be released separately. Existing C requests/layouts stay
compatible. See [search contracts](search-workflows.md).

Mac validation passed 136/136 Mac/core and 105/105 portable core/model tests on Mac,
with focused ASan/UBSan/TSan and strict C11/C++11 checks. This does not update this
platform's native compiler/GUI evidence above. Rebuild locally with the actual
SDK/case flag before accepting target behavior; no hosted build should be enabled
or dispatched. Basic Mac controls exist; advanced controls/indexed execution,
mixed families, export/settings/full acceptance remain pending.

Latest follow-up: `8c0a4ed` shares the exact export item formatter with retained GTK
(C++11 + C11 facade; free with `xiphos_export_item_free`) and characterizes advanced
range/export/bookmark quirks. Full GTK3 desktop linking/runtime is not claimed by
Mac C adapter tests. `a3dcae8` adds an actual-index diagnostic: default optional
mode explicitly skips unavailable frameworks; set
`XIPHOS_REQUIRE_INDEXED_SEARCH_TEST=ON` for a mandatory target gate. This Mac SDK
returned creation status 0 without a usable index, so actual indexed execution
remains unvalidated. The expected failing required-mode probe is not acceptance.
Latest full local totals: 139 Mac/core passes + 1 skip; 108 portable passes + 1
skip on Mac. Export focused normal/ASan checks pass 3/3; earlier search TSan evidence
remains above. These do not replace this OS's native evidence.

The next shared slice is [retained search-result generations](search-export-design.md),
then bounded export. That proposal is not an implemented API. Display prefixes
and final batch last scope cannot reconstruct every group's complete results;
do not create a frontend workaround or implicitly rerun searches. Overall 21/38
groups now have some core/Mac evidence, 0/38 are accepted and 0/3 native releases
are ready. Keep local-only checks and preserve all legacy retirement gates.


## Mac-hosted shared alias search integration (2026-10-22)

Shared registry/catalog/C transport now resolve execution target aliases, retain
literal requests and explicit bounds providers, and expose actual installed source
IDs. Linux model hit/preview/activation uses additive `xc_search_source_module`;
request group labels stay literal. New headless `linux_search_alias_model` checks
raw duplicate/Unicode aliases, distinct actual-source preview, retained export
context/content and activation through the existing shared worker. It is outside
the GTK runtime switch and runs with the portable model suite.

Actually run on Mac: complete portable build and135 passes+1 explicit index skip,
136 registered,79.07s, GTK runtime OFF. Shared scoped ASan/UBSan and TSan11/11 each
also pass on Mac. See [Mac evidence](handoff-macos.md#alias-search-execution-and-source-provenance-2026-10-22)
and [contract](module-abbreviations-workflows.md). This does not replace recorded
Fedora indexed execution or validate Fedora's installed-SWORD sanitizer issue,
GTK desktop interaction, corpus, WinUI/MSVC or whole-feature parity. Current
Fedora recheck remains needed; no hosted workflow was dispatched.


### Mac-hosted portable URI/reference checkpoint (2026-10-22)

Implementation through `c70cca9` is on the fork master. Shared showRef exact
module/main fallback, single-URI alias/unknown fallback, complete retained URI
callback oracle and Bible-only contextual URI range previews now feed the existing
Linux model. Aggregate rendered and link-only previews cap2000 ordered passages
with disclosure; native models retain actual source identity, committed main/source
roles and search/history isolation. Full Mac-hosted portable/Linux build succeeds;
full138-case selection passes137 with1 explicit index skip (106.42s). This is
Mac ARM64 validation with GTK runtime OFF, not current Fedora/GTK desktop or
Windows/MSVC evidence. Final focused ASan/UBSan11/11 and TSan11/11 are also
Mac-hosted with previously documented SDK/instrumentation limits. Native-disabled
C++11 policy/render libraries build and registry/URI oracles2/2 pass. See
[Mac evidence](handoff-macos.md) and [URI contract](passage-uri-workflows.md) for
limitations and the pre-copy admission bound next step. No whole parity claim.
