# Functional, workflow and design parity ledger

Source inventory, not an assertion of runtime parity. Baseline and seams are in
[architecture.md](architecture.md). macOS/Windows/Linux refer to NEW frontends;
the legacy GTK3 application remains the behavioral oracle. **No row is fully
validated for any native platform.** `partial` describes scoped implementation/tests, not accepted parity. `pending` means not implemented/validated. Core fixture tests
provide scoped evidence linked below and in [status.md](status.md), not a
representative Bible/module corpus. Updated 2026-10-12; no acceptance status
changes are implied by these implementation entries.

| ID | Required capability / established workflow | Source evidence | Architectural owner | Acceptance evidence required | Initial core/macOS evidence | WinUI 3 | GTK4 |
|---|---|---|---|---|---|---|---|
| F01 | Module discovery / metadata / about | src/backend/sword_main.cc; src/gtk/about_modules.c | module-service | Compare IDs, descriptions, type, language, version and licenses with legacy; empty library | partial core tested; macOS smoke | pending | pending |
| F02 | Verse entry rendering / normalization / versification | src/backend/sword_main.cc; src/main/navbar_versekey.cc | reader-service | Same reference and XHTML as legacy for KJV and alternate canons, Unicode, missing entries | partial core tested; macOS smoke | pending | pending |
| F03 | Chapter / whole-book / headings / verse highlighting | src/main/display.cc; src/main/modulecache.hh; help/C/xiphos-50-preferences-general-settings.page | content-service | Compare chapter boundaries, headings, selected verse, scrolling and full book mode | partial chapter/navigation + number/highlight options + anchor scroll; full decorations/visual acceptance pending | pending | pending |
| F04 | Book/chapter/verse selectors and direct reference entry | src/main/navbar_versekey.cc; ui/navbar_versekey.gtkbuilder | study-session | Selectors synchronize Bible/commentary; Return resolves ranges and localized names | partial direct entry + adjacent chapter + shared book/chapter/verse choices; composition/localization pending | pending | pending |
| F05 | History / tabs / detach / session save and restore | src/main/tab_history.c; src/gtk/tabbed_browser.c; src/gtk/main_menu.c | study-session + window-host | Back/forward per tab, open/close/detach, restart and explicit saved-session round trip | partial Back/Forward/Clear; complete frozen/current add/fake history adapter characterization; tabs/detach/session persistence pending | pending | pending |
| F06 | Main pane composition / resize / view toggles / fullscreen | src/gtk/main_window.c; help/C/figures/interface.png; ui/xi-menus.gtkbuilder | layout-model + native windows | Sidebar left, preview below; Bible central; commentary upper right; dictionary/book/devotional lower right; validate resizing and toggles | partial macOS shell + resizable commentary/dictionary placement | pending | pending |
| F07 | Module tree category/language grouping / favorites / hidden | src/gtk/sidebar.c; src/main/settings.c | module-library policy | Toggle favorites/hide, group all three ways, restart and preserve selections | pending | pending | pending |
| F08 | Commentary synchronization / chapter mode / detached view | src/gtk/commentary.c; src/main/display.cc | study-session | Reference changes update commentary; headings, chapter mode and separate window match | partial core/macOS paired verse + history; shared sparse/linked chapter + characterized anchors; native chapter/headings/detached pending | pending | pending |
| F09 | Dictionary / lexicon lookup and key navigation | src/gtk/dictlex.c; src/gtk/dictlex_dialog.c | keyed-content-service | Word lookup, next/previous keys, direct entry and separate windows | partial explicit SWLD core/C/Swift snapshots + native lexical click pane; native direct lookup/module selection/key steps; native recorded-click history; nearby-key chooser; shared main-click Unicode casing; full workflow/locale/platform acceptance pending | pending | pending |
| F10 | General books / tree keys / images / book windows | src/main/sword_treekey.cc; src/gtk/gbs.c; src/gtk/treekey-editor.c | tree-content-service | Traverse hierarchy and siblings, view linked images and persistent selection | pending | pending | pending |
| F11 | Daily devotionals | src/gtk/dictlex.c; help/C/xiphos-11-interface.page | devotional-selection | Date/default-module selection and devotional tab behavior | pending | pending | pending |
| F12 | Parallel translations / ordering / up to eight modules | src/main/parallel_view.cc; src/gtk/parallel_tab.c | parallel-service | Compare verse alignment, ordering and navigation in tab/separate windows | pending | pending | pending |
| F13 | Named parallel sets / save / rename / delete | src/main/settings.c; src/gtk/preferences_dialog.c | parallel-set repository | Switch sets, rename/delete and restart without losing ordering | pending | pending | pending |
| F14 | Footnotes / crossrefs / hover preview | src/main/previewer.cc; src/main/url.cc; src/gtk/menu_popup.c | semantic-link + preview-service | Hover/click each link family; dispatch to proper pane/module; retain preview on pointer leave | partial shared note retrieval + macOS body/expanded-list/range-link sidebar preview; native semantic lexical/footnote hover across reader/information/reference-list/note-body/dictionary surfaces with live main fallback, retained information and cross-reference list/information mode; other hover/click/full parity pending | pending | pending |
| F15 | Multi-verse references / verse lists / partial references | src/backend/sword_main.cc; src/main/sidebar.cc; src/main/url.cc | reference-list service | Range/disjoint/partial keys expand and navigate in expected order | partial frozen/actual parser + ordered range/list snapshots + macOS sidebar/navigation; full input/platform parity pending | pending | pending |
| F16 | Original language Strong’s / morphology / lemma / lookup | src/main/global_ops.cc; src/main/url.cc; help/C/xiphos-33-original-language.page | content-options + lookup | Hover lexical tags; choose Greek/Hebrew dictionaries; test lemma search | partial annotation filters + typed lexical parser + [legacy selection characterization](lexical-lookup.md); shared selection + native preferences/click result pane; native lexical hover and explicit XML lexicon import/save; remaining hover/search pending | pending | pending |
| F17 | Content options / accents / vowels / red letters / variants | src/main/global_ops.cc; src/main/modulecache.cc | render-policy | All ConstructFlags options, per-module overrides and cache invalidation | partial 18-field core policy + per-module scopes + 12 native toggles/variant choices; remaining controls/cache/full corpus pending | pending | pending |
| F18 | Simple search / phrase / multiword / regex / case / bounds | src/main/search_sidebar.cc; help/C/xiphos-30-search.page | search-service | Compare ordered results, case sensitivity, bounds and last-search scope | pending | pending | pending |
| F19 | Advanced multi-module / indexed / attribute search | src/main/search_dialog.cc; src/backend/sword_main.cc | search-service | Lucene and attribute results, ranges, footnotes/lemma and multiple module types | pending | pending | pending |
| F20 | Search progress / cancel / preview / result export | src/main/search_dialog.cc; src/gtk/search_dialog.c | search-events + export | Cancel stops work; single click previews, double click navigates; export results | pending | pending | pending |
| F21 | Find within displayed text | src/gtk/find_dialog.c; src/webkit/wk-html.c | native renderer | Find forward/backward, wrap and focus shortcuts without changing passage | pending | pending | pending |
| F22 | Bookmarks / folders / drag-drop / import-export | src/gtk/bookmarks_treeview.c; src/gtk/export_bookmarks.c; src/main/xml.c | bookmark repository | Round-trip XML; folder hierarchy, order, references and module association | pending | pending | pending |
| F23 | Colored bookmark tags / annotations / highlight precedence | src/gtk/bookmarks_treeview.c; src/gtk/bookmark_dialog.c; src/main/display.cc | annotation-policy | Overlapping tags and personal annotation choose same display color as legacy | pending | pending | pending |
| F24 | AndBible SQLite bookmark import / tags / notes / v11n | src/main/import_andbible.cc; src/gui/import_andbible.h | import-service | Fixture DB: label colors, UUIDs, ordinals, unknown v11n, notes, existing bookmarks | pending | pending | pending |
| F25 | Personal commentary / editor / links / save/delete | src/editor; help/C/xiphos-40-personal-commentary.page | note repository + native editor | Create/edit/delete verse notes; formatting and link round trips | pending | pending | pending |
| F26 | Studypad / journals / prayer lists | src/main/prayerlists.cc; src/editor; help/C/xiphos-41-studypad.page; help/C/xiphos-42-journals.page | document-service | Create/open/save content, internal links and tree organization; restart | pending | pending | pending |
| F27 | Module source configuration / refresh / remote/local install | src/backend/module_manager.cc; src/main/mod_mgr.cc; ui/module-manager.gtkbuilder | install-service | Existing sources, local and remote installs, progress/errors and restart | pending | pending | pending |
| F28 | Module maintenance / uninstall / indexing / cipher keys | src/backend/module_manager.cc; src/gtk/cipher_key_dialog.c | install-service | Unlock, remove, build/delete indexes, refresh caches safely | pending | pending | pending |
| F29 | Preferences / special modules / per-module defaults | src/main/settings.c; src/main/configs.cc; ui/prefs.gtkbuilder | settings repository | Read legacy settings, change defaults, round-trip without dropping unknown fields | partial explicit 18-field engine import/save + native panels; native window lexicon choices plus explicit XML import/save; separate read-only preview XML setting; complete profile/global/full preferences pending | pending | pending |
| F30 | Fonts / colors / CSS / zoom / RTL / Unicode | src/main/display.cc; src/main/global_ops.cc; help/C/xiphos-52-preferences-fonts-colors.page | content styles + native appearance | Compare Arabic/Hebrew/Greek, per-module fonts, CSS, zoom/reset and light/dark | pending | pending | pending |
| F31 | BibleSync modes / speaker / audience / privacy | src/main/biblesync_glue.cc; help/C/xiphos-51-preferences-biblesync.page | sync-service | Two clients: personal/speaker/audience, allowed senders, passphrase and mode switching | pending | pending | pending |
| F32 | Passage export / copy / print / selected text | src/main/export_passage.cc; src/gtk/export_dialog.c; src/gtk/menu_popup.c | export-service + native clipboard/print | References, format, selection and printed pagination match established workflow | pending | pending | pending |
| F33 | sword:// links / IPC / external navigation | src/main/url.cc; src/gtk/ipc.c; src/examples/xiphos-nav.c | action-router + platform IPC | External request opens correct module/reference; unsupported schemes fail safely | partial single-reference core + macOS verse-number links; remaining actions pending | pending | pending |
| F34 | Speech / audio / geography / translate selection | src/gtk/menu_popup.c; src/gtk/utilities.c; win32/festival | native capability adapters | Audit availability in each legacy build; preserve supported commands and external URLs | pending | pending | pending |
| F35 | Menus / context actions / shortcuts / focus / accessibility | ui/xi-menus.gtkbuilder; ui/xi-menus-popup.gtkbuilder; help/C/xiphos-02-shortcut-keys.page | command-model + native bindings | Every command reachable; keyboard-only workflows and screen reader audit on each OS | pending | pending | pending |
| F36 | First run / no modules / help / localization / about | src/gtk/xiphos.c; help/C/xiphos-10-start.page; po; src/gtk/about_xiphos.c | startup policy + native host | No-module guidance, help, translated labels, credits/licenses and error dialogs | pending | pending | pending |
| F37 | Legacy user files / sessions / settings / module paths | src/main/configs.cc; src/main/xml.c; src/main/settings.c | storage compatibility | Read representative existing profiles; backups and lossless round trips before writes | partial explicit modops.conf and five-field lexicon XML import/save with unrelated-value preservation/verified replacement; complete profiles/sessions/restoration/migration pending | pending | pending |
| F38 | Packaging / upgrades / platform enhancements | cpack; desktop; win32; .github/workflows/builds.yml | platform packaging | Signed/notarized macOS, Windows installer, Linux desktop integration; upgrade data retention | pending | pending | pending |

## Visual baseline and validation protocol

Inspected `help/C/figures/interface.png`: four sidebar mode buttons, module
hierarchy left, preview below, passage tabs and navigation above the central
Bible, commentary above dictionary/devotional on the right. Supplement with
interface_toolbar, interface_parallel, interface_biblepane-options,
interface_commentarypane, bookmarks, studypad, journal, module and preferences
screenshots. These assets are references, not proof of current behavior:
manually capture the running baseline for each workflow before approval.
Some manual pages have TODOs for exact newer menu wording; source files take
precedence, then validate by running the existing application.

For every row record baseline module/version/license, sample profile,
reference/action sequence, expected results, screenshots, platform/build,
test or manual evidence, reviewer and remaining gaps. Use licensed modules
covering OSIS/ThML/GBF, RTL, multiple versifications, Strong’s/morphology,
commentary, lexicon, tree books, images, encrypted and sparse content. Track
semantic output separately from visual layout and keyboard interaction.
A passing fixture or compilation does not change a row to validated.

Generate detailed discovery checklists on demand:

```sh
python3 doc/modernization/generate_inventory.py
```

The command writes `ui-command-inventory.tsv` and
`manual-section-inventory.tsv` to ignored `build/modernization/`. The first
lists widget classes, IDs and labels (including non-command labels); the second
lists manual page/section titles. Keep the generator in version control rather
than committing reproducible output. Regenerate after rebasing the baseline.
These appendices support discovery and do not replace runtime validation.

[Current milestone status and cleanup gates](status.md) distinguish verified
implementation from accepted platform parity.
