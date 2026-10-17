# macOS chat handoff

Updated 2026-10-16. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Checked shared-core handoff

Branch `master`, fork `orbeavers14/xiphos`. Native separate image preview is checked at `1af7207`, using core-owned
click dispatch `57cf632` and bounded image bytes. No active edit claim is held.
No reset credit or hosted build was used. Preserve Windows lane, `.DS_Store`, GTK3,
tests and licenses.

Source `219d299`, worker/C `799ab02`, atomic publication `6cb35cf` and queued
publication `99056d7` retain selected/descendant/alias sources, bounded configured
DisplayLevel, successful-only state and exact rendered-section note routing.
Native `e52a497` consumes these values without duplicating core policy. View
extraction `49e4561` separates `BookPane.swift` from the main window source.
Raw single-entry and read-only document APIs remain stable. Shared display
presentation `86e4ad0` and native controls `614a81d` preserve raw sources, matching
visible HTML/CSS and root/draft refresh. Resources, book note numbering, general
links and full saved tabs/writes remain pending.

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

At image context `b3f1198` and local bytes `bd487b4`: **107/107 Mac/core**,
**85/85 portable Linux/core on this Mac**, focused **ASan/UBSan 9/9** and C11 pass.
The native image viewer/inline resource loading are not implemented yet. Latest full runs are clean. Prior `eb33f2a`
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
cmake --build build/linux-models-sanitized --target core_book_document_test core_book_document_session_test core_book_document_publication_test core_book_document_presentation_test core_book_navigation_test core_book_session_test core_book_entry_test --parallel 1
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_book_(document|document_session|document_publication|document_presentation|navigation|session)$' --no-tests=error --output-on-failure
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

1. Next shared/native milestone: inline book resource routing using the checked
   shared resource policy. Typed click dispatch, bounded local bytes, background
   decode and separate native PNG preview are checked. Inline resources, CSS, remaining image
   formats/corpus, note numbering and saved-tab/write acceptance remain pending.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.

## Native book image checkpoint — 2026-10-16

Core dispatch `57cf632` returns an owned note or image result through the same
queued worker/C boundary. Native preview `1af7207` copies bytes before releasing
C ownership, decodes an ImageIO thumbnail off the UI thread (2048-pixel limit),
and presents a native sheet with Close. Failed loads and late completions after
window closure retain committed reading state. A generated original-prose PNG
opened in the Mac window; closing it returned to the same book section. The
subsequent accessibility label compiled and passed focused model checks; full
VoiceOver acceptance remains open.

Local validation: **109/109 Mac/core**, **85/85 portable Linux/core on Mac**,
**9/9 ASan/UBSan**, and strict C11 header compilation. After the image label
adjustment, app/model builds and **8/8 focused checks** passed. No hosted builds
or reset credits were used. Portable checks on Mac do not validate current
Fedora GTK or native Windows/MSVC/WinUI.

Next bounded task: characterize and implement inline book resource routing using
the same shared resource policy. Inline images, sizing/CSS, other formats and
animation, full-size viewing, corpus/visual workflows, note numbering and saved
tabs/writes remain pending. Preserve GTK3 and all retirement gates. Overall
planning estimate remains about **8% (6–10%)**, **0/38** whole feature groups
accepted, **0/3** release-ready native apps. This checkpoint is handed off near
the usage ceiling; no active edit ownership is held.
