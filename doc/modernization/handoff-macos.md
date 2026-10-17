# macOS chat handoff

Updated 2026-10-17. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [core scope](shared-core-roadmap.md),
[status](status.md) and relevant parity rows first.

## Current checked source and active work

Fork `orbeavers14/xiphos`, master. Inline URL resolver `56925fb`, core-owned
resource-prepared HTML `efcc544`, Mac consumption `4137374`. Raw SDK and ordinary
presentation HTML remain distinct. Image-off cleanup precedes resource loading;
missing images retain text and expose issues. Bounds, parser normalization and
format limitations are documented in [book workflows](book-tree-workflows.md).

Shared geometry `48d5425` serves the retained GTK adapter (native sizing pending).
Module CSS `4611a9d` / native `65b8ab6` is checked: bounded preferred/default
styles, empty/failed fallback and safe style text. Combined note source `652d8e1`, shared numbering `43ce5ee` and native controls
`a5d9488` now preserve exact legacy terminal-label/whole-document counter scope.
Search checkpoints: `2682d18` pure query policy, `06286b7` Reader, `47401f0`
worker/C, `a413a21` Swift, `bb1d8bb` cooperative owner-safe controls, `c75b187`
Swift cancellation draining, `1dae0e9` explicit intervals/legacy memory fix,
`d77d61c` Swift ranges. Shared search owns queries/results and SDK scopes; no native
search UI exists. See [search contract](search-workflows.md).
Next bounded slice: owned per-tab last-search keys and atomic publication semantics.
Do not create separate frontend sizing algorithms. No hosted build or reset
credit was used. Preserve Windows lane, unrelated `.DS_Store`, GTK3 and licenses.

## Actual local checks

At `d77d61c`, full suites passed 122/122 Mac/core and 95/95 portable Linux/core
on this Mac before the final legacy range buffer fix. After the fix: three focused
Mac/core/Swift, two portable, five ASan/UBSan and three TSan checks pass. Strict C11
header compilation passed. The initial range ASan failure exposed a real legacy
self-buffer use-after-free; copied context fixes it, and the extracted actual
retained routine now passes. An initially mistyped Swift build target was corrected
before the passing focused run. No full GTK3 build/runtime claim is made.
Earlier book geometry C++11/native-disabled helper, module CSS and combined-note
sanitizer/GUI evidence remains in the workflow documents/version history.
Dependencies are not fully instrumented and leak detection is off. Current Fedora
GTK/MSVC/WinUI is unvalidated here; Mac portable checks are not Linux desktop proof.

Disposable original-prose window verified PNG inline, click to native preview,
Close to same book section, image-off text retention, and image accessibility
label. CSS window checked preferred color/pseudo-element and unusable-style
text retention. Numbering window verified n1/x1/n2/x2 across sections, exact
descendant note body, and Off/On with the same selected root. Temporary app/fixture was quit and removed. Full VoiceOver/keyboard,
corpus, native sizing, global/recursive CSS assets, image formats/animation and whole workflow/visual
acceptance remain pending. 0/38 whole groups and 0/3 native releases accepted.

Environment: macOS 27.0.1 (26A434), ARM64; AppleClang 21.0.0; Swift 6.4 toolchain
in Swift 6 language mode; SWORD 1.9.0.3916; GLib 2.86.5; configured libxml2 SDK.
Local dependency/deployment minimum is 26.0, not validation of requested macOS 13.
SDK discovery follows [the Mac build guide](../../platforms/macos/README.md).

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel 2
ctest --test-dir build/macos --no-tests=error --output-on-failure --parallel 2 --timeout 30
cmake -S platforms/linux -B build/linux-models -C build/linux-model-sdk.cmake -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --no-tests=error --output-on-failure --parallel 2 --timeout 30
cmake -S src/core -B build/sanitized
cmake --build build/sanitized --target core_search_test core_search_range_test core_search_control_test core_search_worker_test core_sidebar_search_text_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized -R '^core_(search|search_range|search_control|search_worker|sidebar_search_text)$' --no-tests=error --output-on-failure
```

Focused post-fix commands use targets `core_search_range_test` and
`macos_search_adapter_test`, regex `^(core_search|core_search_range|swift_search_adapter)$`
in build/macos; portable target `core_search_range_test`, regex `^core_search(_range)?$`.
Thread checks use build/thread-sanitized, target `core_search_range_test`, regex
`^core_search(_range|_control)?$`. Its existing Debug cache uses
`-fsanitize=thread -fno-omit-frame-pointer` for C/C++ and thread executable linking.
Do not mix address/thread sanitizers in one directory. The tested SDK's
`XIPHOS_SWORD_SEARCH_ICASE_FLAG=2` must be validated against each target SDK.

Ignored local SDK seeds/caches are machine-specific. Sanitizer cache uses Debug,
C/C++ `-fsanitize=address,undefined -fno-omit-frame-pointer`, and executable link
`-fsanitize=address,undefined`; configure before adding targets. OFF builds no
GTK executable. Fedora uses default-ON with its own dependencies. Repeat broad
checks only for changed source, failures or unresolved concerns.

Earlier restore/navigation/DisplayLevel/note/presentation contracts and exact
checkpoints remain in [book workflows](book-tree-workflows.md). Prior lexical,
spacing, heading and SDK recovery evidence remains in the linked workflow
documents and [versioned prior handoff](https://github.com/orbeavers14/xiphos/blob/e193cf6/doc/modernization/handoff-macos.md).
Older next-task statements are historical; prior intermittent paging/fixture
timeouts and passing isolated reruns remain in history. No legacy source retires.
