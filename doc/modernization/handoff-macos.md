# macOS chat handoff

Updated 2026-10-18. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [core scope](shared-core-roadmap.md),
[status](status.md) and relevant parity rows first.

Active task (2026-10-18, master): shared search advances checked/pushed through
`cf80541`; next characterize advanced module/range setup and export. Continue
bounded shared-core slices with local checks. Preserve unrelated files/GTK3.
This note records intent, not a concurrent ownership lease.

## Current checked source and active work

Fork `orbeavers14/xiphos`, master. Shared attribute policy/execution `9a932ab` /
`572f539`, Swift `a83d3c4`, explicit native-OFF helper configuration `0d0f37a`,
annotation-name policy `3645d4a`, private search context `8ebeddc`, scoped index
probe `92bacfc`, canonical C request `b71980b` / Swift `f4934b2`, distinct advanced
word dialects `69dea11`, and bounded ordered batches C++ `f86293b` / C `ad28750` /
Swift `cf80541` are pushed. Existing basic native controls remain `0ca0bab`.
See [current search contracts](search-workflows.md); full chronology is in Git.

One query can search ordered modules through the same worker, with aggregate
prefix limits, per-group errors and complete final scope/context. Last scope
chains rather than unions. Advanced raw indexed syntax and forced unindexed
multiword are distinct from sidebar automatic AND search. Annotation context is
private per tab; phrase retains prior choices; physical SDK state always restores.
No second frontend engine, SDK manager or scope reconstruction exists.

Next bounded task: advanced module/range setup and export characterization before
shared extraction; advanced controls, actual indexed execution, mixed families,
saved settings and full corpus remain separate gates. Book/session/parallel and
other core work remain in the [roadmap](shared-core-roadmap.md). No hosted job or
reset credit used. Keep Windows lane, unrelated `.DS_Store`, GTK3 and licenses.

## Actual local checks

At `cf80541`: **136/136 Mac/core**, **105/105 portable core/model tests on this
Mac** pass. Batch/context search checks at `f86293b` pass **8/8 ASan/UBSan** and
**6/6 TSan** (fixtures included). C batch/canonical transport at `ad28750` passes
**3/3 ASan/UBSan**, **3/3 TSan**. Six focused Swift search checks pass, including
batch groups, copied failures/budgets, scope/context, progress and both cancellation
modes. Strict C11 header/C++11 policy syntax and genuine native-OFF CoreRender
build pass. Default native/actual parent helper configurations were checked.

Commands for focused sanitizer groups (existing configured instrumentation):

```sh
cmake -S src/core -B build/sanitized
cmake --build build/sanitized --target core_search_batch_c_test core_search_query_test --parallel 2
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/sanitized --output-on-failure --parallel 2 --timeout 60 -R 'core_search_batch_c$|core_search_query$'
cmake -S src/core -B build/thread-sanitized
cmake --build build/thread-sanitized --target core_search_batch_c_test core_search_query_test --parallel 2
ctest --test-dir build/thread-sanitized --output-on-failure --parallel 2 --timeout 60 -R 'core_search_batch_c$|core_search_query$'
cc -std=c11 -pedantic-errors -fsyntax-only -Isrc/core/include src/core/tests/c_header_test.c
c++ -std=c++11 -pedantic-errors -fsyntax-only -Isrc/core/include src/core/search_policy.cpp
cmake --build build/legacy-search-policy --target xiphos_core_render --parallel 2
```

Dependencies are partly instrumented; leak checks are off. Portable checks on Mac
are not Fedora GTK/MSVC/WinUI acceptance. Actual CLucene execution remains pending;
controlled SDK framework tests validate selection/context/restoration only. The
initial capability assertion incorrectly assumed VerseKey RawGenBook remained
Generic Books and was prematurely pushed; `76895f4` corrects it to observed runtime
Biblical Texts and passes checks. Swift batch closure isolation and a test argument
order were corrected before publication. See the consolidated workflow's correction
record and Git history for earlier range/fixture evidence.

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
