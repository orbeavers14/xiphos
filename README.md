# Xiphos native modernization

This fork of [CrossWire Xiphos](https://github.com/crosswire/xiphos) is building
a true 1:1 functional and design adaptation with a shared, platform-independent
C++ application core and native frontends:

| Platform | Target frontend | Current implementation |
|---|---|---|
| macOS | SwiftUI / AppKit | Read-only module and single-verse preview |
| Windows | WinUI 3 | Planned; legacy GTK application retained |
| Linux | GTK4 | Planned; legacy GTK3 application retained |

The goal is to preserve all existing Bible-study functionality, pane layout,
module behavior, data and established workflows while adding appropriate
native menus, accessibility, keyboard controls, appearance and platform
integration. Shared application behavior belongs in C++; platform adapters
must not develop independent engines. SwordKit may become a Swift adapter
through an audited adapter to the shared core.

## Status

The first slice adds installed-module discovery, verse rendering, an
experimental C API and a macOS reading window. The legacy rendering call uses
the same shared function. Local engine, adapter, sanitizer and visible reading
checks passed. The new Linux/macOS CI and legacy Linux PR builds passed at the
recorded implementation commit. Windows cross-builds were still running when
checked. [PR #1](https://github.com/orbeavers14/xiphos/pull/1) remains a draft.

Overall modernization is estimated at **about 5%** (planning range 3–7%).
**0/38 feature groups** have accepted parity across all native platforms.
See the status page for the weighted estimate, ordered next steps and
[source reuse decisions](doc/modernization/status.md#reuse-from-related-projects).
No native platform has full functional, visual or workflow parity yet.
The existing GTK application remains the complete implementation during the
migration. See the [current status and sign-off rules](doc/modernization/status.md),
[feature parity matrix](doc/modernization/parity-matrix.md), and
[architecture and dependency/license audit](doc/modernization/architecture.md).

## Build and development

* Existing GTK application: [INSTALL.md](INSTALL.md).
* Shared core and engine tests: [src/core/README.md](src/core/README.md).
* Native macOS preview and adapter tests: [platforms/macos/README.md](platforms/macos/README.md).
* Translations: documentation in [doc](doc).

The next implementation milestone is shared study-session navigation and
chapter rendering, followed by equivalent reading panes on GTK4 and WinUI 3.
Each milestone needs reviewable changes, regression tests and parity evidence
before the corresponding legacy files can be retired.

Use the repository's `.clang-format` when editing C/C++ source. The upstream
user manual and screenshots remain reference material until replacement
behavior and documentation are validated.

## Repository cleanup

Generated inventories and build outputs belong under the ignored `build/`
directory. Keep their generators, regression tests, current architecture,
parity requirements and compact acceptance evidence. After an accepted
replacement has no remaining legacy callers, remove obsolete code and
associated build/dependency references in the same reviewed cleanup change.
Git history and PRs retain the detailed implementation record.

## License and acknowledgments

Preserve [COPYING](COPYING), [AUTHORS](AUTHORS) and existing source notices.
Inspected Xiphos source grants GPL v2 or later; dependency and module licenses
have their own requirements. See the dependency/license audit before packaging.

Xiphos started as the SWORD Project's Cheatah program. Its development includes
ideas and code from BibleTime and the SWORD Project's BibleCS program. Credit
remains with the Xiphos Development Team, CrossWire, and all contributors whose
work this fork builds upon.
