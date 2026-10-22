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
labels and later duplicate sections survive; there is no delete or whole-profile
rewrite API. The retained C `xml_set_list_item` is compiled as C11 in a test
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
observer does not cancel an accepted save; shutdown drains it. C/Swift transport
and explicit native editing remain the next slice. No user-profile discovery or
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
acceptance remain open. Native profile writing is not yet exposed. These remain separate work, along with
corpus/locale, actual-index and current Fedora/MSVC validation. Existing GTK3
source remains unchanged and no parity/retirement gate passes.
