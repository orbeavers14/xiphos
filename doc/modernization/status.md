# Current development status

Updated 2026-11-17. This page supersedes earlier progress estimates and checkpoint summaries.

The project is a shared C++/SWORD core with native frontend development. GTK3
application/editor/backend code and its old packaging are retired. Frozen behavior
references remain in core tests.

## Implemented scope

- Core reading/navigation, commentary, dictionaries, books and bounded assets.
- Search across supported module families, exports and cancellable operations.
- Parallel chapter matrices and saved sets; bookmarks and personal annotations.
- Rich document/editable-module workflows, library/repository management and HTTPS installation.
- Saved sessions, native Mac tabs/detaching, migration, autosave and recovery.
- BibleSync passage sharing; personal-file/data sync is future work.
- Native Mac Settings and startup preferences; English and partial French/German/Spanish resources.
- Localized book-selector labels retain canonical targets; RTL module direction reaches reading views.
- WinUI 3 reading preview with a SWORD library root, Bible and commentary selection, passage entry, chapter navigation and history.
- WinUI Bible and commentary search with passage bounds, last-result refinement, separate preview and activation, progress, Stop, and text/HTML export.

These are scoped implementations, not complete functionality or release sign-off.

## Latest verification

The latest standalone core suite passed **184 of 185 checks** on macOS 27.2,
with one optional BibleSync network skip and zero failures. The render and policy
targets also built with native core disabled and C++11 selected explicitly.
The [real corpus](real-corpus-validation.md) verified word metadata, literal
and mark-insensitive surface search, mapped and split comparisons, and plain
text and passive HTML export from Hebrew, Greek and Syriac modules. The Mac
preview has separate focused checks in the [Mac handoff](handoff-macos.md).
These Mac runs do not establish native Linux or Windows acceptance.

The Windows 11 25H2 ARM64 core passed all 183 checks across a full MSVC Release
run and a targeted rerun of three timeouts. The SWORD SDK included ICU and
CLucene, and actual index checks were required. See the
[Windows handoff](handoff-windows.md) for the SDK configuration, test results
and WinUI 3 verification. The ARM64 reading preview built and displayed a
generated Unicode Bible passage in WebView2.

## Remaining gates

- Native Settings/navigation redesign and Windows integration for selected-word study and stacked comparisons. GTK4 has both workflows with generated-fixture desktop checks. See the [Linux handoff](handoff-linux.md#gtk4-reading-and-editing-adapters-2026-11-11) for scope and remaining Linux features.
- Complete translations, native localized multi-book input, broader RTL and accessibility acceptance.
- Broader module/canon/profile coverage, remaining reading/export/editor workflows and data compatibility.
- Windows desktop commentary and accessibility checks, native x64 and runtime deployment validation, current physical Linux verification and macOS 27.0 deployment acceptance.
- Dependency resources/notices, signing/notarization, installers and upgrade validation.

There is no current, auditable completion percentage. Earlier weighted estimates
and test counts are historical and must not be quoted as today's progress.

[Priorities](shared-core-roadmap.md) · [Feature requirements](parity-matrix.md) ·
[Historical status](history/status.md)
