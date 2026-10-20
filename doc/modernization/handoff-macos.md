# macOS chat handoff

Updated 2026-10-19. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [core scope](shared-core-roadmap.md),
[status](status.md) and relevant parity rows first.

Active task (2026-10-19, master): implement private retained result generations,
aggregate retention limits and copied stale-safe reads; local checks only.

Previous checked handoff (2026-10-18, master): shared search advances checked/pushed through
`cf80541`, then export/advanced characterization `8c0a4ed`; next implement private
retained search-result generations before bounded export. Continue
bounded shared-core slices with local checks. Preserve unrelated files/GTK3.
All source changes are committed/pushed at this checkpoint; no shared source edits remain in flight. This is not a concurrent ownership lease.

## C++ retention checkpoint (2026-10-19)

Private result generations, aggregate caps and copied stale-safe reads are checked
in this checkpoint; next add C/Swift transport, then bounded visible-row export.
Full local Mac suite: 140 passes + 1 index skip (141 registered). Portable suite
quiet rerun: 109 passes + 1 index skip (110). Focused ASan/UBSan: 6/6; TSan: 4/4,
run serially after builds. Initial concurrent checks timed out (one reported 910 s);
affected portable checks passed 3/3 before the quiet full rerun. Do not count those
initial timeouts as passing runs. SDK instrumentation remains partial/leaks off.
See [retention contracts](search-retention-workflows.md) for state/capacity/ownership,
SDK prepared-query bounds and corrected test-only SWBuf/NUL construction evidence.
No hosted builds, reset credit or legacy source retirement.

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

Next bounded task: retained search-result generations/capability metadata on the
SDK owner, following the [proposed design](search-export-design.md), then bounded
visible-row export before all-results/HTML/native hosts. Current output prefixes
and final batch scope cannot reconstruct all groups. Shared item formatter and
actual setup/export/bookmark quirks are characterized in [export evidence](export-workflows.md).
Advanced controls, mixed families, saved settings, full corpus and other core
services remain separate gates. No hosted jobs/reset credit used; preserve GTK3,
licenses, Windows lane and unrelated `.DS_Store`.

## Actual local checks

Full local suites after export integration: **139 Mac/core passes + 1 explicit
index skip** (140 tests), **108 portable core/model passes + 1 index skip** (109,
on this Mac). Core/C/Swift batch checkpoint `cf80541` previously passed 136/136
and 105/105. Export `8c0a4ed` adds three passing normal/ASan/UBSan checks for exact
item bytes/SDK-call order, real C-to-C++ adapter linking and unchanged advanced
setup/export/bookmark orchestration. Final fail-fast legacy guard/pedantic C11
checks pass three normal and three ASan. Shell pkg-config lacked GLib discovery;
configured CMake SDK paths and strict C11/pedantic targets pass instead.

Batch/context search at `f86293b` passed **8/8 ASan/UBSan**, **6/6 TSan**; C transport
at `ad28750` passed **3/3 ASan/UBSan**, **3/3 TSan**. Six focused Swift checks cover
groups/budgets/scope/context/progress/cancellation/lifetime. Strict C11 header,
C++11 policy and genuine native-OFF CoreRender/default/actual parent routes pass.

Indexed diagnostic checkpoint `a3dcae8` is pushed.

Optional `core_search_index` uses a separate original-prose fixture copy and skips
on this SDK: creation status 0 supplied no usable framework. The required-index
configuration argument and direct `--require-index` negative control are verified
(expected exit 1). This does not pass actual indexed execution. Configure
`XIPHOS_REQUIRE_INDEXED_SEARCH_TEST=ON` on a capable SDK and require a non-skipped
pass before claiming that gate. Normal/ASan optional probes skip; their unexecuted
OR/AND/bounds/last assertions remain future validation, not accepted behavior.

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
