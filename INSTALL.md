# Building the new application

The old GTK3 application, GTK editors and MinGW/CPack distribution have been
retired. Old installation instructions remain in Git history. They do not apply
to the shared-core/native application.

## Shared core

Use CMake3.16 or later, a C++17 compiler and the dependency versions documented
in dependencies. Configure SDK locations when they are not discoverable through
pkg-config/CMake; GLib is still required for shared Unicode behavior.

```sh
cmake -S . -B build/core
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

BUILD_TESTING=OFF omits characterization and regression targets. Frozen legacy
reference snippets are test-only; no GTK headers or GTK application build is
needed. The SDK must match the host architecture and supported OS version.

## Native hosts

- [Mac build and preview](platforms/macos/README.md)
- [Linux GTK4 build and preview](platforms/linux/README.md)
- [Windows core validation](platforms/windows/README.md); WinUI3 remains planned.
- [Dependency inventory](dependencies/README.md)
- [Runtime packaging limits](doc/modernization/native-runtime-packaging.md)

The native applications still have open feature, localization and release gates.
Removing the old implementation does not establish replacement feature parity.
See [status](doc/modernization/status.md) and [removal checklist](doc/modernization/legacy-removal.md).
