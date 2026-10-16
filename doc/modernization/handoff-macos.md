# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Active shared heading-note investigation

Branch `master`, fork `orbeavers14/xiphos`. The owner requested continued shared
core work until the available usage limit. The Mac chat finished lexical block
segmentation (`1f899ca`) and isolated heading-note capture (`91f83c9`). It now
claims opt-in session snapshot publication after checked scoped identity/lookup
(`7c98ebf`). Keep raw metadata and existing appearance unchanged. No hosted builds.
Windows inventory changes through `cf3b24c` were fetched and retained; do not
modify that chat's platform lane or unrelated `.DS_Store`.

## Latest checked behavior

`7c98ebf` adds canonical heading-scoped IDs and configured Reader capture/lookup.
Existing session/worker/C note dispatch resolves these IDs without body collisions,
including after display footnotes are hidden or another passage is rendered.
Canonical/overflow/control checks, metadata-only behavior and ownership pass.
Existing native snapshots do not publish them yet; an explicit migration
capability and validated appearance-preserving replacement are next.

`91f83c9` captures owned heading notes without overwriting source/body attributes.
It handles both Headings/Footnotes filter orders, preserves already-filtered
bodies/IDs and the frozen cleanup label, skips repeated heading extraction, and
restores SDK attribute processing even on filter exceptions. Existing Reader/
session/C/native paths are unchanged. Fragment-local IDs must not be published
as ordinary body-note IDs; scoped identity and lookup are the active next step.

`1f899ca` retains top-level p/h1–h6 wrappers outside inline word spans. Multiple
headings in one SDK fragment and adjacent paragraphs now keep lexical HTML/CSS
without forcing pane fallback. Each inline segment agrees with the frozen legacy
renderer; inter-block whitespace, raw note metadata, labels and URLs are retained.
Malformed/nested supported blocks or crossed inline spans still fall back with
spacing-only CSS. The low-level legacy-compatible renderer remains unchanged.
This is not a general HTML validator or proof of an arbitrary module corpus.

Earlier `aff5f59` / `1d3612f` queued settings and native Bible/commentary controls
remain checked. Core formatting/counters stay shared. Current GTK runtime and
native MSVC/WinUI validation remain pending.

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

At `7c98ebf`: **78/78 Mac/core**, **62/62 portable Linux/core on this Mac**,
focused **ASan/UBSan 5/5** (ID/lookup and capture prerequisites), strict C++11
capture/codec syntax and native-disabled helper build pass. Existing native
controls and legacy-compatible Reader HTML remain unchanged. SDKs are not fully
instrumented; leak checks are off. No new GUI/current Fedora GTK/MSVC acceptance
is claimed. All checks stayed local.

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
cmake --build build/linux-models-sanitized --target core_heading_note_id_test core_heading_note_lookup_test core_heading_notes_test core_heading_note_metadata_test core_heading_collection_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^(core_heading_note_id|core_heading_note_lookup)$' --no-tests=error --output-on-failure
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

At `1f899ca`, a disposable original-prose window imported a generated profile
with lexical layout enabled and showed both OSIS headings, supplied note labels
and body in commentary entry view. The temporary copy was quit and removed.
Prior controls On/Off/On, chapter continuation, note-pane open and independent
commentary settings were checked at `1d3612f`; see the workflow contracts.

Ordinary SDK note markers are supported by the checked complete layout path.
Arbitrary WORD/RTL/font/corpus acceptance remains open. SDK heading-note links
still have empty IDs; formatting does not fix routing or accept heading clicks.
The TEI direct-filter limitation remains separately recorded.

## Next bounded work

1. Current Mac shared task: add explicit session capability and reading-snapshot
   publication, preserving raw source metadata, legacy counters/labels and HTML
   outside identifiers. Unsupported replacements must retain existing rendering.
2. Fedora: rebuild current default-ON GTK and verify numbering, paragraph/heading
   layout, lexical/spacing refresh, profiles and note clicks with real input.
3. Windows: continue native compiler/dependency/core setup in its own handoff lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier requested stop was superseded.
