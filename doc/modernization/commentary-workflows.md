# Commentary workflows and acceptance

Updated 2026-10-12. This is a partial F08/F06 implementation. The GTK3 application
remains the behavioral oracle; full functionality/design parity is not accepted.

## Source-backed contracts

- `src/main/tab_history.c:main_change_verse_tab_history` restores recorded
  `textmod`/`commod` at the reference normalized by Bible navigation. Shared
  history characterization covers reference-only deduplication, first-recorded
  module metadata, forward retention and the 50-entry limit. Changing commentary
  at an existing reference does not rewrite its first recorded pair.
- `src/backend/sword_main.cc:BackEnd::module_type` distinguishes regular
  Commentaries modules from RawFiles personal commentary. The complete frozen
  and current function is compared for loaded RawCom, RawFiles and RawText data.
- `src/main/global_ops.cc:main_new_globals` enables chapter mode only for regular
  commentary, unless `Commentary by Chapter` is exactly `Off` after SWConfig
  decoding. Missing/empty values enable it; personal and other module types stay
  false. Frozen/current assignment comparisons cover defaults, On/Off, case and
  unexpected values. This is a setting seam, not complete global-preference parity.
- `src/main/display.cc:GTKEntryDisp::displayByChapter` starts at verse 1, advances
  the SWORD module with consecutive-link skipping, and stops at the book/chapter/
  error boundary. Selected/missing/trailing anchor conditions have separate
  frozen/current statement comparisons. Cache, heading and personal-module
  renderer branches are not covered by these comparisons.
- `src/main/url.cc:sword_uri` defaults module-less scripture links to the main
  Bible, independent of the source pane. Explicit Bible targets replace the
  Bible; explicit commentary targets replace commentary, retaining the other
  pane. These branches have source-backed implementation tests; the complete
  legacy URI handler is not yet characterized.
- `src/gtk/main_window.c` and `help/C/figures/interface.png` establish central
  Bible, upper-right commentary, lower-right dictionary/book/devotional and left
  sidebar/information placement. Native split views follow the implemented pane
  positions; exact sizing, layout restore, hide/show, fonts, colors, menus and
  complete accessibility remain unaccepted.

## Shared core and adapters

`StudyPassage` owns the Bible and optional commentary entries and module IDs.
The commentary uses the normalized Bible reference in its own versification and
with its own engine preferences. Empty commentary requests no commentary.
Cross-versification mapping is not implemented; a representative differing-
versification corpus must validate legacy equivalence or establish a mapping need.

`read_passage` does not record history. `visit_passage` records only after both
entries render; paired navigation restores both entries before moving the cursor.
Chapter stepping uses Bible versification. Paired visits validate Bible/Commentary
types; history restoration also supports older standalone, unpaired commentary
locations. Failed paired selection/routing leaves history unchanged.

`read_commentary_chapter` returns an owned `Chapter` with sparse iterator entries.
Its array indices do not identify canonical verse numbers: `selected_verse` is
an engine ordinal, and `selected_anchor` is `commentary-verse-N`. Hosts consume
provided HTML/anchors instead of implementing traversal or anchor policy. Module
engine options apply to each section; chapter reads do not record Bible history.

Sessions own per-module `ModulePresentationOptions`. Module import decodes one
SWConfig into both staged engine/presentation maps before committing either.
Failed imports preserve both. Combined save uses the existing verified replacement
transaction, updating chapter mode only for regular commentary and preserving
unrelated, unavailable and personal-module values. The old engine-only save helper
remains compatible. Initial native setters accept regular commentary only;
personal editors and their complete menu/workflow behavior remain pending.

`follow_pane_link_at` separates default Bible navigation from source note/lexical
context. `visit_linked_passage` owns module-type routing of explicit targets.
Other module kinds and abbreviation/range/fragment behavior remain pending.

The FIFO worker copies commands/results. C handles transfer once and retain
copies after worker/request release. Typed presentation scalar getters require
ready/presentation kind and may be repeated; dropping observation does not cancel
accepted edits. Swift copies values without a second SWORD engine or independent
synchronization/default policy. New paired/chapter/mode commands reject embedded
NUL strings before the C-string boundary.

## macOS behavior checked locally

Commentary selection from the module sidebar/picker refreshes the committed
passage without submitting draft input or adding history. Bible visits, selectors
and chapter steps synchronize entries; Back/Forward restore recorded choices.
Commentary Close retains the Bible/draft. Dictionary lookup leaves commentary
intact in the lower-right pane.

The independent **Whole commentary chapter** control reads/updates the core's
per-module preference. Switching modules restores each setting; regular defaults
apply on selection. The Display menu's number/highlight settings apply when
either pane shows a chapter. Existing explicit reading-options import/save now
round-trips commentary mode. Startup profile discovery, saved pane selection and
visibility/layout restore are still pending.

Commands use busy/revision/closed guards. Required snapshots publish together;
failed commentary choices retain committed panes, draft and history. Separate
windows own separate tab preferences and histories. Runtime checks verify two
synchronized references, recorded Back behavior, dictionary coexistence, chapter
mode with unsent input, verse-number navigation, source commentary notes,
selected-row/link contrast, defaults and module-setting restoration.

## Verification and remaining acceptance

Preverse headings have owned core snapshots, shared Headings defaults/import/save,
typed C/Swift adapters and native verse/chapter controls. Heading-only entries
remain visible; toggles preserve body text, draft input and history. OSIS/ThML
fixtures include multiple/linked headings and source-context reference previews. Complete
heading transformations and corpus/visual parity remain pending.

The full local suite passes **63/63**. Core/adapter/model tests include:

| Test | Evidence |
|---|---|
| `core_study_passage` | Real original-prose RawText/RawCom/RawLD pairs, normalization, history, failures, tab/FIFO ownership, link routing and C snapshot lifetime |
| `core_commentary_positions` | Frozen/current iterator statements; sparse/linked content and chapter/book/empty-content boundaries; key/flag restoration |
| `core_commentary_chapter` | Sparse rendering, preferences, escaped attributes, frozen/current anchor boundaries, display flags and C transfer after worker shutdown |
| `core_commentary_preferences` | Complete classification function and mode assignment comparisons; combined import/save; personal/unavailable/unknown preservation; typed scalar/queued edit ownership |
| `swift_passage_adapter` | Copied pairs/chapters/mode metadata, linked routing, per-module isolation and NUL boundary checks |
| `reader_model_commentary` | Draft preservation, pane selection/history, independent chapter modes, profile round trip, dictionary coexistence, window isolation and closed publication |

Focused presentation-adapter address/undefined and thread checks pass **1/1**
each; earlier shared preference checks pass **3/3** and **1/1** respectively.
SWORD/GLib/system libxml2 are not fully instrumented; leak detection was disabled.
Local runtime inspection supports the scoped controls above, not full visual or
corpus acceptance.

| Area | Remaining implementation/validation |
|---|---|
| Chapter presentation | Complete heading transformations/corpus, cache behavior, image resizing, locale/number formatting and exact theme/layout |
| Detached windows | Navigation/hover origin and synchronization rules |
| Personal commentary | Editing, cache invalidation and complete menu behavior |
| Companion modules | Prompt, first companion in Bible pane and additional windows |
| Saved settings | Startup selections, pane visibility/layout and broader profile migration |
| Corpus | Locked/unlocked modules, alternate versification, headings and broader markup/modules |
| Other platforms | WinUI 3 and GTK4 panes using the same core services |

No legacy source is retired by these milestones. Tests, licenses and source
provenance remain required after individual integration changes are checked off.
