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
Active next slice: characterize legacy search requests and extract a bounded
read-only shared search service.
Do not create separate frontend sizing algorithms. No hosted build or reset
credit was used. Preserve Windows lane, unrelated `.DS_Store`, GTK3 and licenses.

## Actual local checks

At `a5d9488`: 116/116 Mac/core, 90/90 portable Linux/core on this Mac; three focused
book-numbering ASan/UBSan checks and twelve native/core focused checks pass. Earlier six focused
ASan/UBSan including fixtures, strict C11 header compilation. Nine focused native/
core checks passed before the full run. Three final sanitizer checks passed after
style-escaping refinement. Strict C++11 geometry and native-disabled render
library checks passed; actual GTK geometry routine compiled with test stubs. The initial sanitizer run lacked two
fixture executables; after building them all six passed. Dependencies are not
fully instrumented and leak detection is off. Current Fedora GTK/MSVC/WinUI is
unvalidated here; these Mac-hosted portable checks are not Linux desktop proof.

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
cmake --build build/sanitized --target core_book_document_test core_book_image_test core_book_inline_images_test core_book_image_resource_test core_book_document_presentation_test core_book_document_publication_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized -R '^core_book_(inline_images|image|image_resource|document_presentation|document_publication)$' --no-tests=error --output-on-failure
```

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
