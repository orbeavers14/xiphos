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
