# Session persistence contracts and remaining work

The owner prioritized complete core session persistence while deferring further
devotional work. The retained `gui_save_tabs`/`gui_load_tabs` format is
`Xiphos_Tabs` XML, separate from the main `Xiphos` preferences file. This is a
compatibility source, not a complete representation of the new core's state.

## Retained session fields

Each `tabs/tab` contains raw text/commentary/dictionary/book module names,
`text_commentary_key`, `dictlex_key`, `book_offset`, `comm_showing`, four pane
visibility attributes and `showparallel`. All `tabs` sections contribute in
source order. `Version` is retained as metadata. Missing and explicit empty
attributes are distinct. Reading raw offsets does not parse them with `atol`
or assume a valid book-tree node. Module identifiers are not resolved at import.

Only the exact lowercase `yes` token is true. The four pane visibility flags
inherit the global defaults together only when `showtexts` is absent. An empty
`showtexts` is present and false, and missing sibling flags become false rather
than individually inheriting defaults. Commentary visibility and parallel flags
always use the exact yes-token policy.

Legacy loading creates native widgets, resets history to an initial visit,
selects the first tab and may rewrite old files when pane flags are missing.
Those effects are separate from the new explicit read-only repository. The new
reader never creates fallback tabs, renders modules, chooses a selected tab,
rewrites a source file or searches the home directory. It accepts up to256 tabs,
32768 bytes per retained attribute and16MiB total retained text, with the shared
secure XML parser rejecting malformed/DTD-bearing/NUL-hidden documents.

## Native restoration design and remaining host gates

The native snapshot design covers per-tab main and dictionary selections/history
with cursors; book raw/display selection; parallel raw intent, resolved control,
reference and explicit options; tab-wide and per-module engine/presentation
preferences; lexical/preview preferences; and tab order/active tab intent.
Search annotation/retention policy is saved; result generations are discarded.
Search query drafts remain host-owned.
Window geometry, detach ownership and native UI selection belong at the host seam.
SDK pointers, rendered DOM objects and active jobs are never persistence objects.

Restore must validate a complete replacement before publishing it, retain the old
session on failure, define unavailable-module behavior and avoid silently trimming
bounded lists. Saved metadata and actual restored module provenance must remain
separate. Storage-only inspection must remain possible without installed modules.
Export/replacement must preserve unrelated data, verify a staged owned snapshot
before atomic replacement and use explicit selected paths. Observed external-change
checks do not replace an interprocess locking/sync protocol.

This groundwork does not accept the full tabs/detach/session parity group. Broader
Windows/Fedora and real native restart workflows remain required alongside portable
core validation.

## Checked compatible repository

The explicit repository now serializes/saves raw legacy tab sessions through the
shared verified same-directory replacement transaction. Known attributes are
replaced or removed according to their optional values. Existing tab nodes are
reused by position across all `tabs` sections, retaining unknown attributes and
children; surplus owned tabs are removed and new tabs append to the last section.
Unrelated sections/comments/root attributes survive semantically. Positional
association is a legacy limitation: the format has no stable tab IDs. It is not
a merge or tab-identity protocol. Serialization may normalize formatting/encoding.

The FIFO worker and owned C interface expose explicit storage read/save without
applying a session. All paths, version strings, tab arrays and fields copy before
submission returns. C admission bounds every scalar and the aggregate before
observer/copy allocation. Worker validation rejects invalid paths and session text
before the queue; tab validity is checked before IO. Result storage is allocated
before accepting a write and committed result publication uses no-throw moves.
Tests inject observer/envelope allocation failures and confirm no write occurs;
successful polling allocates nothing. Dropped accepted saves drain at shutdown.
A zero-tab file stays zero tabs rather than manufacturing a default UI tab.

The characterization host compares20 complete frozen/current legacy load cases
and9 complete writer cases. Widgets and implicit file-discovery branches are
stubbed; this does not claim actual GTK interaction or native restart acceptance.

## Live tab capture (2026-10-31)

Committed reading and dictionary selections are now independent of history lists.
Successful visits and back/forward update them; previews and failed reads do not.
Clearing history retains the current location. Explicit selection-clear commands
are separate. A same-reference visit can therefore retain the actual new module
pair even when the compatible history deduplication keeps its first module pair.
Main-pane lexical clicks commit dictionary intent; dialog/read-only lookups do not.

The FIFO worker and C ABI can capture an explicit ordered list of live tab IDs,
and atomically save that capture through the compatible XML repository. They copy
committed text/commentary/reference, dictionary module/key and selected book offset;
the host supplies pane/commentary/parallel visibility. Unknown, closed or duplicate
tab IDs reject the whole capture before a file changes. Empty tabs remain empty:
there is no guessed first module or SDK-global current-key fallback. Limits remain
256 tabs,32768 bytes per field and16MiB retained text. C input arrays/version are
copied before returning; request/result envelopes are allocated before acceptance.
Committed entry/passage/dictionary/lexical result polling only moves owned data,
including contextual dictionary/lexical alternatives; unused envelopes are never
published as results.

This exports the legacy subset only. It does not restore live tabs, histories,
parallel columns/options, preferences, active tab or window geometry. The complete
replacement/restore contract above remains open. The standalone session-capture
fixture uses only disposable original prose and covers cleared histories, module
switches, ordered saves, failure retention, C ownership and shutdown draining.

## Versioned native core snapshots (2026-10-31)

The shared core now has a distinct strict `XiphosSession` version1 format, with
owned `StudySessionState`/`NativeSessionSnapshot` values. It does not replace or
reinterpret `Xiphos_Tabs`. Native snapshots retain ordered opaque host tab keys,
an optional active key, explicit pane/commentary/parallel visibility, committed
reading/dictionary intent, both complete histories/cursors, raw book target plus
resolved path/offset and document-selection mode, parallel raw columns/control/
reference/options, configured/unconfigured tab and per-module engine options,
explicit presentation preferences, module-default opt-in, optional lexicons,
cross-reference preview policy, search annotation policy and search retention limits.
Absent versus empty optional strings remain distinct. Engine flags retain their
exact values rather than passing through lossy legacy preference encodings.

Rendered HTML/assets, SDK objects, current search results/scopes/generation IDs,
active jobs and migration capability flags are deliberately transient. Search
query drafts, window geometry, detach ownership and native pane/view selection
outside the explicit supplied flags remain host responsibilities. Host rendering
capabilities are passed explicitly to restoration; module availability fields are
recomputed, not trusted from storage. Dormant preferences for uninstalled modules
and unresolved lexicon preference IDs remain metadata, without fallback selection.

The FIFO worker captures/saves the explicit ordered list, and restores/loads the
entire tab set. It first validates the snapshot and stages every replacement
session on the engine thread. Current reading/dictionary/book/parallel content is
validated through existing reading services. Required history/selection module
identities must be installed exactly; aliases and first-module fallbacks are not
substituted. Saved parallel control/reference and book path/offset must still
resolve identically. Intentionally missing parallel column slots remain supported
when the saved control still resolves identically. A missing required module or
changed book/control rejects the whole replacement. No history replay/dedup or
list trimming occurs. Selections can remain present with empty cleared histories.

Only after every tab and the complete result mapping are allocated does the worker
swap the replacement map into place. New IDs are monotonic; successful replacement
invalidates all old IDs and returns key-to-ID mappings plus active-tab/visibility
metadata. Failure preserves every old tab and the ID counter. A zero-tab snapshot
explicitly closes all tabs without creating a fallback. Existing queued work keeps
FIFO ordering; commands addressing old IDs after a successful restore fail normally.

Bounds:256 tabs,50 entries per history,1024 entries per module-preference map,
4096-byte module/tab IDs,32768-byte text fields,16MiB aggregate text and16MiB XML.
The shared secure XML parser rejects DTDs, malformed XML, hidden NUL and invalid
text. Unsupported versions, unknown schema fields/namespaces, duplicate tab keys,
duplicate singleton sections, bad cursors, nonbinary booleans and invalid content
preferences reject. Native saves own the complete strict document: an existing
unrelated, extended or unsupported document is refused rather than overwritten.
Writes use verified same-directory replacement with staged byte/round-trip checks;
observed external-change checks are still not an interprocess lock or sync merge.

The C11 surface provides owned offline parse/serialize, worker read/capture/save/
restore/load and take-once snapshot/mapping handles. Inputs copy before submission
returns. Observer/result envelopes allocate before accepting state-changing jobs;
committed polling performs no result allocation. Accepted dropped saves drain at
shutdown. Handle strings survive source/worker teardown until their owner releases
them. The new standalone fixture covers schema failures, Unicode/escaping, restart,
exact history restoration, cleared-history selection, dormant preferences, failure
retention, book provenance, C admission/allocation/lifetime and drain behavior.

This closes the core state/storage/replacement foundation, not native restart UI
acceptance or full F05 parity. SwiftUI/WinUI/GTK4 still need explicit integration
with host-owned window state and real restart workflows. Physical Windows/Fedora
checks remain open. Next core priorities remain editing, installation and sync.
