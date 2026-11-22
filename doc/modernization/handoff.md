# Shared project handoff

Updated 2026-11-21. Read [AGENTS.md](../../AGENTS.md), [status](status.md),
[architecture](architecture.md), [roadmap](shared-core-roadmap.md),
[parity requirements](parity-matrix.md) and your platform handoff before editing.

## Repository and ownership

Work on `orbeavers14/xiphos`, branch `master`. Fetch before choosing work;
preserve concurrent changes and never force-push. Sequential checked commits and
pushes are owner-authorized. Keep routine builds local; do not enable hosted CI.
Preserve unrelated files, user data, tests, licenses and translator credits.

One SWORD-owning FIFO worker per process; copied results across C/C++/native
boundaries. Shared behavior belongs in src/core. Native code presents/adapts it.
C++17 core, C++11-compatible retained policy helpers, C11 ABI, Swift6.

## Current repository

- src/core: engine/services/C API/tests; frozen independent legacy reference snippets.
- platforms: Mac preview, Linux GTK4 frontend, Windows core tooling and WinUI 3 preview.
- localization: gettext input, reviewed overrides, generated native resources and SWORD book-name catalogs.
- dependencies: SDK requirements and maintained patches; doc/legacy-notices and AUTHORS/COPYING retain credits.
- help/pixmaps: preserved reference material; not evidence of native help integration.

GTK3, top-level win32/desktop/poc and old packaging/build metadata are removed.
Do not restore retired application code to apply an upstream renderer patch.

## Latest checked work

The [2026 native baseline](2026-native-baseline.md) governs maintenance cleanup.
The Mac preview now uses macOS 27.0 and CMake 3.20 minimums. See the
[Linux handoff](handoff-linux.md#maintenance-baseline-on-master) for the local
checks needed before removing shared toolchain fallbacks.

The current snapshot includes Mac selected-word study, stacked comparisons,
Settings/appearance, offline maps and reading styles; Fedora 45 ARM64 GTK4
reading/editing, maps and preview RPM upgrade checks; and the Windows 11 25H2
ARM64 WinUI reading/study preview through general-book links, images and explicit
saved-book selection import. See each platform handoff for dated evidence and limits.

Recorded suites: Mac standalone core 184 passes/one optional skip; Mac native
259 passes/one optional skip; Fedora 203 passes/one optional skip; Windows core
183 passes across a full run and targeted timeout rerun. These are distinct
runs and configurations, not a fresh combined acceptance run. Windows native
sessions/tabs and further management UI remain next work.

English and eight partial native catalogs are generated under localization/.
Private SWORD book labels and localized input have platform-specific coverage.
[Owner feature decisions](feature-review-decisions.md) distinguish retained,
refine and deferred initial-release scope. No study content may enter telemetry.

## Next work and handoff

Use the [roadmap](shared-core-roadmap.md) and [native UI list](native-ui-todo.md).
Each task should record actual checks,
platform/SDK limits and remaining work. Passing tests do not establish whole parity.

[Mac](handoff-macos.md) · [Linux](handoff-linux.md) · [Windows](handoff-windows.md) ·
[Historical integration record](history/handoff.md)
