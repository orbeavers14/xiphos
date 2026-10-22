# Shared legacy named search-list import

Updated 2026-10-22. Initial C++ implementation `4ca44370` reads explicit selected
legacy XML profiles. This is a read-only setup service, not automatic profile
migration or complete advanced-search acceptance.

## Data and lookup contract

`SavedSearchLists` owns ordered `module_lists` and `ranges`. Each
`SavedSearchList` preserves optional `label` and `value` strings: absent XML
attributes remain absent, explicit empty attributes remain empty, and unavailable
module names, whitespace and duplicate labels remain unchanged.

`parse_saved_search_lists` reads `modlists/modlist` and `ranges/range` from the
first matching section, including an empty first section. Later duplicate
sections are shadowed. Only the expected item names are imported; unknown tags
and unrelated profile data are neither exposed nor changed. The legacy dialog's
unfiltered traversal of arbitrary child nodes is not treated as a new data
schema. Input is bounded by the existing 16 MiB profile reader and 1,024 named
items per section; larger collections reject rather than silently truncate.

`saved_search_list_value` matches the actual retained
`xml_get_list_from_label` first-label policy. An absent value on the first
matching label shadows later duplicates. Missing labels can be explicitly
selected with `std::nullopt`; empty labels remain distinct. This lookup does not
normalize labels, split module names, resolve aliases or verify installation.

`saved_search_module_names` implements the legacy comma-token rule without
`strtok` or shared mutable parser state. Empty fields are skipped; spaces,
module order and duplicates are preserved. Empty/all-comma input returns an
empty list, never all installed modules. The existing batch service separately
rejects empty search input. More than 32 modules, names over 4,096 bytes, NUL or
invalid UTF-8 reject. Module validation reuses the shared search-input policy.
No module abbreviation resolution or invented range provider is introduced.

## Ownership and execution

`read_saved_search_lists` reads only the caller-selected regular file through
the existing bounded profile reader. XML-declared encodings become copied UTF-8;
malformed XML, wrong roots, DTD declarations and hidden NUL suffixes reject under
the same parser policy as existing profile services. Source bytes, unknown fields,
file permissions, process locale and global XML parser settings are not changed.

`StudyWorker::read_search_lists(tab, path)` copies its path before submission,
checks the tab on the existing FIFO owner, then returns copied data. It changes
no search generation/scope, annotations, reading settings, history or profile.
A read accepted before close/shutdown drains; a read queued after closure fails
under the existing invalid-argument tab contract. Result strings survive parser,
source file, tab and worker destruction. There is no new SWORD instance or worker.

C/Swift transport at `80e8a3c4` adds owned `xc_search_lists` and
`xc_search_module_names` results through existing queued requests. Read and
module-tokenization inputs copy before return; tokenization accepts empty text
but rejects NULL/over-16-MiB inputs before queueing. The C++ helper enforces the
same text cap. Existing C layouts and entry points remain unchanged.

Counts and borrowed attribute getters distinguish NULL from explicit empty.
Invalid kind/row/null handles return zero/NULL. The effective-index getter clears
its output on invalid inputs and calls shared first-label lookup. A later row with
a duplicate label can therefore display its raw data while selecting the same
first value as legacy behavior. Only small indices are copied for this purpose;
large attribute strings are not duplicated once for every shadowed label.

Results take once after explicit successful polling and outlive their request,
worker and source file. Swift 6 copies Sendable/Equatable raw arrays, optional
strings and effective indices before releasing C handles. Its async actor calls
shared module tokenization, rejects embedded NUL, and contains no XML parser,
first-label engine or comma splitter. GTK/Windows consumption remains pending; frontends must consume these policies.

## Native Mac consumption

At `8f6bacc1`, Search exposes an explicit native profile picker and named choices.
Loading replaces only the available lists after successful parsing. Applying a
module choice uses the core effective index and queued tokenizer; it preserves
query/options/bounds and old results until a new search is submitted. Empty
module lists leave Search unavailable; missing values or invalid/oversized lists
retain the prior selection. Literal spaces and unavailable names stay visible and
are handled by the existing batch service when Search runs.

Applying a range copies its effective raw value into explicit bounds while
retaining the current provider. Empty ranges remain explicit empty bounds and
are validated by the existing core query path; they never switch to all text.
No implicit startup application, profile save, range-provider guessing or rerun
is introduced. Busy/revision/closed-window guards include both loading and module
preparation. Failed imports retain prior choices, drafts, source results and scope.

## Explicit shared editing and persistence

C++ checkpoint `2e599e78` adds `SavedSearchListEdit`,
`serialize_saved_search_list`, `save_saved_search_list` and the queued
`StudyWorker::save_search_list` operation. A typed module-list/range edit supplies
literal label/value attributes. Empty strings are valid; invalid kinds, NUL,
invalid UTF-8, XML-unrepresentable characters, oversized documents and collection
capacity overflow reject. Raw values are not normalized or resolved against
installed modules, matching the legacy editing boundary.

Only the first matching named item in the first section is updated. Missing
sections/items are created. Other attributes/children/comments/settings, duplicate
labels and later duplicate sections survive; there is no whole-profile
rewrite API; explicit removal is documented below. The retained C `xml_set_list_item` is compiled as C11 in a test
adapter, and valid edits compare byte-for-byte with its serialized output.

Selected-file saves reuse existing same-directory staging, readback verification,
permission retention and replacement. New files start with an Xiphos root and
private permissions; source encoding normalizes to UTF-8. Invalid/read-only
files and missing parents remain failures rather than silent fallback paths.
Existing symlink aliases retain their targets under the common file policy.
Observed external-content changes reject before replacement; the shared helper
is not an interprocess lock and does not eliminate the final external-writer race.

The returned owned snapshot is allocated and parsed from the verified staged file
before replacement. On success it describes the committed document without a
second frontend read; no search/history/preferences state is applied. Worker
inputs copy before queueing and closed tabs reject before mutation. Dropping an
observer does not cancel an accepted save; shutdown drains it. C/Swift save transport is available at `c11b15b`; native explicit editing now
consumes this service (see below). No user-profile discovery or
automatic save is introduced by this API.

## Evidence and remaining work

The [Mac handoff](handoff-macos.md) records actual build/test commands and scope.
Tests compare lookup directly with the extracted retained legacy routine,
including missing/empty attributes, duplicate sections/labels, encoded text and
unavailable modules. They check exact/overflow list and batch limits, malformed
names/XML/paths, explicit-file byte/permission preservation, independent parsers,
FIFO read ownership, closed-tab failure and shutdown draining.

Automatic startup application, module installation, saved query serialization,
alias resolution, full mixed-family range setup and complete frontend workflow
acceptance remain open. Native explicit single-item profile writing is exposed; complete list management remains open. These remain separate work, along with
corpus/locale, actual-index and current Fedora/MSVC validation. Existing GTK3
source remains unchanged and no parity/retirement gate passes.

## Owned C/Swift save transport (2026-10-22)

Implementation `c11b15b` adds `xc_saved_search_list_edit` and
`xc_worker_save_search_list`. Existing C layouts/version and read handles remain
unchanged. Required path/label/value strings copy before return; empty label/value
are valid, kind is strict, and each input has a 16-MiB pre-copy ceiling. Invalid
shapes clear the observer before submission; UTF-8/XML/path/tab failures arrive
at poll without exposing a result or silently replacing invalid files.

The C observer and result handle are allocated before accepting a write. The
verified core snapshot moves into that preallocated handle without allocation
when polled (enforced by a nothrow move-assignment assertion). Take-once getters
use the existing optional/effective-index contracts. Dropped observers do not
cancel accepted writes, and shutdown drains them. Values survive request/worker/
source-file teardown. This preserves the prepared-before-replacement core policy
through the C result boundary; it does not introduce an external-writer lock.

Swift `SavedSearchListKind`/`SavedSearchListEdit` and `CoreWorker.saveSearchList`
copy inputs/results and reject NUL/oversized fields before C conversion. They
reuse shared XML, duplicate-label and persistence policy. Cancellation of an
observer does not claim to undo an accepted save; no settings/history are applied.

Local focused checks: normal Mac6/6 (including Swift profile/dictionary fixture),
ASan/UBSan6/6, TSan6/6 and portable core-on-Mac6/6, GTK runtime OFF. Sanitizer/
portable selections cover C/CPP saves, existing read transports/profile parsing
and generic profile save/XML compatibility. Strict C11 header syntax passes.
Dependencies remain partly instrumented; ASan leaks off. Logs use ignored
`build/search-list-save-adapters-*`. This is scoped transport evidence, not new
Fedora/MSVC/native editor/full-parity acceptance. Generated original-prose
profiles only; no user profile, general clipboard or hosted workflow changed.


## Native explicit editor (2026-10-22)

The Saved search lists menu opens a native sheet for kind, literal label/value
and an explicit profile destination chooser. Empty fields remain valid shared
inputs. An existing label updates its first item; a new label creates one. Only
successful verified returned snapshots replace the available module/range choices.
No second file read, XML writer, comma parser or duplicate-label policy exists in
the UI. Saving never applies a choice or changes draft/results/preview/last scope.
Busy setup excludes competing application actions. Chooser cancellation submits
nothing; failed saves retain loaded choices and original invalid files. Closed
windows reject new actions and suppress publication from pending accepted writes;
accepted core writes may still drain, as specified by the transport contract.

Scoped native editor/chooser cancellation/new-file save/menu checks use generated
original-prose fixtures. Model checks include module/range saves, duplicate shadows,
failure retention, unchanged draft/results/scope/preview/other windows and pending
close. Full Mac regression and final focused evidence are in the Mac handoff.
Existing-item prefill/list management, deletion, complete saved setup, physical
input/accessibility/corpus and other-platform acceptance remain open.


## Shared explicit removal (2026-10-22)

`serialize_saved_search_list_removal`, `remove_saved_search_list` and queued
`StudyWorker::remove_search_list` remove only the first matching named item in
the first section, matching the actual retained C `xml_remove_node`. Later
same-label items may become effective. Empty labels are literal; absent label
attributes are not selected by this typed string operation. Unmatched labels
succeed without creating a section. Missing profiles reject without creating a
file. This is explicit selected-file mutation, with no profile discovery or
search/history/settings application.

Original and resulting collections remain bounded; malformed/DTD/wrong-root XML,
invalid kinds, over-limit/NUL/invalid UTF-8/XML labels reject. Serialization retains
unrelated nodes/attributes/comments and later sections, normalizing UTF-8 as for
saves. The common staged/readback/permission/symlink/replacement helper prepares
the owned result before replacement. Existing external-writer/durability limits
remain unchanged. One worker copies arguments, checks the tab before mutation
and drains accepted removals on shutdown or observer loss.

Direct retained-C byte comparisons, duplicates/empty/nonexistent labels, unknown
XML, permissions/symlinks, invalid labels/files, original collection bounds,
copied worker input, closed-tab preservation and shutdown draining are checked.
Normal Mac, ASan/UBSan and portable-on-Mac focused selections each pass4/4;
GTK runtime OFF, ASan leaks OFF, dependencies partly instrumented. Additive
C/Swift removal transport and explicit native list management remain next. No
whole parity/platform/retirement acceptance follows.


## Owned removal C/Swift transport (2026-10-22)

`xc_worker_remove_search_list` is additive and returns the existing owned
`xc_search_lists` snapshot. Kind is strict; required path and label copy before
return with the same16-MiB pre-copy ceiling as saves. Empty labels are literal;
NUL/invalid UTF-8/XML reject through the typed boundary. Shape failures clear
output before submission; semantic/file/tab failures arrive at poll. Observer
and result storage allocate before accepting mutation, and the existing nothrow
snapshot move transfers the verified prepared value on successful poll.
Take-once, lifetime and shutdown-draining rules match saves. No result layout or
ABI version change, second owner, profile discovery or search application.

Swift6 `CoreWorker.removeSearchList` copies the snapshot and rejects embedded
NUL/oversized fields before conversion. Cancelled observers drain accepted
removals. Tests check first-duplicate exposure, state isolation, literal inputs,
pre-poll/take-once/failure ownership, copied submission, invalid shapes/text,
missing profiles, closed tabs, dropped observers and shutdown. Normal Mac8/8
including Swift and original dictionary fixture; C11 header syntax passes.
Sanitizer/portable evidence and implementation checkpoint are in the Mac handoff.
Native explicit removal and existing-choice prefill remain subsequent work.


## Native loaded-choice management (2026-10-22)

Manage named lists provides a loaded-choice prefill menu for the chosen kind.
It consumes copied core effective indices, including duplicate shadows, and
retains absent values as unavailable rather than inventing an empty value.
Explicit Remove opens an existing-profile chooser; cancellation submits nothing.
The shared removal result replaces available choices only after success, and
later duplicate labels can become effective. Draft selections, query/options,
results/preview and last scope remain unchanged. Failures retain prior choices.
Closed windows reject new actions and suppress pending publications; accepted
removals can still drain. No second label/serialization/persistence engine.

Native model/file checks cover successful removal, duplicate exposure, malformed
file retention, busy/prefill guards, search isolation and pending closure. Scoped
window checks cover prefill and chooser cancellation; actual GUI removal completion
and full management/workflow/accessibility/corpus/Fedora/MSVC acceptance remain open.


## Next shared seam: module abbreviation resolution (source audit 2026-10-22)

This audit is planning evidence, not an implementation or acceptance claim.
Saved raw module tokens remain literal today. Retained
`src/main/search_dialog.cc` resolves each token with `main_abbrev_to_name` after
advanced range setup and before installed-module validation/execution. The
existing shared setup characterization already checks raw names at range setup;
resolving aliases during XML import/tokenization would change this ordering.

Two policies must be extracted together. `BackEnd::init_lists` in
`src/backend/sword_main.cc` clears registration and iterates installed SDK modules,
reading `Abbreviation`; it registers only aliases that do not equal any installed
module ID. `src/backend/module_manager.cc` also applies that installed-name
collision exclusion. `src/main/sword.cc` then rejects aliases containing `(` and
maintains exact case-sensitive name-to-alias and alias-to-name maps. Empty aliases
are not excluded by that helper. Repeated alias registrations overwrite the
reverse mapping; repeated names can leave previous reverse entries until clear.
Do not silently replace this with a normalized bijection or first-match lookup.
Actual SDK registration order and reload boundaries need characterization.

A bounded next slice should first compile/compare these retained registration
and lookup routines with original-prose collision/Unicode/empty/re-registration
fixtures. Then place policy and SDK metadata copying under the existing core
owner, stage complete catalog aliases before publication, preserve exact-ID
protection and search setup ordering, and expose copied resolved identities while
retaining requested-token provenance in batch/export metadata. Audit URI,
parallel-reading and BibleSync callers before reusing a broad resolver there;
search-only resolution must not silently change those independent contracts.
No frontend alias maps, new SDK manager, automatic saved-list application or
unvalidated corpus/locale behavior should be introduced.
