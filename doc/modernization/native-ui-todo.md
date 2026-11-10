# Native interface to-do list

Owner requirement,2026-11-04: review Settings and other screens against each
platform’s recommended native interaction patterns. Current functional screens
are intermediate; opening a setting should not routinely spawn another window.

## Mac

- [ ] Replace Settings launch buttons/separate preference screens with one native
  Settings window using coherent sections/sidebar navigation and inline detail.
  Keep reading/display, default modules, lexicons, startup/recovery, language and
  sync preferences in that navigation. Use sheets only for bounded modal tasks;
  separate document/study windows remain appropriate when explicitly requested.
- [ ] Review Module Library, bookmarks/import, annotations, parallel-set management,
  migration and editor flows for platform-native navigation and window behavior.
- [ ] Verify standard menu shortcuts, keyboard focus/order, VoiceOver names,
  accessible status/error announcements, resizing and localized text expansion.
- [ ] Add native macOS appearance support: follow system Light/Dark mode by
  default, with user-selectable System, Light and Dark preferences. Use SwiftUI/
  AppKit semantic adaptive colors across navigation, controls and settings.
- [ ] Make Bible reading, commentary, dictionary and other formatted/HTML or
  WKWebView content theme-aware; ensure verse highlights, search results, links
  and custom module styles remain legible in both appearances. Consider optional
  independent reading themes (including Sepia). Keep appearance/presentation
  decisions in the native UI, not the shared C++ core.
- [ ] Review the design against current Apple guidance before implementation;
  record scoped screenshots and actual workflow checks, not just build success.

## Windows

- [ ] Design Settings as a coherent WinUI3 navigation/settings experience with
  inline detail, native controls and predictable Back behavior; avoid chains of
  independent windows for individual settings.
- [ ] Apply the same review to library/install, bookmarks/import, annotations,
  parallel-set management, migration and editing screens.
- [ ] Verify Windows keyboard/navigation conventions, Narrator/UI Automation,
  high contrast, scaling, resizing and localized text expansion on native Windows.
- [ ] Review against current Microsoft guidance when implementing. No WinUI
  frontend or Windows desktop acceptance is claimed by this planning entry.

Keep application behavior in the shared core. This work concerns native
presentation and navigation, not a second settings or study engine.

## Parallel comparison layouts

Owner request,2026-11-05, illustrated by Bible Hub's stacked Genesis1:1 view:

- [x] Add a focused Mac single-verse comparison with each translation's name above
  its verse text in a vertical list. This complements the current chapter matrix.
- [ ] Offer stacked verse and side-by-side chapter layouts, saved module ordering,
  module visibility and previous/next verse navigation.
- [ ] Keep optional context, commentary, cross-reference and lexical actions
  accessible alongside the comparison without duplicating the shared engine.
- [ ] Adapt the presentation to native Mac/Windows navigation, narrow windows,
  RTL/mixed direction, keyboard focus and accessibility; extend to GTK4 as used.

Reuse the current shared parallel alignment/results and installed module catalog.
The screenshot is a behavior/layout reference, not a request to copy the website
or its text library. Licensed/provider translations remain separate roadmap work.
The Mac preview can switch between the chapter matrix and stacked verses and
can open a verse search result using the active startup comparison profile.
Search hits and comparison cells use SWORD versification mapping. A result
without a verse in the control canon leaves the prior comparison intact.
Verse steps stay within the displayed chapter; chapter controls remain available.

## Selected-word Hebrew, Greek and Aramaic study

Owner request from 2026-11-05, expanded to Greek and Aramaic on 2026-11-09:

- [ ] Offer Search Selected Word from reading/parallel text selection through a
  native context menu and keyboard command, preserving the originating module,
  passage and Unicode text.
- [ ] Offer exact surface-form search in all three languages. Define how vowel
  points and cantillation marks are handled for Hebrew and Aramaic when
  present, and how accents and breathings are handled for Greek. Offer
  lemma or Strong’s search when the module supplies the relevant metadata.
  Label surface-form and lexical searches separately and reuse shared search
  modes, bounds, cancellation and copied results.
- [ ] Show references with contextual previews and open results in verse-by-verse
  comparisons, including the stacked single-verse view planned above.
- [ ] Verify RTL Hebrew and Aramaic, LTR Greek, combining marks, source identity,
  keyboard and VoiceOver operation, and modules without lexical metadata.

The Mac reader and both Mac comparison layouts offer a Search Selected Word
menu for one visible token. Surface-form search passes the selected Unicode to
the existing phrase mode. The shared worker reads SWORD's verse word attributes
and returns source-bound lemma choices, preserving literal SDK search terms.
Modules without reliable metadata still offer surface search. The lookup can
match unmarked selections to marked SDK words when the matching records agree.
Accent-insensitive surface search, keyboard shortcuts and native Linux
integration remain open.
