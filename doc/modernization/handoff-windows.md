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

## ARM64 setup checkpoint (2026-11-16)

On Windows 11 25H2 ARM64, Xiphos commit `26852e1a` passed CMake compiler and
ABI detection with MSVC 19.52.37029, Windows SDK 10.0.26100.0, CMake 4.3.1 and
Ninja 1.13.2. Set `CMAKE_POLICY_DEFAULT_CMP0141=NEW` and
`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded` in a fresh build directory to
work around the compiler's C1902 program database failure in this shell.

The local ARM64 Release SWORD 1.9.0 static SDK used the five reviewed patches,
the `/MD` runtime, bundled zlib and regex, and the Windows Winsock library.
The local SDK source also needed an `extern` declaration for static FTP symbols
where the Windows header normally applies DLL annotations. An MSVC consumer
linked with `ws2_32.lib`, constructed `SWMgr`, and parsed John 3:16 successfully.
This SDK has no CLucene support, so it cannot validate indexed search.

The owner installed the ARM64 vcpkg dependencies from regular Windows PowerShell
after vcpkg hit an `Access is denied` error opening a child-process stdin pipe in
the Codex shell. SWORD was rebuilt against vcpkg's zlib, bzip2 and liblzma to
avoid duplicate zlib symbols. CMake used ARM64 pkgconf to find GLib and explicit
SWORD headers/library and link inputs (`ws2_32` and the three compression libraries).
Keep SWORD's `.pc` file outside pkgconf's search path when selecting the SDK
explicitly. The vcpkg Release DLL directory was added to `PATH` for tests.

The native Release build linked the core and test executables after MSVC fixes
for general-book traversal, worker object size, a missing STL include and legacy
test fixtures. CTest registered 183 checks. Windows core acceptance and WinUI
implementation remain pending.

The first full ARM64 run had 128 passes, one skip, 21 failures (including three
timeouts and one process exception), and 33 checks withheld by failed fixtures.
Two subsequent focused retests passed: Unicode search with `/utf-8`, and staged
rich-module save/reload/rollback after closing SWORD's cached descriptors before
Windows directory renames. These changes need a full rerun. Prioritize the
remaining fixture failures in reader/navigation, profile file paths, image
resources, and library / remote installation before claiming native parity.

On 2026-11-17, four profile XML tests passed with UTF-8 compilation. The
preference-save test also passed after the Windows test executables opted into
a UTF-8 process code page for SWORD and the C runtime's narrow file APIs. The
future WinUI host needs the same process setting. Focused ARM64 retests also
passed reader navigation, book-document publication with a Unicode path,
stylesheet selection and book-image resources. The last two fixtures needed
portable path comparisons and a standard drive-letter file URI. The full
suite still needs a rerun, including verse images and module installation.

The next full ARM64 build linked all test executables, but the drive ran out of
space during CTest. The partial run is not a replacement baseline. Restart the
suite after reclaiming space. A local follow-up applies the SWORD file-handle
release to installation and maintenance and adds curl error details for HTTPS
triage. Both changes still need local verification before publication.
