# Legacy removal checklist

Owner direction, 2026-11-04: remove as much old code and distribution machinery
as possible. This supersedes blanket preservation of the old distribution;
feature/data compatibility and regression evidence remain requirements.

## Removed in the first pass

- [x] Vendored Windows Festival executable, voices, dictionaries and runtime.
  Original three COPYING files are retained under doc/legacy-notices. The old
  Windows packaging no longer installs this runtime. This retires bundled
  Festival; it does not add speech support to the new frontends.
- [x] Fedora30 Windows Docker/MinGW automation and its obsolete instructions.
- [x] Legacy GitHub `builds` workflow which invoked that Docker pipeline, and
  obsolete `.ci-build.yml` GTK distribution recipe.
- [x] Default root configuration of GTK3, legacy editors and distribution tools.
  Root CMake now builds the shared core. The retained reference application
  requires explicit XIPHOS_BUILD_LEGACY_GTK3=ON. Native frontend entry points
  remain under platforms. This is not a native Windows acceptance claim.

## Next removal batches

- [ ] Extract bounded legacy characterization inputs from src/main and src/gtk
  into provenance-preserving reference fixtures. Direct core CMake references:
  src/backend/sword_main.cc; src/gtk/bookmarks_treeview.c, export_bookmarks.c,
  preferences_dialog.c, tabbed_browser.c, utilities.c; src/main/display.cc,
  global_ops.cc, module_dialogs.cc, parallel_view.cc, search_dialog.cc, settings.c,
  sword.cc, tab_history.c, url.cc, xml.c. Check transitive includes as well as
  these16 paths before removing full source files; keep equivalence checks.
- [ ] Replace remaining direct legacy helper compilation/includes in core tests.
- [ ] Remove GTK3 runtime/editor/backend sources and build rules once the above
  dependency graph is clear. Track missing workflows in parity matrix rather
  than silently treating deleted implementation as completed replacement.
- [ ] Remove remaining GTK3-specific Windows/CPack/Linux packaging after reference
  build retirement; keep new WinUI3 and GTK4 frontend projects.
- [ ] Classify ui/pixmaps/desktop/help assets and translation catalogs; preserve
  reusable art, user guidance, translator credit and source catalogs for native
  localization. Native Settings/localization are still incomplete.
- [ ] Remove obsolete documentation/examples after checking runtime/build callers.

Do not rewrite Git history: removed content remains in historical commits.
Deleting tracked binaries reduces checkout size, not existing clone history.
Unrelated local files and Graphify outputs are not cleanup targets.

## Validation of the first removal batch

2026-11-04, Mac27.2/AppleClang21: fresh repository-root configuration with
GTK3 OFF/native core ON builds successfully using released SWORD1.9.0 and the
existing local GLib/dependency SDK paths. Full root suite:183 passed,1 sync skip
(sync disabled in this configuration),1 environment-blocked loopback download
check/185,36.82s. That download check passes separately with loopback access,
1/1,1.97s. Logs: build/retirement-{configure,build,tests,remote-tests}.log.
This combines to184 passing checks and1 expected skip; it is not a native UI,
physical Linux/Windows or legacy GTK3 build validation. No hosted jobs run.

107 tracked files removed,22525448 bytes before retained notices/edits. Existing
Git history is unchanged. Diff and changed-document local-link checks pass.
Graphify's smaller-graph guard initially rejected the refresh; inspection showed
its missing source paths were exactly removed build/Festival scripts, then a
full local deterministic refresh was explicitly allowed. No paid services used.
