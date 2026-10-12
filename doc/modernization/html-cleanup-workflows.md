# Image and font cleanup: current shared slice

Updated 2026-10-12. Supports F03/F08 modernization; complete rendering, automatic
profile/startup behavior and visual/platform parity remain pending.

## Source contract

`src/main/display.cc:ClearImages` blanks exact lowercase `<img ` through the first
following `>` with spaces. It does not parse quoted delimiters, uppercase tags or
alternate whitespace. A missing end stops and reports the remaining suffix.
`ClearFontFaces` masks face-name bytes in exact `<font face="` with dots through
the next double quote, retaining quotes, other attributes and byte length. It does
not recognize uppercase, single-quoted or reordered attributes. A missing quote
stops and reports the remaining suffix. Earlier changes remain on malformed input.

`CleanupContent` applies these under off preferences. Unknown content is discovered
with case-insensitive detection, changing the field to on and saving that module
option. Image cleanup comes first, so font detection sees its resulting text.
Note/xref numbering follows this stage. The legacy save function reads/writes only
module configuration and does not consume the HTML buffer.

## Shared implementation and persistence

The C++11 `blank_images`/`blank_font_faces` return owned HTML and malformed offsets.
`cleanup_html_content` owns HTML, effective states, discovery events and diagnostic
suffixes captured when each stage runs. It validates states and embedded NUL, does
not perform file I/O, and shares the image/font stage with legacy `CleanupContent`.
Legacy callers retain diagnostic/save ordering and subsequent numbering. Existing
renderer source files and licensing remain; no source-file retirement is implied.

`ContentPreference` has unknown (-1), off (0), on (1). Shared decoding matches
`main_new_globals`: missing/empty is unknown, exact On is on, other nonempty decoded
values are off. Legacy initialization delegates these fields and Headings to the
shared parser. Combined core import stages content with engine/presentation fields.
Explicit save writes known states, preserves unrelated/uninstalled settings, and
leaves an existing target declaration intact for unknown. Unknown therefore is not
promised to round-trip over a preexisting known target declaration. Invalid enum
values fail before replacing the target.

Core sessions/workers apply cleanup to Bible entry/chapter bodies and commentary
entry/chapter headings and bodies. Discovery updates copied per-module state; image/
font setters do not visit history. Copied `Chapter.verse_ordinals` retain SDK sparse
positions for rebuilt HTML and linked/selected/missing anchors. Ordinary Reader
output stays unchanged. Entries own malformed cleanup diagnostics. Native reads
perform no automatic profile writes; explicit save persists known/discovered states.
Image blanking retains SWORD's surrounding image link, matching legacy behavior.

## C/Swift adapters and native controls

`xc_request_content_options` returns status and two tri-state outputs; pending,
failed, wrong-kind and invalid-pointer calls reset outputs to unknown. Ready unknown
is valid and getters may repeat. Setters validate exact 0/1 before queueing.
Diagnostic getters expose count, kind (image/font) and borrowed suffix from owned
entry, paired-passage pane or chapter entry handles, with null/bounds guards.
`xc_chapter_verse_ordinal` returns copied SDK metadata. Swift copies diagnostics and
positions into values that remain valid after handle/worker closure.

Swift carries unknown as nil and fetches state after rendering, including later
chapter entries. Reading options and Commentary options show toggles only when
core state is known, matching legacy menu visibility. Main changes refresh the
committed passage; commentary changes refresh headings/body/chapter. Draft input,
history and other windows/modules remain isolated. A temporary generated preview
verified menu checkmarks, independent pane settings, retained draft and composition.
Resource/font names are synthetic: image loading and installed-font appearance
require separate validation. Diagnostic data is carried without new visible controls.

## Evidence and limits

| Local evidence | Scope |
|---|---|
| 27,648 comparisons | Complete frozen/current primitive functions, owned bytes/length/diagnostic offsets |
| 200 profile combinations | Complete frozen/current initializer, tri-state content decoding, casing/whitespace/Unicode; paragraph capability stubbed absent and classification controlled |
| 60,750 combinations | Complete frozen/current cleanup versus owned content stage, state/reset/diagnostic/discovery events |
| 36 numbering cases | Subsequent reported/unreported labels, counters and callback order; full SDK/cache/chapter numbering still pending |
| 144 locale cases | Discovery matches local libc in available C/English/German/Turkish locales |
| Styled module fixtures | SDK output, heading/body consistency, sparse positions/anchors, tab isolation, explicit save/import, worker drain and copied C/Swift metadata |
| Native model/preview | Unknown visibility, busy/closed guards, draft/history preservation, per-window/module settings and explicit save/import |

Complete frozen functions from CrossWire c9976a8 retain GPL notices. Current functions
are extracted at configure time. Full local suite: 63/63; focused address/undefined
and thread snapshot/adapter checks: 2/2 each. Strict C++11 and native-disabled helper
builds pass. External SWORD/GLib are not fully instrumented; leak detection is disabled.
Full GTK runtime, other libc/SDK versions and Linux/Windows remain acceptance gates.
Host locale must be initialized before concurrent engine work.

## Remaining work

- Native automatic save/startup/discovery persistence policy.
- Note/dictionary/reference-preview cleanup and complete SDK note/xref numbering.
- Block rendering, image resources/link routing, sizing, module CSS/fonts/themes.
- Representative corpus and legacy/native runtime, keyboard and visual acceptance.

These scoped checks do not establish complete image/font rendering parity.
