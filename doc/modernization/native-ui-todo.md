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

- [ ] Add a focused single-verse comparison with each translation's name above
  its verse text in a vertical list. This complements the current chapter matrix.
- [ ] Offer stacked verse and side-by-side chapter layouts, saved module ordering,
  module visibility and previous/next verse navigation.
- [ ] Keep optional context, commentary, cross-reference and lexical actions
  accessible alongside the comparison without duplicating the shared engine.
- [ ] Adapt the presentation to native Mac/Windows navigation, narrow windows,
  RTL/mixed direction, keyboard focus and accessibility; extend to GTK4 as used.

Reuse the current shared parallel alignment/results and installed module catalog.
The screenshot is a behavior/layout reference, not a request to copy the website
or its text library. Licensed/provider translations remain the existing separate
roadmap work. No new comparison view is implemented by this planning entry.

## Selected-word Hebrew study

Owner request,2026-11-05:

- [ ] Offer Search Selected Word from reading/parallel text selection through a
  native context menu and keyboard command, preserving the originating module,
  passage and Unicode text.
- [ ] For Hebrew, expose exact surface-form search and deliberate handling of
  vowel/cantillation marks. Offer lemma/Strong's search when module metadata is
  available; label these choices clearly rather than assuming selected text is
  a lemma. Reuse shared search modes, bounds, cancellation and copied results.
- [ ] Show references with contextual previews and open results in verse-by-verse
  comparisons, including the stacked single-verse view planned above.
- [ ] Verify RTL selection, combining marks, source identity, keyboard/VoiceOver
  operation and cases where lexical metadata is unavailable.

Core word/lemma search exists. The native selected-text-to-search action is not
implemented by this planning entry; existing lexical links are separate input.
