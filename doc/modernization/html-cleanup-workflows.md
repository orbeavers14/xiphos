# Image and font cleanup: shared seam

Updated 2026-10-12. Supports F03/F08 rendering modernization; complete cleanup,
module preference side effects and visual parity remain pending.

## Source contract

`src/main/display.cc:ClearImages` replaces bytes from exact lowercase `<img `
through the first following `>` with spaces. It does not parse quoted attributes,
uppercase tags or alternate whitespace. On a missing end it stops and reports
the remaining suffix, retaining earlier changes.

`ClearFontFaces` finds exact `<font face="` and replaces face-name bytes through
the next double quote with dots. It keeps the quote, other attributes and byte
length. It does not recognize uppercase, single-quoted or reordered attributes.
A missing quote stops and reports the remaining suffix.

The C++11 `blank_images` and `blank_font_faces` helpers return owned HTML and a
malformed-suffix offset. They preserve these deliberately narrow transformations.
Owned strings reject embedded NUL; legacy callers already pass null-terminated
SDK text. Legacy wrappers copy the same-length result into the existing buffer
and keep the existing diagnostic text. `CleanupContent` still invokes them under
its existing preference conditions, including the existing unknown-content
state/save behavior. No renderer or source file is retired.

## Evidence and limits

Complete frozen functions from CrossWire c9976a8 retain their GPL notices.
Configure-time extraction tests the complete adapted current functions as well.
`core_html_cleanup` compares frozen/current/owned results and diagnostics over
27,648 bounded cases: valid/repeated/malformed tags, quoted delimiters, casing,
whitespace, attributes, Unicode and links. Byte lengths and malformed offsets
are checked; embedded-NUL rejection is separate from the legacy contract.

Normal, address/undefined and thread checks pass. Strict C++11 syntax and the
native-disabled legacy helper build pass. External SWORD/GLib instrumentation
limits still apply; address checks disable leak detection. The full GTK app is
not locally buildable here, so complete runtime integration remains a gate.

## Shared content preferences

`ContentPreference` explicitly represents unknown (-1), off (0) and on (1).
Shared decoding matches `main_new_globals`: missing/empty is unknown, exact On
is on and any other nonempty decoded value is off. Legacy initialization now
uses the shared decoder for these fields and Headings. Combined native-core
module import stages these states with engine/presentation settings.

Explicit save writes known states; unknown leaves an existing target declaration
intact because no discovery/explicit setting is available. Unrelated and
uninstalled settings remain preserved. Invalid enum values fail before replacing
the target. An unknown state is therefore not promised to round-trip over a
preexisting known target declaration.

`core_content_preferences` compares the complete frozen/current initializer
against shared content decoding over 200 profile combinations, including file
whitespace, casing, Unicode and both commentary classifications. Paragraph
capability is stubbed absent; commentary classification is controlled. This
characterizes the content fields and adjacent settings, not all initialization
side effects. Import/save, failed import/save, FIFO tab isolation and unchanged
history pass. The full suite passes 62/62; focused address/undefined and thread
checks pass 1/1 each, and strict C++11/native-disabled helper builds pass.

## Shared content application/discovery stage

`cleanup_html_content` applies off preferences and discovers unknown image/font
content using legacy case-insensitive detection. Image cleanup precedes font
handling, so font discovery sees the changed text. The result owns HTML, effective
states, discovery events and malformed diagnostic suffixes captured at the time
of each stage. It validates states/embedded NUL and does not perform file I/O.

Legacy `CleanupContent` now delegates this stage, retaining its file-save callbacks,
diagnostic ordering and subsequent note/xref numbering. The legacy save function
only reads/writes the module configuration; it does not consume the HTML buffer.
Current/frozen complete functions agree over 60,750 bounded content/state/reset
combinations. Additional 36 numbering cases compare reported/unreported note
labels, counters and callback order; these do not establish complete SDK/cache/
chapter numbering parity. Case-insensitive discovery also agrees in 144 cases
across C, English, German and Turkish locales available on this host. Host locale
must be initialized before concurrent engine work, as elsewhere in the core.

The full local suite passes 62/62. Updated focused normal/address/undefined/thread
checks pass 1/1 each. Strict C++11 and the native-disabled legacy helper build
pass. Full GTK runtime and other platform/libc behavior remain acceptance gates.
Core sessions/workers now apply the stage to Bible entry/chapter bodies and
commentary entry/chapter headings and bodies. Unknown discovery updates copied
per-module state; typed C++ setters update image/font display without visiting
history. Sparse chapters retain copied SDK verse ordinals so rebuilt HTML keeps
linked sections and selected/missing anchors. Ordinary `Reader` output stays
unchanged. No native automatic file save is implied by discovery events.

`core_content_snapshots` uses original styled ThML Bible/commentary data to check
cleanup against SDK output, header/body consistency, byte lengths, copied SDK
positions, sparse linked anchors, tab isolation, explicit import/save and worker
drain ownership. Owned C++ entries retain malformed cleanup diagnostics. Focused
address/undefined and thread checks pass 2/2 each (fixture plus service). The full
local suite passes 62/62; native state/diagnostic adapters and controls are pending.
Image blanking intentionally retains SWORD's surrounding image link, matching
legacy behavior; image-link routing/resource loading is separate acceptance work.

## Next work

- Native handling of discovery state and explicit save; automatic save/startup policy.
- C/Swift state/diagnostic adapters and native controls; note/dictionary/reference-preview cleanup.
- Note/xref numbering, block rendering, CSS/image paths, sizing, fonts/themes and
  representative legacy/native runtime/visual acceptance.

The helpers alone do not establish native image/font cleanup parity.
