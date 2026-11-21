# Windows development handoff

Updated 2026-11-21. Read [shared instructions](handoff.md) and
[Windows inventory/build tooling](../../platforms/windows/README.md).

Target Windows 11 version 25H2 and check compatibility on 26H2. The native ARM64
core suite has passed on 25H2 with the SDK below. The WinUI 3 reading preview
uses the same shared core through a narrow C ABI DLL.
The old GTK/MinGW win32 distribution is removed. WSL and Mac-hosted portable
checks are not MSVC or native Windows desktop validation.

Track Settings/navigation, keyboard/Narrator, high contrast and
scaling in the [native UI list](native-ui-todo.md). Native x64, 26H2 compatibility,
and Windows runtime deployment validation remain open.

[Historical prerequisite and API evidence](history/handoff-windows.md) remains
available. Its dated compiler/SDK observations must be rechecked on the target machine.

## WinUI 3 reading preview

Build instructions are in the [Windows build guide](../../platforms/windows/README.md).
The unpackaged .NET 10 preview uses Windows App SDK 2.7.0 and a UTF-8 process
manifest. The shell opens a local SWORD library and uses the shared FIFO worker
for the catalog, passage rendering, commentary, navigation and history. WebView2
is configured for passive HTML and routes individual passage links through the
core. Broader reference-list and study workflows remain open.

The ARM64 Release native DLL and WinUI application built successfully on
Windows 11 25H2. A managed smoke check loaded a generated SWORD Bible, rendered
John 3:16, moved to John 4:1, returned through history, paired generated
commentary at John 3:16, and followed a scripture link to John 1:1. In a normal
desktop process, entering John 3:16 in the native window displayed the Unicode
Bible fixture text in WebView2. Visual commentary pairing and broader keyboard,
Narrator, high-contrast and scaling checks remain open. The Codex command sandbox returned HRESULT
`0x8000FFFF` from WebView2 initialization, so run desktop checks from an
ordinary Windows session.

### WinUI search

The search pane submits whole-module Bible and regular commentary queries to
the shared worker. It offers words, phrase, regex, indexed, lemma and exact or
mark-insensitive surface modes, with case matching, a 500-result display limit,
progress and Stop. Results retain the actual source module for activation.
Opening a Bible hit preserves the selected commentary. Opening a commentary
hit keeps the selected Bible and opens the source commentary.

ARM64 managed fixture checks covered Unicode Bible and commentary queries,
opening both source types and reporting indexed fallback on an unindexed
module. In a Windows 11 25H2 ARM64 desktop check, the WinUI search for
`peace` returned John 3:16 and **Open result** displayed the Unicode fixture
in WebView2.

The scope selector also supports an explicit passage range interpreted by a
chosen Bible, and refinement within the worker's last-results scope. A
separate preview reads the source entry without visiting it. An ARM64 Release
build and managed fixture checks passed for range inclusions/exclusions,
last-result refinement and Bible/commentary preview history isolation. A
desktop search for `peace` displayed the John 3:16 preview text in WebView2
while the main reader stayed at John 1:1.

The export menu uses the shared search generation and can save either one
selected result or all retained results as text or passive HTML, or copy
either selection as plain text. The ARM64 Release build and managed fixture
checked selected and all-result Unicode text and HTML payloads. The desktop
check exposed all six export actions and opened Windows Save As from a
selected result. File completion and clipboard paste still need desktop checks.

Rich clipboard formats, multi-module and keyed searches, and cancellation
latency on a large corpus remain open. Continue with parallel reading and
selected-word study.

### WinUI parallel reading

The preview compares two to eight installed Bibles in an explicit order. It
renders the worker's aligned chapter snapshot as a table or stacked verse,
including headings, split mappings and per-column text direction. It keeps
comparison chapter navigation independent of the main passage and history.

On Windows 11 25H2 ARM64 the Release build passed. Managed fixture checks
covered source order, selected verse, Unicode text, chapter stepping and
main-history isolation. A desktop check displayed the two John 3:16 fixture
texts in the chapter table and in two stacked sections while the main reader
stayed at John 1:1.

Named comparison sets use the shared XML profile and revision-checked edits for
creation, order updates, rename, default selection and deletion. A new reader
loads the default order. The Translations menu hides and restores columns in
both comparison layouts, storing Windows presentation state by ordered module
list. The ARM64 managed fixture verified Unicode names, ordered reopening,
default restoration and rejection of stale edits. The desktop check saved
and opened a default set, hid one translation and restored that choice in a
new process. Native comparison link actions and mixed-canon desktop checks
remain open.

### WinUI selected-word study

The Bible pane and both comparison layouts offer a selected-word command.
WinUI reads the selection from the rendered verse, preserving its actual module
and reference, and uses the shared worker for exact and mark-insensitive
surface search and available lemma or Strong's terms. A Bible search hit can
open in the default comparison through shared verse mapping.

The ARM64 Release build and managed fixture checked Greek surface, unmarked
surface and Strong's searches, a source without lemma metadata, and opening a
search hit in the saved comparison. A Windows desktop check selected Greek
from the second Bible in the chapter table, showed its John 3:17 source in
the native dialog, and returned that verse from the search. **Open in comparison**
opened the selected hit in the default set. The native surface search also
returned John 3:16 from an unannotated Bible word.
Physical keyboard and right-click routing, stacked selection, Hebrew and
Aramaic desktop workflows, and mixed-canon corpus validation remain open.

### WinUI dictionary and lexicon reading

The dictionary pane opens installed non-devotional dictionary modules and
supports key entry, nearby keys, adjacent-key browsing and independent
back/forward history. Bible and commentary link dispatch consumes ordinary
dictionary targets through the shared pane-link worker. Dictionary browsing
keeps the main passage in place.

The ARM64 Release build and managed fixture checked a Unicode dictionary URI,
nearby keys, adjacent keys and independent dictionary history. A native
desktop check opened ALPHA, stepped to BETA, displayed the nearby chooser,
then looked up CAFÉ and used dictionary back and forward. WebView2 showed the
Unicode fixture text while the Bible remained at John 1:1. Configurable
original-language lexicon targets are now configurable through the dictionary
pane's Lexicon settings dialog. It supports all five nullable choices, explicit
XML import/save, and typed Strong's and morphology results from reading links.
The ARM64 Release build and focused managed fixture passed preference readback,
XML round-trip and configured Strong's dispatch. Hover remains open.
The desktop settings check applied all
five fixture dictionary choices through the native editable selectors.

Dictionary-pane links now use the shared pane router with the entry's actual
module as source. Managed checks resolved both scripture and dictionary links.
An ARM64 desktop check activated a dictionary URI and opened BETA in the pane.

### WinUI daily devotional

The dictionary pane includes a daily devotional section with module selection,
a native calendar date picker and previous/next day controls. It uses the shared
read-only date policy, distinguishes the requested date from a nearest
available SDK key, and keeps Bible and dictionary histories intact.

The ARM64 Release build passed. A managed fixture checked devotional catalog
classification, leap-day rendering, next-day content and both histories.
The desktop check read today's fixture and stepped forward and back while
the Bible stayed at John 1:1. A saved default devotional and broader corpus
checks remain open.

### WinUI note previews

Typed footnote links from the reading panes open a native dialog with passive
WebView2 note content. The result preserves its source module, passage and
label and is released after copying.

The ARM64 Release build and focused managed checks passed. A desktop check
activated a note link and displayed the original Unicode body in its dialog.
Note-body links, retained hover previews and broader cross-reference workflow
checks remain open.

Multi-verse reference links now open an owned list preview with a native
reference chooser. Selecting **Open reference** navigates through the shared
worker. Previewing preserves reading history and discloses the core's
display-limit and skipped-reference flags. ARM64 managed checks verified
ordered range content, history isolation and activation of the selected verse.
The main passage controls also expose **Reference list** for explicit range,
disjoint and partial reference input through the same shared parser.
A visual Windows check entered John 3:16-17, displayed both fixture texts,
selected John 3:17 using the native chooser and opened its text in the reader.
The PowerShell accessibility tree omitted the chooser and dialog footer;
visual and keyboard verification passed through the computer-use runtime.

### WinUI reading options (2026-11-21)

The Reading options pane queries shared per-module settings without implicitly
initializing them. An explicit defaults action enables the shared legacy
defaults; module-declared filters and textual variants can then be applied or
reset. Shared `modops.conf` import/save is explicit. Refreshing Bible,
commentary and dictionary content uses read-only requests to preserve history.
Asynchronous reader actions restore the invoking control's keyboard focus when
it remains enabled and visible.

The ARM64 Release native DLL and managed app built successfully. Focused managed
fixture checks passed for capability copying, initially unconfigured options,
explicit defaults, footnote on/off rendering, reset, settings round-trip and
Bible/dictionary history isolation. In the native desktop app, enabling defaults
exposed the actual verse footnote marker; keyboard activation opened its correct
Unicode note body. Comparison engine options, presentation settings, broad
module/corpus coverage and full keyboard/Narrator acceptance remain open.
The rebuilt desktop app also retained Passage focus after Enter, with Tab
continuing to Go; this is a scoped focus check rather than accessibility sign-off.
General-book browsing follows in the next checked batch.

### WinUI general-book reading (2026-11-21)

The Books pane uses the shared indexed-tree capability, owned bounded child
pages, exact canonical paths and committed DisplayLevel documents. Parent,
child and sibling navigation stays on the existing FIFO worker. It consumes
prepared inline HTML, module/lexical CSS and resource diagnostics; book
footnotes preserve exact descendant sources in the existing preview dialog.
Reading-options refresh also re-renders the committed book document.

ARM64 managed checks passed for root/absence identity, SDK child order,
130-child pagination without duplication, Unicode descendants, DisplayLevel,
exact book notes, navigation, rejected-path state retention and unchanged Bible
and dictionary histories. The maintained `book_document_test` and
`book_document_publication_test` passed locally against the ARM64 shared library.
The C ABI now permits the canonical empty root path while still rejecting NULL;
the publication test covers that distinction. Native DLL rebuilding succeeded
with existing C4190/C4297 warnings in unrelated transfer/migration/sync helpers.
The managed Release app built with zero warnings and errors.
On the native desktop, exact `/Chapter` input displayed the Unicode chapter and
Greek descendant text. Contents loaded root choices; keyboard selection opened
`/Next` and collapsed contents, and Previous section returned to `/Chapter`.
Close books restored Bible controls at the unchanged John 1:1 passage. The
book workspace hides inactive Bible/search controls to reserve reading space.

Book/module links now use the shared contextual typed router, retaining the
actual book path/anchor, dictionary, scripture or preview destination. Owned
image bytes come from the core's bounded resource reader and render in a passive
preview dialog. Book anchors wait for their matching WebView navigation before
a fixed host query locates an ID or named anchor. Missing anchors are reported.

The ARM64 native DLL and managed Release app built successfully for this follow-up,
with zero managed warnings/errors. Managed fixtures passed typed descendant notes,
book/anchor targets, explicit targets without a main Bible, exact PNG bytes/source,
outside-path rejection, dictionary/scripture/range dispatch, canonical scripture
activation and selection/history isolation for previews. In the desktop app,
the SDK-generated inline PNG rendered, its image command opened the correct
source preview, and keyboard book-link activation opened `/Destination` and
scrolled past forty context lines to the anchored Unicode target.

Explicit book selection import now exposes both shared restore modes: saved
display key and saved startup offset. The Windows XML picker supplies an explicit
path to the existing worker profile reader. Returned module identity comes from
the owned source tree. Missing-book profiles are no-ops; failed imports retain
the committed document and source files remain unchanged.

The native DLL and WinUI ARM64 Release app built successfully with zero managed
warnings/errors. Managed fixtures passed a Unicode filename and UTF-16 profile,
conflicting saved key/offset modes, exact profile-byte preservation, absent and
rejected module retention, and Bible/dictionary history isolation. The desktop
check opened both import choices, completed the native XML picker for Saved
book key, and displayed the expected `/Chapter` Unicode document.

Saved native sessions, separate windows, broader image formats/assets, corpus
and accessibility/resize acceptance remain open. Continue with native session
save/load and tab ownership through the shared persistence service.

## ARM64 setup checkpoint (2026-11-16)

On Windows 11 25H2 ARM64, Xiphos commit `26852e1a` passed CMake compiler and
ABI detection with MSVC 19.52.37029, Windows SDK 10.0.26100.0, CMake 4.3.1 and
Ninja 1.13.2. Set `CMAKE_POLICY_DEFAULT_CMP0141=NEW` and
`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded` in a fresh build directory to
work around the compiler's C1902 program database failure in this shell.

The local ARM64 Release SWORD 1.9.0 static SDK used the five reviewed patches,
the `/MD` runtime, bundled zlib and regex, and the Windows Winsock library.
The local SDK source also needed an `extern` declaration for static FTP symbols
where the Windows header normally applies DLL annotations. An MSVC consumer
linked with `ws2_32.lib`, constructed `SWMgr`, and parsed John 3:16 successfully.
This SDK has no CLucene support, so it cannot validate indexed search.

The owner installed the ARM64 vcpkg dependencies from regular Windows PowerShell
after vcpkg hit an `Access is denied` error opening a child-process stdin pipe in
the Codex shell. SWORD was rebuilt against vcpkg's zlib, bzip2 and liblzma to
avoid duplicate zlib symbols. CMake used ARM64 pkgconf to find GLib and explicit
SWORD headers/library and link inputs (`ws2_32` and the three compression libraries).
Keep SWORD's `.pc` file outside pkgconf's search path when selecting the SDK
explicitly. The vcpkg Release DLL directory was added to `PATH` for tests.

The native Release build linked the core and test executables after MSVC fixes
for general-book traversal, worker object size, a missing STL include and legacy
test fixtures. CTest registered 183 checks. Windows core acceptance and WinUI
implementation remain pending.

The first full ARM64 run had 128 passes, one skip, 21 failures (including three
timeouts and one process exception), and 33 checks withheld by failed fixtures.
Two subsequent focused retests passed: Unicode search with `/utf-8`, and staged
rich-module save/reload/rollback after closing SWORD's cached descriptors before
Windows directory renames. These changes need a full rerun. Prioritize the
remaining fixture failures in reader/navigation, profile file paths, image
resources, and library / remote installation before claiming native parity.

On 2026-11-17, four profile XML tests passed with UTF-8 compilation. The
preference-save test also passed after the Windows test executables opted into
a UTF-8 process code page for SWORD and the C runtime's narrow file APIs. The
future WinUI host needs the same process setting. Focused ARM64 retests also
passed reader navigation, book-document publication with a Unicode path,
stylesheet selection and book-image resources. The last two fixtures needed
portable path comparisons and a standard drive-letter file URI. The full
suite still needs a rerun, including verse images and module installation.

The next full ARM64 build linked all test executables, but the drive ran out of
space during CTest. After reclaiming space, focused ARM64 tests passed local
installation, module maintenance, and encrypted-module maintenance with the
SWORD file-handle release in the library swap. The loopback HTTPS installation
fixture also passed when the process had network access. The curl transport now
reports the error category or unexpected HTTP status for failures.

The full ARM64 Release rerun at `98850e5b` recorded 154 passes, one skip, seven
failures (four timeouts and three assertions), and 21 tests blocked by failed
fixtures. Verse-image handling, local installation, module maintenance and
loopback HTTPS passed. The two search fixtures and wide-book pagination passed
direct runs in roughly 117–118 seconds each. Unicode book publication passed a
direct retest. Windows CTest now allows 180 seconds for those four fixtures.
Book creation and deletion also passed after checking indexed results only
when the SDK had a usable index. The explicit required-index run still requires
CLucene. Mixed keyed search returned zero hits in all three compatibility
modules, and dictionary navigation failed the per-tab options assertion. Retest
with the updated fixture limits, investigate those two assertions, and verify
indexed behavior with an appropriate SWORD SDK before Windows core acceptance.

An ARM64 Release SWORD 1.9.0 rebuild with [ICU4C 79.1][icu-release] passed
`core_mixed_search`, `core_dictionary`, `core_keyed_search`, and `core_rich_module`
without application changes. Use the ARM64 ICU distribution, add its
`libARM64` import libraries (`icuin`, `icuio`, `icuuc`, `icudt`) to the static
SWORD link inputs, and add `binARM64` to the test process's `PATH`. The tested
ZIP `icu4c-79.1-WinARM64-MSVC2026.zip` has SHA-256
`185d32268533a981f070c75a4435a9141ee58c12c342468cf431d746b9b68237`.

On the prior SDK, wide-book pagination and book creation passed again under
CTest. Both search fixtures timed out during a concurrent compiler build, and
the book-document setup fixture timed out, preventing the publication check.
Run the validation suite without concurrent compilation.

The ARM64 SDK with ICU and CLucene 2.3.3.4 passed five focused checks with
`XIPHOS_REQUIRE_INDEXED_SEARCH_TEST=ON`: attribute search, rich-module editing,
indexed book creation/deletion/rollback, keyed index search, and verse index
search. The bundled SWORD Windows patch fixes directory removal after CLucene
deletes index entries. The [dependency recipe](../../dependencies/sword/README.md#windows-arm64-indexed-sdk)
records the source, patches, linker inputs and runtime paths.

The full serial ARM64 Release run with that SDK and required indexed checks
passed 180 of 183 tests in 1,742 seconds. Search range, search text options and
search batch were the only timeouts. All three passed a serial CTest rerun with
180-second Windows limits in 42.75, 142.28 and 132.35 seconds respectively.
The rerun reused the two passing search setup fixtures from the full run.
All 183 checks have a passing result across the two runs. The updated limits
applied only to the three rerun checks after the full run completed.

[icu-release]: https://github.com/unicode-org/icu/releases/tag/release-79.1
