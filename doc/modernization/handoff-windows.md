# Native Windows chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md) and the
[shared handoff](handoff.md) first. No task is currently claimed by this file.

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

## Next handoff entry

Replace this section with the [checkpoint format](handoff.md#evidence-to-record-at-each-checkpoint).
Include native Windows build, architecture, MSVC/SDK/dependency versions, tested
commit, local results, adapter/GUI coverage and platform failures still open.
Push the handoff so the other chats do not assume this work is already complete.
