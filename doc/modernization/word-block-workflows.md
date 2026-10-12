# Word/Strong’s/morphology block layout: characterization

Updated 2026-10-12. Supports F03/F17/F18 rendering work. This is source behavior
characterization, not an owned core block renderer or native layout acceptance.

`src/main/display.cc:block_dump`, `block_render_secondary` and `block_render` group
words and annotations into word/strongs/morph spans. They preserve lexical href
values while changing displayed morphology to lowercase/hyphenated text. Hebrew
pronominal display suffixes are shortened while full link values remain. Padding,
placeholder words, repeated annotations, styled recursion and SDK class shims
participate in layout. CSS then places annotations under each word.

Complete frozen helper/entry functions from CrossWire c9976a8 retain GPL notices.
Current complete functions are extracted at configure time. `core_word_blocks`
compares owned copies of output, warnings and mutated input over 1,457 bounded
valid cases: Unicode words, casing/classes, numeric codes, NASB-style exclamation
values, repeated annotations, styled text and original Greek/Hebrew SWORD output.
The fixture uses original prose, not a redistributed Bible module.

The characterization also exposes inherited defects: duplicate nested style tags
can modify the caller’s supposedly const input buffer, and recursive mixed styles
can emit the wrong closing tag because a static tag buffer is shared by frames.
These behaviors are recorded as defects to repair before owned native adoption;
they are not desired UI behavior. Tests deliberately give legacy code mutable
owned input and compare valid bounded shapes. Malformed/unsupported small/anchor
markup, non-ASCII morphology labels and concurrent calls are not accepted by this
harness. Legacy static output/tag buffers are not a thread-safe public interface.

Full local suite: 64/64. Focused address/undefined and thread checks: 1/1 each;
thread checks here are sequential characterization, not proof of concurrent safety.
External SWORD/GLib instrumentation limits still apply; leak detection is disabled.

Next: an owned, bounded block implementation with explicit supported/fallback
behavior, local recursion state, preserved valid lexical links and characterized
CSS selection. Integrate heading/body snapshots only after those contracts pass;
representative SDK/module, RTL/font, note/paragraph and runtime/visual acceptance
remain gates. No existing block-render source is retired by this characterization.
