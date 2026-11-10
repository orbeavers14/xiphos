# SWORD 1.9.0 hangs rebuilding a RawGenBook index after leaf deletion

**Status:** Reproduced locally. Candidate fix applies to the release source.
Build and end-to-end validation are pending. No upstream report has been filed.

**Environment:** macOS 27.2, arm64, SWORD 1.9.0 with CLucene 2.3.3.4 and ICU
78.3. The test used a disposable UTF-8 RawGenBook with generated text.

## Reproduction

1. Create a writable RawGenBook. Add a populated top-level entry and a
   separate section with one populated child. Build its search index.
2. Delete the index. Position the book on the only child and delete that
   entry through the SDK. Close and reopen the module.
3. Check that the child no longer resolves by path and the parent still
   resolves. Call `SWModule::createSearchFramework()` again.

**Expected:** The rebuild returns. An indexed search finds retained entries
and omits the deleted child.

**Observed in Xiphos:** The child disappeared from path lookup and the parent
survived. The subsequent index rebuild exceeded a 60-second test timeout.
Clearing the child body before unlinking did not prevent the hang. This
sequence was reproduced in Xiphos's general-book fixture. The shorter
SDK-only sequence above still needs independent confirmation.

## Candidate fix

`TreeKeyIdx::remove()` detaches the child from its parent but leaves the
physical index slot. `SWModule::createSearchFramework()` traverses those
slots and temporarily walks siblings to build proximity text. Its
parent/first-child reset can reposition the cursor before a detached slot,
causing the next increment to revisit that slot. Save the original
`TreeKeyIdx` offset before the sibling walk and restore it directly with
`setOffset()` before advancing.

Validation must also confirm that a successful rebuild does not index the
detached entry. The cursor fix alone does not establish that property.

[TreeKeyIdx source](https://crosswire.org/svn/sword/tags/sword-1-9-0/src/keys/treekeyidx.cpp) ·
[SWModule source](https://crosswire.org/svn/sword/tags/sword-1-9-0/src/modules/swmodule.cpp)
