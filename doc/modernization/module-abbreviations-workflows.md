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
