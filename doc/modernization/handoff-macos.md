# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [status](status.md) and relevant parity rows first.

## Active shared general-book source task

Branch `master`, fork `orbeavers14/xiphos`. The owner requested continued shared
core work until the available usage limit. Owned read-only general-book source
`6f61997`, configured rendering `6b2399c` and queued/C transport `b9c0a34` and paging
`f35b6dd`, plus capability `b49c823`, are checked. The Mac chat now claims typed
read-only saved book-selection characterization/import with the existing XML
profile parser. Keep GTK3 editors/writes intact, all
checks local, Windows inventory through `cf3b24c` and unrelated `.DS_Store`
untouched. Fetch before overlapping shared edits.

## Latest checked behavior

`045f27e` binds real SDK book-note URIs to the captured typed source, using shared
command decoding. It rejects wrong module/path/type/ID, external/unknown/malformed
commands and avoids verse short-reference fallback. Session/worker/C use existing
note ownership and engine policy, including metadata-only/alias lookup. Existing
verse parser regressions pass. No native book controls are introduced.


`9888bf0` pairs ordinary book-note metadata/body with exact typed tree identity,
including linked aliases, through existing engine policy and queued C ownership.
The common C++11 note helper accepts an already positioned key and temporarily
enables source-attribute capture, restoring the initial flag after success/failure.
It prevents stale notes even for deliberately disabled callers. Existing enabled
passage output remains exact. Metadata-only skips body rendering. Native book URI
routing/hover/click and heading-note repair remain pending.


`b49c823` publishes indexed-tree capability in Reader/worker/C catalogs, sharing
the query gate. SDK Generic Books with verse keys and dictionaries are excluded;
eligible, empty and deep indexed books are included. Copied metadata, getters and
unsupported-operation rejection are tested. It is no claim of full book support.


`f35b6dd` pages direct children through the same Reader/worker/C owner. Limits
1..256, validated last-child cursors and one lookahead preserve SDK order without
whole-tree/native enumeration loops. A 600-child fixture validates four page sizes,
root/empty/final pages, cursor guards, key/attribute/byte identity and handle lifetime.
It does not freeze module revisions or validate arbitrary corrupt modules.


`b9c0a34` adds queued owned book sources/entries and separate take-once C handles.
It reuses session module/tab/default/unconfigured option precedence, leaves Bible
and dictionary history unchanged, and preserves root/absence distinctions in C
node views. Copied arguments, errors/continued work, borrow lifetimes, close/drain
and module-byte identity are checked. No native tree UI is introduced.


`6b2399c` pairs configured raw SDK book HTML/entry presence with exact tree
identity, including linked aliases and structural-only empty branches. It selects
without replacing the live indexed key, validates source before selection,
rejects unexpected source changes, and reuses filter/processing recovery scopes.
OSIS/ThML options, real toggles, exceptions and module-byte identity are tested.
No native book-note/image dispatch or history/UI is introduced.


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

At `045f27e`: **88/88 Mac/core**, **72/72 portable Linux/core on this Mac**,
focused books/common parser **ASan/UBSan 7/7**, strict C11 header syntax. At
`9888bf0`, note/recovery ASan/UBSan passed 8/8, C++11 note-helper syntax and
native-disabled helper build passed. SDKs are not fully instrumented; leaks are
off. Current Fedora GTK/MSVC remains unvalidated. All checks stayed local.

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
cmake --build build/linux-models-sanitized --target core_book_tree_test core_book_entry_test core_book_worker_test core_book_children_test core_book_note_test core_book_note_link_test core_content_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-models-sanitized -R '^core_book_(tree|entry|worker|children|note|note_link)$|^core_content$' --no-tests=error --output-on-failure
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

At `2390568`, an original-prose disposable window verified OSIS heading marker →
heading body, ordinary marker → body note, and second ThML fragment → distinct
unlabeled heading note. The temporary copy was quit and removed. Earlier layout,
numbering and spacing checks remain in their linked contracts.

Only candidates identical outside their note IDs are repaired. Unsupported mixed
source retains its original rendering; arbitrary corpus, RTL/font and TEI gates
remain open. GTK was model-checked here, not compiled/run on current Fedora.
No complete functional or visual parity is claimed.

## Next bounded work

1. Current Mac shared task: read-only saved book-selection characterization/
   import through the existing XML parser. Do not auto-apply, resolve or rewrite
   saved selections before numeric/path/startup compatibility is characterized.
   Native tree/navigation controls, URI/image dispatch, book history and saved
   selection remain later bounded steps.
2. Fedora: rebuild default-ON GTK; verify heading click/hover and source/body
   separation, linked chapters, numbering/spacing, profiles and real input.
3. Windows: continue native compiler/dependency/core setup in its own lane.

0/38 whole parity groups and 0/3 releases remain accepted. Continue within the
owner's latest instruction; the earlier stop was superseded.
