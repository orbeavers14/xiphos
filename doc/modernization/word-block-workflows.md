# Word/Strong’s/morphology block layout: characterization

Updated 2026-10-15. Supports F03/F16/F17 rendering work. This is source behavior
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
compares owned copies of output, warnings and mutated input over 1,530 bounded
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
native application. Native adoption of the owned block parser remains pending.

`render_word_blocks` now provides an owned C++11 interface around the characterized
valid grammar. It copies mutable working input, uses local output/tag/recursion
state and corrects inherited mixed-style closing tags. Existing valid content,
spacing and lexical value handling remain compared with frozen output, allowing
only that explicit closing-tag correction. The caller's input is unchanged.

The guard requires bounded balanced/case-matched markup, recognized small/em/anchor
annotation shapes and supported labels. Unsupported/malformed input, depth above
64 or input above 1 MiB returns original HTML with applied=false; NUL is rejected.
This is a conservative supported grammar, not a general HTML parser/sanitizer.
Non-ASCII morphology labels and broader SDK shapes remain corpus work. Unicode
words, a Greek lemma annotation's displayed text/full lookup value, and original
Greek/Hebrew SDK output are checked. Strong's/lemma labels do not run through the
inherited ASCII morphology case conversion; unsupported morphology stays raw. Internal
legacy-compatible GLib string operations remain behind this owned boundary and
still need later modernization; no unsafe API is exposed for unchecked input.

The updated harness compares owned results alongside the 1,530 valid baseline
cases, checks 8,000 bounded malformed samples, fallback/depth/size/NUL contracts
and 400 concurrent owned renders. Normal, address/undefined and thread checks
pass 1/1 each; strict C++11 and native-disabled helper builds pass. External SDK/
GLib instrumentation limits remain. The old legacy parser remains the source
oracle and has not yet been switched to this new guarded interface.

## Footnote markers and expanded SDK corpus

The corpus now includes one to eight lexical anchors in a single annotation,
Unicode lemma values, OSIS paragraph/multiple-lemma output and ThML lexical,
paragraph and passage-reference output across the four Strong's/morphology filter
combinations. These use generated original prose and complete frozen/current
comparisons. They do not substitute for a representative licensed module corpus.

SWORD footnotes emit ordinary lowercase anchors containing small/sup markers.
The scanner copies these anchors intact, so the guard now distinguishes their
opaque marker content from a standalone lexical small element. Surrounding words
can be formatted while the complete note marker, URI and displayed label remain
byte-for-byte unchanged. Actual OSIS note/reference output and synthetic markers
adjacent to Strong's/morphology annotations agree with frozen output. Note lookup
and routing semantics are unchanged; this is owned-helper support, not yet a
new frontend capability or visual acceptance claim.

Malformed markers, unsupported anchor shapes, unrecognized standalone small
elements and nested anchors still retain the original HTML with applied=false.
Mixed-case nested anchors are rejected consistently. Uppercase lexical annotation
anchors now fall back before the inherited lowercase-only pointer searches;
allowing those searches would dereference a missing closing-tag match. Ordinary
uppercase passage links without nested small markers remain supported. The unsafe
annotation case is tested only against the guarded helper, not the legacy oracle.

Expanded checks pass normally and under address/undefined and thread sanitizers;
strict C++11 and native-disabled helper builds also pass. SDK instrumentation and
leak-detection limitations above still apply. No public ABI or native consumer
source changes are required for this helper extension.

## Owned annotation formatting internals

The owned helper's annotation formatter now uses checked string offsets and
owned `std::string` edits for NASB exclamation correction, class-marker spacing,
display case/hyphens and Hebrew suffix removal. The scanner's allocations are
adopted with RAII before copying, so those formatting temporaries have one owner
even if string construction throws. Missing structural matches return through
the original-HTML fallback instead of being dereferenced.

The byte-based padding rule, full lexical hrefs, display-only truncation and
two-occurrence legacy class shim are preserved. Additional complete-oracle
comparisons cover empty morphology, short/trailing slash segments, pronominal
and non-pronominal suffixes, mixed case, parentheses and tabs. The corpus total
is 1,530. The surrounding scanner still uses legacy-compatible GLib operations
on its private working copy; this is a bounded internal modernization, not a
claim that the whole parser has been rewritten or adopted by native snapshots.

Next: validate broader SDK/lemma/note/paragraph shapes and fallback policy, then
adopt owned rendering/styles in heading/body snapshots with native controls and
visual evidence. Representative RTL/font/module, keyboard/runtime and full GTK/
other-platform acceptance remain gates. No block-render source file is retired.
