# Linux development handoff

Updated 2026-11-05. Read [shared instructions](handoff.md) and
[Linux setup/build commands](../../platforms/linux/README.md).

The active frontend is GTK4/WebKitGTK6, backed by the shared worker/C API.
GTK3 application code is retired. Existing Linux reader/model work is present;
read the current source before treating a historical gap as unimplemented.

Earlier native Fedora results and validation limits remain in the
[historical Linux handoff](history/handoff-linux.md). Later portable-model checks
run on a Mac are not native Linux desktop/input/clipboard or package acceptance.
No new physical Linux result is claimed by this documentation update.

Next: consume current shared APIs, run the local portable/desktop checks in the
actual Linux environment, then verify reading, search, library/session workflows,
keyboard/screen-reader behavior and packaging. Apply the [roadmap](shared-core-roadmap.md)
without creating independent search/navigation/persistence engines.
