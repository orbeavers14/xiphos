# Shared project handoff

Updated 2026-11-09. Read [AGENTS.md](../../AGENTS.md), [status](status.md),
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
- platforms: Mac preview, Linux GTK4 frontend, Windows core tooling/planned WinUI3.
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

[Localized reading](localized-reading-verification.md) records258 native passes
and2 expected skips, RTL/private selector locales, note-save routing and mixed-canon
annotation handling. [Settings/session contract](native-settings-workflows.md)
records saved defaults, native tabs and restoration. Official upstream905762a
was cherry-picked asad93feb; all41 changed files matched upstream at adapted paths.
Chinese catalogs/help are imported; native Chinese activation remains future work.

## Next work and handoff

Use the [roadmap](shared-core-roadmap.md) and [native UI list](native-ui-todo.md).
The owner requested a stop after the current checked batch; do not infer a new
feature task from this handoff. Each future task should record actual checks,
platform/SDK limits and remaining work. Passing tests do not establish whole parity.

[Mac](handoff-macos.md) · [Linux](handoff-linux.md) · [Windows](handoff-windows.md) ·
[Historical integration record](history/handoff.md)
