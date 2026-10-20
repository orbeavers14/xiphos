# Retained search-result generations

Updated 2026-10-20. Checked C++ session/worker service and additive C transport;
Swift transport is checked below; complete export is the next boundary. No whole F18–F20/F32 parity
or legacy source retirement is accepted. See [search](search-workflows.md),
[export](export-workflows.md) and the [remaining export proposal](search-export-design.md).

## Contract and ownership

Each session owns one immutable logical result generation, separately from its
last-search scope and annotation context. A nonzero ID is monotonic **within that
tab/session**, not a global ID. Always use the originating tab + generation ID.
IDs are ephemeral and must not be persisted across worker/tab replacement.
A generation retains all original requested queries, attempted ordered groups,
per-group failures and bounded visible prefixes. Complete returned SDK keys are
retained per successful group when aggregate capacity allows. Duplicate modules
and keys preserve their positions; earlier groups survive later SDK searches.

Scope keys remain captured before indexed display sorting. Generation keys use
post-sort display order; unindexed groups reuse the immutable scope clone when
its order matches. Every SDK clone, traversal and destruction stays on the engine
owner. No SDK object crosses the worker boundary. Session copies share only old
immutable state and replace it independently. All C++ session calls/lifetimes
remain serialized, including retained reads; native callers use StudyWorker.

`SearchGenerationInfo` copies ID, retention capability, requested/attempted/failed
counts, visible-prefix count, matched/retained SDK element counts, accounted key
bytes and cancelled disposition. `complete` means **all returned keys retained**,
not all requested modules succeeded or completed. Inspect failures, attempted
count and cancellation too. SDK interval elements may expand into multiple
returned verse strings, so element counts are not expanded verse totals.

## Publication and capacity

Successful executed searches (including returned stopped partial results), empty
executions and successful empty-last-scope short-circuits publish a new generation.
Completed batches publish attempted groups even if every group failed. Cancel
before any SDK execution preserves the previous generation; batch cancellation
after execution publishes its completed/returned partial prefix. Validation or
fatal owned preparation failure leaves prior generation, scope and logical context
unpublished. Prepared search-text context can still change on pre-execution stop
under the previously characterized policy; generation identity remains independent.
Explicit scope clear or annotation reset does not discard visible/retained results.

Owned metadata and SDK snapshots stage before one session publication. Root search
control spans that preparation too; it cannot report complete before generation
publication succeeds. IDs never wrap; exhaustion fails before SDK execution.
Stateless Reader searches do not create a session generation.

Retention policy applies only to future generations. Defaults/maxima are 100,000
SDK elements and 16 MiB accounted text/range-text/versification bytes **across all
groups**, rather than per module. Positive smaller limits are accepted; invalid
limits leave policy unchanged. If aggregate capacity overflows, prior staged full
snapshots are released on the owner and complete retention becomes unavailable for
the new generation. Bounded visible prefixes and normal complete last scope remain
usable. The new ID prevents accidental reuse of an older complete result set.

These are logical guards, not exact heap/time limits. Scope retains its independent
existing per-search capacity; exceeding that still fails the search as before.
Public output remains capped at 10,000 references/8 MiB, with smaller defaults.
Original input module/range-module IDs now share the batch's 4 KiB NUL-free UTF-8
validation; expressions remain 32 KiB, range expressions 4 KiB. SDK-prepared copied
query text is capped at 64 KiB and must be NUL-free UTF-8. This bounds retained query
metadata without silently truncating SDK text. SDK preparation itself can allocate
before the check; no total SDK allocation/execution deadline is promised.

## Copied bounded reads

Session/worker `search_generation` observes copied current metadata. A retained
read names tab + ID + requested group index, visible or complete source, and normal
positive copied-output limits. It never reruns a search or changes history/scope.

| Read status | Meaning |
|---|---|
| ok | Owned bounded SearchResult snapshot supplied |
| stale | ID absent/no longer current; copied current generation info supplied |
| invalid_group | Index outside original requested queries |
| not_attempted | Valid requested group not reached by a returned cancelled job |
| failed_group | Original query/failure supplied, no false empty success |
| capacity_exceeded | Complete source unavailable; visible-prefix read still possible |

Visible reads preserve the original limited flag and may be clipped further.
Complete reads clone/traverse retained typed keys on the owner, with complete
prefix strings and a limited flag describing the new read budget. A complete read
can recover earlier matches omitted from display even after last scope changes.
Metadata in the returned SearchResult describes the **originating search**, including
its scope/context/publication flags, rather than side effects of the read. All
strings/values outlive later jobs and shutdown; no SDK object is returned. Temporary
SDK traversal clones are outside the copied-output byte bound.

## Evidence and limits

Original OSIS Bible/commentary fixtures cover independent complete/visible prefixes,
byte clipping, earlier module groups, duplicate/module failures, source queries,
all-error batches, exact/overflow aggregate count and byte caps, empty groups after
budget exhaustion, immutable policy/session copies, reset/history/tab neutrality,
stale IDs, pre-cancel/errors, ID exhaustion and draining worker lifetime. Controlled
SDK tests establish pre-sort scope versus post-sort generation, typed interval
expansion, returned partial stop and SDK state restoration. They do not validate
actual CLucene execution, which still skips on this SDK.

Injected SDK strip filters exercise 64 KiB boundary/overflow and invalid UTF-8/NUL
metadata. The first NUL test used SWBuf::set, which truncates at NUL; constructing
an explicit sized buffer corrected the fixture before acceptance. An initial
mixed-auto declaration compile error was corrected before checks. No failed source
checkpoint was pushed. Search orchestration now lives in search_session.cpp rather
than enlarging the general session implementation.

Full/checkpoint commands and actual totals belong in the [Mac handoff](handoff-macos.md).
Initial concurrent local runs hit elapsed-time limits (including a reported 910 s
portable timeout); quiet/serial rechecks separate those from behavior. Sanitizer
instrumentation is partial and leaks are off. Current Fedora/MSVC/GTK/WinUI,
representative corpus/locales and full native export remain unvalidated.

## C transport checkpoint (2026-10-20)

The additive C11 API exposes current generation metadata, future-only retention
policy and bounded visible/complete reads through the same FIFO worker. Existing
struct layouts and single/batch functions are unchanged. Search and batch handles
identify their originating generation and whether that search published it.

Read request success is separate from its disposition: stale ID, invalid group,
unattempted group, failed module and capacity-exceeded outcomes do not fabricate
a result. Successful reads transfer an owned `xc_search` once. Non-consuming query
metadata borrows the request until release and remains available after taking the
result; copy its strings before releasing the request. Failure/unattempted query
metadata retains requested mode, case, bounds, last-scope and annotation options
without inventing output limits. Metadata getters reject wrong/pending/failed
request types and clear output values. Read flags describe the originating job.

Original-prose C integration checks cover omitted/duplicate groups, independent
tabs, query metadata, bounded copying, future-only copied policy, stale replacement,
capacity fallback, invalid/null access, take-once and drain/shutdown lifetime.
Strict C11/pedantic header check passes. Focused C generation/batch/canonical query
plus fixture pass 4/4 under ASan/UBSan and 4/4 under TSan (partial dependency
instrumentation, ASan leaks off). Swift transport and actual export have separate
acceptance gates. No hosted jobs or reset credit used.

## Swift transport checkpoint (2026-10-20)

The Swift 6 adapter now copies generation/capacity metadata into single and batch
values and exposes current metadata, future retention policy and bounded reads.
Typed stale/failed/unattempted/capacity outcomes carry optional original query and
result values; no SDK handle or borrowed string escapes the adapter. Keep the
originating live worker/tab with the ID. The adapter does not rerun queries or
replace a stale ID. Default task cancellation still drains accepted work.

Ten focused local checks pass: Swift generation/batch/context/attribute/basic
adapters, C generation/batch/canonical query and two fixture setups. Original
fixtures establish complete-versus-visible copying, failed groups, query bounds,
policy/reset independence, stale replacement, capacity fallback and copied
lifetime after shutdown. An initial test argument-order compilation error was
fixed before acceptance. This is adapter evidence, not a native save/copy UI or
complete search/export parity.
