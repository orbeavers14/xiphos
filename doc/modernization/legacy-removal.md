# Legacy removal checklist

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


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

## Source retirement pass

- [x] Captured61 generated characterization snippets and original history header
  as immutable test inputs under src/core/tests/legacy-reference. Manifest records
  fixture hashes, original16 source-file hashes and exact pre-retirement commit.
  Existing tests, fixture setup and expected behaviors remain; CMake no longer
  extracts source blocks. These are reference oracles, not an archived application.
- [x] Removed src/backend, editor, examples, gtk, gui, main, webkit, xiphos_html
  and the GTKBuilder ui tree.
- [x] Removed all remaining top-level win32, cmake and cpack files and obsolete
  resource/install CMake files. Root build is core-only; GTK3 opt-in is retired.
- [x] Preserved Windows Persian GTK catalog under po/legacy-gtk, Debian package
  copyright under doc/legacy-notices; retained project licenses/translations/art/help.

## Remaining cleanup/replacement work

- [ ] Classify remaining pixmaps/desktop/help content; keep reusable resources
  and user guidance, then remove unused legacy-only assets and stale manuals.
- [ ] Native Settings/localization, missing workflows, complete frontend and
  physical-platform/release acceptance remain open in the parity matrix.
- [ ] Reconcile historical source-path evidence links as documentation is touched;
  references to retired code mean the Git version named by that evidence.

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

## Source-retirement verification

2026-11-04: fresh build/retired-clean configures and compiles without any old
application directories. Full suite:184 passed/1 expected sync-disabled skip out
of185,27.08s; loopback download test included. An initial fresh-run failure found
an accidentally removed parallel test config writer; restored before final run.
All61 checked-in snippet bytes match fresh-build copies and manifest SHA256s.
Mac native macos_preview target builds against the retained primary SDK with no
legacy source tree (build/retired-macos-build.log). No visual behavior or native
Windows/physical Linux acceptance claimed. Diff and current entry-document link
checks pass. Graphify refreshed locally after deletion; zero paid AI services.

This pass removes302 tracked paths (35094811 bytes before replacement test
fixtures/notices), including the entire top-level win32 tree. The immutable
reference fixture directory is about288KiB. Existing Git history is unchanged.

## Root metadata cleanup

2026-11-04: removed .bzrignore/.bzrrules (retired Bazaar/Waf metadata),
ChangeLog/RELEASE-NOTES (old GTK release history, preserved in Git), TODO
(old upstream links; current work uses this checklist and shared-core roadmap),
and xiphos.doap.rdf (old upstream project metadata). Repository-wide tracked
reference checks found no live build/runtime consumers. Removed obsolete ignore
entries for deleted Waf/Windows/GTK build paths. Kept .clang-format,
.editorconfig, .gitattributes and .gitignore as active development configuration;
kept AUTHORS/COPYING as credits/license, and current build/installation/readme
and agent instructions. Diff checks pass; no application code or tests changed.

## Legacy desktop metadata and standalone reproducer

2026-11-04: removed desktop/ (unused GTK launcher/AppStream metadata and GTK/
xiphos-nav man templates; duplicate SVG exactly matches retained pixmaps/xiphos.svg)
and poc/ (standalone denylist-bypass demonstration for deleted GTK D-Bus handlers).
Tracked native build/runtime references contain no consumers of either folder.
This removes an old demonstration, not any active core regression test or link
admission validation. History retains the original vulnerability evidence.
Reusable art/help/translations, dependency patches, platform/core code and
GitHub configuration remain. No application behavior changed; diff check passes.
