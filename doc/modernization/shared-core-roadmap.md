# Shared core: remaining scope

Native presentation follow-up: [Mac/Windows Settings and screen review](native-ui-todo.md).

GTK source retirement (2026-11-04): owner requested maximum legacy removal.
The GTK3 application, old editors/backend, legacy Windows distribution and
CPack/CMake install machinery are removed. Native core tests use61 immutable
reference snippets with SHA256/provenance, not source extraction at configure
time. No legacy build option remains. Translations, reusable assets, help and
notices remain for native reuse. Windows WinUI3 is still planned; missing
Settings/localization/frontend parity remains open despite code removal.
See [removal ledger](legacy-removal.md).


Mac-scoped corpus/hardening checkpoint (2026-11-03): six real modules,521 Pilgrim
scripture links, KJVA Tobit and three actual indexed module families now pass.
Asset/local-install/maintenance cancellation is wired through core/C/native UI
with a safe pre-publication gate; uninterruptible SDK indexing drains first.
Final native257 passes/two expected skips; sanitizer and portable models plus
real corpus checks pass. A separate development bundle closes17 SDK-library
references at its actual macOS27 floor. Windows/physical Linux, older macOS and
redistributable packaging remain open. See [current Mac evidence](handoff-macos.md#mac-corpus-and-hardening-checkpoint--2026-11-03),
[corpus](real-corpus-validation.md) and [runtime scope](native-runtime-packaging.md).


Owner clarification (2026-11-03): shared-core hardening remains active alongside
feature development. Check relevant compiler/SWORD compatibility, C/FIFO ownership,
cancellation and persistence/data-safety behavior as affected services change.
This is shared-engine work, separate from frontend presentation. Final packaging,
full physical-platform/release acceptance and frontend acceptance remain closing
gates. Older instructions to defer the entire hardening workstream are superseded;
these gates are bounded acceptance requirements, not an indefinite extra feature.

Owner priority update (2026-11-02): work through search/content, bookmarks and
annotations, parallel saved workflows, editing, then library organization. Remaining
BibleSync networking/group/presence/chat and external IPC/actions are a stretch
goal; delivery is undecided and they are not a core completion requirement.
Preserve existing checked implementations without expanding them. Persistence and
legacy migration remain important compatibility work, separate from this sequence.
Personal data sync and Apple Handoff retain their separate future-roadmap status.

Legacy settings/session checkpoint (2026-11-01, `4bfb38c`): shared read-only snapshots,
strict all-tab conversion, explicit module mapping, exact-original migration
packages/restore and Mac preview/export/next-launch integration now exist. See
[the migration contract](legacy-settings-session-migration.md). Unknown settings,
comments and encodings remain recoverable; unsupported appearance/locale values
are disclosed as preserved, not applied. Representative real-user/cross-host and
whole-parity acceptance remain open. This supersedes the earlier migration TODO.


Priority checkpoint (2026-11-01): [native BibleSync controls and two independent
local clients](native-sync.md) are checked at `526cd1c`; physical LAN/sleep/interface
acceptance remains open. [Legacy repository review](legacy-repository-import.md)
at `9a78747` begins the next storage-compatibility priority with read-only preview and
explicit disabled HTTPS-source setup. Full legacy profile migration remains open;
this supersedes older notes that native sync controls are entirely pending.


Native recovery/management checkpoint (2026-11-01, `78d367a`): macOS now
schedules shared recovery snapshots, offers startup Recover/Discard, reconstructs
reading windows and unsaved source drafts, and preserves original-file conflict
checks. Native Module Library screens expose ordered HTTPS sources/cached catalogs,
explicit transfers, local installation and staged maintenance. First launch creates
an explicit private module library when discovery is empty; existing discovered
libraries retain reading access until an explicit managed folder is selected.
See [native integration and limits](native-recovery-management.md). Native sync
controls, legacy source import, capable-SDK positive indexing, encrypted-module
correctness and physical Windows/Fedora acceptance remain open. No whole-parity
acceptance increase. This supersedes older statements that Mac presentation is next.


Safe sync networking checkpoint (2026-11-01, `04914ca`): an optional shared
BibleSync adapter now owns network configuration, bounded polling, transmit/echo
controls and copied C/FIFO event batches. A pinned downstream SDK patch applies
privacy before initial messages, bounds receive/peer state and rejects truncated
outbound payloads. Full native230 passes+one index skip (231 registered); scoped
instrumented SDK+adapter ASan/UBSan5/5 and TSan5/5. Actual UDP fixtures redirect all
SDK sends to loopback; physical two-device/LAN interoperability and native sync
controls remain open. Networking is OFF by default and rejects an unpatched SDK.
See [the SDK contract](../../dependencies/biblesync/README.md). Recovery/autosave
integration and native management presentation remain next; no whole-parity claim.


Repository/maintenance and recovery/sync checkpoint (2026-11-01): shared ordered
repository profiles and offline catalog persistence now exist, with owned C/FIFO
transport and endpoint cache invalidation. Staged remove/cipher configuration/
index maintenance now reuse verified installation transactions and reject in-use
removals. Recovery snapshots retain exact editor source and a previous valid
session/draft snapshot; sync mode/sender/privacy/echo/routing policy is shared.
These are core services: native management/autosave/recovery presentation, legacy
source import, positive index creation on a capable SDK, real BibleSync transport
and physical Windows/Fedora acceptance remain open. See
[the contracts and limits](repository-management.md). No whole-parity acceptance.


Updated 2026-10-30. This is a planning map, not an acceptance score.
The full project remains roughly 8% (6–10%), with zero whole parity groups
accepted. There is no evidence-based core-only completion percentage yet.
A tested worker/reading foundation does not mean the application core is almost
finished. Whole workflows and data compatibility remain the acceptance units.

## Current owner priorities (2026-10-30)

Defer further devotional work and retain its checked date/read services. Focus
core work on parallel reading, bookmarks/annotations, editing, module installation,
complete session persistence and broader compatibility testing. Start with
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
| Parallel reading | Real-SDK complete interpolation oracle, cloned row plans, bounded owned chapter matrices/C transport and atomic per-tab selection/chapter navigation | Checked named-set repository native management/activation, complete saved-session integration and broad multi-canon/corpus acceptance |
| Search | Bible/commentary basic + exact attribute modes, explicit intervals/complete last scope, private annotation context, advanced word dialects, bounded ordered batches, complete per-group result generations, stale-safe C/Swift reads, bounded selected/all text and restricted HTML export with C/Swift adapters, explicit bounded PNG embedding, native Mac text/HTML save/copy, progress/cancel, explicit saved-list import/application/edit/removal with verified snapshots and native Mac prefill | Broader interval/attribute corpus validation, complete assets/export formats, broader URI/parallel abbreviation consumption, advanced controls/settings and real backend/corpus acceptance; mixed families and actual keyed indexed execution are now checked |
| Library and installation | Shared catalogs/planning, staged local/HTTPS installs, ordered source/cache storage, maintenance and credential-free legacy HTTPS review | Favorites/hiding/grouping native consumption of the shared organization store, broader legacy source compatibility and cross-host acceptance; actual indexing/encrypted fixtures now pass; licensed providers remain future scope |
| Bookmarks and annotations | Owned XML hierarchy/order/colors, revision-checked edits, tag maps, shared highlight priority and verse-note storage | Remaining annotation/color compatibility and full bookmark workflows; shared non-destructive import collision review and explicit skip/keep policies now exist, with real backup/corpus acceptance pending |
| Writing and editing | Revision-checked documents, shared rich formatting/undo, supported SWORD module edits and recovery drafts | Journal/prayer organization, broader legacy rich-markup/link round trips and full workflow acceptance; validated internal study-command links now round-trip |
| Settings and sessions | Scoped profile storage, native all-tab capture/restore/recovery, explicit legacy settings/tabs/modops conversion with exact source packages/restore and module mapping | Representative real profiles, external-reference/workflow review, unsupported appearance controls and cross-host acceptance; complete selected-directory asset copy now exists |
| Sync and external actions (stretch goal; delivery undecided) | Shared BibleSync policy/safe networking, native Mac routing and two independent loopback clients; internal URI routing subset | Optional future work: physical LAN/sleep/interface interoperability, group routing/presence/chat, IPC and remaining semantic actions; personal data sync is separate future scope |

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

First boundary delivered (2026-10-31): the shared [library catalog and installation
planner](library-installation.md) now uses provider-scoped identities, explicit
capabilities/access/permissions, owned installed/local SWORD snapshots and an
original-prose non-SWORD catalog fixture. It does not yet dispatch provider passage
reads, install packages or contact a licensed publisher.

Sequence: define provider/capability contracts alongside library installation;
prove a non-SWORD provider with original-prose fixtures; integrate an authorized
online source; add full offline packages only where the applicable license permits.
Acceptance includes shared navigation/history, reference mapping, correct
attribution, cache/download limits, unavailable-network behavior, cancellation and
preservation of existing SWORD workflows. This remains future scope and does not
change current parity acceptance.

## Personal data sync across devices

Owner-requested future scope, 2026-11-01. This is separate from BibleSync live
passage sharing and local autosave/recovery; neither currently synchronizes user
files between devices. No data-sync implementation or backend is selected yet.

Planned scope:

- Bookmarks, folders, tags, highlights and personal notes.
- Study pads, journals and prayer lists, including supported document organization.
- Reading position, named parallel sets and selected portable preferences.
- Offline edits, stable item identities, schema/version compatibility, conflict
  resolution, deletion tombstones, recoverable history and explicit restore.

Keep merge rules, validation and versioned records in the shared C++ core.
Platform adapters provide transport, account/credential integration and background
scheduling. Evaluate a user-selected folder/file transport first using isolated
original-prose fixtures; select cloud/service options separately. Apple-only
transport must not become a requirement for Windows or Linux data compatibility.
Do not blindly synchronize live profile files, absolute paths, machine-specific
settings, secrets or in-progress recovery files. Define encryption, privacy,
authentication and backup behavior before enabling network synchronization.

Each device normally obtains Bible modules independently. Synchronize portable
module identities/preferences where useful, report missing modules, and preserve
provider licensing restrictions; this roadmap does not authorize copying licensed
module packages. Credentials and provider access remain device/account concerns.

Acceptance: two isolated clients on one host first, then cross-device/cross-platform
checks covering concurrent edits, delete-versus-edit conflicts, offline/reconnect,
duplicate/reordered delivery, interrupted transfer, schema upgrades and recovery
without silent data loss. UI/account choices and transport remain future decisions.

## Apple native Handoff

Owner-requested future platform enhancement, 2026-11-01: continue a reading or
study activity between supported Apple devices. Plan native Apple Handoff as a
small activity-continuation adapter over shared reference/session contracts.
It is distinct from continuous BibleSync passage following and from personal data
sync. No Handoff implementation or iPhone/iPad frontend is currently delivered; these
frontends are now planned under [phones, tablets and visionOS](#phones-tablets-and-visionos).

Start with current module/provider identity, passage/reference and a bounded
portable activity description. Resume using shared navigation and capability
checks. If the destination lacks a module, access right or compatible activity,
explain the limitation and offer an appropriate supported destination. Keep private
notes, document contents, credentials and local filesystem paths out of advertised
activity payloads; a document continuation may use a stable identity only after
personal data sync can safely resolve it.

Before implementation, verify Apple platform/API support, app identity, signing,
entitlements and receiving-app requirements. A Mac-to-iPhone/iPad workflow needs
an appropriate receiving app and is additional frontend scope, not assumed part
of the existing three desktop frontends. Device/account requirements and behavior
must be checked on actual supported devices; isolated clients cannot certify
Apple Continuity behavior. Data-sync transport and Handoff should remain usable
independently where their respective prerequisites are met.

Sequence: define shared portable continuation records; implement and test Mac
activity publication/receipt; add receiving Apple frontend support if selected;
then validate physical-device continuation, missing-content handling, privacy,
startup and compatibility with future product naming/app-identity decisions.

## Phones, tablets and visionOS

Owner-requested future platform scope, 2026-11-01: extend the application beyond
its three desktop frontends to phones, tablets and visionOS. Plan iPhone/iOS,
iPad/iPadOS, Android phones/tablets and Apple visionOS as future targets. Android
is the planning assumption for non-Apple mobile devices; toolkit, minimum OS
versions, delivery order and release dates remain unselected. No mobile or
visionOS frontend is implemented or accepted by this roadmap addition.

Reuse shared C++ reading, search, bookmarks/annotations, editing, library,
session/recovery and future data-sync services through owned platform adapters.
Retain one SWORD-owning FIFO worker per process. Audit SDK availability/builds,
sandbox storage, app lifecycle, memory limits, cancellation, background scheduling,
module acquisition and provider licensing on each target before porting. Desktop
filesystem access and indefinitely running background work are not assumptions.

Phone layouts should prioritize reading and navigation with accessible touch
controls and focused study/editing flows. Tablet layouts should adapt parallel
reading, notes and multiple study panes to available space and input methods.
visionOS needs a native spatial presentation and explicit interaction/accessibility
design rather than assuming the desktop window composition is sufficient. Share
application behavior while defining a target-specific feature/acceptance matrix;
a small screen does not justify silently dropping a supported data workflow.

Connect these targets to [personal data sync](#personal-data-sync-across-devices)
for portable user data and [Apple Handoff](#apple-native-handoff) where the target
actually supports it. Verify Handoff availability separately for each Apple target;
visionOS support must not be assumed. Keep cross-platform data compatibility
independent of Apple-only features. Final app identity, distribution/signing and
upgrade/migration behavior need target-specific decisions.

Sequence: prove shared-core/SDK builds on target toolchains; deliver a minimal
reading/navigation/library vertical slice; add notes, editing and recovery;
integrate data sync and supported Apple activity continuation; validate remaining
workflows. Acceptance needs device/simulator coverage as appropriate, physical
input/accessibility checks, rotation/resizing, suspend/resume/interruption,
offline/reconnect, safe upgrades and content/license handling. These targets expand
future project scope without changing current desktop parity or core completion.

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

## Core priorities checkpoint, 2026-11-01

[Bounded AndBible import, XML interchange, shared verse decoration and actual
indexed/encrypted maintenance](andbible-composition.md) now have generated-fixture
evidence. Remaining: native import/conflict review and composition integration,
real backup/version corpus, generic dictionary/general-book search, additional
semantic-link/rendering coverage, broader canon/RTL visual acceptance and physical
Windows/Linux validation. These are separate from licensed providers/data sync.

Typed keyed-search checkpoint (2026-11-01): [standalone dictionary/general-book
queries](keyed-search.md) now reuse shared execution/cancellation with explicit
result kinds. Full mixed-family batches, typed preview/export and retained keyed
scopes remain open; this supersedes statements that no keyed search service exists.

Exact keyed previews now dispatch current dictionary/indexed-tree content with
explicit source kind, missing-target rejection and copied HTML bounds; see the
[keyed preview contract](keyed-search.md#exact-keyed-previews). Native preview
consumption, typed batch generations and source-specific exports remain next.

Mixed-search checkpoint (2026-11-01): [opt-in typed batches and complete-source
exports](mixed-search.md) now cover Bibles/commentaries/dictionaries/indexed-tree
books, retained identities and verse-scope isolation. Native typed consumption,
keyed scopes/indexed/attribute/assets and broader compatibility remain next.
