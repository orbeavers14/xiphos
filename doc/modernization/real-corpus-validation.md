# Opt-in real-world module validation

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


`core_real_corpus_check` is a manual runner built with core tests, without CTest
registration or automatic downloads. Supply a disposable SWORD library containing
these six modules. Downloaded data and source text are not committed to Git;
the runner logs counts/status and command metadata on failure, not module bodies.

| Module | Inspected version | Driver / markup | Distribution license |
| --- | --- | --- | --- |
| [ASV](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=ASV) | 2.0 | zText / OSIS | Public Domain |
| [OSHB](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=OSHB) | 2.1.4 | zText / OSIS | CC BY 4.0; supplied attribution retained |
| [StrongsGreek](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=StrongsGreek) | 2.0 | zLD / TEI | Public Domain |
| [MHCC](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=MHCC) | 2.0 | zCom / OSIS | Public Domain |
| [Pilgrim](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=Pilgrim) | 1.0 | RawGenBook / ThML | Public Domain |
| [KJVA](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=KJVA) | 3.1 | zText / OSIS; KJVA canon | GPL/general public use grant; supplied notices retained |

Metadata was inspected2026-11-03 on official CrossWire pages and downloaded
configurations. The original notices remain with isolated data. Review new
versions rather than assuming unchanged bytes or licenses. No dataset is downloaded
or enabled by the build.

For a standalone core build:

```sh
cmake --build build/core-indexed --target core_real_corpus_check
build/core-indexed/core_real_corpus_check /path/to/disposable/library
```

The runner checks ASV John3 and OSHB Genesis1, four ASV text modes, unpointed
Hebrew search, KJVA Tobit without KJV-canon substitution, MHCC Genesis1:1,
compressed TEI dictionary search/exact previews and mixed verse/dictionary/book/
commentary export. It visits all29 Pilgrim tree nodes and28 populated documents,
checks UTF-8 presentation and resolves521 distinct-per-page internal reference
links while retaining the selected book source. Pagination/cycles/output traversal
are bounded. The inspected versions produce1867 mixed export rows; counts describe
this dataset rather than universal hardcoded expectations.

This corpus exposed rejected book-origin `showRef` commands. Shared command
classification now sends scripture references through the existing contextual
reference dispatcher using the main Bible. Source-bound note/image handlers stay
in place. Generated URI tests verify main context, range references, invalid types
and unchanged book/history state; existing book note/image tests cover regression.

A separate manual runner creates a verified new clone before building indexes:

```sh
cmake --build build/core-indexed --target core_real_corpus_index_check
mkdir -p build/real-index-run
build/core-indexed/core_real_corpus_index_check /path/to/disposable/library \
  build/real-index-run/real-corpus-index-fixture --require-index
```

The destination must be new and have that exact final folder name. Index cleanup/
creation touches only the clone. Required mode fails if SDK creation/capability
is unavailable; optional mode returns77. Actual indexed execution without fallback
is checked for ASV, StrongsGreek and Pilgrim. The capable released SWORD/CLucene
build reports307/64/22 hits respectively. The primary SDK's inability to create a
usable index remains explicit, not a successful indexed test.

Both runners compare full source-tree SHA256 manifests before/after. Primary,
alternate, sanitizer and portable-on-Mac runs are recorded in [the Mac handoff](handoff-macos.md).
Existing generated Latin-1/UTF-8/RTL, keyed intervals and multi-format asset tests
remain separate evidence. This matrix does not establish every module/encoding/
canon, all semantic families/assets, physical-platform or full visual acceptance.

Inspected raw ZIP SHA256 fingerprints (data remains under ignored `build/`):

| Archive | SHA256 |
| --- | --- |
| ASV.zip | `8f3d708510f7122a947a1c3eaa775a03c8ef5b1d456833abf506b78e689fcf18` |
| OSHB.zip | `99e518b1a8c2263eb6ed3bd96f22ef0e1fb34feacbebf5dcaace99b6d7a44f97` |
| StrongsGreek.zip | `38f7d29db912d7c6a416cf19bfdefd9bdb9882193634229210a0d10b5cccc434` |
| MHCC.zip | `5599108faf01753864d51136a87f120d022e70867cc9df70206a0af3c3a5ef7d` |
| Pilgrim.zip | `3a3b5c246f36754d3da81aee6685f0c28b91a54173ac97a9c6a07a716c077a93` |
| KJVA.zip | `48483b1e7f6e27f3def800941d6c1593877eeb70ce406a11f193d302cbcd2238` |


## Selected-word metadata and comparison

Build and run `core_real_word_study_check` with a disposable module library and
a new comparison profile path outside that library. The runner performs no
downloads and prints no module text. It checks that corpus bytes remain unchanged.

| Module | Tested version | Coverage |
| --- | --- | --- |
| [OSHB](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=OSHB) | 2.1.4 | Pointed and unpointed Hebrew and Biblical Aramaic selections; literal zero-padded Strong's terms |
| [MorphGNT](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=MorphGNT) | 2.0.1 | Greek accents and breathings; word lemma plus Strong's; both terms find the source |
| [Peshitta](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=Peshitta) | 2.0 | Syriac surface search and a clean empty lemma result |
| [RusSynodal](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=RusSynodal) | 1.9.1 | Real Psalm 3:2 search result maps to KJV Psalm 3:1 |
| [ASV](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=ASV) and [KJVA](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=KJVA) | 2.0 and 3.1 | An installed ASV column has no Tobit verse; a missing control verse leaves prior comparison and main history intact |

The SDK lookup first uses canonically equivalent surface text. When no exact
match exists, a mark-folded lookup returns lemma terms only if the candidates
agree. This matching applies to metadata lookup. Explicit literal and mark-insensitive
surface modes share the same bounded verse-search pipeline. Literal matches
preserve marks and accept canonical Unicode equivalents. The mark-insensitive
mode removes combining marks attached to Greek, Hebrew and Syriac characters.
Word metadata lookup uses the same script-specific normalization. Both surface
modes exclude annotations without changing retained settings for other searches.
The explicit query is already UTF-8 and is not passed through the module's
encoding or markup filters. The generated Latin-1 fixture verifies this boundary.
Legacy phrase searches retain their existing behavior. The returned C snapshot owns the
actual module, canonical reference, original surface and ordered terms.

The ordinary chapter matrix keeps its characterized textual behavior.
Search-opened comparisons retain SDK mapping during navigation and restoration.
Cross-canon mapping uses known SWORD tables and a small explicit MT mapping
against the KJV/KJVA anchors. An unsupported pair never silently falls back to matching
verse numbers. A split opens at its first numbered control verse, and a split
cell carries separate HTML and source references for every numbered part.
Merged mappings retain the same actual source verse in each corresponding
control row. Real tests cover Leviticus 14:55–56 in Synodal/KJV, a Psalm mapping
starting at verse zero, and rejection outside verified MT coverage. The MT
slice covers Genesis 1:1–31 and MT Psalm 3:2–9 ↔ KJV/KJVA Psalm 3:1–8.
MT Psalm 3:1 is the title and has no numbered KJV/KJVA row. The corpus checks
both directions and mixed RTL/LTR columns using OSHB and ASV. The numbering
can also be checked against [Sefaria’s Psalm 3](https://www.sefaria.org/Psalms.3)
and the [eBible KJV Psalms](https://ebible.org/pdf/eng-kjv2006/eng-kjv2006_PSA.pdf).


The same runner checks complete plain-text and passive HTML passage export
from OSHB, MorphGNT and Peshitta, preserving the module and duplicate verse
order. It uses the ICU/CLucene-enabled SWORD build, compares the source tree
before and after, and emits no copied passage text.

Native readers can call `xc_worker_map_reference` with the installed source
module, destination module and current reference before switching translations.
Take the owned destination with `xc_request_take_link`. The resolver uses the
comparison mapping, returns the first numbered verse for splits, and leaves
history untouched. On an unavailable mapping it returns no link, so the caller
can keep the current passage open. The corpus runner checks OSHB ↔ ASV through
the standalone resolver, including C result ownership after worker shutdown.

Genesis 1 numbering was checked against [Mechon Mamre](https://mechon-mamre.org/p/pt/pt0101.htm) and the [ASV](https://ebible.org/asv/GEN01.htm).

### Resource export

An optional third runner argument points to a disposable library containing
[FreChry](https://crosswire.org/sword/modules/ModInfo.jsp?modName=FreChry) 2.2
(Public Domain). The module ships JPEGs and a preferred `style.css`. Its
Genesis 1:1 search result returns an explicit unsupported-resource error
when exporting with image embedding enabled. With images disabled for the
module, the same result exports as passive HTML without an image or an
external stylesheet reference. The corpus is checked for byte changes.
With a libjpeg-enabled build, set `XIPHOS_REQUIRE_JPEG_EXPORT=1` to also
verify the explicit raster policy decodes the JPEG and embeds it as PNG.
The earlier PNG/BMP policy continues to reject JPEGs. The opt-in
`xc_worker_export_search_rich_html_with_styles` path also verifies that the
shipped FreChry stylesheet contributes scoped text alignment. Export imports
a limited set of text formatting rules from the declared local stylesheet.
External URLs, fonts, at-rules and inline styles are outside that subset.


The verified MT table also covers Genesis 2:1–25 directly and Psalm 4:2–9
↔ ASV Psalm 4:1–8. The Hebrew title has no numbered English row. Sources
were [Hebrew Genesis 2](https://mechon-mamre.org/p/pt/pt0102.htm),
[ASV Genesis 2](https://ebible.org/asv/GEN02.htm), [Hebrew Psalm 4](https://mechon-mamre.org/p/pt/pt2604.htm),
and Psalm 4 in the installed ASV 2.0 corpus.

The Genesis 32 mapping crosses a chapter boundary. MT Genesis 32:1 maps to
ASV Genesis 31:55, and MT Genesis 32:2–33 maps to ASV Genesis 32:1–32.
The runner verifies both directions and checks adjacent comparison rows on
opposite sides of the boundary. See [Hebrew Genesis 32](https://mechon-mamre.org/p/pt/pt0132.htm).
The English side was checked in the installed ASV 2.0 module.
