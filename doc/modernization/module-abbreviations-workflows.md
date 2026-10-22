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
