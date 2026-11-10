# Development roadmap

Updated 2026-11-05. The final product name is undecided; Xiphos remains the working
name. Earlier percentage estimates are historical, not current completion claims.

## Current work queue

1. Native Mac Settings/navigation and other-screen review; appearance and reading themes.
2. Selected-word Hebrew, Greek and Aramaic search, word/lemma choices and
   reference-to-comparison workflows.
3. Stacked single-verse comparisons alongside the current parallel chapter matrix.
4. Broader translations, typed localized-reference input, RTL/VoiceOver/keyboard acceptance.
5. Remaining reading/editor/export/migration workflows and broader real-world module/profile coverage.
6. Native Windows/frontend work, physical Linux validation and macOS 27.0 deployment acceptance.
7. Release dependency resources/notices, signing/installers and upgrade testing.

The detailed [native UI checklist](native-ui-todo.md) records owner requests and
platform expectations. [Status](status.md) distinguishes implemented scope from
acceptance. Keep shared-core ownership, cancellation, data safety and compatibility
checks active as services change. Further devotional expansion remains deferred.
Do not infer that an item is complete because the old implementation was removed.

## Future roadmap

- Publisher-approved sources for translations without SWORD modules, including
  desired ESV/NASB editions where appropriate agreements allow. Extend the shared
  source/catalog boundary; provider authorization, delivery and offline capabilities
  must be designed before enabling them. No website text import is implied.
- Personal-data sync for bookmarks, annotations, preferences and study material,
  with conflict/recovery behavior. Current BibleSync is passage sharing, not this feature.
- Apple Handoff for continuing study across Apple devices, using stable study
  identities and native presentation while keeping the shared core portable.
- Phones/tablets and visionOS: reuse core services with device-appropriate native
  layouts and accessibility. These are roadmap targets, not implemented frontends.
- Product naming/branding decision before release; avoid a premature global rename.

[Historical scope and detailed requirements](history/shared-core-roadmap.md)
retain the original plans. Expensive or optional expansion remains a bounded
future decision; no work is automatically authorized by the presence of this list.
