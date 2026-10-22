# Click, hover and lexicon-profile contracts

Current scoped implementation and evidence, 2026-10-11. Frozen legacy handlers
come from CrossWire `c9976a8`; executable callback comparisons are distinct from
complete GTK/native runtime acceptance. No feature group is officially accepted.
See [status](status.md), [parity matrix](parity-matrix.md),
[core API](../../src/core/README.md) and [macOS guide](../../platforms/macos/README.md).
The [prior detailed milestone record](https://github.com/orbeavers14/xiphos/blob/7167bb13c1cd8f46c5454c4d9780568075267b20/doc/modernization/hover-workflows.md)
is retained in Git history rather than duplicated in the repository.

## Legacy main-handler contract

| Link/event | Hover | Click | Evidence |
|---|---|---|---|
| Strong's | Select main Greek/Hebrew preference, render raw value into information viewer | Delegate raw value to dictionary display, which normalizes and records its own history | Complete frozen/actual handlers: 135,258 combined lexical comparisons |
| Morphology | Select using committed main source/testament; render raw value into information viewer | Delegate to dictionary display | Same complete-handler characterization, including missing modules/settings, overlapping type tags and rendering failure |
| Footnote (`showNote`, type containing n) | Render Footnote body into information viewer | No corresponding body-display branch in inspected main handler | Complete frozen/actual show_note callback characterization; SDK output/runtime acceptance pending |
| Cross-reference note (type containing x) | If xrefs_in_verse_list is enabled, display sidebar list; otherwise information viewer contains reference links and Back to current verse | Display the sidebar verse list | Complete frozen/actual show_note callback characterization; default setting is enabled; scoped native preference/hover routing implemented, full acceptance pending |
| Explicit reference list (`showRef`) | No action in inspected show_ref | Display sidebar verse list | Shared C++11 dispatcher reused by GTK/native, complete frozen/actual show_ref traces including fallback, scoped native preview checks; whole workflow acceptance pending |
| sword:// dictionary entry | Render dictionary text in information viewer | Module-type routing determines display | Source observation in sword_uri/show_in_previewer; abbreviations/general keyed modules require separate characterization |
| sword:// non-dictionary reference | Update status text or bookmark tag information | Navigate appropriate pane | Source observation; do not infer that every reference hover should render a passage |
| Pointer leaves a link | Set in_url to zero, release cached URI, emit URI_SELECTED with null; no information-preview clear in inspected callback | Not applicable | Source observation in webkit/wk-html.c link_handler |


The WebKit callback copies the URI to main/dialog dispatch with clicked=false;
button press uses clicked=true. Release separately handles double-click text
lookup. Pointer leave clears active-link/URI transport state but retains the last
information content. Modifier/key gestures still require runtime validation.

Mixed n/x note tags take the reference branch on click and body branch on hover.
A clicked footnote sets source context without presenting a body in these main
handlers. Cross-reference list hover invokes the sidebar even for null attributes;
click does so only for nonnull attributes. Information-link mode always adds Back
to the committed main verse. These distinctions survive shared-policy extraction.

Detached-dialog Strong's/morphology selection uses its separate policy/source
key. Click and hover target the information dialog and dialog preview rather
than main dictionary display. Main-origin lexical clicks normalize/record main
dictionary history; dialog-origin and raw preview reads do not. Selection-prefix
characterization covers known differences; full dialog presentation is pending.

No preview-pin command/state was located in previewer, URL, WebKit, settings,
GTK/UI definitions or the English manual. F14's unsupported initial pin gate was
removed. `misc/pinnedtabs` populates linked tabs, a separate synchronization
workflow. A future preview-pin enhancement needs its own design/behavior scope.

## Shared/native implementation

`xiphos_core_note_presentation` is a C++11 CoreOptions policy with a C11 header.
Legacy delegates only destination/precedence decisions; source setup, nullable
attributes, retrieval and HTML remain in existing branches. It builds without
the experimental C++17 native core. Detached-dialog policy remains separate.

`StudyWorker::hover_study_link_at` and the C request API return copied body notes,
ordered reference lists, raw lexical entries or a no-presentation marker. Per-tab
cross-reference list mode defaults true; typed getter/setter commands do not visit
histories. Note/list retrieval uses existing shared settings and parsers. Explicit
showRef and passage hover return no presentation. The parser accepts the strict
local n/x domain; missing/mismatched notes and invalid context/URLs reject.

`contextual_hover_link` reuses the URI decoder. Absent/empty or decoded passages
shorter than five bytes use committed main context; unavailable explicit modules
fall back to the source module, while installed modules/full passages remain.
Raw/click parsers keep their stricter original contracts. Native main-handler
surfaces always supply the live committed main source, including retained older
information/sidebar views. Explicit URI targets remain authoritative; click
callbacks retain captured source context.

Reference-list mode renders owned passage snapshots. Information-link mode uses
owned `reference_targets`, preserving order, duplicates, normalized context,
skipped/limited metadata and the 2,000-verse per-range cap without rendering verse
bodies. Labels are escaped and Back targets committed context. Initial English
labels/theme styling are not localization or visual acceptance.

Cross-reference hover retrieves owned note metadata without rendering a discarded
body. Exact-source entry-attribute generation still occurs. Full-body callers
retain UTF8HTML/renderText processing. All required values are copied before a
body render may replace SDK attributes. Embedded NUL module/reference/identifier
inputs reject. No timing or quantified resource-savings claim is made.

The macOS trusted WKUserScript runs in an isolated client world, with module
scripts disabled and main-frame/type/size checks. Semantic callbacks cover main,
information, reference-list, note-body and dictionary-definition content. The
Swift adapter copies typed results; the model does not parse URI/type policy.
Body/lexical results share retained Information Preview. Reference results update
the sidebar while retaining information. Pointer leave retains content and resets
transport deduplication for reentry. Hover does not mark reading busy or visit
either history; only the latest live revision publishes. Reading/window actions
invalidate pending results. No-presentation/errors retain preview/status. Accepted
engine jobs still drain after observation cancellation. A per-window toggle copies
core reference mode; it is not yet persisted to the legacy XML profile.

Existing native clicked-note body presentation differs from the characterized
legacy footnote click contract and remains a parity gate. Generic dictionary URI,
status/bookmark-reference and detached-dialog hover, heading verse zero,
abbreviations, full nullable/malformed legacy input, keyboard ordering, real
modules, translation/design and Linux/Windows behavior remain pending.

## Lexicon XML profile

`main/settings.c:load_settings_structure` reads `Xiphos/lexicons`, separately
from the engine-options file:

| XML child | Shared preference | Startup default |
|---|---|---|
| greek | Greek Strong's | StrongsGreek |
| hebrew | Hebrew Strong's | StrongsHebrew |
| greek_morph_nt | Greek NT morphology | Robinson |
| greek_morph_ot | Greek OT morphology | Packard |
| hebrew_morph | Hebrew morphology | OSHM |


Strong's defaults apply to null getter results; morphology defaults apply to null
or empty. Empty/child-element-only XML nodes can yield null. First matching keys
in traversal order win, including later duplicate sections when earlier sections
lack the key. Nonempty unavailable module names are retained at loading time.
The legacy update helper changes existing keys only; appends use the first section
and can add duplicates. Preference callbacks write these keys; the three
morphology callbacks explicitly save after valid selection. Adjacent
`defaultdictionary`/`usedefaultdict` are separate policy and survive lexicon writes.

`parse_study_profile`/`read_study_profile` copy five effective choices and optional
`Xiphos/locale/special`; XML-declared encodings decode to UTF-8. They do not mutate
files, discover home profiles, validate installed modules or apply locale.
Independent parser contexts stop at DTD declarations before resource loading;
malformed/wrong-root, invalid NUL code units and over-16-MiB sources reject.
These explicit limits do not establish universal historical-profile compatibility.

`serialize_lexicon_profile`/`save_lexicon_profile` update the five effective nodes,
create missing keys, retain unrelated nodes/attributes/comments outside edited
values, shadow duplicates and saved locale, and normalize encoding to UTF-8.
Literal-text serialization corrects the characterized legacy ampersand data-loss
failure. Null/empty choices that cannot round trip through defaults reject.
Same-directory staged replacement verifies values/bytes and preserves existing
permissions; missing files begin with an Xiphos root. Existing malformed/empty
files, read-only failures and observed external edits remain intact. No
interprocess lock, crash-durability or Windows replacement validation is claimed.

Serialized worker/C/Swift commands and macOS explicit file panels now import/save
lexicon choices. Import publishes copied live results; save changes status only.
Draft/passage/history, other windows and unrelated engine preferences stay intact.
No implicit startup profile access or locale application occurs. Legacy forced
locale excludes translated NONE/literal None and tries saved name then .UTF-8.
That saved-locale startup ordering still needs characterization; changing process
locale in an active worker/window is not equivalent.

`parse_study_preview_preferences`/`read_study_preview_preferences` separately
read `misc/xrefsinverselist`. Missing/null getter results default true; bounded
legacy atoi prefixes yield false only for zero (non-numeric prefixes also yield
zero). Out-of-int-range prefixes reject instead of inheriting integer overflow.
Lexicon-only import ignores this unrelated setting even when it is invalid.
No native startup application, tab mutation or XML preview-mode save occurs.

## Evidence and open acceptance

| Local evidence | Scope and limitation |
|---|---|
| core_lexical_dispatch: 135,258 comparisons | Complete frozen/actual main handlers; 64 module availability combinations, testaments, null/empty preferences, 11 tags, four raw keys including Unicode, click/hover and render failures; explicit backend/display shims |
| core_note_dispatch: 96,792 comparisons | Complete frozen/actual show_note/show_ref; real GLib strings/lists, shim SDK/backend/presentation; guards, source/fallback/heading setup, tag precedence, null/empty attributes, modes, reference order and Back HTML traces |
| core_note_presentation_header: 20 cases | C boundary/null/empty/Unicode/nonzero-flag precedence; all complete-handler traces still agree after legacy adoption |
| core_xml_profile: 296 checks | Six frozen/actual helpers with real libxml2; five keys, Unicode/escaping/CDATA, empty/child-only nodes, duplicates, missing/malformed XML, mutation and unrelated-data preservation; release assertions active |
| core_profile_reader: 367 checks; core_profile_save: 39 checks | Getter/default equivalence, encodings, independent contexts, source/result lifetime, DTD/resource rejection, literal round trips and file failure/preservation cases |
| core_preview_profile: 77 checks | Actual XML getter plus observed bounded atoi/default expression; empty/duplicate nodes, signed/non-numeric prefixes, bounds, encodings, file preservation and independent copied results; not complete startup execution |
| core_hover and adapters/model | Real generated SWORD note/dictionary fixtures, both modes, raw lexical/body equality, histories/tab isolation, copied arguments/results, scalar kinds, no-op, errors, shutdown and stale publication |
| core_reference_targets: 64 generated cases | Equality with full snapshots across module/context/expression, limits/order and owned lifetimes |
| core_note_metadata: 16 passage/filter cases | Full-path metadata equality, settings, invalid/missing inputs and lifetime after later calls/close |
| core_hover_source: 12 generated combinations plus boundaries | Explicit/fallback modules/passages, five-byte/UTF-8 thresholds, parser isolation, errors/history/C lifetime |
| Temporary native GUI fixtures | Profile import/save/error, main body/crossref modes/Back, nested sidebar note hover and pointer-leave retention; draft/main/history isolation |

The current local suite passes 49/49. Scoped address/undefined/thread checks cover
shared ownership/state changes; strict C++11/C++17/C11 checks cover their seams.
SWORD, system libxml2 and GLib are not fully instrumented. The note harness tracks
handler GStrings and reclaims frozen early-return allocations after recording;
current allocation occurs after guards with identical traces. This is not a full
GTK/SDK leak audit. Fixture evidence does not replace representative-module,
legacy/native gesture, full visual/design, packaging or all-platform acceptance.


## Exact reference-command module selection

The shared `dispatch_reference_command` preserves show_ref's complete decision
and callback sequence. Hover returns immediately. Click checks exact installed-ID
membership and replaces unknown requested modules with the main module before
presenting the original context/expression. It deliberately does not consult the
abbreviation registry. Borrowed strings/callbacks live only for this dispatch.

Native pane commands now use navigation_module for omitted and unknown showRef
module tokens while retaining the supplied source reference for relative parsing.
Single-context callers use current_module for both module roles. Explicit installed
module IDs win. Other note/lexical/image source policies are unchanged. The older
single-reference resolver keeps its two-kind transport; contextual calls return
owned reference lists and do not record Bible navigation. This supersedes the
prior exact-only rejection guard introduced with typed URI aliases. See
[the Mac evidence](handoff-macos.md#shared-showref-module-dispatch-2026-10-22-active)
for actual checks and limitations.
