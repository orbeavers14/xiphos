# Linux development

GTK4/WebKitGTK frontend and portable models around the shared core. GTK3
application code is retired. Native desktop acceptance is distinct from tests
run on another host or without a desktop.

## Shared core

On Debian/Ubuntu or Fedora, from the repository root:

```sh
platforms/linux/core.sh --install-deps
platforms/linux/core.sh --sanitize
```

Dependency installation requires administrator/network access; omit
--install-deps when already configured. The full native core needs the libraries
in the [dependency inventory](../../dependencies/README.md), including curl,
libarchive and SQLite. No GTK dependency is required for the standalone core.
core.sh performs local setup/build/test; see --help for flags.

## GTK4 frontend and portable models

GTK4.6+ and WebKitGTK6.0/2.40+ are additional desktop dependencies.

```sh
cmake -S platforms/linux -B build/gtk4
cmake --build build/gtk4 --parallel
ctest --test-dir build/gtk4 --output-on-failure
```

For portable models without the desktop libraries, configure with
-DXIPHOS_BUILD_GTK4_PREVIEW=OFF. This is not GTK4 runtime/input/clipboard verification.
The opt-in real-window smoke flow uses generated study-passage fixtures:

```sh
ctest --test-dir build/gtk4 -R '^(core_study_passage|linux_reader_model)$' --output-on-failure
GSK_RENDERER=cairo build/gtk4/xiphos-linux-preview --smoke-test build/gtk4/core/study-passage-fixture
```

Run desktop checks in an actual Linux graphical session. Keep navigation,
search, persistence and ownership in the shared core. No fresh Fedora or Linux
package acceptance is asserted by this documentation update.

[Linux handoff](../../doc/modernization/handoff-linux.md) ·
[Status](../../doc/modernization/status.md) ·
[Historical validation/advanced setup](../../doc/modernization/history/linux-readme.md)
