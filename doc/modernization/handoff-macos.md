# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Active shared SDK render recovery task

Branch `master`, fork `orbeavers14/xiphos`. The owner requested continued shared
core work until the available usage limit. Heading-link snapshot integration
`3411f45` and native adoption `2390568` are checked and pushed. The Mac chat now
claims bounded SDK attribute-processing recovery in core render/heading/note/
attribute helpers and related tests. Keep output and normal dataflow unchanged;
all checks stay local. Windows inventory through `cf3b24c` and unrelated
`.DS_Store` remain untouched. Fetch before overlapping shared edits.

## Latest checked behavior

Native Mac and Linux reading models opt into shared scoped heading IDs, following
owned capture `91f83c9` and lookup `7c98ebf`. Candidate replacement must match the
legacy HTML outside IDs and retain known local IDs, target metadata and the
cleanup-point label. Raw Reader sources, visible labels/counters, body rendering,
CSS, anchors and history stay unchanged. Unknown/mixed/changed shapes keep their
original rendering. Existing other consumers remain default-off.

`2390568` adds only capability selection and model checks to native adapters.
Heading/body note separation, hover, retained links, linked selection, display
toggles and shutdown are tested. Full F14/corpus/visual acceptance remains open.

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

At `2390568`: **81/81 Mac/core**, **65/65 portable Linux/core on this Mac**,
focused **ASan/UBSan 5/5** (Linux model/snapshot and capture prerequisites).
Core `3411f45` separately passed 80/80, 64/64 and ASan/UBSan 5/5; extra linked-source
coverage passed normal/sanitized 4/4. Strict C11 header syntax passes. Capture/codec
C++11 and native-disabled helper builds passed at `7c98ebf`. SDKs are not fully
instrumented; leak checks are off. No current Fedora GTK/MSVC acceptance is claimed.
All checks stayed local; no hosted jobs were requested.

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
cmake --build build/linux-models-sanitized --target linux_heading_note_model_test core_heading_note_snapshots_test core_heading_notes_test core_heading_note_metadata_test core_heading_collection_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^(linux_heading_note_model|core_heading_note_snapshots)$' --no-tests=error --output-on-failure
clang++ -std=c++11 -fsyntax-only -Isrc/core/include -I/usr/local/include/sword src/core/heading_notes.cpp src/core/heading_note_id.cpp
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

At `2390568`, an original-prose disposable window verified OSIS heading marker →
heading body, ordinary marker → body note, and second ThML fragment → distinct
unlabeled heading note. The temporary copy was quit and removed. Earlier layout,
numbering and spacing checks remain in their linked contracts.

Only candidates identical outside their note IDs are repaired. Unsupported mixed
source retains its original rendering; arbitrary corpus, RTL/font and TEI gates
remain open. GTK was model-checked here, not compiled/run on current Fedora.
No complete functional or visual parity is claimed.

## Next bounded work

1. Current Mac shared task: protect SDK attribute-processing state on exceptions
   in ordinary render/heading/note/attribute helpers, using the proven recovery
   issue without altering healthy output or caller-owned settings.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.
