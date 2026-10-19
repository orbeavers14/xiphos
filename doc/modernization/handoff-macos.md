# macOS chat handoff

Updated 2026-10-18. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [core scope](shared-core-roadmap.md),
[status](status.md) and relevant parity rows first.

Active task (2026-10-18, master): characterize/extract shared attribute-search
rules, then owned execution/transport. Scope/native search `0ca0bab` is checked
and pushed. Local validation only; preserve unrelated files and legacy. This note
records intent, not a concurrent ownership lease.

## Current checked source and active work

Fork `orbeavers14/xiphos`, master. Complete private session scope `c1b0a88`,
worker/C `c38cd8f`, Swift `ade4bdf`, shared search capability `4e3402c` / corrected
SDK classification test `76895f4`, copied progress/interactive Stop `3181502`,
and native search `0ca0bab` are pushed. See [search contracts](search-workflows.md).
Shared snapshots retain complete typed SDK keys independently of public prefix;
C/Swift expose copied metadata only. Basic native sidebar controls use that core.
Next bounded task: shared attribute query/whole-entry policy, then execution.
Book resources, CSS, combined numbering and earlier restore/DisplayLevel contracts
remain in [book workflows](book-tree-workflows.md) and version history.
Do not create separate frontend sizing algorithms. No hosted build or reset
credit was used. Preserve Windows lane, unrelated `.DS_Store`, GTK3 and licenses.

## Actual local checks

At `0ca0bab`: 126/126 Mac/core and 98/98 portable core on this Mac pass; eight
ASan/UBSan search/book checks and four TSan search/scope/control checks pass.
Strict C11 header compilation passes. Focused native model checks cover scope,
preview/navigation, immediate Stop/late completion and close/stale guards.
The initial capability assertion wrongly treated VerseKey RawGenBook as runtime
Generic Books; this SDK reports Biblical Texts. That assertion was prematurely
pushed, then corrected and verified normal/ASan in `76895f4`; execution logic was
already consistent with the SDK. See the workflow for the earlier legacy range
memory fix and corrected fixture/C function-name mistakes.
Dependencies are only partly instrumented; leak checks are off. Portable core
checks on Mac do not establish current Fedora GTK or MSVC/WinUI acceptance.

Search fixture window checked eligibility, automatic/changed-row preview, ordered
results, explicit bounds, last narrowing, clear then empty, case 1 versus 2,
commentary source, Read and physical double-click to a new main reference. A shorter
Clear last label fits; rapid previews keep the list enabled and suppress stale
output. Stop is covered by model/control tests, not a slow-corpus GUI timing claim.
The app/fixture was quit/removed. Sandboxed Launch Services initially rejected the
launch; the same temporary copy launched outside that shell sandbox. No Documents
access prompt was needed. Full keyboard/VoiceOver/corpus/advanced/index/export gates
remain open.

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
cmake --build build/sanitized --target core_search_test core_search_range_test core_search_control_test core_search_worker_test core_search_results_test core_search_scope_test core_search_scope_worker_test core_book_tree_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized -R '^(core_search(_results|_range|_scope|_scope_worker|_control|_worker)?|core_book_tree)$' --no-tests=error --output-on-failure
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

Final whole-suite commands are the full Mac and portable build/ctest commands
above. Current focused native commands build `macos_reader_model_test` /
`macos_preview` and run `^(core_search|reader_model_search)$`. Thread checks build
core_search_test/core_search_scope_test/core_search_scope_worker_test/
core_search_control_test and run `^core_search(_scope|_scope_worker|_control)?$`.
The older focused command examples retain their narrower historical scope.
No hosted job or reset credit was used; current active work is recorded above.
