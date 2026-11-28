# macOS GTK-to-native workflow comparison

Started 2026-11-28. **In progress: live app comparison is blocked by UI-tool timeouts.**
This is a workflow acceptance ledger, not a new completion estimate. The owner
reports substantial missing functionality in the installed native beta. That
report remains open; core APIs and test counts do not establish usable parity.

## Baselines and evidence

- Installed GTK bundle: Xiphos GTK Preview, version 4.5.0, bundle build 1.
- Installed native bundle: Xiphos 5.0 beta 1, version 5.0, build 5.0.0b1.
- Versions were read from bundle metadata. Neither bundle's exact source revision
  was established; do not assume today's source equals the downloaded beta.
- Native source reviewed at `c8a5851`; local GTK-port source at `162ece2e`.
  These are source references, not verified provenance for the installed binaries.
- Graphify was queried as a navigation aid; important findings below were checked
  in source. No application source, modules or user study data were edited.
- Installed GTK resource inspection confirmed Open Session, Link Tabs and ordinary
  bookmark commands in the shipped xi-menus/xi-menus-popup/bookmarks gtkbuilder
  files. Packaged commands prove inclusion, not successful runtime behavior.
- Native and GTK UI inspection attempts timed out. No workflow below has yet
  earned **verified equivalent** from this comparison.

## Classification

**Missing** requires a baseline workflow and evidence of an absent native route.
**Partial** means a native route exists but does not cover the whole workflow.
**Hard to discover** requires live confirmation that a working route is difficult
to find. **Verified equivalent** requires matching actions/results in both apps.
Until live checks finish, source findings are explicitly provisional and usable
routes are described as candidates rather than accepted equivalents.

## Initial source findings and live checks

| Workflow | GTK baseline/source | Native source finding | Provisional classification / next check |
|---|---|---|---|
| Ordinary bookmarks | Bookmark tree/dialog and context actions under src/gtk; F22/F23 | BookmarkManagement.swift exposes open/new destination, AndBible review/import, exact XML export and reading-source application. No ordinary bookmark add/edit/delete/reorder controls were found in that screen or reader commands. Shared worker edit_bookmarks already exists. | **Partial; normal editing UI missing in reviewed source.** Check add-current-verse, open saved target, folder creation/rename/delete, move/reorder, tags and restart in installed apps. |
| Explicit session opening | ui/xi-menus.gtkbuilder exposes Open Session and Save Session | ReaderApp.swift exposes Save/Save As; SessionApp.swift restores the configured path on startup. No explicit Open Reading Session command was found in the reviewed command surface. | **Partial.** Verify whether another installed route opens a selected saved session without changing startup configuration. Preserve unsaved editors during the test. |
| Linked tabs | GTK View menu exposes Link Tabs | Native tabs/detach/restore exist; synchronized detached Bible/commentary views exist. No general Link Tabs control was found in reviewed native commands. | **Unresolved potential gap.** Compare linked passage navigation across ordinary reading tabs; detached synchronization alone is insufficient. |
| Main workspace and panes | GTK View menu independently exposes Bible, Preview, Commentary/Book, Dictionary, attached sidebar and preview location | Native reader has sidebar modes, Bible/commentary/dictionary panes and separate management windows. | **Partial layout candidate.** Compare discoverability, pane visibility, resizing, focus and preview placement; do not require identical GTK widgets. |
| Search and advanced search | GTK Edit menu offers Search and Advanced Search | SearchPane.swift/SearchModel.swift expose multiple modes/scopes, module choices, progress/Stop, previews and exports. | **Implemented-route candidate.** Compare common search, advanced bounds, last results, source identity, cancellation and selected/all export; do not call it missing merely because menus differ. |
| Parallel comparison | GTK View menu exposes parallel in a tab | Native reader/saved-set windows provide chapter/stacked comparisons, ordering and visibility. | **Implemented-route candidate.** Verify opening without prior profile setup, saved sets, source-bound links and annotations, stepping and restart. |
| Notes and annotations | GTK personal commentary/annotation actions and highlight behavior; F23/F25 | Native File menu has Personal Verse Notes; reader accepts explicit note/bookmark sources. | **Partial workflow candidate.** Check adding from the current verse, returning to reading, highlight/tag precedence and persistence without manual path knowledge. |
| Studypad, journals and prayer lists | GTK File menu has Open Studypad and New Prayerlist/Journal | Native File menu has New/Open Study Document and New Journal or Prayer Section; EditorWindow.swift exposes rich/source modes, undo/redo, formatting and links. | **Implemented-route candidate.** Compare organization, create/open/edit/save/delete and recovery; differently named commands are not proof of equivalence. |
| Module library/install/maintenance | GTK Module Manager | Native File > Module Library exposes repository/install/maintenance adapters. | **Partial workflow candidate.** Verify repository setup, refresh, installed-vs-available clarity, download feedback, unlock/index/remove and restart using disposable modules only. |
| Preferences and appearance | GTK Preferences plus View toggles | Native Settings has General, Appearance, Reading, Lexicons and BibleSync; reader also has local preferences. | **Partial workflow candidate.** Compare available settings, scope (global/module/window), immediate application and restart; identify controls split across screens. |
| Detailed reading styles | Installed GTK prefs.gtkbuilder includes font preferences per language, separate verse-number size, current-verse/number/link/highlight colors, display columns and margin justification | ReadingStyleUI.swift provides Serif/Sans Serif/Monospace, bounded base size and text/background color overrides. SettingsUI.swift adds selected-verse highlighting; reviewed style screen has no equivalent granular controls. | **Partial; detailed style controls absent in reviewed style UI.** Audit each retained control and scope before deciding what belongs in native presentation. |
| Context actions and original-language study | GTK reader popup and original-language settings; F14/F16/F34/F35 | Native reader has selected-word surface/lemma study, lexical links and explicit speech/map/translation actions. | **Partial workflow candidate.** Compare actual popup actions, Hebrew/Greek metadata, selection ownership, previews and keyboard access. |
| Help and support | GTK Help menu includes contents, bug report, translation/project/about and releases | Native commands expose Help and About; support/update/reporting are retained requirements in native-ui-todo.md. | **Partial.** Inventory installed Help destinations and support actions. No automatic reporting or network submission during comparison. |

The [38-row requirement ledger](parity-matrix.md) remains authoritative for full
scope. This initial findings table is not the complete gap list; the coverage
checklist below prevents uninspected areas being mistaken for accepted parity.
BibleSync, external sword:// handling and AndBible import retain their existing
implementations but are deferred initial-release gates under the
[owner scope decisions](feature-review-decisions.md).

## Prioritized native backlog

1. **Finish this installed-app inventory before claiming parity or deleting more
   reference material.** Record app version, exact action/menu, expected result,
   observed native result, evidence and source/core availability for each gap.
2. **Ordinary bookmark workflow:** make existing shared edit operations usable
   from native reading and management. Cover current-verse creation, navigating
   saved entries, folder edits/reordering and safe persistence. Detailed UI design
   and implementation are separate work; do not build another bookmark engine.
3. **Session and workspace continuity:** verify/close explicit session opening and
   linked-tab gaps, with unsaved-state protection, pane visibility and restoration.
4. **Complete daily study workflows:** connect notes, personal books and rich
   documents to the current reading context, then verify comparison and search
   routes end to end. Separate absent controls from hard-to-find working controls.
5. **Library and preferences usability:** verify complete source-to-installed-module
   and settings-to-reader paths. Inventory missing font/color/layout controls
   individually, preserving existing safety checks and core ownership.
6. **Keyboard, accessibility and discoverability acceptance:** test every retained
   command and failure path with resizing, RTL and screen-reader use; update help
   with the actual routes. Keep deferred feature scope distinct from missing work.

## Safe live comparison protocol

Use both installed main windows for read-only inspection of menus and current
layout. Use original-prose fixtures and separate disposable profiles for mutations,
installation, session replacement, annotations and recovery tests. Do not edit or
publish user study material, send sync traffic, install/remove user modules or
change their saved preferences to establish parity. Record module availability
separately from absent UI. No screenshot containing personal notes belongs in the
public repository. A newer source implementation is not proof that the installed
beta exposes it.

## Current blocker and next action

The UI tool returned timeouts selecting both installed app paths, including a
runtime reset. Whether the desktop is locked or the accessibility bridge is
unavailable has not been established. The owner has been asked to leave the Mac
unlocked with both reading windows open. Resume live menu/layout inspection when
UI access works; retain all classifications above as provisional until then.

## Full live coverage checklist

All 38 groups below remain **not live-verified** in this comparison. Follow the
listed action in both installed apps, then attach an observation to the findings
table and classify the actual gap. Source-suite results in the main ledger do
not change that status. Deferred scope stays visible rather than silently removed.

| Requirement | Live action / result to compare |
|---|---|
| F01 — Module discovery / metadata / about | Compare IDs, descriptions, type, language, version and licenses with legacy; empty library |
| F02 — Verse entry rendering / normalization / versification | Same reference and XHTML as legacy for KJV and alternate canons, Unicode, missing entries |
| F03 — Chapter / whole-book / headings / verse highlighting | Compare chapter boundaries, headings, selected verse, scrolling and full book mode |
| F04 — Book/chapter/verse selectors and direct reference entry | Selectors synchronize Bible/commentary; Return resolves ranges and localized names |
| F05 — History / tabs / detach / session save and restore | Back/forward per tab, open/close/detach, restart and explicit saved-session round trip |
| F06 — Main pane composition / resize / view toggles / fullscreen | Sidebar left, preview below; Bible central; commentary upper right; dictionary/book/devotional lower right; validate resizing and toggles |
| F07 — Module tree category/language grouping / favorites / hidden | Toggle favorites/hide, group all three ways, restart and preserve selections |
| F08 — Commentary synchronization / chapter mode / detached view | Reference changes update commentary; headings, chapter mode and separate window match |
| F09 — Dictionary / lexicon lookup and key navigation | Word lookup, next/previous keys, direct entry and separate windows |
| F10 — General books / tree keys / images / book windows | Compare configured DisplayLevel depth/section order, hierarchy and siblings, linked images and persistent selection |
| F11 — Daily devotionals | Date/default-module selection and devotional tab behavior |
| F12 — Parallel translations / ordering / up to eight modules | Compare verse alignment, ordering and navigation in tab/separate windows |
| F13 — Named parallel sets / save / rename / delete | Switch sets, rename/delete and restart without losing ordering |
| F14 — Footnotes / crossrefs / hover preview | Hover/click each link family; dispatch to proper pane/module; retain preview on pointer leave |
| F15 — Multi-verse references / verse lists / partial references | Range/disjoint/partial keys expand and navigate in expected order |
| F16 — Original language Strong’s / morphology / lemma / lookup | Hover lexical tags; choose Greek/Hebrew dictionaries; test lemma search |
| F17 — Content options / accents / vowels / red letters / variants | All ConstructFlags options, per-module overrides and cache invalidation |
| F18 — Simple search / phrase / multiword / regex / case / bounds | Compare ordered results, case sensitivity, bounds and last-search scope |
| F19 — Advanced multi-module / indexed / attribute search | Lucene and attribute results, ranges, footnotes/lemma and multiple module types |
| F20 — Search progress / cancel / preview / result export | Cancel stops work; single click previews, double click navigates; export results |
| F21 — Find within displayed text | Find forward/backward, wrap and focus shortcuts without changing passage |
| F22 — Bookmarks / folders / drag-drop / import-export | Round-trip XML; folder hierarchy, order, references and module association |
| F23 — Colored bookmark tags / annotations / highlight precedence | Overlapping tags and personal annotation choose same display color as legacy |
| F24 — AndBible SQLite bookmark import / tags / notes / v11n | Fixture DB: label colors, UUIDs, ordinals, unknown v11n, notes, existing bookmarks |
| F25 — Personal commentary / editor / links / save/delete | Create/edit/delete verse notes; formatting and link round trips |
| F26 — Studypad / journals / prayer lists | Create/open/save content, internal links and tree organization; restart |
| F27 — Module source configuration / refresh / remote/local install | Existing sources, local and remote installs, progress/errors and restart |
| F28 — Module maintenance / uninstall / indexing / cipher keys | Unlock, remove, build/delete indexes, refresh caches safely |
| F29 — Preferences / special modules / per-module defaults | Read legacy settings, change defaults, round-trip without dropping unknown fields |
| F30 — Fonts / colors / CSS / zoom / RTL / Unicode | Compare Arabic/Hebrew/Greek, per-module fonts, CSS, zoom/reset and light/dark |
| F31 — BibleSync modes / speaker / audience / privacy | Two clients: personal/speaker/audience, allowed senders, passphrase and mode switching |
| F32 — Passage export / copy / print / selected text | References, format, selection and printed pagination match established workflow |
| F33 — sword:// links / IPC / external navigation | External request opens correct module/reference; unsupported schemes fail safely |
| F34 — Speech / audio / geography / translate selection | Audit availability in each legacy build; preserve supported commands and external URLs |
| F35 — Menus / context actions / shortcuts / focus / accessibility | Every command reachable; keyboard-only workflows and screen reader audit on each OS |
| F36 — First run / no modules / help / localization / about | No-module guidance, help, translated labels, credits/licenses and error dialogs |
| F37 — Legacy user files / sessions / settings / module paths | Read representative existing profiles; backups and lossless round trips before writes |
| F38 — Packaging / upgrades / platform enhancements | Signed/notarized macOS, Windows installer, Linux desktop integration; upgrade data retention |

## Installed GTK resource fingerprints

SHA-256 of the packaged resources inspected on 2026-11-28. These identify the
menu/preference evidence only; they do not prove binary source provenance.

| Resource | SHA-256 |
|---|---|
| xi-menus.gtkbuilder | `b7e92e11ca7e82cc5bc924d755c96be4673e882efb5315a9d9fef3723ec4b64b` |
| xi-menus-popup.gtkbuilder | `7b85c047ebb43765d78d3a229a5dfe17afa4ea1edd5f8b7f8b510400eb2292ea` |
| bookmarks.gtkbuilder | `abb01a55a0ecc18eab559b85b8d514213203765f0b582fa587b713fc3bfd3f4a` |
| prefs.gtkbuilder | `500e8c83790a2361b2dc129778a6d3dd2106eac85d7438a54ef911fc0e5a6984` |
