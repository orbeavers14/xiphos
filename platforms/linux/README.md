# Linux shared-core automation

For a new chat or computer, first read the [shared handoff](../../doc/modernization/handoff.md)
and [Linux/Fedora task/evidence instructions](../../doc/modernization/handoff-linux.md).

Run from a checkout on Debian/Ubuntu or Fedora:

```sh
platforms/linux/core.sh --install-deps
platforms/linux/core.sh --sanitize
```

The first command installs the compiler, CMake, Ninja, pkg-config, SWORD,
GLib and libxml2 development packages using sudo when necessary. Installation
requires administrator privileges and network access. Omit `--install-deps`
when the dependencies are already installed. No GTK packages are needed.

Each command configures, builds and runs the complete standalone CTest suite,
including generated module fixtures and legacy adapter characterization.
Failures stop the command with a nonzero status. Tests have a 120-second timeout;
an empty test suite fails. The sanitizer build enables AddressSanitizer and
UndefinedBehaviorSanitizer; leak detection defaults off for uninstrumented
system dependencies, matching the existing native-core workflow.

Release and sanitizer builds use separate ignored directories under `build/`.
Use `--jobs N` to control memory/CPU use (default 2) and `--build-dir DIR` to
choose a separate output directory. Relative directories resolve from the
calling directory. Use a different directory when changing compiler or mode.
CMake honors `CC`, `CXX`, and standard dependency discovery variables.
Generated modules never require an installed Bible corpus or user profile.

The `linux-core` GitHub Actions workflow is manual-only to conserve hosted build
allowance. Pushes and pull requests do not start it. An authorized manual dispatch
runs both modes on Ubuntu 22.04 and 24.04 (four jobs).
It preserves CTest diagnostics for seven days, including failed runs.
It is independent of the existing paused native-core/macOS and legacy workflows.
Repository administrators must allow GitHub Actions for manual runs. Do not
require these checks on every change while routine acceptance stays local.

This validates the shared core's existing contracts. It does not establish full
feature parity or complete the shared engine. See the modernization status and
parity ledger before moving to GTK4 frontend implementation.

## Experimental GTK4 reading preview

Fedora prerequisites (GTK4-compatible WebKit, separate from legacy GTK3):

```sh
sudo dnf install gtk4-devel webkitgtk6.0-devel
cmake -S platforms/linux -B build/gtk4 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gtk4 --parallel 2
ctest --test-dir build/gtk4 --output-on-failure --parallel 2
build/gtk4/xiphos-linux-preview
```

The standalone preview discovers a SWORD library read-only. Use `--root DIRECTORY`
to select an explicit library; no user settings or installed modules are written.
Select a Bible and optional commentary, enter a reference and press Return or Go.
Back/Forward restore pairs through the shared core. The right-hand commentary
stays above the reserved dictionary/book/devotional area. This is an initial
verse-reading slice: complete chapter layout and notes/hover controls,
module grouping, tabs, persistence and release packaging are pending. Only
passage links are presented; unsupported links report an error and stay local.
Module scripts and remote resources are blocked by JavaScript settings and CSP.

One core worker serializes engine jobs. GTK polls request completion without
waiting, copies result strings before release and drains shutdown in a background
GTask. Busy/closed actions cannot submit or publish; failed actions retain the
last committed passage. History and module routing stay in the existing core.

For original-prose fixtures, build `core_study_passage_test` and generate its
modules before running the disposable desktop smoke test:

```sh
ctest --test-dir build/gtk4 -R '^(core_study_passage|linux_reader_model)$' --output-on-failure
GSK_RENDERER=cairo build/gtk4/xiphos-linux-preview --smoke-test build/gtk4/core/study-passage-fixture
```

The opt-in smoke test opens and closes a real window, exercises entry/button
signals and waits for WebKit document loads. It disables WebKit hardware
acceleration for VM use and checks pending-work shutdown. It requires a desktop
session and is deliberately absent from default CTest/hosted runs. The ordinary
preview leaves acceleration at the system default. Automated signal tests do
not establish keyboard, screen-reader, visual composition or performance parity.

### VM rendering and desktop actions

Use `build/gtk4/xiphos-linux-preview --software-rendering` when the VM cannot
supply accelerated graphics. This disables WebKit hardware acceleration and
selects GTK's Cairo renderer unless `GSK_RENDERER` is already explicitly set.
The default preview retains the system rendering policy.

- Ctrl+L focuses the passage reference field; Enter submits it.
- Alt+Left and Alt+Right use the same core Back/Forward actions as the buttons.
- Alt+B and Alt+C target the labeled Bible/commentary selectors through GTK
  mnemonics (desktop mnemonic settings apply).

Navigation actions disable while work is pending and obey core history state.
The reference field remains editable: a completed read updates its normalized
reference only if the user has not typed a newer draft. Selectors and reading
panes have explicit accessible labels. The desktop smoke checks action/accelerator
registration, focus placement, draft retention and navigation; real key delivery
and screen-reader usability still require manual acceptance.

### Chapters and reference choices

Bible chapter, verse numbers and selected-verse highlighting are per-window
presentation controls. Commentary chapter mode comes from the core's per-module
preference. Previous/Next chapter resolve through SWORD and visit paired history;
Book/Chapter/Verse selectors consume core-owned labels and targets. Clear history
leaves the committed display intact. Display refreshes do not add history.


### Dictionary pane

Choose a dictionary, enter a word/key and press Enter or Look up. Nearby keys
retain the core's nine entries, including duplicate labels at SDK boundaries.
Previous/Next key and direct lookups do not record history. Dictionary
Back/Forward/Clear consume the shared separate dictionary history, which is
populated by recorded lexical clicks (contextual click integration follows).
Close clears the pane, not history. Failed lookups retain the last result.

### Reading options and profiles

The Reading options popover exposes the same twelve filter controls and textual
variants as the macOS preview. Eligibility, configured values, commentary mode,
headings, image content and font-face states come from core snapshots. Unknown or
unsupported options remain disabled. Refreshes preserve Bible history and drafts.

Import/Save reading options open native file pickers and delegate explicit local
paths to the core's verified profile transaction. Nothing is automatically read
from or saved into a user profile. GTK's older native chooser API is retained for
the declared GTK 4.6 baseline (new GTK emits deprecation warnings).

Additional local checks:

```sh
cmake --build build/gtk4 --target linux_options_model_test core_scoped_options_test core_module_preferences_test --parallel 2
ctest --test-dir build/gtk4 -R '^(linux_options_model|linux_reader_model)$' --output-on-failure
build/gtk4/xiphos-linux-preview --smoke-options-test build/gtk4/core/module-preferences-fixture
```

File-picker selection/cancellation still requires manual desktop acceptance;
profile behavior itself is covered using generated original-prose fixtures.

### Contextual study previews and lexicons

Study-note clicks open the left study preview. Passages expands a note's ordered
reference list; Note returns to its owned body; Close dismisses it. Strong's and
morphology clicks use the shared lexicon policy and recorded dictionary history.
Hover information is retained on pointer leave and unsupported/failed hovers.
The cross-reference list/information choice belongs to the core study tab.
Main navigation invalidates pending hover observation; no module logic is parsed
in GTK, and hover always uses the live committed main source.

Lexicon dictionaries provides five nullable preferences, role-filtered installed
module choices, exact-name editing for unavailable modules, Apply and explicit
XML Import/Save. Unset and empty values remain distinct; core XML save rejects
values that cannot round trip without changing the existing file. No startup
profile writes or locale changes are introduced.

```sh
cmake --build build/gtk4 --target linux_preview_model_test core_dictionary_test --parallel 2
ctest --test-dir build/gtk4 -R '^linux_.*model$' --output-on-failure
build/gtk4/xiphos-linux-preview --smoke-preview-test build/gtk4/core/module-preferences-fixture
build/gtk4/xiphos-linux-preview --smoke-lexical-test build/gtk4/core/dictionary-fixture
```

These smoke modes exercise model-to-widget publication and signals, not physical
pointer/key delivery or manual screen-reader/visual acceptance.

### Windows and selected-verse scrolling

New window (Ctrl+N) opens an independent study tab on the same process-wide core
worker. Histories, display settings, lexicons and hover preference are window
scoped. Closing a window drops its observers and queues tab cleanup; final worker
shutdown runs on a background thread after the GTK loop has no windows left.

Chapter anchors are copied from the core. After document load, a fixed trusted
host expression scrolls to the escaped anchor in an isolated WebKit world;
module JavaScript stays disabled. An asynchronous application hold protects
callback lifetime even when the window closes during scrolling.

Standalone commentaries can be selected as the main reading module. Changing a
paired commentary refreshes the committed passage without adding history.

```sh
cmake --build build/gtk4 --parallel 2
ctest --test-dir build/gtk4 --parallel 2 --timeout 120 --output-on-failure
build/gtk4/xiphos-linux-preview --smoke-window-test build/gtk4/core/study-passage-fixture
```

The window smoke checks actual two-window publication, independent passages and
chapter modes, successful Bible anchor lookup/scroll, pending close and continued
use of the remaining window. It does not certify manual visual/accessibility
parity or accelerated rendering in this VM.

### Current macOS preview scope

The Linux preview now exposes the current macOS preview's implemented reading,
commentary, dictionary, options/profile, contextual preview, lexicon and
multiwindow capabilities through the same core. Normal startup reads the first
Bible at John 3:16. Module descriptions, types and languages are visible beside
selectors. Reading-module changes read immediately; choosing a commentary while
a Bible is displayed updates its paired pane without a history visit. Dictionary
views retain their original source context across key steps and later Bible
navigation. Close window is Ctrl+W; Quit is Ctrl+Q.

WebKit views use ephemeral network sessions, matching macOS's nonpersistent
store. Content scripts/resources remain blocked; fixed isolated host code is
used only for selected-anchor scrolling. Red-letter/reference/selected styling
mirrors the existing macOS host's semantic classes.

Final startup/selection check:

```sh
build/gtk4/xiphos-linux-preview --smoke-startup-test build/gtk4/core/study-passage-fixture
```

This is implementation-scope equivalence with an experimental macOS preview,
not accepted 1:1 desktop parity or a complete Xiphos replacement. Real keyboard,
pointer/hover, file-picker, screen-reader, visual/RTL/font and accelerated-rendering
acceptance, licensed representative modules/profiles and release packaging
remain explicit gates. The legacy GTK3 application stays intact.

### Shared lexical presentation and headless model checks

The reader model copies core-owned HTML, lexical CSS and requested/applied state
before releasing each C snapshot. Main/commentary/chapter views consume matching
pairs, including whole-pane raw fallback. CSS participates in refresh identity.
No annotation parser or layout policy is implemented in GTK. The extracted host
document helper retains the existing resource/script restrictions.

For a machine with the core SDK but no GTK4/WebKitGTK, the portable model tests
can be built separately (normal SDK discovery settings still apply):

```sh
cmake -S platforms/linux -B build/linux-models -DXIPHOS_BUILD_GTK4_PREVIEW=OFF
cmake --build build/linux-models --parallel 2
ctest --test-dir build/linux-models --no-tests=error --output-on-failure --parallel 2
```

The option defaults ON, preserving the regular desktop build above. OFF builds
no GTK executable and cannot validate GTK compilation, desktop behavior or Linux
runtime compatibility. The lexical checkpoint passed 53/53 portable/core checks
on macOS and 4/4 focused ASan/UBSan checks including fixture prerequisites. Fedora
must rebuild the ON target and repeat desktop/physical-input checks before this
new view integration receives native Linux acceptance.
