# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md) and the
[shared handoff](handoff.md) first.

## Active shared-core snapshot task

The macOS chat has resumed from `fff8740` on `master`: shared lexical snapshot
integration, format-before-cleanup, document-wide fallback, copied CSS/status,
worker/C ABI and compatible native adapter consumption. Existing consumers stay
compatible until they explicitly opt in to the new presentation path. Scope:
shared content/session/worker/C API, their tests, native adapter integration and
associated evidence. Other chats should avoid overlapping shared API edits while
this task is active; platform-local acceptance can proceed independently.

## Previous checked milestones

The macOS chat completed the shared lexical corpus/ownership/layout checkpoints
below on `master`, starting from `74a4190`. No file claim remains after this
handoff; the next chat must fetch and announce its bounded shared-core task before
overlapping work. The owner requested local development until the usage allowance
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

1. Integrate the owned HTML/CSS decision into shared heading/body snapshots,
   preserving the legacy lexical-formatting-before-content-cleanup order. Do not
   simply apply it in Swift/GTK after receiving an already-cleaned snapshot.
   Carry matching copied CSS and layout status with the HTML, covering fallback,
   entry/chapter/paired/heading paths, per-module options and saved-state behavior.
2. Add compatible C/Swift/Linux consumption and native visual/interaction evidence
   together; no UI should receive positioned annotation HTML without matching CSS.
   Keep broader SDK/RTL/font/corpus and unsupported-grammar gates explicit; see
   [word block workflows](word-block-workflows.md). The old parser is not retired.
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
