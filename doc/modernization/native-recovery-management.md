# Native recovery and management integration

Checked macOS integration, 2026-11-01, implementation `78d367a`. SwiftUI/AppKit
present shared C++ services through the owned C/FIFO bridge; there is no second
navigation, persistence or installation engine. GTK3 remains untouched.

## Recovery and autosave

`RecoveryUI.swift` coalesces changes with a 300ms debounce and a five-second
periodic capture. Identical snapshots are skipped. Reading windows and editor
source drafts use shared checksummed recovery storage with its previous-valid
snapshot. Save errors pause scheduling and report the problem; ordinary document
Save/Save As remain available. Startup offers Recover or Discard. Clean quit saves
the normal reading session and clears both recovery generations; cancelling quit
retains work. Unsupported/corrupt files are preserved and a fresh destination is
selected rather than overwriting them. Recovery from the previous generation also
uses a fresh destination.

The stable application profile owns `recovery.snapshot`, `repositories.profile`
and the default `session.xml`. Save Reading Session As changes only the selected
reading-session destination. Default profile: Application Support/org.xiphos.native-preview.
`XIPHOS_PROFILE_DIRECTORY` isolates tests; development `XIPHOS_SESSION_PATH` uses
its parent/native-profile. Profile-scoped preferences retain selected destinations.

Drafts retain exact saved baseline/current source, target identity, original file
revision and UTF-8 selection. Recovery rebuilds shared dirty/undo behavior from
that baseline; it does not persist the complete undo stack. Recovered drafts open
in Source mode. Rich-view selection cannot safely map to source and resets to zero.
A changed/deleted original refuses Save; Save As remains available. Missing module
metadata does not discard recovered source. Active module editors lease their
module against removal until close, file export or session replacement.

## Module Library

Sources & Downloads supports ordered add/edit/remove/enable sources, cached
browsing, explicit HTTPS refresh/download, per-item planning, license/attribution,
progress and cancellation. Endpoint edits must be saved before transfers. Shared
revision checks reject stale offers. There are no automatic public downloads.
Install from Folder reviews local offers before explicit install/replacement.
Installed Modules exposes inspect/remove, index build/delete and cipher-key
configuration, with confirmations and shared in-use/transaction checks. A key is
stored in SWORD configuration, not an OS keychain; saving it does not verify it.
Transfers cannot be interrupted after commit. Quit waits for active operations;
a postcommit refresh/cache error does not misreport an installation as uncommitted.

First launch uses an explicit configured library or a private profile/modules
library if SDK discovery is empty. Existing auto-discovered libraries remain
readable; mutation requires File > Choose Module Library Folder or New Module
Library and a restart. The application does not silently hide existing modules.
A newly created library supports installation of its first module.

## Evidence and remaining scope

Recovery/fresh-process and loopback-TLS management tests use original prose only.
The Mac handoff records actual full/scoped/sanitizer and live UI checks. Recovery
fallback, Unicode source/selection, external-file conflicts, unchanged snapshots,
both-generation clearing, cancellation, stale offers, local/remote replacement and
editor-use removal guards are covered. No public Bible corpus was downloaded.

Windows and Linux presentation, complete legacy user-directory/assets migration,
representative real-user recovery and full parity acceptance remain open. Later
checked settings/session migration, capable-SDK actual indexing and native sync
controls are recorded in [the current Mac handoff](handoff-macos.md); the primary
SDK still reports explicit index-creation skips. Those later slices do not imply
physical-platform or full-workflow acceptance.
Autosave is recovery assistance, not an interprocess lock or power-loss guarantee.

## Verse-note cursor recovery — 2026-11-03

The native plain-text verse-note editor now tracks the AppKit cursor/selection.
Recovery snapshots use the existing shared UTF-8 offset fields; the view converts
valid offsets to AppKit UTF-16 ranges. Invalid/mid-codepoint offsets reset the
selection without changing recovered note text or its original revision.
Selection-only changes schedule autosave. Unattached note drafts retain their
selection in the existing untitled-document fallback. Reload/discard resets the
selection; stale saves still preserve the draft. Marked input is left intact
until composition finishes. This does not add rich-note markup or undo-history
persistence, and full input-method/physical-platform acceptance remains pending.
