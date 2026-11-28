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
| Context actions and original-language study | GTK reader popup and original-language settings; F14/F16/F34/F35 | Native reader has selected-word surface/lemma study, lexical links and explicit speech/map/translation actions. | **Partial workflow candidate.** Compare actual popup actions, Hebrew/Greek metadata, selection ownership, previews and keyboard access. |
| Help and support | GTK Help menu includes contents, bug report, translation/project/about and releases | Native commands expose Help and About; support/update/reporting are retained requirements in native-ui-todo.md. | **Partial.** Inventory installed Help destinations and support actions. No automatic reporting or network submission during comparison. |

The [38-row requirement ledger](parity-matrix.md) remains authoritative for full
scope. This initial table is a triage starting point, not the complete gap list.
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
   and settings-to-reader paths, preserving existing safety checks and core ownership.
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
