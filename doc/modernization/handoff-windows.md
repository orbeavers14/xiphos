# Windows development handoff

Updated 2026-11-17. Read [shared instructions](handoff.md) and
[Windows inventory/build tooling](../../platforms/windows/README.md).

Target Windows 11 version 25H2 and check compatibility on 26H2. The native ARM64
core suite has passed on 25H2 with the SDK below. The WinUI 3 reading preview
uses the same shared core through a narrow C ABI DLL.
The old GTK/MinGW win32 distribution is removed. WSL and Mac-hosted portable
checks are not MSVC or native Windows desktop validation.

Track Settings/navigation, keyboard/Narrator, high contrast and
scaling in the [native UI list](native-ui-todo.md). Native x64, 26H2 compatibility,
and Windows runtime deployment validation remain open.

[Historical prerequisite and API evidence](history/handoff-windows.md) remains
available. Its dated compiler/SDK observations must be rechecked on the target machine.

## WinUI 3 reading preview (2026-11-17)

Build instructions are in the [Windows build guide](../../platforms/windows/README.md).
The unpackaged .NET 10 preview uses Windows App SDK 2.7.0 and a UTF-8 process
manifest. The shell opens a local SWORD library and uses the shared FIFO worker
for the catalog, passage rendering, commentary, navigation and history. WebView2
is configured for passive HTML and routes individual passage links through the
core. Reference lists, note previews, and other study views remain open.

The ARM64 Release native DLL and WinUI application built successfully on
Windows 11 25H2. A managed smoke check loaded a generated SWORD Bible, rendered
John 3:16, moved to John 4:1, returned through history, paired generated
commentary at John 3:16, and followed a scripture link to John 1:1. In a normal
desktop process, entering John 3:16 in the native window displayed the Unicode
Bible fixture text in WebView2. Visual commentary pairing and broader keyboard,
Narrator, high-contrast and scaling checks remain open. The Codex command sandbox returned HRESULT
`0x8000FFFF` from WebView2 initialization, so run desktop checks from an
ordinary Windows session.

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
space during CTest. After reclaiming space, focused ARM64 tests passed local
installation, module maintenance, and encrypted-module maintenance with the
SWORD file-handle release in the library swap. The loopback HTTPS installation
fixture also passed when the process had network access. The curl transport now
reports the error category or unexpected HTTP status for failures.

The full ARM64 Release rerun at `98850e5b` recorded 154 passes, one skip, seven
failures (four timeouts and three assertions), and 21 tests blocked by failed
fixtures. Verse-image handling, local installation, module maintenance and
loopback HTTPS passed. The two search fixtures and wide-book pagination passed
direct runs in roughly 117–118 seconds each. Unicode book publication passed a
direct retest. Windows CTest now allows 180 seconds for those four fixtures.
Book creation and deletion also passed after checking indexed results only
when the SDK had a usable index. The explicit required-index run still requires
CLucene. Mixed keyed search returned zero hits in all three compatibility
modules, and dictionary navigation failed the per-tab options assertion. Retest
with the updated fixture limits, investigate those two assertions, and verify
indexed behavior with an appropriate SWORD SDK before Windows core acceptance.

An ARM64 Release SWORD 1.9.0 rebuild with [ICU4C 79.1][icu-release] passed
`core_mixed_search`, `core_dictionary`, `core_keyed_search`, and `core_rich_module`
without application changes. Use the ARM64 ICU distribution, add its
`libARM64` import libraries (`icuin`, `icuio`, `icuuc`, `icudt`) to the static
SWORD link inputs, and add `binARM64` to the test process's `PATH`. The tested
ZIP `icu4c-79.1-WinARM64-MSVC2026.zip` has SHA-256
`185d32268533a981f070c75a4435a9141ee58c12c342468cf431d746b9b68237`.

On the prior SDK, wide-book pagination and book creation passed again under
CTest. Both search fixtures timed out during a concurrent compiler build, and
the book-document setup fixture timed out, preventing the publication check.
Run the validation suite without concurrent compilation.

The ARM64 SDK with ICU and CLucene 2.3.3.4 passed five focused checks with
`XIPHOS_REQUIRE_INDEXED_SEARCH_TEST=ON`: attribute search, rich-module editing,
indexed book creation/deletion/rollback, keyed index search, and verse index
search. The bundled SWORD Windows patch fixes directory removal after CLucene
deletes index entries. The [dependency recipe](../../dependencies/sword/README.md#windows-arm64-indexed-sdk)
records the source, patches, linker inputs and runtime paths.

The full serial ARM64 Release run with that SDK and required indexed checks
passed 180 of 183 tests in 1,742 seconds. Search range, search text options and
search batch were the only timeouts. All three passed a serial CTest rerun with
180-second Windows limits in 42.75, 142.28 and 132.35 seconds respectively.
The rerun reused the two passing search setup fixtures from the full run.
All 183 checks have a passing result across the two runs. The updated limits
applied only to the three rerun checks after the full run completed.

[icu-release]: https://github.com/unicode-org/icu/releases/tag/release-79.1
