# Current development status

Updated 2026-11-21. This page supersedes earlier progress estimates and checkpoint summaries.

The project is a shared C++/SWORD core with native frontend development. GTK3
application/editor/backend code and its old packaging are retired. Frozen behavior
references remain in core tests.

## Implemented scope

- Core reading/navigation, commentary, dictionaries, books and bounded assets.
- Search across supported module families, exports and cancellable operations.
- Parallel chapter matrices, stacked verse comparisons, saved sets and visibility; bookmarks and personal annotations.
- Source-bound selected-word study with literal, mark-insensitive and metadata-backed lemma choices.
- Shared offline map resources and bounded pack validation; Mac and Linux native pack management.
- Rich document/editable-module workflows, library/repository management and HTTPS installation.
- Saved sessions, native Mac tabs/detaching, migration, autosave and recovery.
- BibleSync passage sharing; personal-file/data sync is future work.
- Native Mac Settings and startup preferences; English and eight partial interface catalogs: French, German, Spanish, Italian, Brazilian Portuguese, Simplified Chinese, Arabic and Hebrew.
- Localized book-selector labels retain canonical targets; RTL module direction reaches reading views.
- WinUI 3 reading preview with a SWORD library root, Bible and commentary selection, passage entry, chapter navigation and history.
- WinUI Bible and commentary search with passage bounds, last-result refinement, separate preview and activation, progress, Stop, and text/HTML export.
- WinUI parallel reading with ordered Bible selection, an aligned chapter table, a stacked verse view, named sets and saved translation visibility.
- WinUI selected-word study with source-preserving surface and lemma searches and activation in the default comparison.
- WinUI dictionary and lexicon lookup with nearby keys and independent history.
- WinUI daily devotional module selection and calendar/day navigation.
- WinUI note/reference-list previews and explicit per-module engine filters with shared settings import/save.
- WinUI indexed-tree book browsing with paged contents, exact paths, shared document navigation and explicit saved-selection import.

These are scoped implementations, not complete functionality or release sign-off.

## Latest verification

The latest standalone core suite passed **184 of 185 checks** on macOS 27.2,
with one optional BibleSync network skip and zero failures. The render and policy
targets also built with native core disabled and C++11 selected explicitly.
The [real corpus](real-corpus-validation.md) verified word metadata, literal
and mark-insensitive surface search, mapped and split comparisons, and plain
text and passive HTML export from Hebrew, Greek and Syriac modules. The Mac
preview has separate focused checks in the [Mac handoff](handoff-macos.md).
The native Mac suite also records 259 passes and one optional skip in 260 checks;
later focused checks cover appearance, maps, comparison visibility and word study.
These are separate dated runs, not one aggregate result or a fresh November 21 run.

The Windows 11 25H2 ARM64 core passed all 183 checks across a full MSVC Release
run and a targeted rerun of three timeouts. The SWORD SDK included ICU and
CLucene, and actual index checks were required. See the
[Windows handoff](handoff-windows.md) for the SDK configuration, test results
and WinUI 3 verification. The ARM64 reading preview built and displayed a
generated Unicode Bible passage in WebView2.

Fedora 45 ARM64 records **203 passes and one optional BibleSync skip**, GTK
workflow checks, offline raster/vector maps, desktop link delivery and a preview
RPM install/upgrade preserving the isolated profile and map store. See the
[Linux handoff](handoff-linux.md). Other distributions and physical accessibility
checks remain separate gates. All results above are recorded development evidence,
not tests rerun for this documentation refresh.

## Remaining gates

- Native Settings/navigation redesign and broader Windows, Linux and macOS original-language and comparison workflow checks. See the [native UI list](native-ui-todo.md).
- Complete translations, native localized multi-book input, broader RTL and accessibility acceptance.
- Broader module/canon/profile coverage, remaining reading/export/editor workflows and data compatibility.
- Windows native session/tab ownership, remaining management/editor workflows, export completion, Narrator and native x64/runtime deployment validation.
- Linux broader distribution/corpus/accessibility checks and macOS 27.0 execution acceptance; a measured bundle deployment floor alone is insufficient.
- Dependency resources/notices, signing/notarization, installers and upgrade validation.

Initial-release scope follows the [owner decisions](feature-review-decisions.md):
AndBible import, BibleSync-related workflows and external sword:// opening are
deferred gates, with existing implementations and data safety preserved. Personal
data sync, Handoff and mobile remain future work.

There is no current, auditable completion percentage. Earlier weighted estimates
and test counts are historical and must not be quoted as today's progress.

[Priorities](shared-core-roadmap.md) · [Feature requirements](parity-matrix.md) ·
[Historical status](history/status.md)
