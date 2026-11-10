# Mac development handoff

Updated 2026-11-09. Start with [shared instructions](handoff.md) and
[Mac build/setup](../../platforms/macos/README.md).

## Checked environment and behavior

The preview now targets macOS 27.0 or later. The current development host runs
macOS 27.2 with AppleClang 21 and Swift 6.4.2.
See [packaging audit](native-runtime-packaging.md).

Previous native suite:258 passes/2 expected indexing skips out of260,149.12s;
focused sanitizer4/4,14.66s. Exact logs, live checks, provenance and scope:
[localized reading](localized-reading-verification.md), [Settings/tabs](native-settings-workflows.md),
[real corpus](real-corpus-validation.md), [asset migration](legacy-assets-migration.md).
The native Settings screen is functional intermediate UI. Partial languages and
private SWORD book-label catalogs do not imply full translation or localized typing.

## macOS deployment baseline

On `master`, the deployment-floor change following `8ce6f7e` covers the
preview build, its Swift tests, the bundle plist and disposable fixture launch.
Graphify's scoped structural scan covered 560 Mac and core files. The audit graph
shows the plist and Mach-O metadata checks, and the Swift adapter graph shows the
shared session boundary. CMake inspection confirmed that the same deployment
target feeds native compilation, Swift compilation and the bundle plist.

Local Release build on arm64 macOS 27.2 used CMake 4.4.5, Xcode SDK 27.2,
MacPorts SWORD 1.9.0 and curl 8.23.0, Homebrew GLib 2.90.1 and a workspace-built
libarchive 3.8.6. The local libarchive dylib needed an absolute install name
because the Swift link did not supply its `@rpath`. CMake rejected 13.0, 26.6 and
an empty target. The default plist and Mach-O both declared 27.0, and a separate
27.2 configure propagated the override to the plist. The fixture app started
and initialized its disposable profile. The development bundle passed strict
ad-hoc signature verification and all 29 Mach-O image checks at a declared and
observed floor of 27.0. CTest passed 259 of 260 checks in 66.10 seconds,
with the optional BibleSync network pair skipped and zero failures.

## Compatibility cleanup

The Mac preview requires CMake 3.20 because its fixture targets use
`ctest --test-dir`. Six reader and settings callbacks use current SwiftUI
`onChange` overloads. A local rebuild with CMake 4.4.5 and SDK 27.2 produced no
`onChange(of:perform:)` warnings. The app still declares macOS 27.0. CTest passed
259 checks with zero failures and one optional BibleSync network skip.
See the [Linux handoff](handoff-linux.md#maintenance-baseline-on-master) for the
shared toolchain validation that remains.

## Current next steps

- Follow the [native interface list](native-ui-todo.md): coherent Settings navigation,
  appearance/themes, stacked comparisons and selected-word Hebrew, Greek and Aramaic study.
- Complete broader workflow/VoiceOver/RTL/localization and compatibility acceptance.
- Validate execution on macOS 27.0 and release resources/signing before distribution.
- Evaluate macOS 26.7 as a lower floor after rebuilding or replacing the
  dependencies that require 27.0. Audit the full bundle and launch it on 26.7.

Use disposable profiles/module fixtures for GUI checks. Give each test bundle a
unique identifier and verify the target process: copies sharing a bundle identifier
can cause UI tools to select/reopen the wrong profile. Do not edit a normal user
profile while verifying a disposable one.

[Historical Mac evidence](history/handoff-macos.md) retains earlier logs and
implementation commits; old GTK preservation statements and completion estimates
are superseded by current guidance. Do not start new feature work without a task.

## Documentation refresh

2026-11-05: README and primary status/architecture/roadmap/handoffs now summarize
current native development. Detailed old evidence moved to history with relative
links/anchors preserved. All38 parity requirements remain; affected Mac evidence
is updated. A documentation index, preview guide and explicit historical-help
boundary replace the old GTK release framing.95 Markdown files pass local-target
and heading-anchor checks; no application code changed and no new runtime/test
acceptance is claimed. GitHub About is aligned with the native development fork.
