# Linux / Fedora / WSL2 chat handoff

Updated 2026-10-14. Read [AGENTS.md](../../AGENTS.md) and the
[shared handoff](handoff.md) first. No task is currently claimed by this file.

## Existing evidence and boundaries

`f190c49` (PR #82) adds Fedora/Debian/Ubuntu dependency setup, standalone release
and sanitizer checks, explicit C++17 selection and CMake extraction fixes.
`cf9c853` removes automatic Linux hosted triggers; do not put them back.
The owner reports a passing Fedora run. This chat has inspected the pushed source,
but has not received the run's exact tested commit, Fedora/compiler/dependency
versions, test count or logs. Record those from the Fedora checkout before treating
them as a reproducible platform checkpoint. GTK4 frontend code is not implemented
at this handoff. Core build success does not establish Linux interface parity.

## Next bounded work

1. Capture the existing Fedora validation details; rerun only checks needed to
   establish missing evidence or validate subsequent changes. Confirm the core
   uses C++17 on the selected compiler.
2. Create an isolated native GTK4 frontend build under `platforms/linux`, leaving
   the root GTK3 build operational. Inspect available GTK4/WebKitGTK dependencies
   on Fedora before choosing package names or promising minimum versions.
   The target renderer is GTK4-compatible WebKitGTK; do not reuse the legacy
   GTK3 WebKit package as if it were GTK4-compatible.
3. Implement the first module selection / reference / Bible-and-commentary panes /
   Back-and-Forward slice through the existing asynchronous C API or a thin C++
   adapter to the same worker. Inspect `src/core/include/xiphos/core/c_api.h` and
   the macOS consumer before adding an API. Copy borrowed values before releasing
   results. Never run engine jobs or blocking shutdown on the GTK main loop.
4. Check empty libraries, missing modules, failed navigation, repeated actions,
   close-with-work-pending and link/history behavior. Match the established pane
   composition; module routing, history and persistence must remain core-owned.
   Record keyboard and real desktop interaction evidence separately from build tests.

## Local validation

From the repository root on Fedora or another supported Linux environment:

```sh
platforms/linux/core.sh --install-deps
platforms/linux/core.sh --sanitize
```

Omit `--install-deps` when dependencies are already present. The script builds and
runs standalone core tests; it does not install GTK4 or build a native UI. Use
`--jobs N` to fit the computer, and separate build directories for toolchains/modes.
See [the Linux guide](../../platforms/linux/README.md) for details. Add documented
local GTK4 build/run commands when that frontend actually exists; do not invent
passing GUI evidence before using it.

WSL2 can validate Linux tooling and GUI iterations. Record it as WSL2; it does
not validate native Windows/MSVC or every full Linux desktop integration. A Fedora
desktop/VM can supply those Linux interaction checks. Hosted builds stay manual
and require owner authorization; no runner minutes are needed for this work.

## Next handoff entry

Replace this section with the [checkpoint format](handoff.md#evidence-to-record-at-each-checkpoint).
Include Fedora version, architecture, compiler, dependency versions, tested commit,
normal/sanitizer results, GUI coverage and pending checks. Push the evidence and
implementation so Mac/Windows chats can pull the same contracts.
