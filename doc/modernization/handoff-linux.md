# Linux / Fedora / WSL2 chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md) and the
[shared handoff](handoff.md) first. Active work: bring Linux to the existing macOS preview scope in locally checked,
automatically merged milestones, authorized by the owner. Current branch:
`linux/macos-reading-parity`; shared API changes are not currently needed.

## Existing evidence and boundaries

`f190c49` (PR #82) adds Fedora/Debian/Ubuntu dependency setup, standalone release
and sanitizer checks, explicit C++17 selection and CMake extraction fixes.
`cf9c853` removes automatic Linux hosted triggers; do not put them back.
The owner reported passing Fedora release/sanitizer runs after PR #82.
This Linux chat has now inspected the environment and is recording fresh
reproducible checks of the integrated revision below. The first scoped GTK4 frontend is now implemented under `platforms/linux`;
its implementation and acceptance limits are recorded below. Core build success does not establish Linux interface parity.

## Next bounded work

1. Capture the existing Fedora validation details; rerun only checks needed to
   establish missing evidence or validate subsequent changes. Confirm the core
   uses C++17 on the selected compiler.
2. Create an isolated native GTK4 frontend build under `platforms/linux`, leaving
   the root GTK3 build operational. Inspect available GTK4/WebKitGTK dependencies
   on Fedora before choosing package names or promising minimum versions.
   The target renderer is GTK4-compatible WebKitGTK; do not reuse the legacy
   GTK3 WebKit package as if it were GTK4-compatible.
3. Implement the first module selection / reference / Bible-and-commentary panes /
   Back-and-Forward slice through the existing asynchronous C API or a thin C++
   adapter to the same worker. Inspect `src/core/include/xiphos/core/c_api.h` and
   the macOS consumer before adding an API. Copy borrowed values before releasing
   results. Never run engine jobs or blocking shutdown on the GTK main loop.
4. Check empty libraries, missing modules, failed navigation, repeated actions,
   close-with-work-pending and link/history behavior. Match the established pane
   composition; module routing, history and persistence must remain core-owned.
   Record keyboard and real desktop interaction evidence separately from build tests.

## Local validation

From the repository root on Fedora or another supported Linux environment:

```sh
platforms/linux/core.sh --install-deps
platforms/linux/core.sh --sanitize
```

Omit `--install-deps` when dependencies are already present. The script builds and
runs standalone core tests; it does not install GTK4 or build a native UI. Use
`--jobs N` to fit the computer, and separate build directories for toolchains/modes.
See [the Linux guide](../../platforms/linux/README.md) for details. Add documented
local GTK4 build/run commands when that frontend actually exists; do not invent
passing GUI evidence before using it.

WSL2 can validate Linux tooling and GUI iterations. Record it as WSL2; it does
not validate native Windows/MSVC or every full Linux desktop integration. A Fedora
desktop/VM can supply those Linux interaction checks. Hosted builds stay manual
and require owner authorization; no runner minutes are needed for this work.

## Checkpoint evidence (2026-10-14 / Fedora Linux)

- Task/branch: `linux/gtk4-reader-slice`, first Linux reading slice; checked locally.
- Core implementation tested: `8f12841` (includes PR #82 / `f190c49`).
  Fresh standalone release and ASan/UBSan runs each pass **47/47** tests.
  Commands: `platforms/linux/core.sh --jobs 2` and
  `platforms/linux/core.sh --sanitize --jobs 2`. Generated Ninja flags explicitly
  contain `-std=gnu++17`; no hosted jobs were dispatched or enabled.
- Environment: Fedora 44 Workstation, aarch64, GCC 16.2.1, CMake 4.3.0,
  Ninja 1.13.2; pkg-config reports SWORD 1.9.0, GLib 2.88.4,
  libxml2 2.12.10, GTK4 4.22.5, WebKitGTK 6.0 API library 2.54.1.
  Fedora package metadata confirms `gtk4-devel` and `webkitgtk6.0-devel`;
  the owner installed them in the terminal. Original prose fixtures only.
- Changed behavior: independent GTK4/WebKitGTK build, module selection, direct
  reference entry, paired Bible/commentary snapshots and core Back/Forward.
  One FIFO core worker; nonblocking GTK polling, copied results, GTask drain
  on close. No shared API or legacy GTK3 changes. Unsupported links retain
  the last committed passage; passage links route through the shared core.
- Adapter checks: configured `cmake -S platforms/linux -B build/gtk4 -G Ninja
  -DCMAKE_BUILD_TYPE=Debug`; built preview/model test/core study-passage fixture.
  `ctest --test-dir build/gtk4 -R '^(core_study_passage|linux_reader_model)$'
  `--output-on-failure` passes **2/2**, including fixture setup. Model checks
  cover catalog/empty roots, paired HTML, normalization, history restoration,
  missing modules, invalid moves, external/passage links, busy guards and
  close with startup/visit work pending.
- Sanitized adapter: compiled `reader_model.cpp` and `tests/reader_model_test.cpp`
  with `-std=c++17 -g -fsanitize=address,undefined -fno-omit-frame-pointer`,
  linking the four freshly sanitized core static libraries plus pkg-config
  SWORD/GLib/libxml2 and pthread; executed with
  `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
  All adapter checks pass. External SDKs are uninstrumented; no leak audit.
- Desktop smoke: `GSK_RENDERER=cairo build/gtk4/xiphos-linux-preview
  --smoke-test build/gtk4/core/study-passage-fixture` passes on the Fedora VM.
  This uses real GTK entry/button signals, both WebKit panes and pending-work
  close; smoke mode disables WebKit acceleration. The initial default-renderer
  smoke timed out with VMware EGL/DRI graphics warnings. Accelerated rendering,
  manual keyboard/accessibility, screenshots/visual comparison and representative
  installed modules are **not accepted**. This is not full GTK4 parity.
- Shared API/other platforms: unchanged. Existing macOS/Windows consumers and
  GTK3 build sources are untouched. Central parity totals are unchanged.
- Next: inspect the bounded preview on a real desktop, verify keyboard/focus and
  accelerated/software rendering, then add chapter/preview/dictionary controls
  through the same worker. Keep hosted checks manual-only.

Implementation source tested: `24a07ba353da1a6f921f5bda34be61527f0a9ccb`.
The subsequent handoff commit changes documentation only.

Exact additional sanitizer command (run from the repository root after the
standalone sanitized build and GTK fixture setup above):

```sh
mkdir -p build/linux-adapter-sanitized
c++ -std=c++17 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Iplatforms/linux -Isrc/core/include \
  platforms/linux/reader_model.cpp platforms/linux/tests/reader_model_test.cpp \
  build/linux-core-sanitized/libxiphos_core.a \
  build/linux-core-sanitized/libxiphos_core_render.a \
  build/linux-core-sanitized/libxiphos_core_history.a \
  build/linux-core-sanitized/libxiphos_core_options.a \
  $(pkg-config --libs sword glib-2.0 libxml-2.0) -pthread \
  -o build/linux-adapter-sanitized/reader_model_test
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  build/linux-adapter-sanitized/reader_model_test \
  build/gtk4/core/study-passage-fixture build/linux-adapter-sanitized/empty-library
```

Build the focused GTK targets with:
`cmake --build build/gtk4 --target xiphos-linux-preview linux_reader_model_test core_study_passage_test --parallel 2`.
The GTK guide documents a full frontend/core build before running the complete
CTest registration; only the focused 2/2 frontend/fixture tests and independent
standalone 47/47 core suites were executed for this checkpoint.


## Desktop-controls checkpoint (2026-10-14 / Fedora Linux)

- Task/branch: `linux/gtk4-desktop-controls`; ready for automatic merge after
  scoped local success, as explicitly authorized by the owner.
- Implementation tested: `ade10a7635594eeccf9b6b7974157a7e93aceaac`.
- Same Fedora ARM64 toolchain/dependencies as the preceding checkpoint.
- Changed: `--software-rendering` disables WebKit acceleration and selects GTK
  Cairo unless the user explicitly overrides `GSK_RENDERER`. Named window
  actions connect buttons, Enter and Ctrl+L/Alt+Left/Alt+Right accelerators.
  Bible/commentary selectors have mnemonics; selectors/reference/web panes have
  accessible labels. Pending engine work cannot overwrite a newer reference
  draft. Actions remain disabled until initialization succeeds, during pending
  navigation and at unavailable history boundaries.
- Exact checks: `cmake --build build/gtk4 --target xiphos-linux-preview
  linux_reader_model_test --parallel 2` succeeds; `ctest --test-dir build/gtk4
  -R '^(core_study_passage|linux_reader_model)$' --output-on-failure` passes 2/2.
  Final-source `build/gtk4/xiphos-linux-preview --software-rendering --smoke-test
  build/gtk4/core/study-passage-fixture` passes, verifying accelerator
  registration, focus placement, pending draft preservation, module selection,
  paired panes, Back/Forward actions and close with work pending.
  `build/gtk4/xiphos-linux-preview --help` and `git diff --check` pass.
- Desktop evidence is automated action/signal wiring, not physical key events or
  screen-reader usability. VMware emits EGL/DRI warnings; WebKit subprocesses
  occasionally emit D-Bus name-release warnings during successful shutdown.
  No GLib focus-out warning occurred after preserving reference-field focus.
- Shared API/core/GTK3 unchanged; no hosted jobs dispatched. The previous
  standalone 47/47 release/sanitizer and sanitized adapter evidence remains
  applicable to unchanged core/model sources; those suites were not rerun for
  a UI-only edit. Full visual, manual keyboard/accessibility and accelerated
  rendering acceptance remain pending; central parity scores are unchanged.
- Next bounded implementation: chapter snapshots/navigation and pane previews
  using the existing worker. Obtain representative desktop/manual acceptance
  separately; retain the existing GTK3 app and all unfinished parity gates.


## macOS scope catch-up: chapters (2026-10-14)

- Owner authorized implementing and merging successive checked Linux milestones
  until matching the existing macOS preview scope. This is not full legacy parity.
- Implementation: `d205c0e0c568622bcf478b8cd68e8fd611ed2959` on
  `linux/macos-reading-parity`, based on merged #84 (`867a53a`).
- Added core-owned book/chapter/verse targets, Bible chapter/verse numbers/highlight
  controls, sparse commentary chapters using the core per-module default,
  Previous/Next chapter and Clear History. Refreshes do not create history visits.
  Native CSS supplies selected-row and dark-theme contrast. Selected-anchor copying
  is implemented; host scrolling remains a follow-up.
- Same Fedora ARM64 dependencies. Built `xiphos-linux-preview` and
  `linux_reader_model_test`; focused CTest/fixture 2/2 passes. The exact sanitized
  adapter command above passes with new chapter/mode/history/choice checks.
- Expanded opt-in `--smoke-test` checks chapter controls, chapter navigation,
  selectors and Clear History as well as the prior interactions. One initial
  assertion failed without diagnostics; diagnostic reruns passed twice. Preserve
  this as a GUI stability limitation, not manual visual/keyboard acceptance.
- The first history test expected browser-style truncation; corrected to the
  documented shared legacy rule retaining forward entries. Core behavior is
  unchanged and identical across macOS/Linux. No shared API or GTK3 changes.
- No hosted builds; diff checks pass. Next: dictionary lookup, key steps/chooser,
  separate dictionary history; then reading/presentation options and profiles,
  contextual note/reference/lexical click/hover, scroll and window acceptance.

### Compared with macOS at `867a53a`

| Existing macOS capability | Linux implementation checkpoint |
|---|---|
| Module/passage/commentary, history and actions | #83–84 |
| Chapters, core selectors, chapter/history controls | Chapter checkpoint above; scroll pending |
| Reading filters/variants and presentation preferences | Options checkpoint below |
| Engine preference import/save | Options checkpoint below; picker manual acceptance pending |
| Dictionary lookup, key steps/chooser/history | Dictionary checkpoint below; contextual lexical feeds follow |
| Lexicon preferences and XML import/save | Contextual checkpoint below |
| Note/reference-list/lexical click results | Contextual checkpoint below |
| Semantic hover and retained information | Contextual checkpoint below; manual pointer acceptance pending |
| Cross-reference list/information setting | Contextual checkpoint below |
| Per-window isolation on one process worker | Single-window host; pending |
| Native selected-anchor scrolling | Pending |

Keep remaining gaps explicit; no feature group has complete accepted parity.


## macOS scope catch-up: dictionary (2026-10-14)

- Branch `linux/dictionary-parity`, implementation
  `3590cdf64e670609d66be4a5e37f463e116c2260`, following merged chapter PR #85.
- Same Fedora ARM64 environment. Dictionary catalog classification and owned
  snapshots now feed a lower-right native pane with module/key entry, nine-key
  chooser, Previous/Next keys and separate Back/Forward/Clear/Close controls.
- Direct lookup and key steps remain unrecorded, matching macOS. The adapter
  tests explicit recorded visits and dictionary history restoration separately;
  main lexical clicks will populate the UI history in a subsequent checkpoint.
- Local build succeeds; focused fixture/adapter CTest 2/2 passes. The exact
  ASan/UBSan adapter command above passes updated dictionary checks (real RawLD
  fixture, nine repeated boundary keys, failures retaining display, separate
  recorded history, clear without display loss, and close). The desktop
  `--smoke-test` passes lookup/key-step/close plus all previous reading checks.
- Initialization failure now disables the full host; dictionary stepping/history
  updates normalized keys only if the user has not typed a newer key draft.
- Core API/GTK3/macOS unchanged; no hosted builds. Diff checks pass. No new
  manual visual/keyboard/accessibility or accelerated-rendering acceptance.
- Next: reading filter/variant controls, presentation preferences and explicit
  selected-file profile import/save, followed by contextual links/hover.


## macOS scope catch-up: reading options/profiles (2026-10-14)

- Implementation `ffbc8da43e3193102e259bd8fe5cc3a1eafe2972`, branch
  `linux/options-profile-parity`, based on merged dictionary PR #86.
- Same Fedora ARM64 environment. Added twelve native filter toggles, variants,
  Bible/commentary image/font states, commentary headings and core capability
  guards. Native selected-file Import/Save delegates to core; no automatic
  discovery/writes. The chooser uses GTK's deprecated compatibility API for the
  declared GTK 4.6 minimum. No SDK minimum silently increased.
- `cmake --build build/gtk4 --target xiphos-linux-preview linux_reader_model_test
  linux_options_model_test core_scoped_options_test core_module_preferences_test
  --parallel 2` succeeds. `ctest --test-dir build/gtk4
  -R '^(core_study_passage|linux_reader_model|linux_options_model)$'
  --output-on-failure` passes 5/5 including required fixtures.
- Sanitized `options_model_test.cpp` was compiled with the exact adapter command
  above, substituting that test source and `options_model_test` output, then run
  with the same ASAN/UBSAN environment against
  `build/gtk4/core/module-preferences-fixture` and
  `build/linux-adapter-sanitized/profile-fixture`: passes.
- Real fixture checks cover declared capabilities, changed SWORD HTML, no history
  visits on filter refresh, variant flags, save preserving unrelated Unicode
  values, read-only import restoring flags, and failed import/save retaining state.
- `--smoke-options-test build/gtk4/core/module-preferences-fixture` passes native
  filter/variant/history checks. The existing study-passage `--smoke-test` also
  passes, including chapters and dictionary controls. File-picker interaction,
  manual keyboard/accessibility and representative visual acceptance remain
  pending. VMware/WebKit shutdown warnings remain as previously recorded.
- Core API, GTK3, macOS and hosted workflow files unchanged; no hosted jobs run.
  Diff checks pass. Next: lexicon preferences and explicit XML import/save,
  contextual note/reference/lexical clicks/hover, selected-anchor scrolling and
  per-window isolation through one process worker.


## macOS scope catch-up: contextual previews/lexicons (2026-10-15)

- Implementation `49348037e1e15e3a9f29d8bd5cd9fa150e07f97e`, branch
  `linux/contextual-preview-parity`, following merged options/profile PR #87.
- Same Fedora ARM64 environment. Added copied note/reference/lexical results,
  contextual pane links, expanded note references/back-to-note, limit/skipped
  metadata, retained information, native WebKit hit-test hover and per-tab
  cross-reference list/information routing. Hover observers are independent of
  main actions and invalidate on navigation; underlying engine work stays FIFO.
- Five nullable lexicon preferences now expose role-filtered installed choices,
  unavailable-name editing, Apply and native explicit XML Import/Save. Unset vs
  empty survives copying; XML save rejects unroundtrippable values without writes.
  Main lexical clicks use core Unicode casing and separate dictionary history;
  hovers never record visits. No platform URI or reference parser was added.
- Built preview, three Linux model tests and `core_dictionary_test`.
  `ctest --test-dir build/gtk4 -R '^linux_.*model$' --output-on-failure`
  passes 7/7 including fixtures after correcting the generated XML test root to
  the documented case-sensitive `Xiphos` spelling.
- Sanitized `preview_model_test.cpp` compiled/run with the adapter command above,
  substituting the source/output name and passing module-preferences fixture,
  dictionary fixture and disposable lexicon-profile scratch directory: passes.
  Checks cover note/list/lexical ownership, five repeated references, failed and
  late hover retention, navigation invalidation, nullable preferences, unrelated
  XML preservation/read-only import and failed save/import state preservation.
- Native `--smoke-preview-test build/gtk4/core/module-preferences-fixture` and
  `--smoke-lexical-test build/gtk4/core/dictionary-fixture` both pass widget
  publication/signal flows. These are not physical pointer events or native
  picker/screen-reader/visual acceptance. SDK instrumentation limits and VMware
  EGL/DRI/WebKit D-Bus shutdown warnings remain as before.
- Core/GTK3/macOS/workflows unchanged; no hosted builds. Diff checks pass.
  Next: selected-anchor host scrolling and multiple windows sharing one worker;
  final scope audit against the macOS preview and explicit manual acceptance gaps.
