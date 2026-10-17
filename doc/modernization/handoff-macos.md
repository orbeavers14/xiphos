# macOS chat handoff

Updated 2026-10-16. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Checked shared-core handoff

Branch `master`, fork `orbeavers14/xiphos`. Active shared claim: atomic book
document publication and rendered-section note routing. No reset credit or hosted
build was used. Preserve Windows lane, `.DS_Store`, GTK3, tests and licenses.

Latest source `219d299` captures configured DisplayLevel documents against the
actual legacy traversal. Worker/C `799ab02` copies root, sections and raw combined
HTML with existing tab/module option precedence. It is read-only and does not
replace committed book selection. Bounds reject the whole result. Native document
publication, descendant note routing and display cleanup remain separate gates.

Native key/tree/navigation controls `d65aabc` and ordinary note clicks `bb6b2a7`
consume shared operations; disposable windows checked their scoped behavior.
Shared navigation `0b81ec5` retains legacy numeric reading minimum four and root
path distinction. Explicit file restore `47fc462`, selection `0af4593`, SDK
resolution `eb33f2a`, decoder `929c8d8` and raw profile import `1a19c1d` preserve
separate initial-window/display contexts and successful-only per-tab publication.
No profile discovery/writes or full saved-tab/visibility workflow is implemented.

Contracts and earlier source/render/paging/note checkpoints are in
[book workflows](book-tree-workflows.md). One engine/FIFO, owned copies and strict
verse URI validation remain intact. Full corpus/visual/platform acceptance,
images/resources, generalized book links and editors remain pending. No full F10.

Prior lexical, spacing, note-numbering, native heading-link and SDK recovery
behavior/evidence remains in the linked contracts and versioned handoffs.

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

At document source `219d299` and worker/C `799ab02`: **98/98 Mac/core**,
**80/80 portable Linux/core on this Mac**, focused document/worker/entry/session
**ASan/UBSan 4/4**, C11 and strict C++17 decoder checks pass. Latest full runs are clean. Prior `eb33f2a`
had a Mac paging timeout and sanitizer fixture timeout despite success output;
isolated reruns passed without source or timeout changes. Preserve that evidence
in status/history. SDK dependencies are not fully instrumented; leaks are off.
All checks are local; current Fedora GTK/MSVC and native book UI are unvalidated.

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
cmake --build build/linux-models-sanitized --target core_book_selection_test core_book_session_test core_book_entry_test core_book_profile_restore_test --parallel 1
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_book_(selection|session|profile_restore)$' --no-tests=error --output-on-failure
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

1. Active shared milestone: atomic document selection and rendered-section note
   routing. Keep raw single-entry and read-only document APIs stable. Follow with
   matching native document consumption, display cleanup and resources. Full saved-tab
   XML, visibility/fallback and round-trip writes remain separate gates.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.
