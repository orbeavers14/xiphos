# Legacy click and hover contracts

Inspected 2026-10-11 at the modernization source, with frozen main lexical
handlers from CrossWire baseline `c9976a8`. This separates executable evidence
from source observations; it is not full GTK/native interaction acceptance.

## Main reader behavior

| Link/event | Hover | Click | Evidence |
|---|---|---|---|
| Strong's | Select main Greek/Hebrew preference, render raw value into information viewer | Delegate raw value to dictionary display, which normalizes and records its own history | Complete frozen/actual handlers: 135,258 combined lexical comparisons |
| Morphology | Select using committed main source/testament; render raw value into information viewer | Delegate to dictionary display | Same complete-handler characterization, including missing modules/settings, overlapping type tags and rendering failure |
| Footnote (`showNote`, type containing n) | Render Footnote body into information viewer | No corresponding body-display branch in inspected main handler | Source observation in main/url.cc show_note; full callback characterization pending |
| Cross-reference note (type containing x) | If xrefs_in_verse_list is enabled, display sidebar list; otherwise information viewer contains reference links and Back to current verse | Display the sidebar verse list | Source observation; default setting is enabled in main/settings.c; native preference/hover routing pending |
| Explicit reference list (`showRef`) | No action in inspected show_ref | Display sidebar verse list | Source observation; current native click list service is partial workflow evidence |
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
The next gate is a safe literal-text writer preserving unrelated settings, then
worker/native import/save integration and saved-locale startup characterization.
