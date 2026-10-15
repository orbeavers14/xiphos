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

## Next integration boundaries

1. Use the checked saved-setting decoder while adding native session state.
   Apply the setting before expanding persistence; retain unhandled target values
   until the new behavior is implemented and checked.
2. Integrate opted-in session numbering using owned Reader sources. Preserve
   per-fragment heading cleanup before body cleanup, document counter reset/
   continuation, diagnostic ordering, whole-pane lexical fallback and paired panes.
3. Add queued worker/C commands and compatible native controls after shared
   presentation tests pass. Keep existing consumers compatible and histories neutral.
4. Expand real module/TEI/RTL/corpus and platform acceptance. The direct-filter TEI
   empty identifier remains a recorded narrow limitation, not an accepted note workflow.

Legacy source is not eligible for retirement. Passing these helpers does not
check off whole F14/F17 parity or enable automatic GitHub builds.
