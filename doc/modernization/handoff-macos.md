# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md) and the
[shared handoff](handoff.md) first.

## Active shared-core snapshot task

The macOS chat has resumed from `74a4190` on `master`: shared lexical snapshot
integration, format-before-cleanup, document-wide fallback, copied CSS/status,
worker/C ABI and compatible native adapter consumption. Existing consumers stay
compatible until they explicitly opt in to the new presentation path. Scope:
shared content/session/worker/C API, their tests, native adapter integration and
associated evidence. Other chats should avoid overlapping shared API edits while
this task is active; platform-local acceptance can proceed independently.

## Latest checked follow-up: owned attributes and heading metadata

`858ea63` shares UTF8HTML attribute reads across native heading collection and the
legacy backend; 80 frozen/current/owned cases plus refresh/lifetime/NUL guards.
`e3fcdb2` copies SDK Preverse fragments and exact cleanup-point note-label metadata;
its private fixture reproduces frozen heading-then-body numbering with owned
counters. Current suites: 72/72 Mac; 56/56 portable Linux/core **on Mac**. Focused
ASan/UBSan 2/2 including the heading prerequisite, strict C++11 and native-disabled
helper build pass. Full legacy GTK runtime and native Fedora/MSVC remain untested.

Commands: normal configured Mac and headless Linux full-suite commands above;
new sanitizer target `core_heading_note_metadata_test` plus
`core_heading_collection_test`, followed by
`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_heading_note_metadata$' --no-tests=error --output-on-failure`.
Standalone C++11 syntax checked `entry_attribute.cpp` and `headings.cpp` with the
core/SWORD include directories. Rich metadata is not yet used by Reader snapshots
or native numbering controls. The shared task remains active for owned SDK
filter-state restoration and compatible opt-in Reader snapshot adoption.
See [the note seam](note-numbering-workflows.md) for exact semantics and limits.

## Latest checked follow-up: note-numbering helpers

`c39b024` extracts owned literal counters into shared C++11 and retains legacy
cleanup behavior; `8572594` shares the SDK label switch and characterizes all four
XHTML filters. Current Mac suite passes 71/71; portable Linux/core passes 55/55 on
Mac. Focused cleanup ASan/UBSan and TSan each 1/1; SDK-switch ASan/UBSan 1/1;
strict C++11 and native-disabled helper builds pass. No native numbering controls
are enabled yet. See [contracts, commands and next boundaries](note-numbering-workflows.md).
The active task now includes owned entry-attribute access and refresh semantics;
other chats should avoid shared render/attribute API edits while this continues.

Commands used after configured local SDK discovery:

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --no-tests=error --output-on-failure
cmake -S platforms/linux -B build/linux-models -C build/linux-model-sdk.cmake -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --no-tests=error --output-on-failure --parallel 2 --timeout 30
cmake --build build/linux-models-sanitized --target core_content_cleanup_test core_sdk_note_numbers_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^(core_content_cleanup|core_sdk_note_numbers)$' --no-tests=error --output-on-failure
cmake --build build/thread-sanitized --target core_content_cleanup_test --parallel 2
ctest --test-dir build/thread-sanitized -R '^core_content_cleanup$' --no-tests=error --output-on-failure
```

Focused sanitizer names were run separately for recorded 1/1 results. External
SDKs remain uninstrumented; no new GUI claim accompanies these helper extractions.
The three temporary Mac fixture apps launched during this run were quit and their
known generated copies removed. Repository source, user data and unrelated files
remain intact. No hosted build allowance or reset was used.

## Latest checked follow-up: double spacing

`25cf291` adds exact legacy double-spacing defaults/persistence and queued
session/worker/C ABI support; `68562af` connects Mac controls and `420ae13` Linux
controls. Current source passes 70/70 Mac CTests and 54/54 portable Linux/core
CTests on this Mac. Focused ASan/UBSan checks pass 4/4 core/preference and 3/3 Linux
model including fixture prerequisites. Strict C++11 presentation/C11 header checks
and native-disabled helper build pass. Actual Mac GUI on/off chapter spacing
refresh retains passage/selection. Full commands/contracts and limits are in
[double-spacing workflows](double-space-workflows.md).

Additional commands used (same configured local SDKs):

```sh
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --no-tests=error --output-on-failure
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --no-tests=error --output-on-failure --parallel 2 --timeout 30
cmake --build build/linux-models-sanitized --target core_double_space_test core_content_preferences_test core_lexical_snapshots_test core_heading_collection_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^(core_double_space|core_content_preferences)$' --no-tests=error --output-on-failure
cmake --build build/linux-models-sanitized --target linux_lexical_model_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^linux_lexical_model$' --no-tests=error --output-on-failure
```

Next bounded shared task: characterize note-number rendering (saved
`XrefNoteNumbers`, SDK setters, user-supplied labels, legacy counters and restore
behavior) before altering shared render paths. The active core task continues.
No GitHub build allowance or reset was used; GTK/Fedora/MSVC gates remain pending.

## Current checked checkpoint

Implementation commits: `1393edd` shared nesting guard/captured heading wrappers,
`d953adc` Mac HTML/CSS view integration, `0b40574` portable Linux adapter. Swift
value copying was checked at `0af3689` (67/67 before the added native model test).
The combined current source passes 68/68 Mac CTests and 53/53 portable Linux/core
CTests on this Mac. Focused ASan/UBSan runs pass 4/4 (heading fixture, lexical
snapshot, Linux lexical model, word blocks); leak checks off, external SDKs not
fully instrumented. Strict C++11 syntax and native-disabled legacy helper builds
pass. No hosted checks, resets or installed module/profile changes were used.

Actual disposable Mac window checks: read John 4:1, enable Strong's, verify 3056
beneath its word, switch to chapter and select John 4:2, then read John 6:1 and
verify unsupported font nesting retains readable inline annotations without
positioning CSS. Quit the temporary app afterward. Paired commentary and draft/
history/window isolation were checked by models, not manually claimed here.

Commands (configured local SDK/deployment settings retained):

```sh
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --no-tests=error --output-on-failure
cmake -S platforms/linux -B build/linux-models -C build/linux-model-sdk.cmake -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --no-tests=error --output-on-failure --parallel 2 --timeout 30
cmake --build build/linux-models-sanitized --target linux_lexical_model_test core_lexical_snapshots_test core_heading_collection_test core_word_blocks_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^(linux_lexical_model|core_word_blocks)$' --no-tests=error --output-on-failure
clang++ -std=c++11 -fsyntax-only -Isrc/core/include -I/usr/local/include/sword src/core/word_blocks.cpp
cmake --build build/legacy-seam-build --parallel 2
sh platforms/macos/launch-fixture.sh build/macos lexical
```

The ignored SDK seed is copied from this Mac's configured dependency cache, not a
portable SDK. Sanitizer build uses Debug and address/undefined flags on C/C++ and
linking. New GTK application source is not compiled here; Fedora and MSVC/native
Windows remain separate gates. Full corpus/RTL/font/accessibility/visual parity
and original-parser retirement remain unaccepted.

The double-spacing follow-up above completes that bounded implementation; broader presentation and corpus gates remain.
The active task claim remains in force while this Mac chat continues shared work.

## Previous checked milestones

Current-run core checkpoint: `7fd1bce`. Opt-in session/worker entry/chapter/paired
snapshots carry matching lexical CSS/status, stage formatting before cleanup and
roll unsupported panes back as whole documents. Full local suite 66/66; focused
normal/ASan/UBSan/TSan runs 2/2 including the private fixture setup. Existing
consumers stay on the compatible plain path. Later current-run checkpoints above add C/Swift/native consumption.

Current-run C ABI checkpoint: `8c0f4f2`. Strict queued opt-in plus typed borrowed
CSS/status getters for entry, paired panes, chapter and verses; C11 and owned C
snapshot/null/bounds/lifetime tests pass. Full local suite 66/66; ASan/UBSan and
TSan snapshot runs each 2/2. Later checkpoints above add Swift value copying and matching native view CSS.

The macOS chat completed the shared lexical corpus/ownership/layout checkpoints
below on `master`, starting from `74a4190`. These older completed checkpoints do not supersede the active claim above;
other chats must fetch before overlapping shared-core work. The owner requested local development until the usage allowance
is reached. No reset credit or hosted build allowance was consumed.

Completed/pushed implementation checkpoint: `f2a2c61`, 2026-10-15. Owned lexical
layout now accepts opaque SWORD note markers, preserves their links/labels, checks
OSIS/ThML paragraph and multi-anchor data, and guards uppercase lexical anchors
before unsafe inherited searches. 1,502 legacy-output comparisons; 65/65 macOS
suite, focused ASan/UBSan 1/1 and TSan 1/1, strict C++11 and native-disabled helper
builds passed. No public API/frontend adoption changed.

Completed/pushed follow-up: `f9a8e0e`, 2026-10-15. Checked owned-string annotation
edits retain NASB/class-shim/spacing/morphology behavior over 1,530 oracle cases.
65/65 local suite; focused ASan/UBSan and TSan each 1/1; strict C++11 and legacy
helper builds pass.

Completed/pushed scanner checkpoint: `77b97a5`, 2026-10-15. Scanner word/annotation
state and recursive working text now use C++ owned strings; the new source no
longer calls GLib allocation/free/search functions. 1,638 complete-oracle cases,
8,000 noise inputs, 3,039 structured mutations and 400 concurrent renderer calls
pass; full macOS suite 65/65; focused ASan/UBSan and TSan each 1/1; strict C++11
and native-disabled helper builds pass. Legacy source remains the retained oracle.

Completed/pushed layout checkpoint: `0f7aed0`. Additive C++ `layout_lexical_content` bundles copied display
flags, rendered HTML, matching CSS and requested/applied state for later shared
snapshot adoption. 48 policy cases and 400 concurrent combined calls pass;
full local suite 65/65, focused ASan/UBSan and TSan each 1/1, strict C++11 and
native-disabled helper builds pass. It does not change the C ABI or either native
frontend yet.

Completed/pushed integration-test checkpoint: `7986d20`. Real worker-generated
note and ranged-reference URLs survive formatting and still resolve to the right
note body or reference list. Normal, ASan/UBSan and TSan focused runs each pass
3/3 (including fixture prerequisites). Production source is unchanged since the
65/65 suite at `0f7aed0`; the subsequent test additions were checked with these
focused runs. No GUI acceptance or native snapshot adoption is claimed.

Environment: macOS 27.0.1 (26A434), ARM64; AppleClang 21.0.0,
Swift 6.4 toolchain (Swift 6 language mode), SWORD 1.9.0.3916 and configured
GLib 2.86.5. Local deployment target remains 26.0, not evidence of macOS 13.
External SWORD/GLib/libxml2 libraries are not fully sanitizer-instrumented;
ASan leak detection is disabled. Windows/Fedora reruns of these new core commits
are pending. No module corpus or user data was downloaded or modified.

Commands to reproduce these local milestones (SDK discovery was already configured):

```sh
cmake --build build/macos -j4
ctest --test-dir build/macos --output-on-failure
cmake --build build/sanitized --target core_word_blocks_test core_study_note_test -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized -R '^core_word_blocks$|^core_study_notes$' --no-tests=error --output-on-failure
cmake --build build/thread-sanitized --target core_word_blocks_test core_study_note_test -j4
ctest --test-dir build/thread-sanitized -R '^core_word_blocks$|^core_study_notes$' --no-tests=error --output-on-failure
clang++ -std=c++11 -fsyntax-only -Isrc/core/include -I/usr/local/include/sword src/core/word_blocks.cpp
cmake --build build/legacy-seam-build -j4
```

The final command uses the pre-existing ignored native-disabled helper harness;
it does not build or validate the complete GTK3 application. The two focused
test names were run separately for the recorded 1/1 and 3/3 results above;
the combined regex is a convenient rerun. Build-cache paths/configuration are
local, so use the platform build guide to configure a different computer first.

## Existing evidence and boundaries

At `9e288b3`, the macOS preview/shared-core suite had 65/65 passing local checks.
Scoped address/undefined and thread sanitizer checks, C++11 helper builds and
disposable fixture GUI checks are recorded in [status](status.md). These are
historical evidence, not a claim that a fresh checkout was retested today.
AppleClang 21 / Swift 6 and local SWORD 1.9.0.3916 / GLib 2.86.5 were used.
Those local dependency builds require macOS 26, so they do not validate the
requested macOS 13 deployment minimum. No full native release is accepted.

The SwiftUI/AppKit preview already adapts core reading, paired commentary,
histories, options, dictionary and note/reference/hover behavior. Consult the
platform README and tests before rebuilding any of this independently.

## Next bounded work

1. Characterize note-number rendering, SDK filter state, explicit labels and
   legacy chapter counters before adding the next shared presentation command.
2. Broaden SDK/RTL/font/note/paragraph layout and fallback evidence. New native
   views consume shared snapshots; Fedora must validate its GTK source/runtime.
   See [word block workflows](word-block-workflows.md). The old parser stays.
3. Preserve main/commentary/dictionary pane composition, draft input, histories,
   unknown option eligibility, busy/closed/stale-result guards and window isolation.
4. Review Linux/Windows requests for shared API adjustments before changing those
   same contracts. Larger search/storage/editor work follows scoped parity evidence.

## Local validation

Follow [the macOS build guide](../../platforms/macos/README.md) for SDK discovery
and deployment settings. From the repository root, with configured dependencies:

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --output-on-failure
```

Use focused checks while iterating and the appropriate regression suite at a
milestone; do not repeat full builds without a reason. For GUI work, use the
documented disposable fixture launcher outside Documents, inspect the actual
interactions and quit the app when done. Do not weaken macOS privacy protections.
Run sanitizer checks relevant to ownership/concurrency changes; report external
SDK instrumentation limits. Do not claim Linux/MSVC acceptance from these checks.

## Next handoff entry

Replace this section with the [checkpoint format](handoff.md#evidence-to-record-at-each-checkpoint)
when work begins/finishes. Include the tested implementation commit, actual results,
remaining gaps and shared API impact. Commit and push it so other OS chats can read it.
