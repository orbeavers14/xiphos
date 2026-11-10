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
agree. This matching applies to metadata lookup. Surface searches still use
the module's current phrase search behavior. The returned C snapshot owns the
actual module, canonical reference, original surface and ordered terms.

The SDK does not supply shifted mappings for every pair of canons. The test
uses the supported Synodal mapping. The ordinary chapter matrix keeps its
characterized textual behavior. Search-opened comparisons use SDK mappings
and retain that mode during navigation and session restoration.
