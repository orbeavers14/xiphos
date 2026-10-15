# Native Windows core validation

This directory prepares the native MSVC validation lane. No WinUI frontend or
passing Windows core suite is claimed yet. Read the
[Windows handoff](../../doc/modernization/handoff-windows.md) before working.
The MinGW/GTK application and WSL builds do not satisfy this lane.

## Inventory first

From native PowerShell (Windows PowerShell 5.1 or PowerShell 7):

```powershell
./platforms/windows/core.ps1 -Inventory | Format-List
```

If Windows PowerShell's local execution policy blocks this trusted checkout,
use a process-only invocation, without changing the machine's policy:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./platforms/windows/core.ps1 -Inventory
```

Inventory does not install tools, write files or run a build. Paths in its output
are local diagnostics; scrub them before recording evidence in this public fork.
It reports the host and process architectures separately: an emulated x64 shell
on an ARM64 host does not make the host x64. An empty dependency discovery path
does not prove no SDK exists elsewhere. CMake must still discover and link the
actual libraries; inventory is not dependency or parity acceptance.

## Required toolchain and dependencies

Install the stable Microsoft C++ Build Tools with Desktop development with C++,
the target's MSVC tools and a Windows SDK. The installer requires administrator
access. On an ARM64 host, include the ARM64 tools. Follow Microsoft's
[MSVC installation instructions](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc)
and [Windows on Arm guidance](https://learn.microsoft.com/en-us/visualstudio/install/visual-studio-on-arm-devices).
Also provide CMake >=3.16 (including its sibling `ctest.exe`) and Ninja in PATH;
the Visual Studio CMake component can supply these tools. A portable
[official CMake ZIP](https://cmake.org/download/) can be unpacked outside the
checkout; pass its executable through `-CMakeExecutable` if it is not in PATH.

Open a developer PowerShell with the intended target (`arm64` or `x64`). The
script verifies its target, compiler, MSVC headers/runtime libraries and Windows
SDK headers/import libraries before configuring. It uses Ninja and `cl` from
that shell, so it does not guess a Visual Studio generator/version or accept
MinGW as native MSVC evidence. Use a new build directory when changing the
compiler, target, runtime, dependency set or generator.

Build/provide SWORD >=1.8.1, GLib >=2.32 and libxml2 development SDKs for the
same target/compiler/runtime/configuration. Do not use Linux or MinGW libraries.
Record actual upstream versions, source URLs/revisions, checksums, license terms,
compiler/runtime and build flags. SWORD and bundled dependencies/modules retain
their individual notices and source obligations; no modules are downloaded by
this script. SDK discovery inputs are described in the
[core build guide](../../src/core/README.md). In particular:

- SWORD: `SWORD_INCLUDE_DIR`, `SWORD_LIBRARY`, optionally
  `XIPHOS_SWORD_EXTRA_LIBRARIES` for static transitive dependencies.
- GLib: `GLIB_INCLUDE_DIR`, `GLIB_CONFIG_INCLUDE_DIR`, `GLIB_LIBRARY`, optionally
  `XIPHOS_GLIB_EXTRA_LIBRARIES`.
- libxml2: `LIBXML2_INCLUDE_DIR`, `LIBXML2_LIBRARY`; supply its static transitive
  libraries through the SDK's supported CMake configuration as required.

Prefer a coherent SDK prefix or toolchain over mixing individual binaries.
pkg-config is optional; when used, its metadata must describe the Windows target
and the selected configuration. Inventory only reports its executable/search
path; it does not verify the provenance or ABI of an SDK.

## Configure, build, test

For an ARM64 dependency prefix, in a matching developer shell:

```powershell
$sdkOptions = @('-DCMAKE_PREFIX_PATH=C:/sdk/arm64')
./platforms/windows/core.ps1 -Architecture arm64 -Configuration Debug -CMakeArgument $sdkOptions
```

`C:/sdk/arm64` is an example placeholder, not an installed SDK. Use `x64` with
a matching shell/SDK for x64 validation. `-BuildDirectory` selects a separate
build location; the default is ignored `build/windows-core-<target>-<config>`.
`-Jobs` defaults to two. The entry point forces C++17 and enables all existing
core tests, fails on configure/build/test errors and rejects an empty test run.
Generated original-prose modules stay in the selected build directory.

Passing preflight is only permission to attempt configuration. Record compiler
identification, dependency versions, the implementation commit, exact commands,
CTest count/failures and runtime DLL deployment in the Windows handoff. File
replacement/Unicode behavior must be tested separately before data compatibility
is accepted. After the complete native core suite passes, proceed to the thin
WinUI 3/WebView2 reading slice described in the handoff. Keep routine checks local.
