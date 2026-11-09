# Current development status

Updated 2026-11-09. This page supersedes earlier progress estimates and checkpoint summaries.

The project is a shared C++/SWORD core with native frontend development. GTK3
application/editor/backend code and its old packaging are retired. Frozen behavior
references remain in core tests. Windows WinUI 3 is planned, not implemented.

## Implemented scope

- Core reading/navigation, commentary, dictionaries, books and bounded assets.
- Search across supported module families, exports and cancellable operations.
- Parallel chapter matrices and saved sets; bookmarks and personal annotations.
- Rich document/editable-module workflows, library/repository management and HTTPS installation.
- Saved sessions, native Mac tabs/detaching, migration, autosave and recovery.
- BibleSync passage sharing; personal-file/data sync is future work.
- Native Mac Settings and startup preferences; English and partial French/German/Spanish resources.
- Localized book-selector labels retain canonical targets; RTL module direction reaches reading views.

These are scoped implementations, not complete functionality or release sign-off.

## Latest verification

The macOS 27.2 Release build at the new 27.0 deployment floor passed **259 of 260
checks**, with the optional BibleSync network pair skipped and zero failures.
See the [Mac handoff](handoff-macos.md) for deployment, launch and bundling checks.
Earlier focused sanitizer checks passed 4/4. A separate
capable SWORD build proved real indexed verse/dictionary/book searches. See
[localized reading evidence](localized-reading-verification.md),
[corpus manifest](real-corpus-validation.md) and [runtime packaging limits](native-runtime-packaging.md).
No fresh Windows or physical Linux acceptance is implied by Mac-hosted checks.

## Remaining gates

- Native Settings/navigation redesign, selected-word Hebrew search and stacked verse comparisons.
- Complete translations, typed localized-reference input, broader RTL and accessibility acceptance.
- Broader module/canon/profile coverage, remaining reading/export/editor workflows and data compatibility.
- Windows frontend/native validation, current physical Linux verification and macOS 27.0 deployment acceptance.
- Dependency resources/notices, signing/notarization, installers and upgrade validation.

There is no current, auditable completion percentage. Earlier weighted estimates
and test counts are historical and must not be quoted as today's progress.

[Priorities](shared-core-roadmap.md) · [Feature requirements](parity-matrix.md) ·
[Historical status](history/status.md)
