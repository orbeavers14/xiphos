# Opt-in real-world module validation

`core_real_corpus_check` is a manual runner built with core tests. It has no
CTest registration and never downloads content. Supply a disposable SWORD library
containing these four modules; do not point it at an actively edited library.
No downloaded module or module text is stored in Git or printed by the runner.

| Module | Inspected version | Driver / markup | Distribution license |
| --- | --- | --- | --- |
| [ASV](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=ASV) | 2.0 | zText / OSIS | Public Domain |
| [OSHB](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=OSHB) | 2.1.4 | zText / OSIS | CC BY 4.0; attribution retained in supplied configuration |
| [StrongsGreek](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=StrongsGreek) | 2.0 | zLD / TEI | Public Domain |
| [MHCC](https://www.crosswire.org/sword/modules/ModInfo.jsp?modName=MHCC) | 2.0 | zCom / OSIS | Public Domain |

License/version metadata was inspected2026-11-03 on CrossWire's official pages
and downloaded module configurations. The original notices remain with the
isolated data. Download links are available on those pages; use HTTPS and review
new versions rather than assuming an unchanged archive or license.

For a standalone core build:

```sh
cmake --build build/core-indexed --target core_real_corpus_check
build/core-indexed/core_real_corpus_check /path/to/disposable/library
```

The runner checks ASV John3 and OSHB Genesis1 chapter composition, four ASV text
search modes, an unpointed Hebrew query through OSHB filters, MHCC Genesis1:1
commentary composition, compressed TEI dictionary search/exact previews and a
mixed Bible/dictionary/commentary text export. A complete source-tree SHA256
manifest before/after confirms the inputs remain byte-identical. Outputs are
counts/status only; inspected runs produced36/31 chapter verses and1464 export
rows. These counts describe the inspected versions, not a hardcoded universal
result count or acceptance of every verse and link.

Both local SWORD builds and the sanitizer build are exercised. This expands
real-world evidence beyond original generated fixtures. It does not establish
all modules/encodings/canons, actual indexing of these real modules, general-book
resources, every semantic link/asset, full visual workflows or physical-platform
compatibility. See [Mac handoff](handoff-macos.md) for exact checks and limitations.

Inspected raw ZIP SHA256 fingerprints (content stays under ignored `build/`):

| Archive | SHA256 |
| --- | --- |
| ASV.zip | `8f3d708510f7122a947a1c3eaa775a03c8ef5b1d456833abf506b78e689fcf18` |
| MHCC.zip | `5599108faf01753864d51136a87f120d022e70867cc9df70206a0af3c3a5ef7d` |
| OSHB.zip | `99e518b1a8c2263eb6ed3bd96f22ef0e1fb34feacbebf5dcaace99b6d7a44f97` |
| StrongsGreek.zip | `38f7d29db912d7c6a416cf19bfdefd9bdb9882193634229210a0d10b5cccc434` |
