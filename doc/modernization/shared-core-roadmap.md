# Development roadmap

Updated 2026-11-21. The final product name is undecided; Xiphos remains the working
name. Earlier percentage estimates are historical, not current completion claims.

## Current work queue

1. Windows native session save/load and tab ownership through shared persistence;
   remaining library, annotation/editor and management UI adapters.
2. Complete desktop acceptance of implemented selected-word study, chapter/stacked
   comparison, translation visibility, Settings, appearance and reading styles.
3. Broader translations and localized typed references; RTL, keyboard, VoiceOver,
   Narrator and Linux screen-reader checks. English plus eight partial catalogs
   does not mean complete interface coverage.
4. Remaining reading/editor/export/migration workflows and real-world module/profile
   coverage; finish native save/clipboard/print and interruption checks.
5. Windows x64/26H2 and deployment, Linux additional distributions and accessibility,
   and actual macOS 27.0 execution. Fedora 45 ARM64 desktop/RPM and Windows ARM64
   core/preview have recorded scoped checks already.
6. Dependency notices/resources, release signing/notarization, installers and
   cross-version upgrade/data retention; preserve the checked Fedora preview RPM path.
7. Review retained update, bug-report and diagnostics requirements before implementation;
   metrics UI remains undecided and remote non-content metrics default OFF.

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

## Owner-approved feature scope (2026-11-11)

The [feature review decisions](feature-review-decisions.md) retain the reviewed
Bible-study, native UI, library, release-support and diagnostics capabilities,
with four items requiring refinement and six deferred from the first-release
scope. No feature was marked for removal. Future mobile, personal-data sync and
Handoff remain roadmap items rather than initial desktop-release gates.
Preserve existing deferred implementations and data safety unless a later owner
decision explicitly authorizes removal.
