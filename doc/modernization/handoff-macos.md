# macOS chat handoff

Updated 2026-10-16. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Checked shared-core handoff

Branch `master`, fork `orbeavers14/xiphos`. Active shared/native claim: book navigation controls
consuming the checked shared operations. The usage window refreshed automatically;
no reset credit was consumed. Preserve Windows lane, `.DS_Store`, GTK3, tests and
licenses. Keep all checks local; fetch before overlapping edits.

Latest `106577b` shares the legacy numeric reading minimum of 4 across typed
selection and decoded restores. Explicit root paths and low-level source queries
retain root 0; requested intent remains available in the per-tab snapshot.
Profile-file restore `47fc462` queues bounded XML import and selection on the
single engine worker, copying the explicit path before returning to C callers.
Malformed/missing/unsupported inputs preserve committed state and source files.
No home/profile discovery, writes, locale changes or native controls are included.

Per-tab publication `0af4593` commits owned requested/resolved sources only after
validation, configured rendering and staging. SDK resolution `eb33f2a` checks
errors before copying nodes and preserves successful relative/root normalization.
Decoder `929c8d8` keeps initial-window offsets distinct from tab/display keys;
read-only profile import `1a19c1d` preserves raw values. Optional queued C results
retain explicit presence and take-once ownership. Absent restore is a no-op.

Book sources/render/worker-C/paging/capability/notes/URI contracts are in
[book workflows](book-tree-workflows.md), with implementations `6f61997`, `6b2399c`,
`b9c0a34`, `f35b6dd`, `b49c823`, `9888bf0`, `045f27e`. One engine/FIFO, owned
results and existing module/tab option precedence remain intact. Native book
controls, URI presentation, images, full saved-tab integration and editors are
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

At navigation `0b81ec5` and Swift transport `aa3b307`: **95/95 Mac/core**,
**78/78 portable Linux/core on this Mac**, focused navigation/entry/session
**ASan/UBSan 3/3** and strict C11 pass. Latest full runs are clean. Prior `eb33f2a`
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

1. Active shared/native milestone: book tree/navigation controls consuming
   shared selection, with Swift adapters containing no duplicate core policy.
   Full saved-tab XML, visibility/fallback context and round-trip profile writes
   remain separate acceptance gates; explicit file restore is now implemented.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.
