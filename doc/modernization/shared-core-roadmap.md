# Shared core: remaining scope

Updated 2026-10-30. This is a planning map, not an acceptance score.
The full project remains roughly 8% (6–10%), with zero whole parity groups
accepted. There is no evidence-based core-only completion percentage yet.
A tested worker/reading foundation does not mean the application core is almost
finished. Whole workflows and data compatibility remain the acceptance units.

## Current owner priorities (2026-10-30)

Defer further devotional work and retain its checked date/read services. Focus
core work on parallel reading, bookmarks/annotations, editing, module installation,
sync, complete session persistence and broader compatibility testing. Start with
parallel reading; develop persistence foundations alongside stateful services,
and run compatibility checks throughout rather than treating them as a final step.
Publisher-approved translation providers remain future installation/library scope;
this priority change does not select a final product name or expand frontend work.

First parallel foundation: [retained contracts and next slice](parallel-reading-workflows.md).
Complete detached orchestration and named-set helpers are now characterized;
shared layout arithmetic is reused by the retained renderer (`ca3a90a`). Native
aligned snapshots, saved-set repositories and full parallel workflow remain open.
This direction supersedes earlier next-step notes that resume devotionals.

| Core area | Current evidence | Remaining shared work |
|---|---|---|
| Engine ownership and transport | FIFO worker, copied C/C++/Swift results, shutdown draining | Cancellation for other active operations, native Windows compiler/SDK validation, stable dynamic packaging |
| Scripture and commentary | Reference choices, chapter/navigation/history, sparse/linked sources and options | Complete display decorations, corpus/locale/versification workflows and restoration |
| Dictionary and lexical content | Lookup/key steps/history, shared casing, lexical HTML/CSS and notes | Complete lookup/hover/search families and full corpus/locale acceptance |
| General books | Tree/navigation/DisplayLevel documents, scoped restore, note/image clicks, bounded inline resources/module CSS, shared geometry and combined note numbering | Native sizing/global CSS/assets/formats, complete semantic links/corpus acceptance, book windows/tabs and saved workflows |
| Parallel reading | Complete detached orchestration and named-set helper characterization; shared retained layout arithmetic | Actual SDK alignment oracle, owned ordered snapshots, parallel navigation/options, named sets and persistence |
| Search | Bible/commentary basic + exact attribute modes, explicit intervals/complete last scope, private annotation context, advanced word dialects, bounded ordered batches, complete per-group result generations, stale-safe C/Swift reads, bounded selected/all text and restricted HTML export with C/Swift adapters, explicit bounded PNG embedding, native Mac text/HTML save/copy, progress/cancel, explicit saved-list import/application/edit/removal with verified snapshots and native Mac prefill | Remaining advanced module/range setup, broader URI/parallel/BibleSync abbreviation consumption, actual indexed execution, mixed families, advanced controls/settings, complete preview/export and full backend/corpus acceptance |
| Library and installation | Module metadata/discovery subset | Favorites/hiding/grouping, sources/install/uninstall/index/cipher workflows; future translation providers and licensed offline downloads beyond SWORD modules (see below) |
| Bookmarks and annotations | Pending | Hierarchy/order/colors, XML round trips, AndBible import, rendering precedence |
| Writing and editing | Pending | Personal commentary, study pads, journals/prayer lists, compatible document storage |
| Settings and sessions | Explicit scoped option import/save and book restore subsets | Complete profile/session/tab restoration, lossless migration/backup and unknown-field preservation |
| Sync and external actions | Internal URI routing subset | BibleSync modes/privacy, IPC and full semantic action/export policies |

Native controls, accessibility, clipboard/print dialogs, window layout and platform
packaging also remain, but are frontend responsibilities rather than a second
implementation of these services. The detailed requirements remain in the
[parity matrix](parity-matrix.md). Implement shared services in vertical slices
with a consuming frontend to expose boundary problems early. Existing GTK3 is
the behavior/design reference; keep it operational and retain retirement gates.

Devotional date selection now has a shared C++11 policy consumed by retained
rendering, with an explicit safe fallback for invalid month indexing (`f0cd37d`).
See the [contract and next owned-service boundary](devotional-workflows.md).
Explicit devotional reads now have owned worker/C transport; native selection
state, URI integration and presentation remain pending.

## Future translation sources beyond SWORD modules

Owner requirement, recorded 2026-10-30: eventually support translations such as
ESV and NASB even when no compatible SWORD module is available. This is planned
scope, not implemented functionality or a promise that a publisher will grant
full offline distribution rights.

Extend the shared core with a translation-source boundary supporting existing
SWORD modules, publisher-approved online providers, authorized offline packages
and imports of supported, licensed files. Keep the existing single SWORD-owning
FIFO worker and shared navigation/history/session policies; do not build separate
study engines in each frontend. Any wider provider execution model needs its own
ownership/cancellation design before implementation.

A common catalog should identify translation, edition, provider, language,
versification, attribution and access state. Reading and study tools should use
shared references and results while exposing actual capabilities: online access,
offline availability, permitted caching, local/provider search, notes/crossrefs
and export. Do not promise identical search or annotations when a provider cannot
supply them. Plan for authentication, secure credential storage, bounded caching,
network failures, download verification, cancellation, updates and removal.

Distinguish online access from full offline download. As checked 2026-10-30,
[Crossway's ESV API](https://api.esv.org/) limits local storage to 500 verses or
half a book, whichever is less; full offline distribution requires additional
permission. [API.Bible's official catalog documentation](https://docs.api.bible/)
lists NASB, but application eligibility, edition and offline rights require
provider-specific verification. Recheck terms before implementation and release;
keep publisher attribution and licensed text separate from public source code.
Use original-prose fixtures for automated tests.

Sequence: define provider/capability contracts alongside library installation;
prove a non-SWORD provider with original-prose fixtures; integrate an authorized
online source; add full offline packages only where the applicable license permits.
Acceptance includes shared navigation/history, reference mapping, correct
attribution, cache/download limits, unavailable-network behavior, cancellation and
preservation of existing SWORD workflows. This remains future scope and does not
change current parity acceptance.

## Future product name

Owner direction, recorded 2026-10-30: the finished product may be named
**Swordreader** or **Sword Reader** rather than Xiphos. These are candidates;
no final name or immediate rename is selected.

Keep user-facing branding separate from stable technical identifiers. A later
branding pass can update display names, icons, menus, help, installer labels and
website text. Centralize new branding metadata as packaging matures rather than
scattering a final name through services. Internal C++ namespaces, ABI symbols
and repository names do not need to match the displayed product name.

Choose permanent application/package identifiers and distribution identity before
public release where possible. Changing bundle/package IDs, credential-store
identities, settings paths, update channels or file associations after release
requires an explicit compatibility/migration plan. Preserve existing user data,
legacy profile readers and supported link formats; changing the product name does
not require renaming the SWORD engine or breaking sword:// links. Preserve all
inherited copyright, license and attribution notices. Record the final name and
its packaging/upgrade decisions when the owner selects it.

Current checkpoint: retained result generations C++ `37d9004`, C `155e0cb`, Swift
`d1a15df`; selected visible text export C++ `3faadea`, C `6f0e3c1`, Swift `ec26889`.
See [retention](search-retention-workflows.md) and [text export](search-text-export-workflows.md)
for actual bounds/ownership. Complete returned text is checked at C++ `b7dafd4`, C `1bb2615`, Swift `80589a4`;
native Mac text actions at `5d52a11`. Restricted HTML C++ `1b327d9`, C `e7c141e`, Swift `e8a8ec8` share configured
sources/selection. Explicit static PNG embedding is now shared at `4fecce8` /
`374147d`; scoped native Mac HTML save/copy is checked at `f89c585`. Rich clipboard
plain-text fallback, broader resource/link policy and browser/visual acceptance
remain distinct gates.
The [resource reuse audit](resource-export-design.md) records shared private
filesystem extraction `ff46f9b` and NUL-path guard `7e9ff93`; later shared static PNG export reuses that
source infrastructure. Exact [item formatting](export-workflows.md)
is shared with GTK; advanced header/first-row quirks are characterized. Actual
indexed execution, mixed families, advanced controls/settings and saved workflows
remain partial work.
Book link/session, native
sizing and corpus acceptance remain tracked work alongside the existing reading
seams. Parallel reading and complete persistence remain later core services.
Reorder only with evidence or owner steering; do not block all native work until
an imagined final core freeze. Current evidence/active task lives in the
[shared handoff](handoff.md) and [Mac handoff](handoff-macos.md).


The latest saved-list slice now has shared verified single-item editing/removal,
owned C/Swift transport and explicit native Mac controls. See the
[saved-list contract and next abbreviation source audit](saved-search-lists-workflows.md#next-shared-seam-module-abbreviation-resolution-source-audit-2026-10-22).
Installed-ID collision exclusion, SDK registration order and post-range-setup
resolution must be characterized together before implementing aliases. This
source audit accepts no new behavior or whole parity group.


Shared execution-time module abbreviation resolution now preserves raw request
provenance and actual retained/export source IDs, with copied C/Swift transport
and Mac/Linux hit consumers. Full Mac/portable and scoped sanitizers are recorded
in the [current contract](module-abbreviations-workflows.md). Typed single-passage URI
consumption now reuses the catalog. showRef exact-module/main-module fallback
shares retained C++11 dispatch with native contextual previews. Complete
sword_uri callback characterization now supports unknown single-passage fallback
and installed Bible contextual range previews with aggregate caps/disclosure.
Unknown/non-Bible range navigation, anchors and keyed URI families remain open; broader families/corpus remain open.
