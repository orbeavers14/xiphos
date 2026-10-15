# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Handed-off native controls checkpoint

Branch `master`, fork `orbeavers14/xiphos`. Latest implementation `1d3612f`
(native adapters/controls), following `aff5f59` (queued worker/C API) and `1bd924c`
(shared session numbering). The owner resumed after the prior stop and explicitly
requested native controls. That bounded task is complete; shared ownership is
released. Preserve unrelated `.DS_Store`, fetch before future shared edits, and
never force-push.

## Latest checked behavior

Mac Reading options and Commentary options now expose separate “Footnote and
cross-reference numbers” toggles. The Mac and Linux models opt into the checked
core policy, copy availability/value and refresh committed entry/chapter context
without adding history or losing input drafts. Profiles save/restore per-module
settings. The GTK options panel has the matching two controls; its portable model
passes on this Mac, but the current GTK executable still needs native Fedora
validation. No Windows UI exists yet. One shared core owns formatting/counters;
no platform-specific numbering implementation was introduced.

Worker/C requests preserve FIFO ordering, strict booleans, typed ready/unavailable
state, closed/unknown-tab errors and copied result lifetime. Tests cover adapter
copies, settings/reads interleaved in submission order, paired panes, chapter
counters/anchors, profiles, failure retention, busy/closed models and shutdown.

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

At `1d3612f`: **75/75 Mac/core CTests**, **59/59 portable Linux/core CTests on
this Mac**, focused **ASan/UBSan 5/5** (worker/C and Linux note models plus three
fixture prerequisites), and strict C11 header syntax pass. External SDKs are not
fully instrumented; sanitizer leak detection is off. No full GTK3 runtime, current
Fedora GTK or MSVC/WinUI acceptance is claimed. No hosted build credits were used.

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
cmake --build build/linux-models-sanitized --target core_note_controls_test core_note_snapshots_test core_heading_note_metadata_test core_heading_collection_test linux_note_model_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^(core_note_controls|linux_note_model)$' --no-tests=error --output-on-failure
clang -std=c11 -fsyntax-only -Isrc/core/include src/core/tests/c_header_test.c
```

The ignored SDK seed is copied from this Mac's cache, not a portable file. The
sanitizer cache uses Debug, C/C++ `-fsanitize=address,undefined -fno-omit-frame-pointer`
and executable link `-fsanitize=address,undefined`. Reconfigure before adding new
targets. OFF builds no GTK executable. Fedora uses its normal default-ON build
and local dependencies. Do not repeat full checks without a changed source,
failure or unresolved concern. Prior detailed checkpoints remain in the
[versioned handoff](https://github.com/orbeavers14/xiphos/blob/b7f1bbb/doc/modernization/handoff-macos.md).

## GUI evidence and limits

At `1d3612f`, a disposable generated-fixture Mac window checked entry numbering
On/Off/On, chapter continuation/selection, opening a Study note pane from its
numbered marker, and independently disabling commentary labels while the Bible
remained numbered. The temporary app was quit and removed. The new `notes` fixture
kind in `launch-fixture.sh` reproduces this outside Documents; do not weaken macOS
privacy. Prior lexical/spacing GUI evidence remains in the linked workflow docs.

Unsupported WORD lexical footnote markup still falls back. The SDK's generated
ThML heading links have empty note IDs; heading-click/corpus acceptance remains
open alongside the recorded direct-filter TEI issue. No full note parity is claimed.

## Next bounded work

1. Fedora: rebuild current default-ON GTK and verify numbering controls, lexical/
   spacing behavior, profiles and note clicks with physical keyboard/pointer.
2. Shared core: characterize unsupported WORD lexical note markup and heading-note
   identifiers against retained behavior before expanding supported grammar.
3. Windows: validate dependencies/core with native MSVC before WinUI bring-up.
   Broader RTL/font/corpus/accessibility and retirement gates remain open.

The requested controls and checks are complete and pushed; no follow-on task or
hosted build is scheduled. 0/38 parity groups and 0/3 releases are fully accepted.
