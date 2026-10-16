# macOS chat handoff

Updated 2026-10-16. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Checked shared-core handoff

Branch `master`, fork `orbeavers14/xiphos`. Active shared claim: resolve typed
book restore candidates through the SDK and queued owned results. Initial-window
and tab/display restore remain distinct. Preserve Windows lane, `.DS_Store`, GTK3,
tests and licenses. Keep all checks local; fetch before overlapping edits.

Read-only profile `1a19c1d` imports raw saved book module/key/offset values
through the existing bounded XML parser. Actual legacy getters, duplicates/nulls,
Unicode, source ownership/bytes and parser guards are tested. It does not apply
startup defaults, resolve numeric/path ambiguity or write profiles. Legacy numeric
reading clamps to 4; path reads may leave an older saved offset. Those startup
semantics need a separate typed resolution decision before native integration.

Decoder `929c8d8` now owns candidate/default/prefix rules for the two explicit
contexts, with bounded, overflow-safe numeric handling. The extracted actual
legacy display function is compared headlessly. No SDK selection or UI publication
occurs yet.

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

At `929c8d8`: **90/90 Mac/core**, **74/74 portable Linux/core on this Mac**,
new decoder **ASan/UBSan 1/1**. Prior profile `1a19c1d` passed 89/89, 73/73 and
ASan/UBSan 3/3. SDKs are not fully instrumented; leaks are off. Current Fedora
GTK/MSVC remains unvalidated. All checks stayed local.

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
cmake --build build/linux-models-sanitized --target core_book_selection_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_book_selection$' --no-tests=error --output-on-failure
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

1. Active shared task: SDK resolution/queued ownership of the typed restore
   candidate. Preserve relative-path SDK normalization while rejecting missing
   snaps before copying stale nodes. Do not add profile writes or implicit UI
   publication. Saved-tab XML/visibility/fallback policy remain separate gates.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.
