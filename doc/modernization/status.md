# Modernization status and acceptance

Updated 2026-10-10. Goal: preserve Xiphos functionality, visual composition,
data and workflows through one shared C++ application core and separate
SwiftUI/AppKit, WinUI 3 and GTK4 frontends. Native enhancements supplement
that contract. The [parity matrix](parity-matrix.md) defines acceptance evidence;
[architecture.md](architecture.md) describes boundaries and dependencies.

## Verified implementation, pending acceptance

Evidence below applies to implementation commit
`04f9787da3c66f3d34db2d506ba34e857d31f735` in
[draft PR #1](https://github.com/orbeavers14/xiphos/pull/1). Later commits must
be checked independently. Checkboxes here record the scoped evidence stated,
not full native parity or permission to delete its tests.

- [x] Source/build/dependency/license inventory and 38-group parity ledger.
- [x] Shared rendering seam used by the existing backend and new reader.
- [x] Read-only module/verse C API with ownership and error checks.
- [x] macOS preview and thin Swift adapter compiled and tested.
- [x] Visible macOS reading flow checked with two generated fixture verses.
- [x] Linux core and macOS reader CI passed on the implementation commit.
- [x] Legacy Arch, Fedora, Ubuntu and Debian PR builds passed.
- [ ] Legacy Win32/Win64 cross-builds: still running at this status snapshot.
- [ ] Initial PR review/sign-off and merge: PR remains a draft.
- [ ] Representative module corpus and runtime/visual workflow validation.

CI evidence: [native-core PR run](https://github.com/orbeavers14/xiphos/actions/runs/38079690029)
and [legacy application PR run](https://github.com/orbeavers14/xiphos/actions/runs/38079689911).
Build success checks compatibility at build time; it does not establish runtime
or design parity. No parity row is accepted on all native platforms.

## Remaining milestones

- [ ] Shared per-tab navigation/history and chapter rendering/options.
- [ ] Equivalent reading layout and interactions on GTK4 and WinUI 3.
- [ ] Search, parallel sets, semantic links and preview behavior.
- [ ] Compatible settings, sessions, bookmarks/tags/imports, notes and editors.
- [ ] Module installation/maintenance and BibleSync.
- [ ] Accessibility, localization, keyboard workflows and platform integration.
- [ ] Packaging/signing, license notices, upgrades and complete parity review.

The current macOS preview has a module sidebar and a single-verse pane. It
is not yet the established full study layout. Windows and GTK4 native
frontends are not implemented. SwordKit integration/license review is pending.
The GTK app remains the complete implementation; no legacy feature is retired.

## Local validation evidence

Local checks on 2026-10-10, Apple Silicon, AppleClang 21, installed SWORD
1.9.0.3916 (static SDK):

* Standalone Debug core: engine/C ABI test passed.
* macOS preview: app and Swift adapter compiled; CTest 2/2 passed.
* Address + undefined behavior sanitizer build: core test passed, with leak
  detection disabled because the externally built SWORD library is not
  instrumented and has process-global lifetime state. This is not a leak audit.
* Explicit include/library SDK discovery: core build/test passed (macOS only).
* Generated dictionary fixture: discoverable but rejected as unsupported by
  the verse reader. Empty roots, missing module and empty reference tested.
* Visible app check: selected ParityTest; John 3:16 displayed
  “Original fixture: café & peace.”; changing reference to John 3:17 and
  clicking Read displayed “Second fixture verse.” in WKWebView. Confirmed
  both screenshot and accessibility text. This is not visual parity testing.
* Source inventory: 387 UI labels and 101 manual section titles extracted.

The local SDK requires macOS 26.0, so the preview was tested with
`CMAKE_OSX_DEPLOYMENT_TARGET=26.0` and explicit zlib/bzip2/lzma libraries.
The project defaults to 13.0; compatibility on that OS requires an appropriate
SDK and separate runtime verification.

GTK3/WebKitGTK are absent locally, so the legacy desktop app has not been
launched on this Mac. Full native workflows, MSVC/WinUI builds, older macOS
runtime compatibility, storage and distribution packaging remain unvalidated.

## Official sign-off and file retirement

A scoped milestone is accepted when its specified implementation, regression
checks, relevant CI and parity evidence have been reviewed and its PR merged.
Record the accepted commit/PR and evidence here, and update only the parity
rows/platforms that were actually validated. A passing test alone does not
make every file associated with a milestone obsolete.

After acceptance, remove disposable plans, superseded notes and generated
reports once useful decisions/evidence are captured in this status page or
architecture. Generated build/test fixtures and inventories stay under ignored
`build/`; keep the code that regenerates them. This cleanup consolidates the
initial validation note here and removes tracked generated inventories.

Retire legacy source, assets, build rules or dependencies only when:

1. The replacement covers each affected parity requirement on the platforms
   still using that code, including data compatibility and workflow checks.
2. No supported caller, build, test, documentation or packaging path needs it.
3. The cleanup removes stale references and relevant checks pass afterward.
4. The removal and acceptance evidence are recorded in a reviewed PR.

Keep regression tests after features are checked off. Keep current parity
requirements, architecture, supported user documentation and required license/
attribution files. Git history and linked PRs preserve removed implementation
and detailed review history; avoid a growing archive of duplicate work notes.
