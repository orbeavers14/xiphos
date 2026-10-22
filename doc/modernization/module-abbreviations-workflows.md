# Shared module abbreviation policy

Updated 2026-10-22. This is partial extraction, not whole search/URI/parallel or
corpus acceptance. Existing GTK3 remains the reference implementation.

## Exact registry behavior

`ModuleAbbreviations` is a C++11 owned value class shared by the retained
`main_add_abbreviation`, clear and bidirectional lookup wrappers. It preserves
case-sensitive byte matching, empty names/aliases, the rejection of aliases
containing `(`, last registration per map key and older inverse entries that
survive re-registration until clear. This is deliberately not a normalized
bijection. Raw registration requires non-null terminated strings as before;
null queries return absent. Empty mapped values differ from absent mappings.
Borrowed internal lookup strings must be copied before mutation/destruction or
transport. Native validated inputs and copied snapshots are subsequent work.

Installed-module-ID collision exclusion stays at the existing catalog callers
in `BackEnd::init_lists` and the module manager. The current helper does not
change that ordering or enable native alias resolution. Search still needs
resolution after advanced range setup, with requested-token provenance retained.
URI/parallel/BibleSync callers are unchanged.

## Characterization and boundaries

The test freezes the actual four retained map routines from `a112980`, preserving
GPL-2.0-or-later attribution, and also compiles the current GTK wrapper routines
extracted from source. Both are compared with the shared class across repeated
names/aliases, duplicate collisions, rejected re-registrations, case/Unicode,
empty versus absent values, copied inputs, independent registry copies and clear.
No installed/user modules or profiles are used. Strict C++11 normal Mac,
ASan/UBSan, portable-on-Mac and native-disabled builds pass the focused test;
GTK runtime OFF, SDK instrumentation limits and ASan leak setting remain as
recorded in the Mac handoff. A full GTK3 application build/desktop check and
current Fedora/MSVC acceptance remain open.

Next: stage installed catalog aliases on the existing Reader/worker owner,
including installed-ID protection, source SDK order, bounded copied metadata,
reload semantics and owned resolution results. Do not resolve raw saved tokens
at import or introduce a frontend map. No whole parity/retirement gate passes.


## Bounded native catalog resolution

`Reader::resolve_module_name` and queued `StudyWorker::resolve_module_name` return
an owned `ModuleNameResolution` with literal requested token, optional installed
module ID and explicit alias-origin flag. Unknown tokens are successful absent
results. Queries require nonempty valid UTF-8, at most4096 bytes, with no NUL.
Exact installed IDs bypass alias metadata and always retain their identity.

On the first non-exact query, the Reader lazily stages SDK catalog registration
in actual module iteration order. Every installed ID is protected before any
alias registration, including later IDs. Null alias metadata stays absent; empty
aliases and the shared parenthesis/collision rules remain literal. Metadata is
bounded to65536 modules,4096 bytes per ID/alias and16MiB aggregate string bytes;
invalid UTF-8/NUL/overflow or duplicate synthetic IDs reject without publishing a
partial cache. SDK strings are bounded before copying. The cache belongs to the
existing Reader and is immutable for its existing fixed SDK lifetime; any future
install/reload service must invalidate/rebuild it transactionally.

Lazy validation means malformed alias metadata cannot disable ordinary reads or
exact-ID resolution. Alias queries reject that unsupported catalog consistently;
there is no truncation/fallback map. This read-only service changes no search,
scope, history, options or profile. Worker inputs copy before queueing, closed
tabs reject, accepted operations drain and copied values outlive the owner.
No search/URI/parallel/BibleSync behavior is enabled implicitly by this slice.

Tests compare the actual retained backend registration block with frozen map
routines, actual SDK catalog order and copied Reader/worker results. They cover
installed-ID/alias collisions, case/Unicode, absent/empty/rejected aliases,
per-field/module-count/aggregate bounds, copied inputs, shutdown, closed tabs and
ordinary reading after metadata rejection. C/Swift transport and explicit search
execution with correct retained/export provenance remain next.


## Owned C/Swift resolution transport

Additive `xc_worker_resolve_module_name` returns owned `xc_module_resolution`
through the existing request channel. Required nonempty UTF-8 token is bounded
before copying to4096 bytes; malformed UTF-8/catalog/tab errors arrive at poll.
Unknown tokens succeed with a null module getter. Requested token and alias-origin
flag remain separate. Pre-poll/failed/wrong-kind takes yield null; successful
take transfers once. Result strings borrow only from that handle until release,
and survive request/worker/SDK teardown. Null getters/release are safe. Existing
C layouts/version and query/search behavior remain unchanged.

Swift6 `ModuleNameResolution` copies Sendable/Equatable requested/optional module/
alias fields. The actor rejects empty/NUL/oversized Swift strings and consumes
shared registration/resolution policy without a frontend alias map. Accepted
read-only observations drain on cancellation/shutdown; no current state applies.
Normal4/4 including Swift/fixture, and ASan/UBSan, TSan, portable-on-Mac3/3 each
pass; strict C11 header syntax passes. Details/limits are in the Mac handoff.
Next: resolve actual search targets while preserving raw requested provenance,
explicit range providers and authoritative preview/retained/export module IDs.


## Search execution and authoritative source identity

Reader search resolves only the execution target, using the same cached catalog
under its owner lock. `SearchQuery.module`, ordered batch requests and explicit
range-provider strings remain literal. Missing aliases produce the existing
module-not-found path; bad alias metadata produces the existing invalid-request
path. Exact-ID searches bypass unsupported alias metadata. Explicit bounds
providers retain their existing exact-ID policy and are never guessed/resolved
from the search target. Existing search scope/filter/cancellation publication
rules remain shared.

`SearchResult.resolved_module` owns the selected installed ID before session
publication. `source_module()` supports older isolated source fixtures by falling
back to requested.module when no Reader identity was supplied. Retained metadata
owns both identities. Visible and complete retained export rows use the actual
source, including configured text/HTML/resource lookup. Optional context headers
and source summaries additionally disclose a different resolved identity; default
exact-ID output stays unchanged. Raw request provenance is never overwritten.

The existing C `xc_search_module` getter still returns the literal request.
Additive `xc_search_source_module` supplies actual source identity to the Swift
adapter and Linux model. Swift results preserve requestedModule separately from
module; both frontends build hits/preview/activation from actual sources while
leaving grouped request labels and editable saved lists raw. No frontend alias
map, second query engine or automatic saved choice application is introduced.

Distinct original prose per installed module tests wrong-source reads directly.
Coverage includes duplicate/Unicode aliases, installed-ID protection, retained
visible/all text/HTML/rich companions, partial failure disclosure, unchanged
bounds providers, scope chaining, cancellation, closed/shutdown ownership and
native Mac/Linux model preview/activation/export. Platform/corpus/complete
advanced-search acceptance remains independent; URI/parallel/BibleSync alias
consumption is subsequent work.
