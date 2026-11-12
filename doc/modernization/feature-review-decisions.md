# Feature review decisions — 2026-11-11

The owner reviewed the 15-category feature inventory for the modern Xiphos rebuild. This document records **scope preferences**, not implementation status or permission to delete code. Refer to [feature/workflow acceptance matrix](parity-matrix.md), [current status](status.md), and [native UI to-do](native-ui-todo.md) for implementation and acceptance evidence.

## Decision rule

**Keep** all items in the reviewed categories below unless explicitly listed as **Refine** or **Defer** in the exception tables. This rule includes every individual feature listed in the owner's review; no features were marked **Remove**.

The reviewed categories are:
1. Bible Reading & Navigation
2. Translations & Parallel Bible Comparison
3. Bible Search
4. Original Language Study
5. Commentaries & Reference Material
6. Personal Study Tools
7. Workspace & Windows
8. Bible Library & Downloads
9. Display, Themes & Accessibility
10. Export, Print & Sharing
11. Preferences & Profiles
12. Sync & Device Continuity
13. Updates, Installation & Releases
14. Bug Reports & Diagnostics
15. Metrics & Application Health

## Refine — retain capabilities; specifications remain open

| Category | Feature | Decision / open design |
| --- | --- | --- |
| Bible Search | Cancel searches and export results | **Refine**: retain both cancellation and export; review progress/Stop behavior, export formats, selected-vs-all scope, and presentation before final acceptance. |
| Metrics & Application Health | Local performance dashboard | **Refine**: retain a local view; determine live vs historical metrics, presentation, retention and diagnostic export. |
| Metrics & Application Health | Optional remote developer metrics dashboard | **Refine**: evaluate operational costs, access, aggregation, hosting and whether to include in initial release. No remote collection without explicit opt-in. |
| Metrics & Application Health | Protect passages, searches, and notes from telemetry | **Refine** the safeguards, **not** the protection requirement: never collect sensitive study content. Specify redaction, data minimization, inspection/deletion and tests. |

## Defer — no initial-release gate

| Category | Feature | Decision |
| --- | --- | --- |
| Personal Study Tools | Import AndBible bookmarks and notes | **Defer** |
| Export, Print & Sharing | Share passage navigation through BibleSync | **Defer** |
| Export, Print & Sharing | Open supported external sword:// links | **Defer** |
| Sync & Device Continuity | BibleSync live passage sharing | **Defer** |
| Sync & Device Continuity | BibleSync speaker and audience modes | **Defer** |
| Sync & Device Continuity | Privacy and session controls | **Defer** (BibleSync-related scope, not application privacy/security controls) |

**Defer does not mean remove**: preserve existing implementations and data compatibility unless a later explicit decision authorizes cleanup. In particular, do not eliminate general privacy settings or consent controls because BibleSync privacy controls were deferred.

## Explicitly retained future scope

The owner marked **Keep** for future synchronization of notes/bookmarks, personal settings synchronization, Apple Handoff and mobile/tablet native interfaces. These remain **future roadmap** capabilities, not claims that they are implemented or required before the first desktop release. Similarly, publisher-authorized translation sources remain subject to lawful licensing and agreements.

## Operations / developer tools retained

- **Updates:** native installers, manual/automatic checking, verified updates, release notes, preservation of profiles/modules, signing and recovery.
- **Bug reports:** Help > Report a Bug, in-app description, optional screenshot/diagnostics, preview/redaction, destination/tracking reference and offline log export.
- **Metrics:** local startup/memory/search/module/error measurements; optional anonymous opt-in metrics and a potential developer dashboard; user-controlled clear/export/disable and no study-content telemetry.
- Details are in [native-ui-todo.md](native-ui-todo.md).

## Guidance for implementation

1. Prioritize desktop native UI and shared-core completion without regressing Keep requirements.
2. Refine the four open decisions with the owner before final UI/telemetry requirements.
3. Move the six Defer items out of first-release acceptance criteria, without removing already-working code or compatibility/data-safety checks.
4. Never equate Keep with shipped, or Defer with removed.
