# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Active task and ownership

Branch: `master` on `orbeavers14/xiphos`. The Mac chat is continuing shared
presentation/note-numbering work. Scope: core Reader/session/render/presentation,
worker/C ABI, compatible adapters, tests and evidence. Other chats should avoid
overlapping shared API edits while this continues; platform-only checks can
proceed independently. Fetch before work or integration; never force-push or
overwrite unrelated changes. Sequential checked master commits/pushes are authorized.

## Latest checked implementation

`c499324`: owned per-filter note-label restoration and opt-in C++ Reader sources.
The private Reader establishes an explicit Off baseline, matching the checked SDK
default, then scopes configured entry/dense/sparse commentary reads. Real SDK
renderer aliases, nested/exceptional scopes, mixed baselines and partial-owner
failure are tested. Raw `Entry.note_source` distinguishes unavailable metadata
from captured empty labels and owns body/heading cleanup-point values after SDK
teardown. Existing Reader overloads and native consumers retain their current
presentation behavior. Automatic native numbering/settings/controls are not enabled.

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

At `c499324`: **72/72 Mac/core CTests**, **56/56 portable Linux/core CTests on
this Mac**, focused **ASan/UBSan 3/3** (SDK scope test, heading fixture and Reader
metadata test), strict C++11 note-rendering syntax and native-disabled helper
build pass. Prior counter extraction also passed focused TSan 1/1. External
SWORD/GLib/libxml2 libraries are not fully instrumented; leak detection is off.
No complete legacy GTK runtime, current Fedora GTK or native MSVC test is claimed.

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
cmake --build build/linux-models-sanitized --target core_heading_note_metadata_test core_heading_collection_test core_sdk_note_numbers_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^(core_heading_note_metadata|core_sdk_note_numbers)$' --no-tests=error --output-on-failure
clang++ -std=c++11 -fsyntax-only -Isrc/core/include -I/usr/local/include/sword src/core/note_rendering.cpp
cmake --build build/legacy-seam-build --parallel 2
```

The ignored SDK seed is copied from this Mac's configured cache, not a portable
file. Sanitizer build uses Debug and address/undefined flags on C/C++ and linking;
configure/reconfigure it before a new target. OFF builds no GTK executable. Fedora
uses its normal default-ON build with local SDK discovery. Do not repeat full
checks without a change/failure or an unresolved concern that warrants them.

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

1. Characterize `XrefNoteNumbers` saved defaults/import/cache behavior against the
   full legacy initializer. Preserve unknown/unhandled values until a setting is applied.
2. Integrate owned numbering into opted-in session snapshots using Reader metadata.
   Keep per-SDK-fragment heading cleanup before body cleanup, document counter
   reset/continuation, copied diagnostics, whole-pane lexical fallback and independent
   paired panes. The current C++ Reader option controls SDK supplied labels only.
3. Add queued worker/C settings and compatible native controls after the shared
   presentation contract is checked. No formatting policy belongs in Swift/GTK.
4. Coordinate Fedora rebuild/desktop checks and native Windows bring-up through
   their handoffs. Broader RTL/font/corpus/accessibility and full legacy retirement
   remain gates. 0/38 complete parity groups and 0/3 native releases are accepted.

Keep all routine checks local. No hosted build or reset credit was requested.
Update this rolling handoff with the next implementation commit/results/limits;
link historical evidence instead of accumulating conflicting current checkpoints.
