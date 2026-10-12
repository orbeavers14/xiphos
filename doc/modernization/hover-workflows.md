# Legacy click and hover contracts

Inspected 2026-10-11 at the modernization source, with frozen main lexical
handlers from CrossWire baseline `c9976a8`. This separates executable evidence
from source observations; it is not full GTK/native interaction acceptance.

## Main reader behavior

| Link/event | Hover | Click | Evidence |
|---|---|---|---|
| Strong's | Select main Greek/Hebrew preference, render raw value into information viewer | Delegate raw value to dictionary display, which normalizes and records its own history | Complete frozen/actual handlers: 135,258 combined lexical comparisons |
| Morphology | Select using committed main source/testament; render raw value into information viewer | Delegate to dictionary display | Same complete-handler characterization, including missing modules/settings, overlapping type tags and rendering failure |
| Footnote (`showNote`, type containing n) | Render Footnote body into information viewer | No corresponding body-display branch in inspected main handler | Complete frozen/actual show_note callback characterization; SDK output/runtime acceptance pending |
| Cross-reference note (type containing x) | If xrefs_in_verse_list is enabled, display sidebar list; otherwise information viewer contains reference links and Back to current verse | Display the sidebar verse list | Complete frozen/actual show_note callback characterization; default setting is enabled; native preference/hover routing pending |
| Explicit reference list (`showRef`) | No action in inspected show_ref | Display sidebar verse list | Complete frozen/actual show_ref characterization; current native click list service is partial workflow evidence |
| sword:// dictionary entry | Render dictionary text in information viewer | Module-type routing determines display | Source observation in sword_uri/show_in_previewer; abbreviations/general keyed modules require separate characterization |
| sword:// non-dictionary reference | Update status text or bookmark tag information | Navigate appropriate pane | Source observation; do not infer that every reference hover should render a passage |
| Pointer leaves a link | Set in_url to zero, release cached URI, emit URI_SELECTED with null; no information-preview clear in inspected callback | Not applicable | Source observation in webkit/wk-html.c link_handler |

The WebKit callback passes the copied URI to the main or dialog handler with
clicked=false. Link button press invokes clicked=true; release handles the
separate double-click selected-text lookup workflow. The information viewer
retains its last content on pointer leave; a native async implementation must
characterize stale-result ordering and retain that workflow rather than clearing
the panel on every leave event. Full modifier/key and click gesture behavior
still requires runtime validation.

## Main versus detached dialog

Dialog Strong's/morphology handlers select with the separate dialog policy and
source key. Their click and hover destinations are information dialog and
dialog preview, respectively; they do not call the main dictionary display.
Existing selection-prefix characterization covers the known main/dialog
differences. Complete dialog presentation callback characterization remains
pending. The new recorded lexical API therefore normalizes/records main-origin
clicks only; dialog-origin and raw preview reads do not visit main history.

## Correction to the initial feature inventory

No preview-pin command/state was located in the inspected previewer, URL,
WebKit, settings, GTK/UI definitions or English manual. The original F14
inventory's pinned-preview requirement was unsupported and has been removed.
The settings key `misc/pinnedtabs` populates `settings.linkedtabs`, a separate
tab-synchronization workflow; it is not evidence of preview pinning.
A future native preview-pin enhancement would require its own product/design
scope and shared behavior contract. It is not counted as a legacy parity gate.

## Characterization scope and implementation seam

`core_lexical_dispatch` compiles complete actual main handlers extracted from
main/url.cc and compares return values plus ordered selection/context/render/
display/preview calls against frozen baseline functions. It covers 64 module
availability combinations, both testaments, configured/null/empty morphology
preferences, 11 type strings, four raw keys, click versus hover, successful versus
failed rendering and null/empty Strong's preferences. Unicode values reach the
hover render and click delegate unchanged; normalization belongs downstream.
It uses explicit backend/presentation shims, not a live GTK or real-module run.

## Initial macOS lexical transport

The native reader now sends main-frame pointer-over links through a trusted
WKUserScript in the isolated client world, with module scripts disabled. The
core's existing no-record lexical API parses/selects/renders the URI using the
view's committed source context. A separate information sidebar holds copied
Strong's/morphology results; leaving a link retains content and resets transport
deduplication so reentry can refresh it. Payload type/frame/size are checked.

Hover reads do not mark the reading controls busy or visit either history.
Only the latest request publishes; reading actions/window close invalidate
pending publication, and failures retain the previous preview/status. Accepted
engine actions still drain when a Swift observation is canceled. Dictionary
click/close operations keep the information preview independent. This transport
currently handles lexical links in the main reader/information view; notes,
reference lists, dictionary-definition hover and detached dialogs are pending.
Rapid pointer/keyboard ordering and representative real-module/platform checks
remain acceptance gates. This is a scoped initial hover implementation.

The existing worker's no-record lexical snapshots provide the shared lookup
seam. Native hover transport should supply the committed source from its view,
leave URI parsing/selection/rendering in C++, and publish copied results to the
information preview rather than replacing the dictionary click pane. Do not
claim all-link hover parity from lexical-only transport. Notes, cross-reference
preference routing, status-only references, detached dialogs, keyboard behavior,
real modules and all three native frontends remain separate acceptance checks.

## Lexicon profile persistence seam

Source audit of the current fork, 2026-10-11, supplemented by executable
characterization of six complete XML helpers; no native importer is implemented.
`main/settings.c:load_settings_structure` reads the following XML children
under `Xiphos/lexicons`; they are separate from the engine-options file.

| XML child | Shared preference | Startup default |
|---|---|---|
| greek | Greek Strong's | StrongsGreek |
| hebrew | Hebrew Strong's | StrongsHebrew |
| greek_morph_nt | Greek NT morphology | Robinson |
| greek_morph_ot | Greek OT morphology | Packard |
| hebrew_morph | Hebrew morphology | OSHM |

Strong's defaults apply when `xml_get_value` returns null. Morphology defaults
apply to null or empty returned values. XML node presence must be characterized
separately: `xml_get_value` calls `xmlNodeListGetString` on the child list, so
an empty element may return null even though the node exists. The shared API's
explicit nullable/empty values do not establish file-level import semantics.
Nonempty configured names are retained at this loading seam without checking
whether their modules are installed.

`main/xml.c:xml_set_value` updates an existing node only; it does not create
missing nodes. `xml_add_new_item_to_section` appends a child only when the
section already exists, without replacing a duplicate child. The current
morphology startup default path uses that append operation; the Strong's path
uses the update-only operation. `xml_find_prop` returns the first matching
child in traversal order. Do not silently normalize duplicate/missing nodes
before fixtures establish their observable behavior and a repair policy.

The preference-dialog callbacks write these exact keys. The three morphology
callbacks also explicitly save the settings document after a valid selection.
Adjacent `defaultdictionary` and `usedefaultdict` settings belong to the
separate default-dictionary policy and must survive lexicon-only updates.

Saved locale is `Xiphos/locale/special`. The legacy startup applies a forced
locale after loading settings, excluding the translated NONE sentinel and
literal None; it tries the saved name and then its .UTF-8 suffix. The native
host currently initializes only the environment locale before worker startup.
Importing this setting requires a startup-order contract; changing process
locale inside a worker or while other windows are active is not equivalent.

Next implementation gate: disposable XML fixtures for absent/empty/duplicate
nodes, escaping, unavailable module names, malformed input and unrelated
sections; then a shared owned profile snapshot and explicit import/save that
preserves unrelated data and verifies replacement before publication. Never
infer XML compatibility from the existing engine-options import/save tests.

`core_xml_profile` now compares six complete current helpers against frozen
CrossWire `c9976a8` helpers using real libxml2, with 296 comparisons/assertions.
Fixtures cover all five keys, unavailable names, Unicode, escaped text, CDATA,
whitespace, child elements, duplicate keys/sections, absent sections/keys and
malformed documents. The getter returns null for empty elements and for a
child-element-only value; update-only writes leave absent keys unchanged,
while appends use the first section. A later section supplies a key only when
earlier sections lack that key. Serialized documents agree after mutations,
including unrelated content and comments. Assertions remain active in release
builds. These checks exercise XML helper semantics, not complete startup/UI.

The legacy update setter interprets entity references: an unescaped ampersand
produces a parse diagnostic and clears content in the checked libxml2 build.
The test records this observed failure rather than expecting literal-text
round trips from that helper. A new explicit writer must serialize literal
names safely without reproducing that data-loss behavior; document this scoped
correction and validate escaping/round-trip preservation independently.

The shared `parse_study_profile`/`read_study_profile` reader is now implemented
with 367 local checks, including comparisons with the actual getter/default
rules. It copies five effective preferences and optional saved locale, decodes
XML encodings into UTF-8, preserves unavailable names and does not mutate files
or process locale. Independent parser contexts and owned results support
concurrent read callers. DTD declarations stop parsing before resource loading;
malformed/wrong-root input, invalid NUL code units and over-16-MiB sources reject.
These explicit limits are not evidence of accepting every historical profile.
The explicit literal-text writer now passes 39 preservation/round-trip/failure
checks. It retains unrelated XML outside edited values, duplicate keys and saved
locale, normalizes encoding to UTF-8 and verifies effective values before staged
file replacement. It corrects the characterized ampersand data-loss behavior.
Null/empty choices cannot round trip through legacy defaults and reject. Existing
malformed/empty files and observed external edits remain intact on failure. The
next gate is worker/native import/save integration and saved-locale startup
characterization; complete storage/platform acceptance remains pending.

## Complete main note/reference callback characterization

`core_note_dispatch` compares full frozen/current show_note and show_ref handlers
over 96,792 ordered callback/return comparisons. Backend, module/key and display
operations are explicit shims; GLib strings/lists are real. Cases cover click/
hover, active-link/null/empty identifier guards, installed/fallback modules,
short/current/explicit passages, heading verse-zero setup, overlapping n/x tags,
null/empty/populated attributes, render success/failure, cross-reference preference
modes and empty/single/repeated parsed references. It also checks generated
reference-link and Back-to-current-verse HTML through the presentation trace.
These comparisons establish dispatch, not real SWORD parser/render output,
versification, GTK gestures, translation or native full-link parity.

The harness tracks only the handler GString lifecycle and reclaims frozen
early-return allocations after recording them. The actual handler now allocates
after the active-link/identifier guard; all traced destinations/arguments remain
identical and its tracked GString is released or never allocated on every case.
This corrects the source-visible early-return leak; scoped sanitizer checks are
not a complete leak audit of GTK, SWORD or uninstrumented GLib.

Note tags containing both n and x dispatch as reference lists on click, but
use body preview on hover because the n branch comes first. A clicked footnote
sets source/module context but does not show its body in these handlers.
Cross-reference hover in list mode calls the sidebar even for a null attribute;
the click branch calls it only for a nonnull attribute. Information-link mode
always appends Back to the committed current verse. Preserve these distinctions
when extracting policy rather than merging apparently similar branches.

The main note presentation decision now lives in the C++11 CoreOptions helper
`xiphos_core_note_presentation`, with a C11 header. The legacy handler delegates
only its destination/precedence decisions; source setup, retrieval, nullable
attribute handling and generated HTML remain in their existing branches. All
96,792 complete frozen/current traces still agree after adoption. Twenty direct
C boundary/precedence cases cover null/empty/Unicode tags and nonzero flags.
The default legacy build includes the helper without the C++17 native core.
This is the policy seam for native note hover; the native hover transport is
still lexical-only at this checkpoint. Detached-dialog policy is separate.

## Shared native main-hover service

`StudyWorker::hover_study_link_at` and its C request API now return owned body
notes, ordered reference lists, raw lexical entries or a copied no-presentation
marker. Each tab owns the cross-reference list preference with the legacy true
default; typed setter/getter commands do not visit histories. Notes use existing
exact-source/settings retrieval and the shared main routing policy. List parsing
uses committed view context, not unsubmitted input or the hovered note passage.
Information-link mode returns escaped reference labels plus Back to that context;
its initial English label/theme styling is not full localization/visual parity.

The service accepts the existing strict local n/x parser domain. Missing/type-
mismatched notes and invalid context/URLs reject; copied results remain valid
after later calls and shutdown. It does not reproduce every legacy malformed/
nullable-attribute case, heading verse-zero setup, abbreviation/module fallback
or short-passage substitution. Explicit showRef and passage links have no
presentation; status/bookmark/dictionary-URI and detached-dialog hover remain
pending. The existing raw preview/click APIs and native click behavior are
unchanged in this service milestone. Native click destination adaptation must
be validated separately against the characterized note policy.

`core_hover` uses generated real SWORD note/dictionary modules to validate both
reference modes, note-body equality with the existing service, contextual list
order/duplicates, raw lexical behavior, tab/history isolation, copied arguments/
results, scalar/result kinds, no-op markers, failures and shutdown ownership.
These tests supplement the frozen callback harness; they do not replace full
GTK/native gesture, representative-module or platform acceptance. The new
service is not yet connected to the Swift/native pointer transport here.
