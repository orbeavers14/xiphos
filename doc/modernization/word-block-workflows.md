# Word/Strong’s/morphology block layout: characterization

Updated 2026-10-12. Supports F03/F17/F18 rendering work. This is source behavior
characterization plus an initial owned core renderer; native layout acceptance
and snapshot adoption remain pending.

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

Full local suite: 65/65. Focused address/undefined and thread checks: 1/1 each;
thread checks here are sequential characterization, not proof of concurrent safety.
External SWORD/GLib instrumentation limits still apply; leak detection is disabled.

The C++11 `lexical_block_style` helper now returns owned CSS for two annotation
rows, one annotation row, double spacing or no additional styles. Strong's/lemmas
share one row; annotation rows override double spacing. Declarations retain the
baseline spacing/RTL rules. All three legacy renderer call sites delegate to the
helper and keep their other font/theme/module CSS handling. Temporary owned CSS
remains valid through the immediate formatter call.

`core_lexical_style` compares frozen declarations/selection, the owned helper and
all three extracted actual call expressions over 768 call-site cases, including
nonzero integer-to-bool conversion. Another 2,048 concurrent selections check the
owned constant data, independently of the unsafe legacy block parser. Strict C++11
and native-disabled helper builds pass; focused address/undefined/thread checks
pass 1/1 each. This establishes CSS data/selection, not RTL/font pixel layout or
native application. The native block parser remains pending.

`render_word_blocks` now provides an owned C++11 interface around the characterized
valid grammar. It copies mutable working input, uses local output/tag/recursion
state and corrects inherited mixed-style closing tags. Existing valid content,
spacing and lexical value handling remain compared with frozen output, allowing
only that explicit closing-tag correction. The caller's input is unchanged.

The guard requires bounded balanced/case-matched markup, recognized small/em/anchor
annotation shapes and supported labels. Unsupported/malformed input, depth above
64 or input above 1 MiB returns original HTML with applied=false; NUL is rejected.
This is a conservative supported grammar, not a general HTML parser/sanitizer.
Non-ASCII annotation labels and broader SDK shapes remain corpus work, while
Unicode words and the original Greek/Hebrew SDK fixture are checked. Internal
legacy-compatible GLib string operations remain behind this owned boundary and
still need later modernization; no unsafe API is exposed for unchecked input.

The updated harness compares owned results alongside the 1,457 valid baseline
cases, checks 8,000 bounded malformed samples, fallback/depth/size/NUL contracts
and 400 concurrent owned renders. Normal, address/undefined and thread checks
pass 1/1 each; strict C++11 and native-disabled helper builds pass. External SDK/
GLib instrumentation limits remain. The old legacy parser remains the source
oracle and has not yet been switched to this new guarded interface.

Next: validate broader SDK/lemma/note/paragraph shapes and fallback policy, then
adopt owned rendering/styles in heading/body snapshots with native controls and
visual evidence. Representative RTL/font/module, keyboard/runtime and full GTK/
other-platform acceptance remain gates. No block-render source file is retired.
