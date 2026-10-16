# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Active shared general-book source task

Branch `master`, fork `orbeavers14/xiphos`. The owner requested continued shared
core work until the available usage limit. Owned read-only general-book source
`6f61997` is checked. The Mac chat now claims configured book-entry rendering and
source identity as a bounded next slice. Keep GTK3 editors/writes intact, all
checks local, Windows inventory through `cf3b24c` and unrelated `.DS_Store`
untouched. Fetch before overlapping shared edits.

## Latest checked behavior

`6f61997` adds C++17 Reader tree snapshots by typed offset and canonical path.
Private SDK keys preserve the live key, attributes and module data files. Root,
Unicode/numeric titles, structural branches, sibling order, empty/deep trees and
stale SDK errors are tested. No book rendering, queued/C transport or native UI
is introduced yet. See [the book source contract](book-tree-workflows.md).


`2b756bf` puts every shared SDK text render behind one internal C++11 boundary.
It restores the caller's attribute-processing flag on stored/buffer render errors,
including deliberately disabled callers. It preserves healthy output/key behavior
and does not roll back source attributes that normal renders must populate.
Entry, attribute refresh, heading and note-body helpers now recover for later
reads instead of inheriting a disabled SDK state. Heading capture retains its
stronger full-attribute scope. Frozen output and native behavior remain unchanged.

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

At `6f61997`: **83/83 Mac/core**, **67/67 portable Linux/core on this Mac**,
focused book **ASan/UBSan 1/1**. The depth-limit test added after full-suite runs
also passes individually in both caches and sanitized. Previous render recovery
`2b756bf` passed 82/82, 66/66 and ASan/UBSan 5/5 plus legacy/C++11 checks. SDKs
are not fully instrumented; leak checks are off. No current Fedora GTK/MSVC
acceptance is claimed. All checks stayed local.

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
cmake --build build/linux-models-sanitized --target core_book_tree_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_book_tree$' --no-tests=error --output-on-failure
cmake --build build/macos --target core_book_tree_test --parallel 2
ctest --test-dir build/macos -R '^core_book_tree$' --no-tests=error --output-on-failure
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

1. Current Mac shared task: configured general-book entry rendering/identity on
   the validated source tree, with original-prose output and state tests. Queued
   worker/C transport and native UI remain later bounded steps.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.
