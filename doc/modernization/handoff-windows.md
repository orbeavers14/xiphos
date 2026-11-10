# Windows development handoff

Updated 2026-11-09. Read [shared instructions](handoff.md) and
[Windows inventory/build tooling](../../platforms/windows/README.md).

Target Windows 11 version 25H2 and check compatibility on 26H2. The repository has an MSVC build
helper. Native Windows core validation and WinUI 3 implementation are pending.
The old GTK/MinGW win32 distribution is removed. WSL and Mac-hosted portable
checks are not MSVC or native Windows desktop validation.

Next: inventory actual Windows SDK/compiler dependencies, validate the shared
C++17/C11 interface locally, then implement WinUI3 adapters around the same FIFO
worker. Track coherent Settings/navigation, keyboard/Narrator, high contrast and
scaling in the [native UI list](native-ui-todo.md). No current Windows build or
frontend acceptance is claimed by this documentation update.

[Historical prerequisite and API evidence](history/handoff-windows.md) remains
available. Its dated compiler/SDK observations must be rechecked on the target machine.
