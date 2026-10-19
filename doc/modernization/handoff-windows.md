# Native Windows chat handoff

Shared API notice (2026-10-18): complete per-tab last-search scope is now private
to the shared core (`c1b0a88`); worker/C query/clear and scalar metadata commands
are in `c38cd8f`. Existing normal/range searches reseed this tab's complete scope,
independent of display limits. Search capabilities copy from the shared runtime
predicate (`4e3402c`, SDK classification correction `76895f4`). Basic Mac controls
`0ca0bab` use copied scope/progress; other hosts should adapt the same APIs.
Validate `XIPHOS_SWORD_SEARCH_ICASE_FLAG` against this platform's SDK (tested Mac
uses 2; C++11 regex uses 1). Mac-hosted portable 98/98 is not new native-platform
acceptance; preserve the actual results below. See [search contracts](search-workflows.md).


Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md) and the
[shared handoff](handoff.md) first.

Current task on `master`: validate the shared core on native Windows ARM64.
The prerequisite inventory and local build entry point are published; compiler
installation is pending owner action. Shared core/API ownership is not claimed.
Native build acceptance remains open.

## Existing evidence and boundaries

The native core supports explicit SDK discovery paths, but native Windows/MSVC
linking, file replacement behavior and a WinUI 3 frontend remain unvalidated.
There is no new Windows frontend at this checkpoint. The old MinGW/GTK packaging
is a legacy application path, not evidence of WinUI 3 parity. Fedora/WSL passes
are Linux checks and do not close the Windows gates.

## Incoming shared note controls

`aff5f59` adds queued C capability/settings and typed copied value/availability;
`1d3612f` adopts them in existing Mac/GTK preview controls. Use the same API for
future WinUI controls. Default-off capability and engine configuration are
separate from the saved per-module value. Formatting/counters stay in the core.
Local Mac and portable tests do not establish MSVC or WinUI acceptance. See the
[note contract](note-numbering-workflows.md); shared ownership is released.

## Next bounded work

1. Inventory the actual Windows architecture and installed compiler/Windows SDK,
   CMake and SWORD/GLib/libxml2 development libraries. Build a compatible dependency
   set with one compiler/runtime/architecture; do not link Linux/MinGW binaries into
   MSVC or assume the Mac SDK paths exist. Record versions and licensing provenance.
2. Configure/build/test `src/core` natively with MSVC and C++17 before adding UI.
   Use [the core build guide](../../src/core/README.md) for discovery variables;
   fix genuine platform defects in the shared core with regression evidence and
   coordination, rather than bypassing tests or implementing Windows-only logic.
3. Once the core passes, add an isolated `platforms/windows` WinUI 3 build and a
   thin adapter to the existing C ABI. Preserve one worker per process, native UI
   thread dispatch, copied UTF-8 results, error handling and draining shutdown.
   Validate any required DLL exports/runtime deployment explicitly; current static
   core targets are not a packaged Windows interop DLL.
4. Build the same module selection / passage / commentary / history slice as
   Linux/macOS. WebView2 presents core-owned HTML; study navigation and URI routing
   stay in the core. Keep module scripts/network content disabled. Check Unicode
   paths/text, empty libraries, failure/history behavior, keyboard and pending-close.
5. Record Windows file-save/replacement compatibility separately; existing Mac
   tests do not establish Windows data migration or durability guarantees.

## Local validation

Example from a native Windows developer shell, after dependencies are discoverable
and the selected CMake generator/compiler have been recorded:

```powershell
cmake -S src/core -B build/windows-core -DCMAKE_CXX_STANDARD=17 -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows-core --config Debug --parallel 2
ctest --test-dir build/windows-core -C Debug --output-on-failure
```

These are intended commands, not recorded passing Windows results. Stop and inspect
failures before proceeding. Explicit dependency paths/extra libraries may be needed;
use a new build directory when changing generator/compiler/architecture. `--config`
and `-C` select the configuration for multi-configuration generators. A shell running
in WSL uses Linux tools instead; label its output as Linux evidence.

WinUI/.NET/C++ interop choice and exact build commands must be documented when
implemented and tested. Keep hosted checks off; do not dispatch the legacy MinGW
workflow to stand in for an unavailable native Windows environment.

## 2026-10-15 native Windows prerequisite checkpoint

- Task/branch: Windows ARM64 core validation on `master`; build tooling checked,
  native compiler/dependency validation pending. Owner was given the official
  Build Tools installer and ARM64/Windows SDK/CMake component directions and
  offered to install them with administrator access.
- Implementation commit checked: `0ad8917726e1214329f153b9d149238b3062f7bb`.
- Host: native Windows build 26300, ARM64 host and ARM64 PowerShell process.
  PowerShell 7 and Windows PowerShell 5.1 inventory/native-exit checks passed.
  No MSVC, Windows SDK, Ninja, pkg-config or .NET SDK CLI was found in PATH or
  standard install locations. No compatible SWORD/GLib/libxml2 SDK was located;
  this does not rule out an SDK elsewhere. The session has no administrator token.
- Added [core.ps1](../../platforms/windows/core.ps1) and
  [Windows build instructions](../../platforms/windows/README.md): read-only
  inventory, developer-target/SDK prerequisites, Ninja/MSVC C++17 configuration,
  all existing core tests, native-error propagation and empty-suite rejection.
  No core, C ABI, legacy application or hosted workflow behavior changed.
- Local-only tool: official Kitware CMake 3.31.8 ARM64 ZIP, downloaded from
  `https://github.com/Kitware/CMake/releases/download/v3.31.8/` and verified
  against the release's `cmake-3.31.8-SHA-256.txt`. Archive SHA-256:
  `d37ca89ec60aece080664ee936f7aaffdfdbde0a201b165c45ecfaf68a4c1fa9`.
  Its `bin` directory was used as `$cmakeBin` below; no binary is committed.
- Checks actually run from the repository root:

  ```powershell
  ./platforms/windows/core.ps1 -Inventory | Format-List
  ./platforms/windows/core.ps1 -Inventory -CMakeExecutable "$cmakeBin/cmake.exe"
  ./platforms/windows/core.ps1
  & "$cmakeBin/cmake.exe" --version
  & "$cmakeBin/cmake.exe" -S src/core -B build/windows-core-arm64-probe -G 'Visual Studio 17 2022' -A ARM64 -DCMAKE_CXX_STANDARD=17 -DBUILD_TESTING=ON
  git diff --check
  ```

  Inventory and portable CMake/sibling CTest discovery passed. Default build
  invocation correctly rejected the missing compiler/SDK/tools. CMake reports
  3.31.8; direct configuration failed at `project()` because no Visual Studio
  instance exists. **Zero core tests ran; no MSVC linking or parity is accepted.**
  PowerShell parser validation passed. Windows PowerShell's default execution
  policy blocked the script; a process-only `-ExecutionPolicy Bypass` invocation
  passed inventory without changing the machine policy.
- Native helper checks used the real CMake/CTest executables:

  ```powershell
  $null = . ./platforms/windows/core.ps1 -Inventory
  Invoke-Checked "$cmakeBin/cmake.exe" @('--version')
  # Expected exception: native exit code 1, checked in both PowerShell versions.
  Invoke-Checked "$cmakeBin/cmake.exe" @('-E', 'false')
  New-Item -ItemType Directory -Force build/windows-empty-test-probe
  # Expected exception: CTest found no tests, rather than reporting a passing suite.
  Invoke-Checked "$cmakeBin/ctest.exe" @('--test-dir', 'build/windows-empty-test-probe', '--no-tests=error')
  ```

- Publication: command-line Git credential-manager invocation failed in this
  sandbox; the connected GitHub API published the checked files with an expected
  branch-head check and no force update. No hosted jobs were dispatched.
- Not checked: native core compile/link/tests, dependency versions/provenance,
  DLL deployment, Unicode/file replacement behavior, WinUI/GUI and accessibility.
- Next bounded task: after the owner's MSVC/ARM64/Windows SDK installation,
  initialize a matching developer shell, build a coherent dependency set, then
  run `core.ps1` and inspect every failure before implementing WinUI.

## Shared API checkpoint to consume (2026-10-18)

Pull checked shared work through `cf80541`: attribute modes 3–5, explicit multiword
6/raw indexed 7, canonical `xc_search_query` with optional annotation context,
complete per-tab last scope and ordered bounded multi-module requests. Core batches
`f86293b`, C `ad28750`, Swift `cf80541` use one worker; do not duplicate scope/context
or query preparation in the frontend. `xc_worker_search_modules` takes one common
query (module NULL), ordered module IDs and optional total output limits. It returns
owned batch/per-group status; successful child searches are borrowed const until
batch release and must not be released separately. Existing C requests/layouts stay
compatible. See [search contracts](search-workflows.md).

Mac validation passed 136/136 Mac/core and 105/105 portable core/model tests on Mac,
with focused ASan/UBSan/TSan and strict C11/C++11 checks. This does not update this
platform's native compiler/GUI evidence above. Rebuild locally with the actual
SDK/case flag before accepting target behavior; no hosted build should be enabled
or dispatched. Basic Mac controls exist; advanced controls/indexed execution,
mixed families, export/settings/full acceptance remain pending.

Latest follow-up: `8c0a4ed` shares the exact export item formatter with retained GTK
(C++11 + C11 facade; free with `xiphos_export_item_free`) and characterizes advanced
range/export/bookmark quirks. Full GTK3 desktop linking/runtime is not claimed by
Mac C adapter tests. `a3dcae8` adds an actual-index diagnostic: default optional
mode explicitly skips unavailable frameworks; set
`XIPHOS_REQUIRE_INDEXED_SEARCH_TEST=ON` for a mandatory target gate. This Mac SDK
returned creation status 0 without a usable index, so actual indexed execution
remains unvalidated. The expected failing required-mode probe is not acceptance.
Latest full local totals: 139 Mac/core passes + 1 skip; 108 portable passes + 1
skip on Mac. Export focused normal/ASan checks pass 3/3; earlier search TSan evidence
remains above. These do not replace this OS's native evidence.

The next shared slice is [retained search-result generations](search-export-design.md),
then bounded export. That proposal is not an implemented API. Display prefixes
and final batch last scope cannot reconstruct every group's complete results;
do not create a frontend workaround or implicitly rerun searches. Overall 21/38
groups now have some core/Mac evidence, 0/38 are accepted and 0/3 native releases
are ready. Keep local-only checks and preserve all legacy retirement gates.
