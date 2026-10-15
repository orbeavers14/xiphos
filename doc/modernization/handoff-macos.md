# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Handed-off checkpoint and ownership

Branch: `master` on `orbeavers14/xiphos`. Implementation `1bd924c`
finishes the bounded C++ session-numbering slice. The owner requested finishing,
pushing and stopping here; **no shared-edit claim or follow-on task is active**.
Worker/C commands and native controls were deliberately left for the next task.
Fetch before claiming overlapping shared work. Preserve unrelated changes and
never force-push. The unrelated untracked `.DS_Store` was left untouched.

## Latest checked implementation

`1bd924c` adds default-off consumer capability, configured per-module
note settings, selective persistence and owned document counters in `StudySession`.
The existing Reader, worker/C and native paths remain compatible. Copied SDK
heading fragments retain legacy cleanup/label order before each body; chapter
counters continue in source order, paired panes reset independently, and raw
metadata remains owned and unchanged. Whole-document lexical fallback and CSS,
cleanup diagnostics, tab isolation and navigation-only history are preserved.

The new private original-prose test compares complete frozen legacy cleanup with
entry/dense/sparse session output, labels and linked sources, heading fragments,
independent cross-reference/footnote counters, resets, panes, SDK restoration,
cleanup order, persistence/failure isolation, fallback and lifetime. Literal
markers in surviving SDK image-link URLs retain the legacy counting quirk.
Unsupported WORD lexical note markup still falls back; this does not expand the
lexical grammar or claim a full note workflow.

Earlier current-run milestones:

| Area | Checked implementation | Evidence |
|---|---|---|
| Lexical snapshots/C/Swift copies | `7fd1bce`, `8c0f4f2`, `0af3689` | [Word/layout contract](word-block-workflows.md) |
| Safe nesting and native HTML/CSS consumption | `1393edd`, `d953adc`, `0b40574` | Same contract; new GTK source awaits Fedora |
| Per-module double spacing | `25cf291`, `68562af`, `420ae13` | [Spacing contract](double-space-workflows.md) |
| Owned counters, SDK switch, attributes and heading metadata | `c39b024`, `8572594`, `858ea63`, `e3fcdb2` | [Note-numbering seam](note-numbering-workflows.md) |

Older exact commands/results remain in the
[versioned prior handoff](https://github.com/orbeavers14/xiphos/blob/b47deaa/doc/modernization/handoff-macos.md)
and the linked workflow documents. Their older next-task statements are historical.

## Actual local checks

At `1bd924c`: **73/73 Mac/core CTests**, **57/57 portable Linux/core
CTests on this Mac**, and focused **ASan/UBSan 3/3** (session test and both heading
fixture prerequisites). Strict C++11 presentation syntax and native-disabled
legacy helper build pass. External SWORD/GLib/libxml2 libraries are not fully
instrumented; sanitizer leak detection is off. No full GTK3 runtime, current
Fedora GTK or native MSVC test is claimed. No hosted build credits were used.

Environment: macOS 27.0.1 (26A434), ARM64; AppleClang 21.0.0; Swift 6.4 toolchain
in Swift 6 language mode; SWORD 1.9.0.3916; GLib 2.86.5; configured libxml2 SDK.
Local dependency/deployment minimum is 26.0, not validation of requested macOS 13.
SDK discovery follows [the Mac build guide](../../platforms/macos/README.md).

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --no-tests=error --output-on-failure
cmake -S platforms/linux -B build/linux-models -C build/linux-model-sdk.cmake -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --no-tests=error --output-on-failure --parallel 2 --timeout 30
cmake -S platforms/linux -B build/linux-models-sanitized -C build/linux-model-sdk.cmake -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models-sanitized --target core_note_snapshots_test core_heading_note_metadata_test core_heading_collection_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_note_snapshots$' --no-tests=error --output-on-failure
clang++ -std=c++11 -fsyntax-only -Isrc/core/include -I/usr/local/include/sword src/core/presentation.cpp
cmake --build build/legacy-seam-build --parallel 2
```

The ignored SDK seed is copied from this Mac's cache, not a portable file. The
sanitizer cache uses Debug, C/C++ `-fsanitize=address,undefined -fno-omit-frame-pointer`
and executable link `-fsanitize=address,undefined`. Reconfigure before adding new
targets. OFF builds no GTK executable. Fedora uses its normal default-ON build
and local dependencies. Do not repeat full checks without a changed source,
failure or unresolved concern. Prior detailed checkpoints remain in the
[versioned handoff](https://github.com/orbeavers14/xiphos/blob/b7f1bbb/doc/modernization/handoff-macos.md).

## GUI evidence and limits

At `d953adc`, a disposable fixture window checked Strong's beneath its word,
chapter selection/scroll and readable raw fallback for unsupported font nesting.
At `68562af`, Double space on/off visibly changed chapter spacing while retaining
passage/selection. Paired commentary, drafts/history, profiles and window/closed
isolation were checked by models. No new GUI claim accompanies the note helpers.
The three known temporary GUI copies were quit and removed; installed modules,
user profiles and unrelated repository files were preserved. Use the disposable
launcher outside Documents for further GUI checks; do not weaken macOS privacy.

## Next bounded shared work

1. Add queued worker/C capability and per-module note-setting commands, typed
   copied state, ownership/error tests and compatible Swift/GTK adapters.
2. Enable native controls after those contracts pass. All numbering policy stays
   in the shared core; preserve one serialized SWORD worker and nonblocking UIs.
3. Coordinate Fedora rebuild/desktop checks and native Windows bring-up through
   their handoffs. Broader WORD/TEI/RTL/font/corpus/accessibility and retirement
   gates remain open. 0/38 complete parity groups and 0/3 releases are accepted.

Work is stopped at the owner's requested checked/pushed checkpoint. Resume only
on a new instruction; no follow-on work or hosted builds have been scheduled.
