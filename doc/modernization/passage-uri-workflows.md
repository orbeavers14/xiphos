# Passage URI dispatch boundaries

Current implementation through `5460417` includes ordinary dictionary clicks,
owned Book/prayer URI targets, native book-origin dispatch and transient pane
anchors. Full consumer evidence corrected unknown fallback pane visibility and
dictionary anchor transport. Real offscreen WebKit now exercises actual DOM link
activation through native worker services and automatic scrolling of shared
book/dictionary output. Chronological sections below describe the earlier scoped
steps; later corrections supersede their open-item notes. Native devotional pane
state, status/bookmark behavior and full platform/corpus acceptance remain open.

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

## Resolved general-book/prayer URI targets

Contextual module URIs now recognize installed indexed Generic Books, including
prayer lists, before verse parsing. After owner alias resolution and retained
anchor splitting, book_display_target interprets the raw display key; private SDK
query keys return the owned actual tree source. BookUriTarget preserves raw key,
canonical tree/offset and optional anchor. Numeric-prefix/minimum-four behavior,
relative display paths, slash numeric titles, empty explicit root, Unicode,
percent-looking text and punctuation use existing characterized book services.
Missing snaps, unsafe key drivers and oversized/overflowed addresses reject.

This target lookup does not select/render a book, visit Bible/dictionary history,
change search or write a profile. C exposes a distinct take-once book target with
a borrowed owned tree/key/anchor, surviving request and worker close. Older strict
single-passage APIs remain unchanged; book hover stays no-presentation. Native
book-target consumption and cross-links originating inside a book are the next
separate slice. No whole book/prayer workflow acceptance follows.

## Native book-target consumption and book-origin links

Mac and Linux models now consume copied book URI targets through existing shared
display-key restoration/document selection. They preserve path-root versus numeric
startup, actual target module, presentation/resource options and transient anchors.
Ordinary book navigation clears the anchor; local presentation revisions support
repeat targets. Mac BookPane uses the existing trusted isolated renderer; the GTK
book host is adapted to the same UTF-8 literal helper with weak-label lifetime and
obsolete-render checks, pending a current Fedora compile/runtime pass.

A shared book-pane command requires a committed document. It keeps note/image
commands on that owned book source, while module URIs reuse contextual routing
with separately supplied live main reference. This prevents relative Bible range
previews from using a tree path and keeps omitted modules tied to main context.
Explicit installed URI modules may work without main context; omitted/unknown
modules reject when no main context exists. C admission bounds inputs before
copies; worker rejection precedes FIFO admission. Original book-only APIs retain
their previous contracts. Main-origin and book-origin results use the same native
study-target presentation helper rather than separate navigation logic.

Scoped model checks cover actual Book/prayer documents, named/empty anchors,
main/draft/history/search/sibling isolation, book-origin Bible navigation and
relative previews, root/startup distinction and failure retention. Physical URI
clicks and full book/prayer/corpus/platform acceptance remain open. The next
anchor audit must include the full retained HtmlOutput consumer: unknown-module
fallback calls commentary before Bible, and dictionary display can consume the
global anchor despite receiving a stripped key. Earlier callback-only traces did
not emulate HtmlOutput consumption; those two native anchor cases remain open.

## Complete retained anchor consumer characterization

A new strict C++11 oracle freezes complete HtmlOutput from `39ead32`, retains its
GPL notice and compiles the current extracted function. It reuses the full URI
callback host and runs both complete URI/HtmlOutput pairs through deterministic
stream/jump seams: 432 frozen/current traces match across families/aliases,
references/ranges, commentary visibility, notebook switches and literal/empty
anchors. Original 18,484 callback and 19,562 split checks remain separate.

The full consumer proves that a successful render consumes special_anchor once.
Known Bible/commentary/dictionary/book/prayer paths consume in their target pane;
unknown fallback consumes in commentary when shown, otherwise Bible. Dictionary
HtmlOutput sees the global anchor even though its display callback receives only
the stripped lookup key. Bible range sidebar returns before output and retains
the previously characterized split leak. This characterizes stream/callback
behavior, not GTK DOM rendering or broad legacy resource correctness. Native
fallback/dictionary anchor corrections follow from these stronger observations.

## Unknown fallback anchor visibility correction

Anchored unknown-module targets now retain a copied main-fallback flag. Explicit
shared application receives current commentary visibility and assigns the
transient anchor to commentary first when visible, otherwise main Bible. Known
Bible/commentary targets retain their ordinary actual-pane rule. The flag is
separate from reference/history and survives copied C/Swift transport. Existing
linked-visit C variants keep known-target semantics; an added policy entry point
accepts exactly 0/1 fallback/visibility flags with the same pre-copy bounds.

Mac supplies actual book/commentary display state, including a selected but
hidden commentary while a book is shown. Linux supplies its visible paired
commentary state. Core and native models check both visible/hidden cases and
known-commentary switching from the book pane. This corrects the earlier native
unknown-anchor assumption using complete retained output evidence. Dictionary
anchor transport remains the following correction.

## Dictionary URI anchor consumption

Contextual dictionary results now carry an owned optional anchor beside the raw
entry. The retained splitter supplies the suffix; explicit follow normalizes and
records only the requested lookup key. A distinct DictionaryUriEntry wrapper
keeps shared C++11 content and hover results unchanged. The C entry getter copies
through Swift/Linux; nil means no scroll, empty means top. Pane revisions permit
repeated identical targets. Ordinary entry reads, hover and history snapshots
carry no URI anchor. This corrects the earlier callback-only assumption using
the full HtmlOutput oracle. Native model checks cover copying, repeat/top and
history clearing; physical activation and GTK DOM acceptance remain separate.

## Complete dictionary/devotional display characterization

A strict C++11 complete-function oracle compares 2,880 frozen/current dispatch
traces. On a changed module, DailyDevotion redirects to its own pane; keys with
exact NN.NN shape pass unchanged, all others use local today. This is shape-only
validation, including 99.99. Absent/current dictionary module selection bypasses
that redirect and follows ordinary dictionary casing/history. Null key defaults
to Grace; matching uppercase entry refreshes rather than activates. Invalid
UTF-8 returns after module/pane updates but before history/tab publication. The
frozen function and deterministic clock preserve these observable ordering
rules. Native devotional routing remains pending its dated-pane state service.

The complete dispatcher now delegates its changed-module decision to shared
C++11 dictionary_display_route. The policy returns dictionary, supplied
devotional key or host-local-today without SDK/UI/clock access or allocation.
Retained host handling preserves local-date lookup and callback order. The
complete-function oracle now covers all 10,000 numeric date shapes and seven
short/long prefixes (12,887 total comparisons). Native devotional presentation
remains a separate service; this extraction preserves the characterized rules.

## Real native WebKit activation evidence

Original-prose offscreen DOM links now exercise the production WK navigation
delegate and native model/worker services. Browser navigation is canceled before
shared Bible/dictionary/range dispatch; invalid UTF-8 preserves committed output.
Book-origin activation uses an existing owned document, resolves Book/prayer
targets and relative main references, and switches to known commentary. Actual
shared book/dictionary HTML also consumes copied named anchors automatically
on load in real WebKit. Module JavaScript remains disabled. This strengthens
callback/model evidence while physical interaction and GTK rendering stay open.
