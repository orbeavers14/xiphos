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

## Next work

- Shared Image Content/Respect Font Faces preference decoding and import/save.
- Explicit handling of legacy unknown-content discovery and save side effects.
- Native snapshot application under those settings; heading/body consistency.
- Note/xref numbering, block rendering, CSS/image paths, sizing, fonts/themes and
  representative legacy/native runtime/visual acceptance.

The helpers alone do not establish native image/font cleanup parity.
