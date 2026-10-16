# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Checked shared-core handoff

Branch `master`, fork `orbeavers14/xiphos`. Work is handed off near the usage
ceiling with no active shared edit claim. Owner authorization to continue/push
remains; fetch before resuming. Preserve Windows inventory through `cf3b24c`,
unrelated `.DS_Store`, GTK3, tests and licenses. Keep builds local.

Latest implementation `1a19c1d` imports raw saved book module/key/offset values
through the existing bounded XML parser. Actual legacy getters, duplicates/nulls,
Unicode, source ownership/bytes and parser guards are tested. It does not apply
startup defaults, resolve numeric/path ambiguity or write profiles. Legacy numeric
reading clamps to 4; path reads may leave an older saved offset. Those startup
semantics need a separate typed resolution decision before native integration.

Book sources/render/worker-C/paging/capability/notes/URI contracts are in
[book workflows](book-tree-workflows.md), with implementations `6f61997`, `6b2399c`,
`b9c0a34`, `f35b6dd`, `b49c823`, `9888bf0`, `045f27e`. One engine/FIFO, owned
results and existing module/tab option precedence remain intact. Native book
controls, URI presentation, images, saved selection application and editors are
pending. Existing verse URI validation remains strict. No full F10 acceptance.

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

At `1a19c1d`: **89/89 Mac/core**, **73/73 portable Linux/core on this Mac**,
profile **ASan/UBSan 3/3** (book/profile-reader/preview-profile). Prior URI
`045f27e` passed books/common-parser ASan/UBSan 7/7 and C11. Note `9888bf0`
passed recovery/book ASan/UBSan 8/8, C++11 and native-disabled helpers. SDKs are
not fully instrumented; leaks are off. Current Fedora GTK/MSVC remains unvalidated.
All checks stayed local; no hosted allowance or reset credit was used.

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
cmake --build build/linux-models-sanitized --target core_book_profile_test core_profile_reader_test core_preview_profile_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_(book_profile|profile_reader|preview_profile)$' --no-tests=error --output-on-failure
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

1. Next shared task (unclaimed): characterize safe typed saved-selection/startup
   resolution against legacy numeric/path/default behavior. Then native book
   controls can consume the existing queued core APIs. Do not auto-apply raw saved
   values or invent a separate frontend navigation/persistence engine.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.
