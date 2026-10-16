# Note-numbering migration seam

This is partial shared-core extraction and characterization. Native automatic
note numbering is not enabled by these checkpoints. Note retrieval and semantic
note links retain their existing implementation.

## Owned counters and legacy integration

`c39b024` extracts CleanupContent's literal `*n`/`*x` insertion into
`number_note_markers`. Counters are explicit owned values instead of new global
state. The retained GTK wrapper probes the same first note's `n` attribute,
preserves its suppression rule and callback order, then copies back the result
and counters. A reported label suppresses both automatic counters for that entry.
The legacy wrapper still owns its existing document counters and reset points.

The helper intentionally preserves literal substring matching, separate note/
cross-reference counters, continuation between entries and legacy four-digit
truncation. It is not a semantic HTML parser. NUL/negative state is rejected;
integer overflow raises an error before publishing output/state rather than
performing undefined signed arithmetic. No frontend duplicates this policy.

Evidence: 60,750 complete cleanup combinations, 36 prior numbering checks,
2,268 frozen/current/owned numbering comparisons and 400 concurrent owned calls.
The same test also checks 144 cases in four available locales. Full Mac suite
70/70 at this checkpoint; focused ASan/UBSan and TSan each 1/1. Strict C++11 and
native-disabled helper build pass. No complete GTK3 runtime check is claimed.

## SDK label switch

`8572594` moves the legacy OSIS/ThML/GBF/TEI XHTML switch into
`set_render_note_numbers`. It updates every matching module render filter and
ignores unrelated filters. The GTK wrapper preserves its null-options no-op and
nonzero-flag interpretation. The helper mutates state; it is not a scoped setting
and is not yet called by native Reader sessions.

The installed SDK headers expose setters without matching getters. Public
[OSIS renderer documentation](https://crosswire.org/sword/apiref/classOSISXHTML.html)
and the [upstream change record](https://crosswire.org/pipermail/sword-cvs/2020-April/006195.html)
provide source context; actual local behavior is checked against SWORD
1.9.0.3916. No SDK source is vendored or modified.

The new `core_sdk_note_numbers` test covers 40 format/type/label cases. Every case
checks fresh/default Off, On/Off restoration, both duplicate matching filters,
frozen/actual/core switches with 0/1/-1/2 flags, missing-options behavior, semantic
URI identity, and survival through lexical layout/owned numbering. Temporary
filters detach before destruction; unrelated filters remain untouched.

Observed local differences are preserved explicitly: OSIS/ThML display supplied
labels directly; GBF encodes spaces/UTF-8 labels and uses a note marker even for
the tested cross-reference type. In the direct-filter TEI case with an ordinary
attribute-free closing note tag, the identifier is empty and supplied label is
absent. The strict core link parser rejects that target. This narrow case does
not establish behavior of every real TEI producer; do not claim TEI note parity,
relax identifier validation, or replace its label rules without corpus evidence.

Local evidence at `8572594`: 71/71 Mac/core checks; 55/55 portable Linux/core checks
**on Mac**; focused SDK ASan/UBSan 1/1, strict C++11 and native-disabled helper build
pass. External SDK libraries are not fully instrumented and leak detection is off.
The new test does not require a desktop, installed module corpus or user settings.
Native Fedora/MSVC and representative-module acceptance remain separate gates.

## Owned attribute and heading-fragment metadata

`858ea63` shares the UTF8HTML entry-attribute reader between native heading
collection and the retained legacy backend. Eighty frozen/current/owned cases
cover empty values, UTF-8, arbitrary paths and refresh modes. Copied values survive
later rendering; invalid NUL paths reject before changing engine attributes.

`e3fcdb2` adds copied `HeadingFragment` values at each legacy SDK Preverse index.
A fragment can contain several HTML headings. `reported_note_label` is the exact
Footnote/1/n value at the legacy cleanup point; it can remain the main entry's
label and must not be substituted as visible heading text. The existing string
API retains its output and dataflow through a shared collector.

A private fixture compares metadata at the full frozen CacheHeader/CleanupContent
boundary and reproduces heading-then-body numbering using owned counters. It
covers OSIS and multiple ThML fragments, labelled/unlabelled/UTF-8 note sources,
option Off/On, missing headings, key/source-attribute restoration and ownership
after SDK teardown. Shared fixture modules and installed libraries are untouched.
Current suites pass 72/72 Mac and 56/56 portable Linux/core on Mac. Focused
ASan/UBSan passes 2/2 with the heading prerequisite; strict C++11 and the
native-disabled helper build pass. Reader snapshot adoption and native numbering
controls are still pending; no new GUI parity is claimed.

## Owned renderer scopes and opt-in Reader sources

`c499324` adds `NoteRenderingState` and `ScopedNoteNumbers`. Registration establishes
an explicit baseline on new filter objects; known aliases keep their current
tracked values. Scope construction validates/stages all matching filters before
mutation. Nested scopes, mixed baselines, real SDK cross-module aliases and
exception unwinding restore prior owned values. An unregistered later filter
cannot leave an earlier one changed. Registration/scopes require serialized
engine ownership and prohibit external writes to tracked filters.

The private Reader establishes Off, matching the checked SDK default, at engine
startup. Configured opt-in entry/chapter/commentary overloads scope supplied-label
rendering and copy raw `Entry.note_source` values. `captured=false` means unavailable;
`captured=true` with an empty label is a checked empty value. Heading metadata
keeps SDK fragment boundaries and cleanup-point semantics. Dense/sparse entries
copy their source before later renders; values survive Reader/manager destruction.
Existing overloads and all current session/C/native consumers keep their prior path.

The boolean controls SDK supplied labels, not automatic counter insertion. The
new source metadata remains internal C++ input to the future shared presentation
stage; no frontend needs its own numbering logic. Raw metadata is not exposed by
new C getters or used as a saved-preference default.

Current evidence: 72/72 Mac, 56/56 portable Linux/core on Mac, focused ASan/UBSan
3/3 (SDK scopes, heading fixture, Reader metadata), strict C++11 and native-disabled
helper build pass. No new GUI/native Fedora/MSVC acceptance is claimed.

## Saved-setting and final coverage follow-up

`d1175af` extracts `note_numbers_from_preferences`: On unless the decoded value is
exactly Off. The complete frozen/current initializer, 22 file/duplicate cases and
direct map cases preserve SDK decoding/order without extra normalization. Native
save tests preserve unhandled `XrefNoteNumbers` values in existing and different
target files. It is not a new native presentation field or activated setting.
The legacy menu saves the option then refreshes its pane/dialog; ConstructFlags
uses XrefNoteNumbers bit 26 and CacheIsValid compares flags. Native caching and
setting application remain separate work.

`5da4a7a` avoids the second output allocation/copy when literal numbering finds no
markers. Frozen comparisons and concurrent cases remain unchanged. Full suites
at that production checkpoint pass 72/72 Mac and 56/56 portable Linux/core on Mac;
focused preference/counter ASan/UBSan 2/2 and counter TSan 1/1 pass. Strict C++11
and native-disabled helper builds pass.

`084fdde` adds distinct/empty/linked dense Bible sources and a real shared
Bible/commentary renderer restoration check. Focused normal 2/2 and ASan/UBSan
2/2 pass including the heading fixture. This adds coverage without changing
production behavior; no additional GUI/native Fedora/MSVC acceptance is implied.

## Opt-in shared session presentation checkpoint

Implementation `1bd924c` applies numbering inside C++ `StudySession`.
`set_note_numbering_enabled` is a consumer capability, default false and never
persisted. It requires configured engine options; unknown engine state stays on
the existing Reader path. `set_note_numbers` is a per-module preference, decoded
On unless exactly Off. Only available consumers write `XrefNoteNumbers`; older
consumers preserve the target file's unhandled value. Import does not enable the
capability, and failed imports preserve the current session.

The session consumes copied Reader sources, retaining each SDK heading fragment
and its cleanup-point label. One document-wide lexical decision precedes image/
font cleanup and numbering of each heading fragment, then its body. Counters
reset for an entry or chapter, continue across chapter source order, and remain
independent between paired panes. Raw metadata remains unchanged; published HTML,
diagnostics and matching CSS are owned. History records only navigation and
rerenders the current preference. No global numbering state is added.

The new `core_note_snapshots` test compares session output against the complete
frozen legacy postprocessor on original-prose private fixtures. It covers supplied,
empty and linked labels; multi-fragment headings; independent cross-reference and
footnote counters; dense/sparse ordinals and selection; entry reset, paired panes,
tab isolation, SDK restoration, failed imports, selective saves, lifetime, cleanup
order and whole-document lexical fallback with spacing-only CSS. Legacy literal
marker counting includes surviving SDK image-link URLs: removing the image tag
does not remove its wrapping link. This behavior is preserved, not silently fixed.

Checked locally: 73/73 Mac/core tests, 57/57 portable Linux/core tests on this Mac,
and focused ASan/UBSan 3/3 including both fixture prerequisites. Strict C++11
presentation syntax and native-disabled legacy helper build pass. External SDKs
are not fully instrumented; leak detection is off. No new GUI, native Fedora or
MSVC acceptance is claimed. Unsupported WORD footnote lexical markup still uses
the established readable fallback; this checkpoint does not expand that grammar.

## Queued commands and native controls

`aff5f59` exposes the checked session policy through queued worker/C commands:
`xc_worker_set_note_numbering_enabled`, `xc_worker_set_note_numbers`, and typed
repeatable `xc_request_note_numbers` value/availability outputs. Setters accept
only 0/1 and remain FIFO, tab/module-local and history-neutral. Unknown engine
state or a disabled consumer leaves the setting unavailable. Getter outputs clear
on null/pending/failure/wrong-kind results; copies survive subsequent requests,
closed tabs and worker destruction. Existing consumers remain default-off.

`1d3612f` opts the Mac and Linux preview models into that contract, copies state,
and adds separate Bible/commentary “Footnote and cross-reference numbers” controls.
Refresh uses committed passage context and the core's current policy; it preserves
input drafts/history and paired-pane independence. Reading-option profiles save
and restore the applied shared setting. No Swift/GTK numbering algorithm exists.
Swift represents unavailable state as nil; the Linux model uses -1.

Local evidence at `1d3612f`: **75/75 Mac/core CTests**, **59/59 portable Linux/core
CTests on this Mac**, **ASan/UBSan 5/5** (worker/C and Linux model tests plus three
fixture prerequisites), and strict C11 header syntax. Tests cover FIFO ordering,
strict inputs, unknown/closed tabs, copied arguments/state, profiles, failed saves/
imports retained by existing regression tests, window isolation, chapter counters,
selection, drafts and shutdown. External SDKs are not fully instrumented; leak
checks are off. No hosted builds were run.

A disposable Mac window visibly switched entry markers from *n1/*n2 to *n and
back; chapter counters continued through supplied/linked labels to *n2/*n3 at
verse 5. Clicking a numbered Bible marker opened the Study note pane. Toggling
commentary numbers removed its labels while the Bible remained numbered. The
fixture app was quit and its temporary copy removed. GTK controls were source
reviewed and their model tested here; current Fedora GUI and MSVC/WinUI validation
remain pending. The ThML heading fixture also emits empty SDK note-link IDs,
which remain a recorded SDK/heading follow-up, not an accepted click workflow.

## Isolated heading-note source capture

`91f83c9` adds C++11 `capture_preverse_heading_notes` as an internal source service.
It refreshes the selected verse, isolates heading note attributes, copies note
bodies/metadata and the legacy cleanup-point label, and restores the complete
source attribute map. It also restores SDK attribute processing after exceptions
from initial refresh, heading render or body render. Disabled attribute processing
rejects before mutation. Metadata-only capture does not render note bodies.

Headings must not be option-filtered twice: doing so strips ThML's
`fromEntryAttributes` marker and hides its output. Other option filters process
unfiltered heading notes in isolation. If Footnotes ran before Headings, the saved
fragment already has SDK IDs; those original IDs/bodies are retained without
rerunning Footnotes or importing the source body's notes. Mixed filtered/raw
notes, missing note attributes, malformed tags and NUL source reject explicitly.
Empty note bodies remain distinct from missing metadata.

Local SDK tests cover both OSIS/ThML filter orders, labels On/Off, footnotes On/Off,
Unicode, mixed study/cross-reference/empty notes, source-key identity, complete
state restoration, injected failures, malformed input, subsequent ordinary body
lookup and ownership after manager destruction. Full local suites pass 76/76
Mac/core and 60/60 portable Linux/core on Mac; focused ASan/UBSan 3/3, strict C++11
and native-disabled helper builds pass. External SDKs remain partly
uninstrumented; leak detection is off.

**The captured IDs are fragment-local internal input, not body-note IDs.** This
service is not enabled in Reader/session/C/native snapshots yet. Heading-scoped
identifiers and shared lookup remain the next boundary; no native heading-click
repair or new GUI/native Fedora/MSVC acceptance is claimed at this source step.

## Next acceptance boundaries

1. On Fedora, rebuild the default-ON GTK preview and verify both controls,
   chapter/entry updates, profile round trips and note clicks with real input.
2. Expand WORD lexical note markup, real module/RTL/corpus and heading-note ID
   coverage. Existing unsupported lexical markup retains the readable fallback.
   The direct-filter TEI empty identifier remains a separate narrow limitation.
3. Bring up native Windows dependencies/core and a thin WinUI adapter using the
   same queued commands, not separate numbering logic.

The requested native control checkpoint is complete and pushed; shared ownership
is released. Legacy source is not eligible for retirement. These checks do not
accept whole F14/F17 parity or enable automatic GitHub builds.
