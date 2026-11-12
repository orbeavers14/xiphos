# Linux development handoff

Updated 2026-11-11. Read [shared instructions](handoff.md) and
[Linux setup/build commands](../../platforms/linux/README.md).

The active frontend is GTK4/WebKitGTK6, backed by the shared worker/C API.
GTK3 application code is retired. Existing Linux reader/model work is present;
read the current source before treating a historical gap as unimplemented.

Earlier native Fedora results and validation limits remain in the
[historical Linux handoff](history/handoff-linux.md). Later portable-model checks
run on a Mac are not native Linux desktop/input/clipboard or package acceptance.


## GTK4 offline map packs, desktop links and BibleSync (2026-11-11)

Master from `b4b34204`. The GTK4 map uses the shared offline map page.
Local packs and checksum-verified HTTPS downloads stage in private storage.
The page checks PMTiles metadata before the app replaces the installed pack.
The CMake install includes relocatable map and locale assets plus a desktop
entry for `sword://` links.

On Fedora 45 ARM64 with GTK4 and WebKitGTK6, the staged desktop entry opened
an external verse at startup and routed a second verse into the same instance
in an isolated D-Bus session. A GTK/WebKit map fixture rendered a staged
raster pack, checked that a corrupt replacement preserved it, and removed
the installed copy. The GTK study smoke also passed. A Linux vector extract and the map download dialog still
need desktop verification.

With the reviewed BibleSync 2.2.0 SDK built against the local UUID runtime,
the GTK panel connected to an isolated loopback peer, opened an incoming
review item, shared a verse and disconnected. Both shared SDK transport tests
passed. Automatic outgoing navigation, physical LAN behavior and
distribution-package acceptance remain open. The [owner’s feature review](feature-review-decisions.md)
defers BibleSync and external links from the initial release.

## GTK4 window, startup and sharing workflows (2026-11-11)

Master from `1dfa636d`. Detach opens Bible or commentary in a secondary
window. Synchronized panes follow the main passage and can navigate it.
Independent panes read through the same FIFO worker and leave main selection
and history intact. A `sword://MODULE/REFERENCE` argument opens an exact
installed module through the shared URI resolver. A running default profile
receives application-open events.

The Settings popover saves startup Bible, commentary, devotional, appearance
and zoom beside the session file. Appearance and default zoom apply to open
readers. Selected words can open map or translation lookups and can be read by
Speech Dispatcher when available. The BibleSync panel uses shared network
configuration, polling and incoming decisions with explicit connection consent.
It supports manual sharing, direct navigation and a bounded review list. The
optional SDK was unavailable during that checkpoint.

The Fedora 45 ARM64 serial build, URI and Sync policy/transport checks, and GTK
startup and study smokes passed. The external startup smoke also checks that an
independent detached read leaves main history intact and that settings survive
a write and reload in a disposable profile.

## GTK4 reading and editing adapters (2026-11-11)

Work on master from `a05f4b78`. The GTK reader now offers chapter and stacked
comparison layouts, verse stepping, translation visibility by ordered module
list, and comparison from a search result using the default saved set. Split
mapping fragments carry their own source references for link and word dispatch.
The layouts and visibility persist with each window. Word study in the Bible
and comparison panes uses shared literal, mark-insensitive and lemma search.

The reader adds native Help and About, a selected-word shortcut, in-pane find,
zoom, scoped passage copy and text/HTML save,
printing of the displayed passage, and restricted per-module font, size and
colors. Bible switching uses shared versification mapping when the reference
field matches the committed passage. German, Spanish and French use the bundled
private SWORD locales for typed Bible references and selector labels. The
devotional pane supports module and date selection through the shared reader.
The library adds writable module templates and ZIP archives. The editor can
rename, move and delete book sections using checked core edits.

The Biblical Map window reuses the Mac HTML, script, styles and notices and
accepts a local PMTiles file for preview. The Fedora WebKitGTK probe rendered
all four bundled marker nodes. Linux native install, checksum-verified
download and removal of regional packs are still pending.

Fedora 45 ARM64, released SWORD 1.9, GTK 4.24.1 and WebKitGTK 2.54.2. Serial
Debug builds use `-O0 -g0` and the matching locally extracted libarchive headers.
The full serial CTest suite passed 203 checks with one optional BibleSync skip.
The native app startup smoke passed. The GTK workflow smoke covers
the isolated parallel link, persisted layout and translation visibility, Unicode
editing, notes, saved reading styles, source-bound word lookup and selected-verse
export. No hosted jobs were dispatched.

See the newer window and startup checkpoint above for progress since this slice.
Offline map-pack management, complete localized menus, real-corpus word/RTL
checks, print output, desktop accessibility and distribution acceptance remain
open.

## Maintenance baseline on master

Fetch the fork's `master` and read the [2026 native baseline](2026-native-baseline.md)
before changing Linux build requirements. The Mac preview now targets macOS 27.0,
requires CMake 3.20 for its fixture commands, and uses current SwiftUI change
callbacks. The Linux and standalone core minimum remains CMake 3.16, and the
shared core remains C++17.

For the shared build cleanup, record the CMake and SDK versions on the maintained
Linux targets and run both the standalone core and GTK4/WebKitGTK builds and
tests locally. Use that evidence alongside native Windows validation before
raising the shared CMake minimum and removing the CMake 3.16 libarchive shim.
Keep SWORD discovery and SDK-specific handling, module and profile migration,
and regression fixtures during this pass. Keep hosted Actions disabled.

## Fedora maintenance check

The Linux helper now installs curl and libarchive development packages and the
Python 3 and OpenSSL fixture tools. Preflight checks SQLite, curl and libarchive
alongside the existing dependencies.

On Fedora 45 ARM64, standalone core and GTK4 preview configuration passed at
master baseline `ac325ed4` using CMake 4.3.0, GCC 16.2.1, SWORD 1.9.0, GLib
2.90.1, GTK 4.24.1, WebKitGTK 2.54.2, curl 8.21.0 and libarchive 3.8.9. The
libarchive headers came from a locally extracted Fedora development RPM matching
the installed runtime. No system packages were installed.

The separate `linux/november-mac-parity` checkout at `050fdf91` with local edits
rebuilt the GTK4 preview and passed its basic graphical smoke. Its CTest suite
had 203 passes and one optional BibleSync skip after rerunning two HTTPS fixtures
with loopback access. These test results cover that branch. The merged master build subsequently passed 203 CTest checks and the GTK study
smoke, with one optional BibleSync skip. The remaining matrix checks are Fedora
44 compatibility and native Ubuntu 24.04 LTS and Debian 13 on x86-64.

## Linux parity integration

The Linux parity branch adds native library, repository, session, recovery,
parallel reading and study UI adapters. Its Fedora desktop smoke verified
parallel link navigation from nested markup with module inline handlers
disabled. The pre-merge branch recorded 203 CTest passes and one optional
BibleSync skip.

## Shared word study and comparison

GTK can pass a selected surface, actual module and verse to
`xc_worker_word_study`, then use the returned ordered terms with
`XC_SEARCH_LEMMA`. The owned result can contain zero terms. Use
`xc_worker_visit_parallel_profile_from` with the search hit's source module
and reference for cross-canon comparisons. Empty cell references represent
unavailable mappings. Split cells expose additional verse references and HTML
through the `xc_parallel_cell_additional_*` accessors. See [the real corpus checks](real-corpus-validation.md#selected-word-metadata-and-comparison)
for tested cases. GTK selection and stacked presentation remain to be wired. The optional
`xc_worker_resolve_localized_reference` API can canonicalize a typed leading book
name from a private UTF-8 SWORD locale before a normal visit.

## Next validation

Consume current shared APIs, run the local portable/desktop checks in the
actual Linux environment, then verify reading, search, library/session workflows,
keyboard/screen-reader behavior and packaging. Apply the [roadmap](shared-core-roadmap.md)
without creating independent search/navigation/persistence engines.
