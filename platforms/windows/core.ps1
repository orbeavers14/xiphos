# SPDX-License-Identifier: GPL-2.0-or-later
<#
.SYNOPSIS
Inventory or configure, build and test the shared core with native MSVC.
.DESCRIPTION
Run from an MSVC developer PowerShell for the selected target architecture.
No software is installed and no hosted jobs are started. Inventory mode is
read-only; its paths describe this computer and must not be committed publicly.
.EXAMPLE
./platforms/windows/core.ps1 -Inventory
.EXAMPLE
./platforms/windows/core.ps1 -Architecture arm64 -CMakeArgument @('-DCMAKE_PREFIX_PATH=C:/sdk')
#>
[CmdletBinding()]
param(
    [switch]$Inventory,
    [ValidateSet('arm64', 'x64')][string]$Architecture,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')][string]$Configuration = 'Debug',
    [string]$BuildDirectory,
    [ValidateRange(1, 256)][int]$Jobs = 2,
    [string]$CMakeExecutable = 'cmake',
    [string[]]$CMakeArgument = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) {
    throw 'This entry point requires native Windows; WSL is Linux validation.'
}
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$hostArchitecture = [Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString().ToLowerInvariant()
if (-not $Architecture) { $Architecture = $hostArchitecture }
if ($Architecture -notin @('arm64', 'x64')) {
    throw "Unsupported target architecture: $Architecture. Select arm64 or x64."
}

function Find-Application([string]$Name) {
    $command = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command) { return $command.Source }
    return $null
}

function Test-File([string]$Directory, [string]$Child) {
    if (-not $Directory) { return $false }
    return (Test-Path -LiteralPath (Join-Path $Directory $Child) -PathType Leaf)
}

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    # Handle native exit status consistently even if the caller enabled PS 7's
    # optional native-command error preference. This variable is function-local.
    $PSNativeCommandUseErrorActionPreference = $false
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$([IO.Path]::GetFileName($Program)) failed with exit code $LASTEXITCODE." }
}

$cmake = Find-Application $CMakeExecutable
$ctest = $null
if ($cmake) {
    $candidate = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
    if (Test-Path -LiteralPath $candidate -PathType Leaf) { $ctest = $candidate }
}
$cl = Find-Application 'cl.exe'
$ninja = Find-Application 'ninja.exe'
$sdkVersion = $env:WindowsSDKVersion
if ($sdkVersion) { $sdkVersion = $sdkVersion.TrimEnd('\', '/') }
$sdkInclude = $null
$sdkLibrary = $null
if ($env:WindowsSdkDir -and $sdkVersion) {
    $sdkInclude = Join-Path $env:WindowsSdkDir "Include/$sdkVersion"
    $sdkLibrary = Join-Path $env:WindowsSdkDir "Lib/$sdkVersion"
}
$report = [pscustomobject][ordered]@{
    OS = [Runtime.InteropServices.RuntimeInformation]::OSDescription
    PowerShellVersion = $PSVersionTable.PSVersion.ToString()
    HostArchitecture = $hostArchitecture
    ProcessArchitecture = [Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture.ToString()
    TargetArchitecture = $Architecture
    DeveloperTarget = $env:VSCMD_ARG_TGT_ARCH
    MSVC = $cl
    MSVCToolsVersion = $env:VCToolsVersion
    MSVCHeaders = (Test-File $env:VCToolsInstallDir 'include/vector')
    MSVCRuntimeLibrary = (Test-File $env:VCToolsInstallDir "lib/$Architecture/libcmt.lib")
    WindowsSDKVersion = $sdkVersion
    WindowsHeaders = (Test-File $sdkInclude 'um/Windows.h')
    UCRTHeaders = (Test-File $sdkInclude 'ucrt/stdio.h')
    WindowsImportLibrary = (Test-File $sdkLibrary "um/$Architecture/kernel32.lib")
    UCRTLibrary = (Test-File $sdkLibrary "ucrt/$Architecture/ucrt.lib")
    CMake = $cmake
    CTest = $ctest
    Ninja = $ninja
    PkgConfig = (Find-Application 'pkg-config.exe')
    # These are discovery inputs, not claims that compatible dependencies exist.
    CMakePrefixPath = $env:CMAKE_PREFIX_PATH
    PkgConfigPath = $env:PKG_CONFIG_PATH
}
if ($Inventory) { return $report }

$missing = @()
foreach ($tool in @('MSVC', 'CMake', 'CTest', 'Ninja')) {
    if (-not $report.$tool) { $missing += $tool }
}
foreach ($check in @('MSVCHeaders', 'MSVCRuntimeLibrary', 'WindowsHeaders', 'UCRTHeaders', 'WindowsImportLibrary', 'UCRTLibrary')) {
    if (-not $report.$check) { $missing += $check }
}
if ($missing.Count) {
    throw ('Missing native build prerequisites: ' + ($missing -join ', ') +
        '. Install the C++ tools/Windows SDK and open the matching developer PowerShell; see platforms/windows/README.md.')
}
if ($env:VSCMD_ARG_TGT_ARCH -ne $Architecture) {
    throw "Developer shell targets '$env:VSCMD_ARG_TGT_ARCH'; requested '$Architecture'. Open a matching developer PowerShell."
}
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $repositoryRoot "build/windows-core-$Architecture-$($Configuration.ToLowerInvariant())"
}
# Resolve relative build paths against the caller's directory, as CMake does.
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
if ($BuildDirectory -eq $repositoryRoot -or $BuildDirectory -eq (Join-Path $repositoryRoot 'src/core')) {
    throw 'Use a separate build directory, not the repository or core source directory.'
}

Invoke-Checked $cmake @('--version')
Invoke-Checked $ninja @('--version')
$configure = @('-S', (Join-Path $repositoryRoot 'src/core'), '-B', $BuildDirectory, '-G', 'Ninja')
$configure += $CMakeArgument
# Required values follow caller SDK options; tests and language level are gates.
$configure += @('-DCMAKE_C_COMPILER=cl', '-DCMAKE_CXX_COMPILER=cl', '-DCMAKE_CXX_STANDARD=17',
    '-DCMAKE_CXX_STANDARD_REQUIRED=ON', '-DBUILD_TESTING=ON', "-DCMAKE_BUILD_TYPE=$Configuration")
Invoke-Checked $cmake $configure
Invoke-Checked $cmake @('--build', $BuildDirectory, '--config', $Configuration, '--parallel', "$Jobs")
Invoke-Checked $ctest @('--test-dir', $BuildDirectory, '-C', $Configuration, '--output-on-failure',
    '--parallel', "$Jobs", '--timeout', '120', '--no-tests=error')
