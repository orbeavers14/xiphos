# Shared Xiphos application core

Build/test independently of the GTK application:

```sh
cmake -S src/core -B build/core -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core --parallel
ctest --test-dir build/core --output-on-failure
```

Requires CMake >=3.16, C++17, SWORD >=1.8.1, GLib >=2.32 and libxml2 headers/library. Legacy-linked helpers retain
C++11 compatibility. pkg-config discovery is preferred; SDKs can instead supply
`SWORD_INCLUDE_DIR` and `SWORD_LIBRARY`, with
`XIPHOS_SWORD_EXTRA_LIBRARIES` for static SDK transitive libraries. The fallback
SDK version and architecture must be checked by the SDK provider. Windows
SDK/MSVC linking has not yet been tested.

GLib supplies the existing locale-sensitive dictionary-click Unicode casing;
it does not bring GTK or a platform UI into the core. pkg-config is preferred.
Prebuilt SDKs can supply `GLIB_INCLUDE_DIR`, `GLIB_CONFIG_INCLUDE_DIR` and
`GLIB_LIBRARY`, plus `XIPHOS_GLIB_EXTRA_LIBRARIES` for static dependencies.
SDK providers must verify the binary version/architecture and deployment target.
Host locale initialization must happen before engine work starts; changing the
process locale concurrently with workers is unsupported. The core does not
silently change the application's locale.

Tests generate a small on-disk RawText OSIS module with original prose.
They validate discovery isolation, UTF-8 XHTML, legacy setKey/renderText
comparison, C header consumption, handle/result ownership, missing modules,
empty libraries and invalid arguments. Fixtures remain in the build directory
for the Swift smoke test and manual preview; no live user data is modified.

The default legacy build remains available with its get_render_text delegate
calling the same rendering function; full GTK build/runtime acceptance is
still required. The standalone core build avoids all GTK,
WebKit, help and packaging dependencies. Search and complete user-data storage
remain pending. The C ABI is experimental and documented in c_api.h.
Follow the [architecture](../../doc/modernization/architecture.md) and
[parity ledger](../../doc/modernization/parity-matrix.md) before expanding it.

## Study-session history

`StudySession`/`xc_session` adds one history per tab or window. The C++ session
borrows its Reader; C ABI sessions retain the reader and remain valid after
`xc_close`. Visit and navigate return independently owned entries; release each
once. Serialize calls and teardown across all handles sharing a reader.

Navigation direction is -1/+1. An unavailable destination returns invalid
argument without moving the cursor. Failed visits do not add history. Clear
History leaves displayed content to the frontend. Legacy behavior uses
reference-only identity, retains forward entries on new visits and keeps the
first visit's module metadata on a duplicate reference. Capacity is 50.
Commentary restoration, synthetic entries, history export and persistence are
later slices. The shared C++11 helper is also used by legacy tab_history.c.

## Chapter snapshots and navigation

`Reader::read_chapter` / `xc_read_chapter` returns a copied title, normalized
anchor and ordered rendered verse entries, including empty text. Enumeration
uses the module's VerseKey/versification. Release C snapshots exactly once;
borrowed snapshot strings survive reader close until chapter release.
`StudySession::navigate_chapter` / `xc_session_navigate_chapter` resolves verse 1
of the preceding/following chapter using SWORD, then records a successful visit.
Only -1/+1 are accepted; Bible ends do not wrap. Failed destinations leave
session history unchanged. This is not full decorated Xiphos chapter output;
full decorations, remaining options/actions and synchronized study panes are pending.

## Study-note retrieval

`Reader::study_note`, `StudySession::study_note` and `StudyWorker::study_note`
accept an explicit module, verse and SWORD note identifier. They regenerate
that verse's entry attributes and return owned reference/type/label/body HTML
and cross-reference-list metadata. Body conversion/rendering matches the
existing backend; sessions apply their effective module options without
visiting or moving history. Missing notes reject rather than inserting empty
SDK attributes or reusing attributes from the last chapter verse. The helper
is C++11-compatible in CoreRender; native host lifetime/thread rules still
apply. `xc_worker_study_note` returns a polled request whose result transfers once
through `xc_request_take_note`; note strings survive worker/request close until
`xc_note_release`. Swift `CoreWorker.studyNote` copies the result strings
into a Sendable value. Native link handling, main/information hover and expanded
reference lists are implemented in scoped form; complete link-family/platform
acceptance remains pending.

`study_note_link` decodes local SWORD `showNote` commands, including the fixed
`xiphos-preview://study/` native origin. Query values decode individually;
unknown actions/parameters, duplicates, malformed escapes, control characters,
external origins and passage ranges reject. `StudyWorker::study_note_link` and
`xc_worker_study_note_link` resolve and render in one engine job with tab
settings, validating the link's note/cross-reference type against entry metadata.
Swift exposes the same operation without a frontend URL parser.

`resolve_study_link` also accepts passage URIs, returning a passage target or an
owned note while preserving history. `xc_worker_resolve_study_link` stores the
corresponding result kind; `CoreWorker.resolveStudyLink` exposes a typed Swift
enum. Notes carry their source module so nested links have the correct fallback.

Shared `study_link` decoding also accepts local `showRef&type=scripRef`
commands for one scripture reference. Dispatch validates the module with the
existing reader, and the eventual visit uses that module's own SWORD key for
normalization. Native hosts pass the note's source module as the fallback.
The existing single-reference API retains its contract. The additive
`contextual_study_link` parser and `StudyWorker::resolve_study_link_at` accept
showRef lists/ranges and return a configured `ReferenceList` with explicit
source reference context. Single showRef commands also return lists, matching
the legacy clicked sidebar workflow. Passage URIs still return navigation
targets; notes still return owned bodies. All resolution preserves history.
Hosts must pass committed source context rather than an unfinished input draft.
`xc_worker_resolve_study_link_at` exposes the operation with nonempty source
context and URI, copied submission arguments and one owned result kind.
`CoreWorker.resolveStudyLinkAt` returns a Sendable passage/note/references enum.
Native note-body dispatch follows separately.

## Reference-list parser seam

`parse_reference_list` is a C++11-compatible shared helper in CoreRender. It
uses a clone of the module's own SWORD key to expand lists/ranges, retaining
the fork's ordering, duplicates, invalid-element skipping and 2000-verse guard
per range. Owned results report the guard (`limited`) and skipped elements.
The legacy `BackEnd::parse_verse_list` now delegates to it while preserving its
current-key mutation and GList result. Characterization compiles both a frozen
pre-extraction adapter and the actual current adapter without GTK, comparing
56 cases across KJV/KJVA; full GTK application validation remains a gate.
`Reader::read_reference_list`, `StudySession::read_reference_list` and
`StudyWorker::read_reference_list` now return owned module/context/expression,
entry snapshots, shared HTML and limit/skipped metadata. They use effective
module options without visiting history and restore the module key after
rendering. Missing text positions and duplicate entries remain present.
`xc_worker_read_reference_list` queues a snapshot that transfers once through
`xc_request_take_reference_list`. Its strings/entries remain owned until
`xc_reference_list_release`, including after worker/request close. Empty
expressions are valid; module and context are required. Swift's async
`CoreWorker.readReferenceList` copies the result into a Sendable snapshot.
Native ordered-list presentation is implemented; complete platform/visual acceptance remains pending.

## Variant-selection legacy seam

`xiphos_core_variant_selection` is a C-callable, C++11-compatible policy in
CoreOptions. It extracts the legacy reading_selector's primary/secondary/all
flag mapping. The actual GTK callback delegates to it, retaining option-save,
dialog/main navigation and invalid-choice behavior. A frozen callback and the
actual extracted adapter are characterized without requiring GTK libraries.
`select_variant_reading` applies the characterized mapping to a copied
EngineOptions value, retaining every unrelated flag and rejecting invalid
choices. `StudySession::select_module_variant` and its worker operation require
configured settings and a declared variant filter, then update one module
without visiting history. `xc_worker_select_module_variant` exposes the operation with the shared
XIPHOS_VARIANT_* constants; invalid choice/module/output arguments reject
before submission. Swift submits a typed value and retrieves effective options
from the core. The native menu follows the existing three legacy selections.

## Dictionary snapshots

`Reader::read_dictionary` requires an explicit installed SWORD dictionary
(SWLD) and a nonempty key, then uses the same shared setKey/renderText seam as
legacy BackEnd::get_render_text. Results own module, post-render key and HTML.
SWORD controls case handling, padding, Unicode and nearest/missing-key behavior;
this API does not promise an exact-key search. Session/worker requests apply
effective module settings and never visit Bible history. Preference/filter
scopes and worker ownership match the existing reader. The asynchronous C API
validates nonempty module/key before submission, reports engine/type failures at
poll time and transfers an owned dictionary handle once. Getters borrow strings
until handle release; the handle survives request/worker shutdown. Swift copies
these fields into a Sendable RenderedDictionaryEntry through CoreWorker.
Shared preferred-lexicon selection, Strong's/morphology dispatch and the native
dictionary pane are implemented in scoped form; full dictionary parity remains pending.


## Lexical command parsing

`lexical_link` returns an owned typed Strong's/morphology command with source
module, raw key/type and optional passage. It shares strict local-origin and
query decoding with note/reference parsing. It does not select a lexicon,
normalize a key or change existing contextual dispatch. The main/dialog
selection differences and subsequent gates are recorded in
[lexical-lookup.md](../../doc/modernization/lexical-lookup.md).


## Shared lexicon selection

The C11-callable `xiphos_core_select_lexicon` in CoreOptions owns the legacy
main/dialog Strong's and morphology selection policy. Both legacy handlers
now delegate to it while retaining their display, history and source-testament
behavior. Preferences are borrowed nullable strings; synchronous callbacks
provide installed-module checks and a source testament only when needed.
Returned strings borrow preferences or static fallback storage and must be
copied before preference changes. A missing selection returns NULL; an empty
configured string remains distinct from NULL. Main fallback availability and
overlapping Hebrew/Greek checks intentionally differ from dialog selection.
No global settings, SWORD manager or GTK types enter the helper. Native copied
preferences/context and dictionary dispatch now use this helper.


## Copied lexical lookup sessions

LexiconPreferences owns five optional module names with legacy defaults;
setting them explicitly configures one tab without reading/writing a profile.
Null versus empty values survive copying and borrowed-helper conversion.
Embedded NULs reject before state replacement. Unconfigured tabs reject lookup.

StudySession::read_lexical parses the command, selects with the shared policy
and renders through its existing dictionary path, applying the selected
module's engine settings. It returns owned command/source-reference/dictionary
values without adding Bible history. The supplied reference is the committed
source context; a query passage is preserved as metadata, not substituted for
that context. Explicit URI module still overrides fallback module.
Reader::select_lexicon queries a cloned, validated source VerseKey only when
the policy requires a testament. No independent canon parser or source-key
mutation is introduced. Missing selected modules and wrong module types fail
through the normal renderer. The worker copies settings/arguments and executes
selection plus rendering FIFO. C/Swift bindings, native panes, scoped main
hover and recorded-click history now use the shared services described below.


## Lexical C/Swift adapters

The worker C API explicitly enables core defaults or copies five nullable
lexicon names. Its getter transfers an owned configured/unconfigured snapshot;
name getters borrow strings until release and preserve NULL versus empty.
Lexical previews transfer owned command/source/dictionary values once. Source
module/reference may be empty for source-independent Strong's; Greek context
validation and dictionary errors occur on the worker. URI/origin validation
happens before submission. Snapshots survive request/worker teardown.
Swift copies preferences and typed results into Sendable values, checks ABI
origin/kind constants, and rejects embedded NUL preferences/source/URI before C
submission. Defaults come from the core rather than a second Swift policy.
Native preference panels, dictionary presentation, hover/click history and
explicit five-field XML persistence are implemented in scoped form; full
profile/platform parity remains pending.


The copied catalog now exposes dictionary classification and declared lexicon
roles. Classification follows the legacy dictionary type and excludes
DailyDevotion, with SWLD validation; role matching uses the exact first SWORD
Feature entry (GreekDef/HebrewDef/GreekParse/HebrewParse). It does not union all
Feature entries or promise effective lookup compatibility. Both synchronous
and worker Swift adapters expose the same copied values. Native selection
controls can use these flags without accessing SWORD or duplicating matching.


The contextual worker dispatcher now returns a fourth owned result kind,
LexicalEntry, for Strong's/morphology commands after tab lexicons are explicitly
configured. It uses the same query parsing, selection and preview service;
source-reference and no-Bible-history rules are retained. The C request exposes
this kind through take_lexical_entry alongside existing takes. Older study_link
and resolve_study_link APIs retain their two-kind contract. Swift contextual
dispatch/pane presentation is implemented in scoped form; explicit readLexical
already exposes copied values.


Dictionary key navigation is a shared Reader/session/worker/C request, with
direction -1/+1 and the existing owned dictionary result. It resets the current
key, takes one SDK step, copies the returned key and renders it using tab module
options. SDK boundary/nearest-key behavior remains authoritative. Empty/NUL
module or key and invalid direction reject; no Bible-history visit is made.
Native key controls and separate dictionary history follow independently.


The dictionary step helper belongs to the legacy-compatible C++11 CoreRender
subset and is used by the existing GTK handler as well as native Reader. Its
owned key prevents SDK pointer lifetime from crossing presentation work. Actual
legacy handler wiring is compiled into the fixture characterization harness.


DictionaryHistory is a separate C++11 value type in CoreHistory. It deduplicates
only the current module/key pair, retains forward data on a duplicate, clears
forward data on a different visit and evicts oldest entries above 50. Invalid
moves reject before mutation; visits build replacement state before publishing.
The legacy actual/frozen adapters agree over 10,287 tested transitions. Session
commands, legacy delegation and frontend history controls are not connected yet.


Each study tab now owns separate dictionary history with explicit visit, history
navigation, snapshot and clear requests. Visits render before recording the
requested module/key, and history moves render before cursor commit. This layer
does not infer click case normalization. Existing dictionary previews and key
steps retain their no-record behavior. C transfers a distinct owned history
kind with count/cursor, module/key entries and back/forward flags; bounds return
null/zero and snapshots survive worker teardown. Native integration follows.


Main-origin lexical clicks render before recording the selected dictionary and
the legacy GLib-uppercase requested key. Command metadata retains raw value.
Dialog-origin clicks, hover and preview reads use raw keys without main history.
The worker/C click dispatcher shares the contextual parser/result path with
preview resolution and records only successful lexical commands. Passage
targets, notes and reference lists retain their existing no-Bible-visit behavior;
older preview resolution remains unrecorded. C++ owns normalization; Swift does
not implement casing. Hosts initialize process locale before worker creation.
Malformed Unicode main clicks reject without changing history. A normalized
requested key is retained even when the SDK resolves a different nearby entry.


The nearby dictionary chooser returns nine owned keys in the legacy order,
including repeated labels at SDK boundaries. Its C++11 helper is used by the
actual legacy menu adapter. Reader validates SWLD, positions the requested key,
copies the keys and restores the preceding dictionary key text. Worker requires
an open tab and C returns a separate owned choices kind; no history visit occurs.
Native menu presentation follows separately.

## Legacy XML lexicon profile subset

The native core requires libxml2 headers/library (the existing legacy XML
dependency); CMake discovers them with FindLibXml2. `parse_study_profile` accepts
XML bytes with an XML-supported declared encoding. `read_study_profile` reads
one explicit regular file. Both return owned UTF-8 lexicon preferences and
optional saved locale, with legacy missing/empty/duplicate-node lookup rules.
Unavailable module names are retained. No home discovery, source-file writes,
locale changes or preference publication occurs. DTD, malformed/wrong-root,
invalid NUL and over-16-MiB inputs reject. XML comments/unrelated sections are
not exposed or modified. These APIs are not a native import/save UI.

`core_xml_profile` compares complete frozen/current XML helpers;
`core_profile_reader` compares shared results with the actual getter and checks
owned lifetimes, encoding, file preservation, failure cases and parser isolation.
Local sanitizer coverage excludes the system libxml2 implementation.

`serialize_lexicon_profile` updates five effective values with safely escaped
literal text, preserves unrelated XML nodes/attributes/comments outside those
values and leaves shadowed duplicate keys plus saved locale intact. It verifies
reload values and normalizes XML serialization to UTF-8. Null/empty choices
reject because legacy default loading cannot round trip them.
`save_lexicon_profile` stages and verifies an explicit file before replacement,
retains existing permissions and symlink aliases, and creates a private new file
when requested. Existing malformed/empty files reject rather than being replaced.
It shares replacement/cleanup with engine-preference saving and rejects observed
external changes. This is not interprocess locking or power-loss durability;
Windows replacement behavior remains a platform acceptance gate.

`StudyWorker::import_lexicon_profile` prepares all choices before per-tab
publication and returns copied preferences. Save requires a configured tab and
uses the existing shared profile writer. Both execute FIFO on the worker, leave
Bible/dictionary history and engine options unchanged, and never apply saved
locale. C APIs `xc_worker_import_lexicon_profile` and
`xc_worker_save_lexicon_profile` use existing observer lifetimes; import
transfers one configured snapshot through `xc_request_take_lexicons`, while
save completes without a snapshot. Releasing an accepted observer does not
cancel file work; shutdown drains it. macOS selected-file controls now delegate
through the typed adapter; full profile/restoration/platform parity is pending.

Preview settings are read separately through
`parse_study_preview_preferences`/`read_study_preview_preferences`, returning
owned `StudyPreviewPreferences`. `misc/xrefsinverselist` uses actual getter
traversal with the observed legacy null/default-true and bounded atoi-prefix
semantics; zero/non-numeric prefixes disable, nonzero signed prefixes enable.
Out-of-int-range prefixes reject safely. The same bounded XML/file reader is
reused. This API does not apply settings to tabs, save them or modify the
lexicon-only importer. `core_preview_profile` checks 77 compatibility, failure,
encoding, file-preservation and independent-result cases; full startup/native
restoration remains pending.

## Main note presentation policy

`xiphos_core_note_presentation` in CoreOptions preserves main-reader tag
precedence and returns distinct body-hover, clicked-reference-list, hovered-
reference-list, hovered-reference-link or no-presentation destinations. It is
C++11 compatible with a C11 header and is already used by legacy main/url.cc.
It does not retrieve notes, alter source/module state or implement dialog rules.
The complete callback oracle retains nullable refList distinctions and heading
setup; frontends must not merge these branches or reimplement tag matching.
The owned main-hover worker service and main/information native pointer
transport now use this policy; complete click/hover workflows remain pending.

`hover_study_link_at` returns a `StudyHoverLink`: monostate for explicit no
presentation, StudyNote for body/link-only information, ReferenceList for
sidebar destinations, or raw LexicalEntry. It does not visit either history.
Each tab owns the enabled-by-default cross-reference list mode through copied
setter/getter requests. C callers use existing snapshot transfers and the
`xc_request_no_presentation` copied marker; a successful void request is not a
no-presentation result. Explicit reference/passages are no-ops in this slice;
status/dictionary URI/dialog, heading and complete source/fallback compatibility remain pending.

`Reader::reference_targets` / `StudySession::reference_targets` return owned
`ParsedReferenceList` names/context/guard metadata without verse HTML or module
key/filter changes. They reuse the existing characterized parser; embedded NUL
arguments reject. Information-link hover uses these targets directly, while
sidebar hover retains rendered ReferenceList snapshots. The target-only API is
C++ internal-facing at this milestone; no new frontend schema/parser is added.

`read_study_note_metadata` in CoreRender is C++11 compatible and performs the
existing exact-verse attribute generation without a second note-body render;
its owned StudyNote has empty html. Reader/session wrappers apply the same
effective rendering options. Cross-reference hover uses this metadata path;
regular footnote/body callers retain the full renderer. Metadata fields are
copied before SDK attributes can change, and NUL module/reference/id arguments
reject. No frontend schema or XML parser is duplicated.

Main-hover note source parsing now uses `contextual_hover_link`, sharing the
existing decoder while substituting committed context for decoded note passages
shorter than five bytes. The worker uses the supplied source module for an
unavailable explicit note module. Existing strict raw/click parsers are kept;
installed explicit modules/full passages are not replaced. Malformed/control/
duplicate parameters still reject. Full foreign-view/global-main, alias and
heading compatibility remains a separate acceptance gate.

## Legacy tab-history integration evidence

`core_tab_history_adapter` compares complete frozen CrossWire c9976a8 add/fake
history functions with current functions extracted automatically from
`main/tab_history.c`. It uses actual history layout/limits and explicit GUI/type
shims, comparing all history slots, count/cursor/first-back state and ordered
menu-update callbacks over 49,560 bounded cases. Null/empty/Unicode module names,
duplicate positions, all counts through capacity, maximum fitting keys and
sequential eviction are covered. Assertions remain runtime checks in release.
A temporary negative control deliberately changed first-back behavior and was
detected, leaving production sources intact. Strict C++11 and address/undefined
checks pass. This commits previously temporary adapter evidence for repeatable
CI; full navigation/GTK display/runtime and oversized-invalid legacy input are
separate gates. Keep the frozen license/attribution and regression harness after
acceptance.

### Synchronized study passage (initial seam)

`read_passage`, `visit_passage`, `navigate_passage`, and
`navigate_passage_chapter` return an owned `StudyPassage` containing the Bible
module, normalized Bible entry and optional commentary module/entry. An empty
commentary name requests no commentary. Bible/Commentaries types are validated;
both renders must succeed before visit/navigation changes history. Each render
uses its module-specific engine preferences. Reference-only history deduplication
retains the first recorded module pair, matching the characterized legacy rule.
Existing single-entry APIs retain their behavior; hosts should use the paired
commands consistently when implementing synchronized panes. No locked-module,
companion-module, commentary chapter/headings or detached-window parity is claimed.

The async C ABI exposes these commands through `xc_worker_*_passage*` and
`xc_request_take_passage`. The owned passage handle has borrowed string getters
and an explicit commentary-present flag; strings survive worker/request release
until `xc_passage_release`. A wrong-kind or second take returns NULL. The Swift
actor copies this into `RenderedStudyPassage`, without calling SWORD directly.

Paired history restoration also supports unpaired standalone commentary entries
recorded through the older single-entry API. `xc_history_commentary` reports the
current first-recorded commentary name (empty for an unpaired location, NULL for
empty history). This does not change the strict types required by paired visits.

`commentary_positions` is a C++11 traversal helper characterized against frozen
and current main commentary iterator statements. It restores key/skip state
and returns owned references while skipping consecutive linked entries. It does
not yet provide commentary chapter rendering, headings or personal-module edits.

`read_commentary_chapter` on Reader/StudySession/StudyWorker now returns an owned
`Chapter` using those sparse references. Unlike Bible chapters, its `verses`
array is not indexed by canonical verse number. `selected_verse` is still the
engine verse ordinal; hosts use the returned HTML and `commentary-verse-N` anchor.
Module options apply to every rendered section and reads leave history unchanged.
Selected/missing/trailing anchor placement has frozen/current legacy statement
characterization; complete headings/cache/theme/personal-module parity is pending.

The async C/Swift adapters now expose commentary chapter reads through the
normal owned chapter snapshot type. `follow_pane_link_at` separates navigation
Bible context from the source pane's note/lexical context. `visit_linked_passage`
selects the Bible/commentary pair centrally according to the explicit target's
module type. Other module kinds, abbreviations/ranges/fragments and complete
legacy URI-handler characterization remain pending.

### Module presentation preferences

Presentation settings remain separate from SWORD engine flags. The first
`ModulePresentationOptions` field is commentary chapter mode, with the exact
legacy regular/personal module distinction and missing/default/Off decoding.
Sessions/workers own getters and regular-commentary updates. Combined module
import stages both maps from one `read_module_option_values` decode; combined
save reuses verified same-directory replacement. Only regular-commentary
presentation sections are written. The old engine-only save helper delegates
with an empty presentation map and remains compatible. Other display settings
and the personal-editor workflow are pending.
