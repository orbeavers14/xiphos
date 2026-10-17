# Linux / Fedora / WSL2 chat handoff

Shared API notice (2026-10-17): initial verse search now owns query/results,
optional explicit range-source module/bounds, atomic cooperative progress/cancel,
and copied C/Swift values (`06286b7` through `d77d61c`). No native search UI or
full search parity is claimed. Validate `XIPHOS_SWORD_SEARCH_ICASE_FLAG` against
this platform's SDK (tested Mac SDK uses 2; C++11 regex backend uses 1). These
Mac checks do not change this platform's recorded results below. See
[search contracts](search-workflows.md) before consuming the new APIs.


Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the [shared handoff](handoff.md),
[status](status.md), [architecture](architecture.md) and [parity ledger](parity-matrix.md).

## Current incoming changes — native Fedora rerun required

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
