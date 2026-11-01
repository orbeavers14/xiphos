# Repository management, maintenance, sync and recovery

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


Shared-core checkpoint, 2026-11-01. The original GTK3 application remains the
reference; these services do not introduce a GTK dependency or another SDK owner.

## Saved repository profiles

`repository_store.hpp` / `repository_store.cpp` provide ordered, stable-ID HTTPS
sources, names, enable/disable, add/update/remove/reorder, and explicit catalog
refresh retention. Endpoint changes discard their old cache; renaming/disabling
preserves it. Deleting a source never uninstalls its modules. Shared value edits,
FIFO read/save and owned C handles let each frontend use the same implementation.
The C API can attach a fetched remote catalog and copy a cached catalog back to
an independent remote-install handle.

A caller selects the profile path. No implicit home discovery, credentials,
network calls or access acknowledgement are persisted. Offline catalogs retain
all provider identities, versions, capabilities and unknown/allowed/denied rights.
Timestamps are supplied Unix seconds, not proof of freshness. Cached destination
fingerprints remain subject to the existing installer checks. After a library
change, refresh installation state rather than bypassing its revision guard.
Enabled state is a saved preference: explicitly calling the existing transfer API
is still an explicit request, rather than an implicit background refresh.

The version-1 length-delimited format rejects unknown versions, malformed lengths,
trailing bytes, invalid endpoints/UTF-8, duplicate IDs, catalog hash mismatches and
endpoint/cache mismatches. Limits: 64 sources, 16,384 cached items in aggregate,
32 MiB serialized file, plus the existing per-catalog bounds. SHA256 expected-file
revisions reject observed external changes. Creation requires no existing file;
replacement requires its last revision. Staging is read-back verified and uses
the shared same-directory replacement helper. There is no interprocess lock or
power-loss/fsync guarantee. Future schemas are refused rather than erased.

## Staged module maintenance

`module_maintenance.hpp`, `local_install.cpp`, `Reader` and `StudyWorker` support:

- Preview with full-library revision, cipher declaration and actual SDK index state.
- Remove one module's isolated configuration/data directory, retaining siblings.
- Set or clear an existing CipherKey configuration; preserve other source lines.
  Existing indexes are invalidated before key changes. This stores the explicit
  key in SWORD's configuration, not an OS keychain, and does not prove the key is
  correct, grant a license, or return the key in result metadata.
- Build or delete a SWORD search framework. Build succeeds only if the SDK reports
  success **and** a usable framework exists. The selected local SDK cannot produce
  one; its failure/rollback path is checked, not positive indexed execution.

Updates/replacements already use the checked local/HTTPS installer; opaque version
strings are never ordered as semantic versions. Same/different-version replacement
still needs an explicit replacement request.

Maintenance reuses full-root bounds, isolated driver/layout checks, clone/hash
verification, same-parent two-rename commit, rollback and preserved-backup behavior.
Discovery cache files are removed in the staged root. A fresh SDK Reader and tab
reconstruction must succeed before the old state is released. SDK-owned search
results are invalidated without reusing generation IDs. The worker rejects removal
if a live tab's reading/commentary/dictionary/book/parallel selection or retained
history uses that module; the caller must close those tabs. Failed refresh rolls
back the filesystem and Reader. Unsupported/overlapping/shared layouts fail closed.

These operations run on the existing FIFO owner. Index creation has no new
interruptible SDK callback or streaming maintenance-progress interface. Existing
installation transaction limits still apply: 512 MiB root, 16,384 entries, no
symlink/hardlink/special-file roots, no interprocess lock, and no crash-atomic pair
of directory renames. Native Windows open-file/rename behavior remains unverified.

## Recovery foundation

`recovery.hpp` / `recovery.cpp` persist one versioned native session plus up to
64 editor drafts. Each draft retains exact current and saved UTF-8 source, target,
origin revision, title and selection in validated UTF-8 byte offsets. File,
existing module and untitled targets are distinct. Source text is limited to
4 MiB per field and 16 MiB across all draft current/saved fields; the complete
checksummed envelope is limited to 40 MiB. No passphrases or sync opt-in are stored.
Recovery files contain private document text and should live in the user's profile.

Verified revision-checked saves retain the last valid primary as `.previous`.
Reading a missing/corrupt primary offers that validated previous snapshot with an
explicit fallback flag. A stale save does not rotate the backup. Unknown primary
or backup formats are preserved. A fallback may be explicitly saved to a new
nonexistent destination; it cannot silently replace the damaged primary. Backup rotation and primary replacement are
separate operations, not a locked/fsynced multi-file transaction. This is recovery
from readable snapshots, not proof against power loss or disk failure.

Reading recovery never writes a document/module or restores live tabs. Compare a
fresh source revision before normal Save; changed/deleted sources require conflict
resolution or Save As. Use the existing strict native-session restoration to apply
an explicitly selected session. Native autosave scheduling, recovery presentation,
draft/editor reconstruction, interrupted installation reconciliation and physical
host crash testing remain pending. This checkpoint is shared storage and transport.

## Optional safe sync networking (2026-11-01)

Implementation `04914ca` adds `sync_transport.hpp` / `sync_transport.cpp`, shared
worker commands and owned C event handles. Configuration, transmit and receive run
on the existing FIFO thread; no GTK callback or second SWORD engine is introduced.
Startup creates no sockets. Explicit identity/session metadata and network
acknowledgement are required. A failed network reconfiguration disables the prior
connection. One adapter-managed conduit may be active per process; policy-only
configuration stops any active conduit. SDK callbacks are contained on the owner
thread and exceptions do not cross the SDK callback boundary.

The optional SDK must include the [reviewed safety patch](../../dependencies/biblesync/README.md).
This applies personal TTL before initial beacon/presence, processes at most64
packets per effective receive, bounds SDK peers to256 and rejects oversized
outbound payloads instead of truncating references. Host calls are throttled to
one effective receive per second, matching beacon/expiry cadence. Event snapshots
contain at most32 copied entries, overflow/skipped-event state and active-at-poll
state. No passphrase/raw packet dump is returned. Navigation is a wire event,
not automatic navigation: feed it to shared `plan_incoming_sync`, then consume its
direct/reference-list intent through existing application actions.

The protocol session phrase is a plaintext matching filter, not cryptographic
authentication or encryption. The SDK also sends its normal identity/device
metadata. SDK/network configuration errors are returned without raw dumps.
Networking is OFF by default; a configure-time API/link probe rejects the
unpatched library. Enabling requires an explicit safe SDK path/setup; no global
SDK installation or dependency download happens during app configuration.

Real UDP tests use the actual SDK and a synthetic protocol peer. Test-executable
interposition redirects every SDK send to an ephemeral loopback sink and observes
TTL on the first messages. Incoming beacon/navigation, session mismatch, echo
suppression, single-conduit ownership, disable/failure and C ownership are checked.
These are not physical two-device or LAN/multicast-routing acceptance. Native
sync settings/timer/navigation consumption, chat/friendly presence presentation,
network sleep/interface changes, package audit and native Windows/Fedora remain
open. The earlier policy checkpoint below is retained as its behavioral contract.

## Sync foundation and remaining network work

`sync.hpp` / `sync.cpp` extract application policy from the retained BibleSync
workflow: disabled/personal/speaker/audience modes; none/selected/all senders;
group/domain filters; direct single-reference versus reviewable reference-list
routing; optional group 1–9 to zero-based tab intent; and explicit no-retransmit
policy for received navigation. Unknown modules stay reviewable instead of
implicitly selecting another Bible. The worker resolves installed aliases using
the existing Reader and returns copied navigation intent. It does not auto-navigate.

Startup remains disabled and listen-none. Personal privacy is a host-only transport
requirement, matching the retained TTL behavior; it is not encryption. No passphrase
or socket is implemented in this policy layer. An actual BibleSync adapter still
must validate/authenticate wire events and manage passphrases, presence, multicast,
mode transitions, privacy changes and shutdown before events enter this boundary.
Native consumption and a real two-client interoperability check remain required.
No BibleSync dependency is added by these application-policy services.

## Evidence and open acceptance

Original-prose fixtures cover repository round trips, exact cached rights, endpoint
cache invalidation, value edits, stale/unknown-file preservation, maintenance
isolation/in-use/stale requests/configuration/index failure/refresh rollback,
recovery truncation/checksum/UTF-8-selection/source conflicts/backup fallback and
fresh-process read, plus sync mode/listen/privacy/echo/filter/routing combinations.
C checks cover input copying, owned results, take-once handles, allocation failure
before write admission, and allocation-free successful result polling.

These scoped services do not accept full F05/F25/F26/F27/F28/F31 parity. Native
repository/maintenance presentation, legacy source/profile import, capable-SDK
positive index creation, encrypted-module correctness, real BibleSync networking,
automatic native recovery and current physical Windows/Fedora checks remain open.
See the [Mac handoff](handoff-macos.md) for actual executed suites and limits,
[HTTPS contract](https-transfer.md), [local installation](library-installation.md),
[session storage](session-persistence.md) and [editing](editing-workflows.md).
