# Xiphos native modernization

This fork of [CrossWire Xiphos](https://github.com/crosswire/xiphos) is building
a true 1:1 functional and design adaptation with a shared, platform-independent
C++ application core and native frontends:

| Platform | Target frontend | Current implementation |
|---|---|---|
| macOS | SwiftUI / AppKit | Reading/history/options, note/reference previews, lexical/direct dictionary lookup and key steps |
| Windows | WinUI 3 | Planned; legacy GTK application retained |
| Linux | GTK4 / WebKitGTK | Reading/commentary/history/options, dictionary, contextual previews, profiles and shared-worker windows; legacy GTK3 retained |

The goal is to preserve all existing Bible-study functionality, pane layout,
module behavior, data and established workflows while adding appropriate
native menus, accessibility, keyboard controls, appearance and platform
integration. Shared application behavior belongs in C++; platform adapters
must not develop independent engines. SwordKit may become a Swift adapter
through an audited adapter to the shared core.

## Status

The shared core and macOS preview now cover module/verse/chapter reading,
Bible history, scoped reading options, engine-preference import/save and
note/reference-list previews. Dictionary work includes characterized lexical
selection, native preferences and click results, direct lookup, key steps and
shared separate history commands, native dictionary Back/Forward/Clear and a
nearby-key chooser. Main clicks use shared Unicode casing; the macOS host
initializes its environment locale before engine startup. Strong's/morphology
hovers and footnote bodies use a separate information sidebar; cross-reference
hovers follow a core-owned per-window list/information preference. Explicit lexicon XML import/save uses native file pickers. Other hover families,
complete persistence and established pane composition remain in progress.
Synchronized commentary now includes paired history, sparse/linked chapter
reading, shared pane-link routing and per-module chapter defaults/import/save.
Core commentary snapshots include owned headings and shared display preferences.
The native commentary pane sits above dictionary content; complete heading transformations, detached
windows and broader visual/corpus acceptance remain pending.

The macOS suite passes 71 checks. The new portable Linux/core suite passes 55 checks on macOS; its GTK view changes still need Fedora validation. The prior native Fedora checkpoint passed 51 core/adapter checks and six desktop smoke modes. Both are supplemented by scoped sanitizer,
strict-language and temporary-fixture GUI checks recorded in the status ledger.
The legacy app retains its existing UI and delegates selected seams to shared
helpers. Routine development and testing stay local to conserve GitHub builds;
platform CI and full acceptance are reserved for meaningful checkpoints.

The last weighted modernization estimate was **about 8%** (planning range 6–10%) before the Linux catch-up; it needs reassessment.
**0/38 feature groups** have accepted parity across all native platforms.
See the status page for the weighted estimate, ordered next steps and
[source reuse decisions](doc/modernization/status.md#reuse-from-related-projects).
No native platform has full functional, visual or workflow parity yet.
The existing GTK application remains the complete implementation during the
migration. See the [current status and sign-off rules](doc/modernization/status.md),
[feature parity matrix](doc/modernization/parity-matrix.md), and
[architecture and dependency/license audit](doc/modernization/architecture.md).

## Build and development

**Starting a new chat or switching computers?** Read [AGENTS.md](AGENTS.md) and
the [shared cross-platform handoff](doc/modernization/handoff.md). It contains
the common goal, coordination rules, current checkpoints, per-OS next tasks and
a starter message to paste into each chat. Pull the latest repository before
working, and push updated handoff evidence when finishing.

* Existing GTK application: [INSTALL.md](INSTALL.md).
* Shared core and engine tests: [src/core/README.md](src/core/README.md).
* Linux dependency setup and automated checks: [platforms/linux/README.md](platforms/linux/README.md).
* Native macOS preview and adapter tests: [platforms/macos/README.md](platforms/macos/README.md).
* Translations: documentation in [doc](doc).

Initial PRs #1–80 are merged after local review and integration checks; later
checkpoints include the Linux core setup in PR #82. The existing legacy and
native-core hosted workflows remain paused to conserve GitHub credits. The Linux
core workflow is manual-only; routine builds/checks run locally. Keep changes
small and reviewable. Sequential checked commits to this fork's `master` are
owner-authorized; use short-lived branches/PRs to coordinate overlapping chats.
Next are representative module/profile validation, remaining rendering/hover and
storage behavior, and small equivalent reading panes on GTK4 and WinUI 3 before
declaring the core complete. See each OS handoff for its bounded next task.
See [Linux evidence and acceptance limits](doc/modernization/handoff-linux.md).
Each milestone needs reviewable changes, regression tests and parity evidence
before the corresponding legacy files can be retired.

Use the repository's `.clang-format` when editing C/C++ source. The upstream
user manual and screenshots remain reference material until replacement
behavior and documentation are validated.

## Repository cleanup

Generated inventories and build outputs belong under the ignored `build/`
directory. Keep their generators, regression tests, current architecture,
parity requirements and compact acceptance evidence. After an accepted
replacement has no remaining legacy callers, remove obsolete code and
associated build/dependency references in the same reviewed cleanup change.
Git history and PRs retain the detailed implementation record.

## License and acknowledgments

Preserve [COPYING](COPYING), [AUTHORS](AUTHORS) and existing source notices.
Inspected Xiphos source grants GPL v2 or later; dependency and module licenses
have their own requirements. See the dependency/license audit before packaging.

Xiphos started as the SWORD Project's Cheatah program. Its development includes
ideas and code from BibleTime and the SWORD Project's BibleCS program. Credit
remains with the Xiphos Development Team, CrossWire, and all contributors whose
work this fork builds upon.
