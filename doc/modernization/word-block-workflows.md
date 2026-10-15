# Word/Strong’s/morphology block layout: characterization

Updated 2026-10-15. Supports F03/F16/F17 rendering work. This is source behavior
characterization plus an owned core renderer and opt-in session snapshots;
native layout acceptance and frontend consumption remain pending.

`src/main/display.cc:block_dump`, `block_render_secondary` and `block_render` group
words and annotations into word/strongs/morph spans. They preserve lexical href
values while changing displayed morphology to lowercase/hyphenated text. Hebrew
pronominal display suffixes are shortened while full link values remain. Padding,
placeholder words, repeated annotations, styled recursion and SDK class shims
participate in layout. CSS then places annotations under each word.

Complete frozen helper/entry functions from CrossWire c9976a8 retain GPL notices.
Current complete functions are extracted at configure time. `core_word_blocks`
compares owned copies of output, warnings and mutated input over 1,638 bounded
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
ASCII morphology case conversion; unsupported morphology stays raw. The helper
now uses owned C++11 strings for scanner and annotation temporaries, with guarded
matches and bounded private lookahead; it still follows the characterized grammar
rather than implementing a general HTML parser.

The updated harness compares owned results alongside the 1,638 valid baseline
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
display case/hyphens and Hebrew suffix removal. Scanner word/annotation state and
recursive working text are owned strings, so temporary storage is released through
normal C++ lifetimes, including exceptions. Missing structural matches return through
the original-HTML fallback instead of being dereferenced.

The byte-based padding rule, full lexical hrefs, display-only truncation and
two-occurrence legacy class shim are preserved. Additional complete-oracle
comparisons cover empty morphology, short/trailing slash segments, pronominal
and non-pronominal suffixes, mixed case, parentheses and tabs. The corpus total
is 1,638, including 108 empty/style-boundary state cases across lowercase/uppercase
bold, italic and underline spans. The scanner no longer calls GLib allocation,
free or string-search functions; it owns recursive text copies, replaces the old
goto-based transitions and retains bounded lookahead for the characterized grammar.
Duplicate styles are flattened only on the private copy, and unsupported states
return through the existing fallback. The helper still uses SWORD's owned output
buffer; the overall core still needs GLib for other existing Unicode behavior.

Tests also check 3,039 structured deletions/insertions around valid lexical,
recursive-style and footnote inputs, in addition to 8,000 noise inputs and 400
concurrent renders. These bounded corpora pass normal/address/undefined/thread
checks; they are not exhaustive parser or native visual acceptance. Strict C++11
syntax checking now needs only the core and SWORD headers for this source file.
The legacy parser remains retained as the oracle. The new session snapshot path
below opts into this implementation; native frontends have not enabled it yet.

## Shared HTML/CSS layout decision

`layout_lexical_content` returns owned HTML, matching CSS and separate requested/
applied flags in one C++11 value. Strong's, lemmas or morphology request word
blocks; annotation rows take precedence over double spacing exactly as in the
legacy selection. Disabled layout leaves HTML unchanged. This helper does not
apply SWORD filters; it consumes already-rendered HTML and copied display flags.

When the supported grammar falls back, raw HTML is retained and annotation
positioning CSS is suppressed, since raw anchors may use the same class names
without the required word-span containers. Optional double-spacing CSS remains.
That is explicit fallback behavior for unsupported content, not a claim that
the unsafe legacy parser accepted it. NUL is rejected for every option state.

All 16 option combinations compare supported HTML with the frozen block renderer
and CSS with frozen declarations, and check unsupported fallback and NUL behavior
(48 policy cases). Another 400 concurrent combined-layout calls share no mutable
result state. The existing 400 renderer calls and other corpus tests remain.
This additive C++ helper is now used by opt-in session snapshots below. C/Swift/
Linux adapters and UI layout have not yet adopted it. Adoption must publish HTML and its matching CSS
together and preserve pane, option, heading/cleanup ordering and history contracts.

The real worker study-note fixture also passes its note-bearing snapshot through
this layout helper, extracts the unchanged SWORD-generated note URL and resolves
it through the existing core. A formatted ranged-note body similarly retains its
exact URL and expands through the shared reference-list service. Normal and
address/undefined/thread focused runs each pass 3/3, including prerequisite fixtures.
These integration checks exercise the helper explicitly; existing consumers retain
their prior behavior until enabling the separate snapshot path below.

That milestone must apply lexical formatting **before** image/font cleanup, as
the legacy body and heading paths do. Current session snapshots already perform
cleanup; simply formatting their returned HTML in each frontend would put the
operations in the wrong place and duplicate policy. Integrate the combined stage
inside the shared core, then carry copied HTML/CSS through the adapters together.

## Opt-in session and worker snapshots

`StudySession::set_lexical_layout_enabled` and its FIFO worker command enable a
consumer capability for a tab. This migration opt-in is separate from saved
module preferences and is off for existing consumers until they can consume HTML
and CSS together. Configured Strong's/lemma/morphology flags select layout through
the shared policy; unconfigured engine state is never guessed. Double-spacing preference adoption is recorded in
[the later shared workflow](double-space-workflows.md).

`Entry.lexical` owns CSS and requested/applied state for its body and heading;
`Chapter.lexical` reports one document-wide decision. Header/body layout is staged
before the existing image/font cleanup. Unknown-content discovery, diagnostics,
per-module state and heading-before-body cleanup remain core-owned. Dense and
sparse chapter HTML is rebuilt with its original SDK ordinals/selected anchors.

Since one pane shares one CSS document, one unsupported heading or verse causes
the whole pane to retain raw content and omit annotation positioning CSS. Nothing
from the staged layout is published partially. Entry body/header and chapter
neighbors therefore cannot accidentally inherit incompatible positioning rules.
This is explicit unsupported-content fallback, not proof of full legacy grammar
or visual acceptance. Paired panes each retain their own independent decision.

`core_lexical_snapshots` uses a private copy of generated original-prose modules;
it does not modify a shared fixture or installed library. It checks configured/
unconfigured/disabled states, image/font cleanup, headers, dense/sparse chapters,
paired history, unsupported headings/neighbors, tab isolation, save/import and
worker drain/closed-tab recovery. Native adapters remain unchanged at this first
core checkpoint; no GUI or legacy-source retirement acceptance is claimed.

The full local macOS suite passes 66/66; focused normal, ASan/UBSan and TSan
snapshot runs each pass 2/2 including the heading fixture prerequisite. External
SDK instrumentation and disabled leak-detection limits remain as above.

The C ABI now adds `xc_worker_set_lexical_layout_enabled` (strict 0/1, queued,
tab-local and history-neutral) and typed `xc_lexical_presentation` metadata for
entries, paired panes, chapters and chapter verses. Getters return `XC_OK` for
disabled and fallback snapshots, with separate requested/applied flags; CSS is
borrowed until its snapshot is released. Invalid/absent/bounds outputs reset to
`{NULL,0,0}` and return `XC_INVALID_ARGUMENT`. Valid output pointers never retain
stale CSS from another owner. C11 header consumption and C ownership tests cover
repeat access, absent commentary, invalid setters, closed tabs, later reads and
HTML/CSS lifetime after request/tab/worker destruction. Adapters must copy CSS.

The Swift adapter now copies this metadata into immutable Sendable values for
entries, paired panes, dense/sparse chapters and individual verses. Its explicit
capability command uses the same async worker without UI-thread waits. The new
`swift_lexical_snapshots` private-fixture check covers disabled/applied/fallback
states, per-tab isolation, history neutrality, closed-tab errors and values held
after all handles/worker are released. Its focused run passes 3/3 including the
two core fixture prerequisites. Later checkpoints below connect native view consumption without a second Swift formatting policy or SWORD engine.

Next at that checkpoint was snapshot/native adoption; the later sections below record its scoped implementation. Broader SDK/lemma/note/paragraph shapes and fallback policy still require validation. Representative RTL/font/module, keyboard/runtime and full GTK/
other-platform acceptance remain gates. No block-render source file is retired.

## Native consumption and browser-nesting guard

`1393edd` preserves captured h1–h6 wrappers outside the inline scanner and rejects
structurally unbalanced generated output in the combined layout API. A real Mac
window exposed a multiword font crossing word spans and displacing its annotation.
That case now returns the original HTML with no annotation CSS; SDK snapshot and
unit tests retain it. The low-level legacy-compatible formatter remains unchanged.
This conservative guard is not a complete HTML content-model validator: additional
same-tag/paragraph/font/corpus shapes still require characterization.

`d953adc` connects copied HTML/CSS to Mac main/commentary entry/chapter views;
CSS-only changes reload the document. The 68-test suite and scoped actual Strong's,
chapter and raw-fallback window checks pass. `0b40574` adds equivalent Linux model
copying and host document consumption; its 53 portable/core checks pass on Mac,
plus 4 focused ASan/UBSan checks with fixture prerequisites. Its GTK source still
requires Fedora compilation and desktop acceptance. Both document hosts retain
resource restrictions. Note/reference/dictionary preview snapshots retain their
previous presentation path; those families need their own scoped adoption.
