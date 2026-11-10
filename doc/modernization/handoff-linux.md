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

## Next validation

Consume current shared APIs, run the local portable/desktop checks in the
actual Linux environment, then verify reading, search, library/session workflows,
keyboard/screen-reader behavior and packaging. Apply the [roadmap](shared-core-roadmap.md)
without creating independent search/navigation/persistence engines.
