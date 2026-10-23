# Passage URI dispatch boundaries

Updated 2026-10-22. This records retained behavior and partial native coverage;
no whole workflow or new parity group is accepted.

## Complete retained callbacks

The C++11 oracle freezes complete `show_in_previewer` and `sword_uri` callbacks
from `e0e9a86`, with their GPL notice, and compiles current callbacks extracted
from `main/url.cc`. Deterministic host callbacks compare 18,436 traces and final
state across explicit/missing/unknown modules, aliases (including Unicode and
uninstalled alias targets), every retained module family, punctuation/ranges,
anchors, commentary visibility, notebook page, bookmark tags, preview render
failure, null navigation results and recursive entry. Each hover gets a mutable
copy of the URI and must restore its temporary slash change. This characterizes
callback dispatch, not actual GTK/SWORD/WebKit rendering or malformed legacy URL
safety. Native parsers retain their stricter supported-origin/syntax boundary.

Observed click behavior:

- Resolve a nonempty declared module or the main-module default through the
  abbreviation map first.
- A Bible module plus any `-;,` reference marker opens the sidebar before the
  final history flag. Other families do not take this range branch.
- Known Bible/commentary/dictionary/book/prayer modules use their family route.
  Commentary/book routes change the notebook page as needed and preserve the
  anchor pointer around that page change.
- Unknown modules navigate the main reader through its navigation controls, then
  update Bible/commentary/sync callbacks. They do not simply return an error.
- The normal click tail sets addhistoryitem; recursive URI entry returns zero.

Observed hover behavior: dictionary targets render the information viewer;
other families update status text, preferring bookmark tag information. Hover
performs no click navigation/history dispatch.

The frozen Bible-range early return retains a split-string allocation. The
oracle records that allocation behavior and reclaims buffers after every trace,
so the matrix itself remains bounded. This is recorded evidence, not a claim
that this retained resource defect has been repaired.

## Current native scope

Typed single-passage sword/bible resolution consumes the bounded owner alias
catalog and returns owned actual module IDs. Command showRef module fallback is
separate and shared with GTK. Unknown single-passage URI modules now fall back
to the exact installed main module after alias lookup reports ordinary absence; invalid alias metadata is
still an error. The owner checks installed verse-key module identity without navigating. A
single-target result preserves the decoded reference; ordinary session reading
performs the SDK reference parse when the host applies it.
Native pane dispatch preserves original URI and main/source roles, so fallback
does not accidentally use an unknown offered name or the commentary source.
An unavailable fallback or known non-verse target still fails.

Contextual native sword/bible range/list URIs now use the extracted retained
Bible-only sidebar decision. Literal URI tokens and decoded expressions stage
before owner alias/type resolution. Installed Bible targets return bounded owned
reference snapshots, preserving supplied source reference, ordered duplicates,
per-tab rendering options, actual source identity and limited disclosure. Missing
URI modules use main navigation context. Unknown range modules now return a
canonical target in the exact installed main module; known regular/personal
commentary ranges return a canonical target in their actual module. Those cases
use a cloned live VerseKey and shared retained normalization, with full canonical
reference text rather than its English-only short display label. The host applies
the target through existing shared navigation/history. Bible ranges still return
sidebar previews. Dictionary/book range routes remain separate typed work.
Contextual URI input is capped at 32768 bytes; rendered and link-only native
previews cap the ordered prefix at 2000 total entries with visible truncation
disclosure. Older two-kind/Reader single-target APIs keep strict single references.
No main/history/search mutation occurs when the preview is returned.

The retained GTK callback now calls the same C++11 Bible/range decision; complete
18,436-trace frozen/current comparison preserves type-lookup callback order.
Native anchors, keyed-module click navigation and status/bookmark behavior remain
separate work. Dictionary URI hover now has the scoped copied path below.
See [alias policy](module-abbreviations-workflows.md),
[hover policy](hover-workflows.md) and [platform evidence](handoff-macos.md).


Native URI decoding now rejects incomplete/non-hex percent escapes before owner
module lookup or unknown-module fallback. Valid encoded percent signs/literal
plus/Unicode decode once; percent-looking text produced by one decode is not
decoded again. This tightens the native typed parser boundary; the complete
retained callback oracle does not claim malformed legacy URL safety.


Study-link admission now rejects URIs over32768 bytes and each module/source
reference over4096 bytes at all existing main study-link C entry points, before
observer/string allocation. Bounds exclude the NUL terminator. Existing nullable
current-module and required-context rules remain unchanged. Invalid C submissions
return XC_INVALID_ARGUMENT and clear output immediately, rather than returning a
queued request that later fails. Valid syntax/module/tab checks remain on the
single SDK owner. C++ by-value inputs are already caller-owned; oversized/NUL
values return ready exceptional futures before FIFO admission. This does not
claim to prevent allocations in caller argument construction. Synchronous
Reader.resolve_link uses the same bounds. Swift guards imported C constants and
embedded NUL before withCString, including older resolveStudyLink/studyNoteLink.

The admission test interposes C++ allocation on the submitting thread and proves
oversized/unterminated C calls create neither an observer nor a string copy.
It also checks exact byte boundaries with Unicode, invalid navigation/context
fields, seven request routes, synchronous C, ready futures, valid copying/drain,
search/history isolation and post-owner result lifetime. Other APIs (book-only
links, direct reads, resources) retain their separate admission contracts.


Navigation normalization now shares retained get_valid_key's key mutation and
label operation in C++11 CoreRender. The wrapper's fallback/malloc ownership and
locale/type conditions remain intact. A complete frozen/current adapter matrix
compares13,446 real SDK key traces across RawText/RawCom/RawFiles and KJV/KJVA.
Native cloned target consumption now compares72 cases against the frozen full
get_valid_key and module classification using original Text/alternate-canon/
regular/personal fixtures. Cloned resolution preserves subsequent relative key
behavior and publishes no history/search change. Full navbar/XML/IPC and broad
locale/corpus acceptance remain separate.

The retained click anchor split now uses a shared strict C++11 helper. It mutates
only a caller-owned reference buffer, returns a borrowed suffix (including an
empty suffix), and prefers the first hash over any earlier bang. The complete
frozen/current callback matrix remains equal; 19,562 short/Unicode-prefix cases
also compare every resulting byte and anchor offset with the literal retained
branch. This helper alone does not add native anchor presentation.

## Dictionary URI hover

Main hover recognizes bounded exact sword/bible origins, decodes once with the
retained plus-space rule, resolves declared aliases on the SDK owner, and reads
actual dictionary-family modules (including DailyDevotion SWLD modules) with
per-tab options. The full decoded keyed path is preserved, including `-;,/?!#`;
hover does not apply the click anchor split or scripture parser. Module URI
parsing returns copied strings and rejects malformed escapes, control bytes,
missing/empty paths and input beyond 32768 bytes. Older single-passage APIs retain their existing narrower syntax; contextual
ordinary dictionary clicks use the keyed route described below.

The retained hover's separator search is limited to the first 20 module bytes:
only nonempty declared tokens shorter than 20 bytes can preview. A short alias
may resolve to a longer actual ID. An empty declared module does not inherit
the main Bible. Missing/non-dictionary/too-long declared modules return the
existing no-presentation result; status/bookmark text remains a separate gate.

A new DictionaryEntry hover alternative uses the existing C dictionary take
handle with take-once ownership; it is not a lexical result. Swift and Linux
models consume that copied value in the information viewer. Navigation, reading
drafts, search generations, dictionary history and other tabs remain unchanged.
Accepted requests drain during shutdown and copied entries outlive the owner.
The complete retained callback oracle adds 48 keyed dictionary traces (18,484
total), including raw punctuation/Unicode, daily dictionaries and 19/20-byte
declared names. Model checks are not physical GUI interaction or corpus proof.

## Ordinary dictionary URI clicks

Contextual URI dispatch now recognizes ordinary installed dictionaries before
scripture parsing. It resolves explicit aliases (without the hover token-length
limit); omitted tokens use main navigation context. Keys retain slash/list/query
bytes, then the shared click helper strips the first hash, or first bang if no
hash exists. A hash takes precedence over an earlier bang. Empty resulting keys
reject. Explicit follows apply the existing Unicode uppercase key policy and
record the actual module plus normalized requested key through shared dictionary
history. The returned DictionaryEntry independently carries the SDK-matched key.
Resolve-only requests read without adding history. Unknown/non-dictionary paths
keep their prior verse routing; older two-kind/Reader single URI APIs stay strict.

C contextual results use take_dictionary_entry; Swift/Linux present through
existing dictionary choices/history services with no fabricated lexical source.
Reading drafts, Bible navigation/history, committed search and sibling windows
are preserved. Back/Forward uses existing dictionary navigation. Native dictionary
clicks do not consume their stripped anchor as a scroll destination; the retained
dictionary display branch likewise receives only the stripped key.

DailyDevotion hover is a raw SDK dictionary read, but its click route has a
separate retained date/pane/module-switch contract in main_display_dictionary.
Contextual DailyDevotion clicks explicitly reject pending that typed service,
rather than adding an ordinary dictionary history entry. Book/prayer clicks,
Bible/commentary anchor presentation and status/bookmark behavior remain separate
acceptance gates. No whole URI workflow or GUI/corpus acceptance is claimed.

## Owned contextual verse anchors

Contextual sword/bible parsing now separates the decoded click anchor from the
reference before routing. PassageLink and staged ReferenceListLink carry an
optional owned suffix, retaining absent versus explicit-empty anchors, hash-over-
bang precedence, Unicode and single-decode semantics. Anchor bytes remain literal
data, including quotes, slashes and percent-looking text. Query/slash reference
syntax, controls, malformed escapes and empty references still reject.

Anchored single Bible/commentary/unknown-main targets use the cloned canonical
navigation helper; commentary/unknown range targets retain the copied anchor.
Bible range URIs still return a sidebar snapshot after stripping the anchor and
do not navigate. C xc_link_anchor returns NULL for absent and a borrowed empty
string for explicit-empty; ownership follows the link handle. Swift copies that
optional value before release. Older single URI APIs still reject fragments.
The initial transport step (`65fa7e4`) did not apply a view action. Shared explicit
pane application now follows below; resolution itself still publishes no history.

## Explicit pane anchor application

Shared visit_linked_passage returns a transient PassageAnchor naming the actual
Bible/commentary target module, prepared before the visit publishes history.
The anchor appears only on that returned snapshot. Ordinary reads, refreshes and
history navigation have no transient anchor; histories contain only normalized
references. C adds an optional-anchor linked visit and borrowed passage getters,
preserving NULL/empty semantics and copied lifetime. Both linked-visit C variants
bound modules to 4096 bytes and reference/anchor to 32768 before argument copies
or observer allocation; C++ rejects oversized/NUL fields before FIFO admission.

Mac and Linux models consume the core target-pane identity, override only that
pane's default selected-verse anchor, and retain the other pane's default. A local
presentation revision permits repeated same-content/same-anchor clicks to scroll
again. Explicit empty means top. Native hosts use isolated trusted code with
module scripts disabled; ID and legacy named anchors work, literal UTF-8/quotes/
backslashes remain data, and missing targets report a presentation failure. Mac
discards obsolete async scroll failures after a newer anchor/navigation.

An offscreen Mac WebKit fixture now exercises real DOM scrolling, including
Unicode/quote/backslash/closing-script text, named targets, top/repeat, missing
anchors, obsolete completions and disabled module scripts. This is rendering
evidence, not a physical end-user URI click or full workflow acceptance. Linux
model/helper checks run on Mac with GTK runtime OFF; changed GTK host code still
requires a current Fedora build/runtime check. Windows presentation remains open.
