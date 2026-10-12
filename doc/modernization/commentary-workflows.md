# Synchronized commentary: initial implementation

Updated 2026-10-12. This records a partial F08/F06 slice, not complete commentary
or visual parity. The GTK3 application remains the behavioral oracle.

## Source-backed contract

`src/main/tab_history.c:main_change_verse_tab_history` restores the recorded
`textmod` and `commod` at the reference normalized by the Bible navigation
controls. The shared history characterization already covers reference-only
deduplication, first-recorded module metadata, forward retention and the 50-entry
limit. Changing a commentary at an existing reference does not rewrite that
reference's first recorded pair.

`src/main/sword.cc:main_display_commentary` gates display on commentary visibility
and availability, accepts regular/personal commentary types, sets the module key,
and handles optional `Companion` modules. `src/main/display.cc:GTKEntryDisp`
has a separate commentary chapter path with linked-entry skipping, headings,
cache behavior and personal-commentary exceptions. These presentation details
must be characterized before reusing the Bible chapter renderer for commentary.

The main layout acceptance source is `src/gtk/main_window.c` and
`help/C/figures/interface.png`: Bible central, commentary upper right and
dictionary/book/devotional lower right, with sidebar/information panes left.
The native preview now follows the commentary/dictionary placement and has
resizable native split views. Exact sizing, hide/show preferences, restored
layout, fonts, colors, menus and platform accessibility remain unaccepted.

## Shared core and adapters

- `StudyPassage` owns Bible and optional commentary entries plus their module IDs.
  The commentary reads the normalized Bible reference, using its own module
  versification and engine preferences. Empty commentary requests no commentary.
- `read_passage` does not record history. `visit_passage` records a pair only after
  both entries render. `navigate_passage` restores that pair before moving the
  cursor; `navigate_passage_chapter` steps using the Bible's versification.
- Paired visits require Biblical Texts/Commentaries module types. Restoration
  also retains old standalone, unpaired commentary history from existing
  single-entry APIs. Unknown/wrong-type modules and unavailable destinations
  report errors without committing the paired visit/cursor.
- The FIFO worker copies commands/results. The C ABI uses an owned passage
  handle with transfer-once take and borrowed getters. Swift copies the result
  without introducing a second SWORD engine or passage synchronization policy.
- Cross-module versification mapping is not implemented: each module interprets
  the same normalized reference. A representative differing-versification corpus
  is still needed to validate legacy equivalence and any mapping requirement.

## macOS behavior checked locally

A commentary can be selected from its picker or the installed-module sidebar.
Selection refreshes the committed passage without submitting draft reference
text or adding a visit. Bible visits, reference selectors and chapter stepping
synchronize both entries; Back/Forward restore recorded choices. Commentary
Close hides its content while retaining the Bible and draft. Whole chapter is
currently a Bible display option; commentary continues to show the selected
verse. Dictionary lookup leaves commentary intact in the lower-right pane.

Model commands serialize through the existing busy/revision/closed guards.
Results publish together after all required snapshots are available. Failed
commentary choices retain the committed panes, draft and history. Separate
windows own separate tab history and selections.

## Evidence and remaining acceptance

The generated `core_study_passage` fixture uses original prose in real RawText,
RawCom and RawLD modules. It checks normalized references, restored module pairs,
reference-only deduplication, optional commentary, failures, FIFO ownership,
tab isolation, old standalone commentary history, and C snapshot lifetime.
`swift_passage_adapter` checks typed copies and paired navigation;
`reader_model_commentary` checks draft preservation, selection, histories,
chapter steps, dictionary coexistence, window isolation and closed publication.

Local runtime inspection additionally checks sidebar selection, rendered Bible
and commentary text at two references and recorded Back behavior, plus dictionary lookup in the lower-right
pane while retaining commentary. Four focused address/undefined checks and four
thread checks also passed; SWORD/GLib/system libxml2 are not fully instrumented
and leak detection was disabled. These checks
are scoped implementation evidence. The following remain required:

| Area | Pending validation/implementation |
|---|---|
| Commentary chapter mode | Actual legacy linked-entry traversal, heading handling, layout and options |
| Detached windows | Separate commentary window, navigation/hover origin and synchronization rules |
| Personal commentary | Editable modules, cache invalidation and editor integration |
| Companion modules | Prompt, first companion in Bible pane and additional windows |
| Saved settings | Startup module selection, pane visibility/layout and profile migration |
| Corpus | Locked/unlocked modules, linked/empty entries, headings, alternate versification and markup |
| Other platforms | WinUI 3 and GTK4 panes using the same core commands |

No legacy source is retired by this slice. Tests and source provenance remain
required even after individual integration milestones are checked off.

## Chapter traversal characterization

`commentary_positions` now copies the main legacy iterator frontier: start at
verse 1, advance the SWORD module with consecutive-link skipping, and stop at
the book/chapter/error boundary. It restores the prior key identity, key flags
and skip-links setting. `core_commentary_positions` compares it with frozen
CrossWire c9976a8 statements and statements extracted from the current
`GTKEntryDisp::displayByChapter` source at configure time. Generated linked/sparse
RawCom data covers missing verses, linked sections, chapter/book boundaries and
empty OT content. The normal and address/undefined checks pass. This characterizes
iterator statements only, not the complete renderer/cache/headings or selection
anchor presentation. The native pane still shows a single commentary verse.

## Initial shared chapter service

`Reader`, `StudySession` and the FIFO worker expose `read_commentary_chapter`.
The owned `Chapter` snapshot contains sparse iterator entries rather than the
Bible's dense canonical verse list. `selected_verse` remains the engine verse
ordinal; `selected_anchor` uses `commentary-verse-N`. Hosts consume the provided
HTML/anchor and do not infer section ordinals from array indices. The service
applies module engine preferences and does not record a Bible history visit.

Selected, missing, linked and trailing anchors are checked against the frozen
and configure-time extracted complete anchor-condition statements from the
main legacy chapter renderer, with a minimal key/section shim. Numbered and
unnumbered presentation retain the section boundary for the selected anchor.
This compares anchor placement semantics, not complete legacy HTML/theme.

The full local suite passes 54/54, including filter preference changes, sparse
rendering, chapter boundaries, escaped attributes and queued snapshot ownership.
Four focused address/undefined and three thread checks pass; the same external
SDK/instrumentation limits apply. Headings, caching, image resizing, personal
commentary handling and native chapter controls are still pending.
