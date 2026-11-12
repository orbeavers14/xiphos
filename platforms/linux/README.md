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
libarchive and SQLite. Its test fixtures also require Python 3 and the OpenSSL
command-line tool. No GTK dependency is required for the standalone core.
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
search, persistence and ownership in the shared core.

## Desktop installation

```sh
cmake --install build/gtk4 --prefix "$HOME/.local"
xdg-mime default org.xiphos.LinuxPreview.desktop x-scheme-handler/sword
```

Use the second command if you want Xiphos to handle `sword://` links by default.
For this prefix, include `$HOME/.local/bin` on the desktop session's PATH.
Open **Biblical Map** from the reader menu to preview or install an offline
PMTiles archive. See the [map controls and storage](../../doc/modernization/biblical-maps.md).

## Fedora preview RPM

Install Fedora's `rpm-build` tool on the build host. Build the GTK frontend
with the default relative binary and data directories on the Fedora version
and architecture you plan to target:

```sh
cmake -S platforms/linux -B build/gtk4
cmake --build build/gtk4 --target xiphos-linux-preview --parallel 1
cpack --config build/gtk4/CPackConfig.cmake -G RPM -B build/gtk4/packages
```

The unsigned preview RPM contains the desktop entry, bundled map and locale
assets, and license notices. RPM derives library dependencies from the built
executable and also requires the system CA certificates. Profiles, installed
map packs and SWORD modules remain in their user-owned locations. For a local
upgrade test, set a higher package release with
`cpack -D CPACK_RPM_PACKAGE_RELEASE=2` along with the same arguments above.

[Linux handoff](../../doc/modernization/handoff-linux.md) ·
[Status](../../doc/modernization/status.md) ·
[Historical validation/advanced setup](../../doc/modernization/history/linux-readme.md)
