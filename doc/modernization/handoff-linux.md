# Linux development handoff

Updated 2026-11-09. Read [shared instructions](handoff.md) and
[Linux setup/build commands](../../platforms/linux/README.md).

The active frontend is GTK4/WebKitGTK6, backed by the shared worker/C API.
GTK3 application code is retired. Existing Linux reader/model work is present;
read the current source before treating a historical gap as unimplemented.

Earlier native Fedora results and validation limits remain in the
[historical Linux handoff](history/handoff-linux.md). Later portable-model checks
run on a Mac are not native Linux desktop/input/clipboard or package acceptance.
No new physical Linux result is claimed by this documentation update.

## Maintenance baseline on master

Fetch the fork's `master` and read the [2026 native baseline](2026-native-baseline.md)
before changing Linux build requirements. The Mac preview now targets macOS 27.0,
requires CMake 3.20 for its fixture commands, and uses current SwiftUI change
callbacks. The Linux and standalone core minimum remains CMake 3.16, and the
shared core remains C++17.

For the shared build cleanup, record the CMake and SDK versions on the maintained
Linux targets and run both the standalone core and GTK4/WebKitGTK builds and
tests locally. Use that evidence alongside native Windows validation before
raising the shared CMake minimum and removing the CMake 3.16 libarchive shim.
Keep SWORD discovery and SDK-specific handling, module and profile migration,
and regression fixtures during this pass. Keep hosted Actions disabled.

## Fedora maintenance check

The Linux helper now installs curl and libarchive development packages and the
Python 3 and OpenSSL fixture tools. Preflight checks SQLite, curl and libarchive
alongside the existing dependencies.

On Fedora 45 ARM64, standalone core and GTK4 preview configuration passed at
master baseline `ac325ed4` using CMake 4.3.0, GCC 16.2.1, SWORD 1.9.0, GLib
2.90.1, GTK 4.24.1, WebKitGTK 2.54.2, curl 8.21.0 and libarchive 3.8.9. The
libarchive headers came from a locally extracted Fedora development RPM matching
the installed runtime. No system packages were installed.

The separate `linux/november-mac-parity` checkout at `050fdf91` with local edits
rebuilt the GTK4 preview and passed its basic graphical smoke. Its CTest suite
had 203 passes and one optional BibleSync skip after rerunning two HTTPS fixtures
with loopback access. These test results cover that branch. The merged master build subsequently passed 203 CTest checks and the GTK study
smoke, with one optional BibleSync skip. The remaining matrix checks are Fedora
44 compatibility and native Ubuntu 24.04 LTS and Debian 13 on x86-64.

## Linux parity integration

The Linux parity branch adds native library, repository, session, recovery,
parallel reading and study UI adapters. Its Fedora desktop smoke verified
parallel link navigation from nested markup with module inline handlers
disabled. The pre-merge branch recorded 203 CTest passes and one optional
BibleSync skip.

## Next validation

Consume current shared APIs, run the local portable/desktop checks in the
actual Linux environment, then verify reading, search, library/session workflows,
keyboard/screen-reader behavior and packaging. Apply the [roadmap](shared-core-roadmap.md)
without creating independent search/navigation/persistence engines.
