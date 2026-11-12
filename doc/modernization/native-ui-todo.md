# Native interface to-do list

Owner requirement,2026-11-04: review Settings and other screens against each
platform’s recommended native interaction patterns. Current functional screens
are intermediate; opening a setting should not routinely spawn another window.

## Mac

- [x] Provide one native Settings window with inline General, Appearance,
  Reading, Lexicons and BibleSync panels.
- [ ] Review Module Library, bookmarks/import, annotations, parallel-set management,
  migration and editor flows for platform-native navigation and window behavior.
- [ ] Verify standard menu shortcuts, keyboard focus/order, VoiceOver names,
  accessible status/error announcements, resizing and localized text expansion.
- [x] Add profile-scoped System, Light and Dark appearance preferences with
  immediate AppKit application and adaptive reading WebKit defaults.
- [ ] Make Bible reading, commentary, dictionary and other formatted/HTML or
  WKWebView content theme-aware; ensure verse highlights, search results, links
  and custom module styles remain legible in both appearances. Consider optional
  independent reading themes (including Sepia). Keep appearance/presentation
  decisions in the native UI, not the shared C++ core.
- [ ] Review the design against current Apple guidance before implementation;
  record scoped screenshots and actual workflow checks, not just build success.

### Mac follow-up

| Area | Implemented or verified | Remaining |
| --- | --- | --- |
| Passage find | Command-F and wrapping forward/backward search in the Bible pane. Offscreen WebKit match and miss checks passed. | Check shortcut routing, focus and VoiceOver. |
| Biblical map | Bundled sourced markers and offline PMTiles preview. Regional water, place symbols, labels and corrupt-archive fallback verified. Native install, reopen, removal and HTTPS download verified on 27.2. A bad checksum preserved the prior pack. | Broader regional coverage. See [map design](biblical-maps.md). |
| Study and comparison | Literal selected-word shortcut, translation visibility in both layouts, saved translation ordering, and persisted visibility per ordered module list. Native comparison and saved-profile checks passed. Hidden translation restored after a full app restart on 27.2. | Verify keyboard routing with selections in both panes. |
| Printing | Formatted HTML is the default. Plain text remains a separate action. The unattended PDF attempt stalled in AppKit. | Verify PDF or paper pagination and styling, the native print dialog, and plain-text output. |
| Packaging | JPEG and patched SWORD rebuilt targeting 27.0 with ICU and CLucene retained. The ad-hoc-signed development bundle passed dependency audit at a 27.0 floor and launched on 27.2. | Run on 27.0, then complete release signing, notarization and upgrade testing. |

See the [Mac handoff](handoff-macos.md) for validation details.

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
menu for one visible token, with literal and mark-insensitive surface choices. The shared worker reads SWORD's verse word attributes
and returns source-bound lemma choices, preserving literal SDK search terms.
Modules without reliable metadata still offer surface search. The lookup can
match unmarked selections to marked SDK words when the matching records agree.
GTK4 now exposes selected-word study in Bible and comparison panes with shared literal, mark-insensitive and lemma choices. The GTK shortcut is Ctrl+Alt+F. Physical keyboard and real-corpus native validation remain open.

## Application support, updates and metrics

Owner request, 2026-11-11. These are product requirements, not claims of
completed implementation. Share non-UI policy where portable and keep native
menus, dialogs and platform update integration in each frontend.

### In-app bug reporting

- [ ] Add **Help > Report a Bug** to native apps; include a concise issue form,
  optional reproduction steps, optional screenshot/attachments and a way to
  view the report before submitting.
- [ ] Offer explicit user-approved inclusion of app version, OS, module/plugin
  inventory and sanitized diagnostics. Never automatically attach Bible text,
  searches, notes, journals, Studypad, personal commentaries, user-written content,
  user file paths or content-derived identifiers. Explicit report text/attachments
  require preview and user approval; provide redact/review and export-only paths.
- [ ] Define a maintained submission destination (e.g. GitHub Issues or project
  intake), failure/offline behavior, duplicate/privacy guidance and a report ID
  or link after successful submission.

### Updating the application

- [ ] Add **Check for Updates** and version/release-notes access in Help/About.
- [ ] Design verified, signed release-channel update checks, manual download and
  installation with clear user consent; optionally configurable background checks.
- [ ] Use suitable platform-specific update mechanisms, preserving profiles,
  installed modules and recovery/rollback expectations across upgrades.
- [ ] Verify macOS signing/notarization and update flow, Windows installer/update
  flow, and Linux package-managed update behavior; support offline/manual updates.

### Metrics and diagnostics

- [ ] Clarify and deliver user-visible **local app health metrics**: startup time,
  memory/CPU use when available, module counts, search and render performance,
  error/crash counters and diagnostic export, with sensible retention limits.
- [ ] Keep aggregate **developer usage/quality telemetry** strictly optional,
  off by default, transparent and privacy-preserving. **Zero user-authored
  content** in telemetry, including notes, journals, prayer lists, Studypad,
  personal commentaries, annotations, bookmark labels, searches, drafts, paths,
  content hashes and any content-bearing crash/error payloads. No scripture
  selections or unique module content either. Use strict non-content event allowlists.
- [ ] Provide a Settings privacy control with inspect/clear/export/disable behavior;
  define retention, aggregation, transport and server access if remote metrics
  are approved. Do not depend on telemetry for core offline behavior.
- [ ] Investigate GitHub-first developer insights (Releases asset download totals,
  Issues for reviewed reports, repository traffic and Actions usage). These do
  **not** measure actual installed users or app usage. Any richer remote metrics
  require separate approved opt-in ingestion; never embed maintainer GitHub
  tokens in distributed apps. Keep workflows and operating costs bounded.
- [ ] Local metrics dashboard UI is undecided; measure what is useful before
  choosing presentation, history and retention. Review the current search
  interface and search/export behavior before proposing changes.
- [ ] Decide whether remote administrative dashboards are warranted separately
  from the local diagnostics UI; validate cost and maintenance burden first.

