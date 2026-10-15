# Linux shared-core automation

Run from a checkout on Debian/Ubuntu or Fedora:

```sh
platforms/linux/core.sh --install-deps
platforms/linux/core.sh --sanitize
```

The first command installs the compiler, CMake, Ninja, pkg-config, SWORD,
GLib and libxml2 development packages using sudo when necessary. Installation
requires administrator privileges and network access. Omit `--install-deps`
when the dependencies are already installed. No GTK packages are needed.

Each command configures, builds and runs the complete standalone CTest suite,
including generated module fixtures and legacy adapter characterization.
Failures stop the command with a nonzero status. Tests have a 120-second timeout;
an empty test suite fails. The sanitizer build enables AddressSanitizer and
UndefinedBehaviorSanitizer; leak detection defaults off for uninstrumented
system dependencies, matching the existing native-core workflow.

Release and sanitizer builds use separate ignored directories under `build/`.
Use `--jobs N` to control memory/CPU use (default 2) and `--build-dir DIR` to
choose a separate output directory. Relative directories resolve from the
calling directory. Use a different directory when changing compiler or mode.
CMake honors `CC`, `CXX`, and standard dependency discovery variables.
Generated modules never require an installed Bible corpus or user profile.

The `linux-core` GitHub Actions workflow is manual-only to conserve hosted build
allowance. Pushes and pull requests do not start it. An authorized manual dispatch
runs both modes on Ubuntu 22.04 and 24.04 (four jobs).
It preserves CTest diagnostics for seven days, including failed runs.
It is independent of the existing paused native-core/macOS and legacy workflows.
Repository administrators must allow GitHub Actions for manual runs. Do not
require these checks on every change while routine acceptance stays local.

This validates the shared core's existing contracts. It does not establish full
feature parity or complete the shared engine. See the modernization status and
parity ledger before moving to GTK4 frontend implementation.
