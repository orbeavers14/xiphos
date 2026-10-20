# macOS chat handoff

Updated 2026-10-20. Read [AGENTS.md](../../AGENTS.md), the
[shared handoff](handoff.md), [core scope](shared-core-roadmap.md),
[status](status.md) and relevant parity rows first.

Active task (2026-10-20, master): initial C++ passive HTML documents now pass
8 normal, 8 ASan/UBSan and 8 TSan focused checks plus portable-on-Mac full
117 + 1 index skip (118 registered). Source/selection stays shared with text.
Next copied C/Swift HTML transport. Browser local-file preview was blocked by
URL policy; no workaround or visual HTML acceptance. Local checks only.
Checkpoint sections below preserve original scoped evidence; earlier next steps
are superseded by this active entry.

Previous checked handoff (2026-10-18, master): shared search advances checked/pushed through
`cf80541`, then export/advanced characterization `8c0a4ed`; next implement private
retained search-result generations before bounded export. Continue
bounded shared-core slices with local checks. Preserve unrelated files/GTK3.
All source changes are committed/pushed at this checkpoint; no shared source edits remain in flight. This is not a concurrent ownership lease.

## C++ HTML document checkpoint (2026-10-20)

Bounded source pushed at `2666599`. Initial selected/all passive HTML document
service shares the text selection resolver/formatter and configured source path.
Normal 8/8, ASan/UBSan 8/8 and TSan 8/8 checks pass, including text/C regressions
and prerequisite fixtures (partial SDK instrumentation/leaks off). Images/inline/
module CSS/active content reject as typed unsupported; no missing-source success.
Known core styles are isolated per wrapper; partial source is labelled. Real SDK
bare-ampersand links required structured canonicalization instead of XML-strict
wellFormed gating; the corrected rich-source test passes.
Portable-on-Mac full recheck passed 117 + 1 index skip (118 registered), 88.79 s,
GTK runtime OFF. In-app browser rejected local file
preview by URL protocol policy; no workaround attempted and no visual HTML/CSP
runtime acceptance claimed. See [HTML contract](html-export-workflows.md).
Next copied C/Swift HTML handles, preserving text ABI and native controls.

## Bounded HTML body checkpoint (2026-10-20)

Configured source/ThML correction pushed at `0fffdbd`. Bounded compact body/CSS
capture now passes 7 focused checks each on normal Mac, portable-on-Mac, ASan/UBSan
and TSan (source/copier/options+fixtures). The default 1 MiB/ceiling 8 MiB counts
returned module/reference/body/CSS; sized SDK body validation precedes C-string
truncation. No source/pref partial publication; live/legacy paths unchanged.
See [source limits](html-source-workflows.md). Next document/CSS isolation and
asset policy, not native HTML save. Full earlier suites retain their own hashes.

## Configured HTML source / ThML checkpoint (2026-10-20)

Reference guards are pushed at `c3218e6`. C++ configured body/commentary snapshots
now share the live renderer with non-publishing image/font discovery. Source
HTML/CSS/notes remain owned and profile/history/search neutral. Initial source
checks exposed the earlier guard wrongly rejecting ThML's counted terminal NUL;
a shared copier now accepts that SDK convention and still rejects embedded NUL/
malformed Unicode. Real ThML query and scripture regression cases pass.
ASan/UBSan 10/10 and TSan 10/10 pass with prerequisite fixtures/options/spacing
checks (partial instrumentation/leaks off). Initial ASan setup omitted one fixture
executable, leaving three dependents Not Run; the corrected complete rerun passes.
Full normal Mac: 150 passes + 1 index skip (151 registered), 127.53 s, with
desktop access for the isolated clipboard test. Current portable-on-Mac full recheck: 115 passes + 1 index skip
(116 registered), 86.92 s, GTK runtime OFF. See [source contracts](html-source-workflows.md).
Next bounded HTML document/CSS/asset policies; no HTML save/print parity yet.

## Native text actions checkpoint (2026-10-20)

C++ complete export `b7dafd4`, C `1bb2615`, Swift `80589a4` are pushed. Native
selected/all text menu, scripture toggle, progress/Stop, copied result identity,
clipboard and save dialog now consume those services. Full local Mac: 148 passes
+ 1 explicit index skip (149 registered), 126.84 s, with desktop access for an
isolated named-pasteboard check. Restricted pasteboard runs failed, then desktop
rerun passed; do not count the restricted attempts as passed. Temporary-file atomic
replacement and failed-payload preservation pass; general clipboard untouched.
Disposable window/AX and save checks confirmed ordered 199-byte original-prose
UTF-8 scripture export and selected dialog Cancel. Initial crowded labels were
moved onto a separate Export row and rechecked. See [the detailed contract](search-text-export-workflows.md).
Native HTML/print, advanced controls, corpus/physical input/accessibility remain.
No hosted job/reset credit, new Fedora/MSVC claim or legacy source retirement.

## Swift complete-result transport checkpoint (2026-10-20)

C complete-result bridge pushed at `1bb2615`. Swift 6 all/visible export shares one
copied-result adapter and passes 6/6 focused core/C/Swift+fixture checks. No second
source-selection or document engine. Next consuming native selected/all text
save/copy controls with stale/close/cancel protection and local window checks.
Full legacy/native parity, HTML/print and current Fedora/MSVC acceptance remain.

## C complete-result transport checkpoint (2026-10-20)

Complete C++ text service pushed at `b7dafd4`, including full portable-on-Mac
113 + 1 index skip (GTK runtime OFF). Additive C all-result descriptor and result
selection getter pass normal 2/2, ASan/UBSan 3/3 and TSan 3/3 (C/all-C+++fixture),
plus strict C11 header. Existing C layouts unchanged. Swift transport is under
focused checks; native hosts/HTML and representative corpus remain gates.

## Complete retained text checkpoint (2026-10-20, C++ only)

Selected export pushed through `ec26889`. Full Mac suite at that checkpoint passed
145 checks + 1 explicit index skip (146 registered), 122.52 s. This is not current
Fedora/MSVC/native GTK acceptance. Complete returned text export now passes 4/4
normal (all/visible/C+fixture), final 3/3 ASan/UBSan and 3/3 TSan (all/visible+fixture).
It shares document assembly, requires explicit incomplete-source acceptance and
rejects clipped all-results output. See [the contract](search-text-export-workflows.md).
Portable-on-Mac full recheck passed 113 + 1 index skip (114 registered), 87.47 s,
GTK runtime OFF. Next C/Swift
complete-result transport and native selected save/copy; keep HTML/corpus gates.
Hosted workflow audit: builds/native-core remain disabled; linux-core has only
manual dispatch; latest remote run is still Oct 15. No hosted jobs/reset credit.

## Swift selected text export checkpoint (2026-10-20)

C export bridge pushed at `6f0e3c1`. Swift 6 copied selected-row text adapter passes
5/5 focused C++/C/Swift export/generation+fixture checks, including exact Unicode
byte budgets and cancellation. Full local Mac/portable rechecks follow. Next
native save/copy consumption, then full retained results/HTML and broader parity.

## C selected text export checkpoint (2026-10-20)

Core selected visible export pushed at `3faadea`. Additive C11 owned request/result
now passes normal original-fixture checks, strict C11 header, 3/3 ASan/UBSan and
3/3 TSan (C export/generation+fixture). No partial payload or external write.
Swift export adapter is in progress; native save/copy remains pending.

## Selected text export checkpoint (2026-10-20)

Swift retention transport pushed at `d1a15df`. C++ session/worker selected visible
text export is now checked: original-fixture normal 2/2, ASan/UBSan 5/5 and TSan
3/3, with dependency instrumentation limits/leaks off. See [the contract](search-text-export-workflows.md).
Next add owned C/Swift export transport before native save/copy. Complete retained
results/HTML, advanced controls, platform runtime/corpus and full parity remain.

## Swift retention transport checkpoint (2026-10-20)

C bridge pushed at `155e0cb`. Swift 6 copied generation/query/policy/read adapter
and original-fixture test pass with the existing search adapters: 10/10 focused
checks including two fixture setups. Next implement bounded visible-row text
export using the shared formatter, then copied transport/native host actions.
No advanced GUI, complete export, platform release or retirement gate accepted.

## C retention transport checkpoint (2026-10-20)

C++ generation service is pushed at `37d9004`. Additive C11 transport now passes
original-fixture integration, strict C11 header checks, 4/4 ASan/UBSan and 4/4
TSan generation/batch/query+fixture checks. Read/query/policy ownership, stale
IDs and capacity fallback are documented in [retention contracts](search-retention-workflows.md).
No existing C layout changed. Swift transport is the next checked boundary;
complete export and native controls remain pending. Local checks only.

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
