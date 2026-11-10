# SWORD 1.9.0 hangs rebuilding a RawGenBook index after leaf deletion

**Status:** Reproduced locally. A candidate fix passed the SDK-only reproducer in an
isolated build. No upstream report has been filed.

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
sequence was reproduced in Xiphos's general-book fixture and independently
with the [SDK-only reproducer](rawgenbook-index-reproducer.cpp). The stock
SDK printed `Rebuilding after deletion` and reached a six-second alarm.

Compile against an installed CLucene-enabled SWORD, then pass a fresh
directory. The alarm prevents the reproduction from running indefinitely.

```sh
c++ -std=c++17 rawgenbook-index-reproducer.cpp $(pkg-config --cflags --libs sword) -o reproduce
perl -e 'alarm 6; exec @ARGV' ./reproduce "$(mktemp -d)/library"
```

## Candidate fix

`TreeKeyIdx::remove()` detaches the child from its parent but leaves the
physical index slot. `SWModule::createSearchFramework()` traverses those
slots and temporarily walks siblings to build proximity text. Restoring the saved cursor offset after the sibling walk was insufficient.
The indexer can already stall when it attempts sibling navigation on the
detached slot. The [candidate patch](rawgenbook-index-candidate.patch)
checks that each tree slot still resolves to the same offset by path before
indexing it, and restores the original offset after a sibling walk.

Built from the SWORD 1.9.0 release tag with CLucene 2.3.3.4 and ICU 78.3.
On the same macOS machine, the patched SDK rebuilt the index and returned
one indexed hit for the retained entry and zero hits for the deleted entry.
The Xiphos deletion prototype also passed its general-book regression with
the patched library loaded and a usable CLucene index required. It verified
that the parent and open-tab state were consistent, the retained entry was
searchable, and the detached leaf was absent from rebuilt search results.
Xiphos integrates deletion as an opt-in for builds using the
[carried SDK patches](../../dependencies/sword/README.md).

[TreeKeyIdx source](https://crosswire.org/svn/sword/tags/sword-1-9-0/src/keys/treekeyidx.cpp) ·
[SWModule source](https://crosswire.org/svn/sword/tags/sword-1-9-0/src/modules/swmodule.cpp)
