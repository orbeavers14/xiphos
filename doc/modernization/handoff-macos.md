# Mac development handoff

Updated 2026-11-05. Start with [shared instructions](handoff.md) and
[Mac build/setup](../../platforms/macos/README.md).

## Checked environment and behavior

Most recent inspected environment: Mac27.2, AppleClang21, Swift6.4.2. The project
baseline is macOS13, but installed SDK libraries require a newer OS; it is not
validated macOS13 support. Current development runtime closure requires macOS27.
See [packaging audit](native-runtime-packaging.md).

Full native suite:258 passes/2 expected indexing skips out of260,149.12s;
focused sanitizer4/4,14.66s. Exact logs, live checks, provenance and scope:
[localized reading](localized-reading-verification.md), [Settings/tabs](native-settings-workflows.md),
[real corpus](real-corpus-validation.md), [asset migration](legacy-assets-migration.md).
The native Settings screen is functional intermediate UI. Partial languages and
private SWORD book-label catalogs do not imply full translation or localized typing.

## Current next steps

- Follow the [native interface list](native-ui-todo.md): coherent Settings navigation,
  appearance/themes, stacked comparisons and selected-word Hebrew study.
- Complete broader workflow/VoiceOver/RTL/localization and compatibility acceptance.
- Validate older-OS SDKs and release resources/signing before distribution.

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
