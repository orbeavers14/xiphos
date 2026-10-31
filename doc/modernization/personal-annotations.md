# Verse annotation storage and retained cache contracts

These are the main-profile `Xiphos/osisrefmarkedverses` plain-text verse notes,
separate from RawFiles personal-commentary modules and their rich editor. The
owner prioritized bookmarks/annotations, editing and persistence while deferring
further devotional work. Native presentation and whole annotation parity remain
unaccepted.

## Explicit owned metadata and cache

b589779 adds an explicit bounded reader and safe retained-cache model, compared
against126 complete frozen/current cache-fill/check/XML-cursor cases. Only the
first annotation section participates; later sections remain source data and are
counted as shadows. All first-section element records retain raw element names,
optional label/note attributes, safe decoded addresses and cursor-prefix visibility.
Unknown labeled elements can participate in the retained cache, which does not
filter by element name. No home lookup, startup conversion, saved locale application
or installed module is needed to inspect the profile.

Labels split at the rightmost two dots and first space, preserving the retained
atoi numeric-prefix behavior. A leading space means a generic note. Module/book
matching is ASCII-insensitive. The packed cache key is1000*chapter+verse; last
matching records win, including generic/specific duplicates and packed-key
collisions in imported data. Cache entries retain the winning source record.
Integer prefixes, intermediate multiplication and final addition are checked;
unsafe legacy overflow produces malformed-prefix metadata rather than executing
undefined arithmetic. Existing malformed raw labels remain inspectable.

The retained cursor starts at the first child and stops when any child lacks a
label, including whitespace/comments. A malformed label aborts the remaining
cache fill after retaining its valid prefix. The reader preserves all later raw
records and reports cursor barriers; cache results disclose omitted data and the
malformed source row. A trailing barrier with no omitted records does not falsely
claim incompleteness. This characterizes legacy behavior; it does not silently
repair/reformat an existing profile or accept native rendering of those cases.

Missing note attributes become empty cache strings, retaining note presence.
Escaping matches the retained six replacements for ampersand, angle brackets,
newline, double quote and apostrophe. The result is copied plain-note HTML, not
rich editor content. Limits are10000 records,4096 bytes per label/context/name,
1MiB per raw note and16MiB aggregate owned profile/cache text. Cache bounds count
retained replacement data rather than discarded duplicate copies; oversized HTML
expansion rejects without silently dropping notes. Source XML has the shared16MiB
secure parser limit and malformed/DTD/hidden-NUL rejection.

## Verified compatible writes and transport

70ffb33 adds save/remove with27 complete retained XML helper comparisons. Save
updates the first exact `markedverse` label or appends; remove deletes only the
first match and can repair malformed/empty raw labels. Later duplicates and
sections, unknown attributes/children/comments and unrelated preferences remain.
Thus a first-item edit can still be shadowed by a later duplicate used for display;
the raw snapshot and cache provenance expose that discrepancy. New saves require
positive canonical decimal coordinates with verse<1000 and safe packed arithmetic.
This is format validation, not SWORD/canon validation. Existing noncanonical data
remains readable/removable. Missing files are created only by explicit save.

Writes use verified same-directory replacement and prepare the owned result before
commit. Existing permissions survive. Serialization may normalize encoding/format,
while retaining existing child text nodes/cursor barriers. Observed external-change
checks are not an interprocess lock or sync merge protocol.

f6d9bf4 exposes raw profiles, retained cache queries, numeric-safe core lookup and
explicit edits through the FIFO worker/C ABI. Paths and bounded arguments copy
before submission returns; tab validity precedes IO. C structural rejection precedes
observer/copy allocation. Persistent result storage is allocated before acceptance,
and successful polling uses no-throw moves. Tests inject observer/envelope failure,
verify no write, and cover take-once/source-independent ownership and shutdown drain.
Reads/edits leave current reading selection, histories/search and sibling tabs alone.
Cache module/book arguments are explicit offline inspection scopes; no SDK resolution
or new application state is implied.

Next: core-owned SWORD reference/OSIS target normalization and verse-note save/remove
commands, so native consumers need no label/canon parser. Then explicit presentation,
cache refresh, import/repair decisions and full platform/corpus acceptance, alongside
the owner's other core priorities.

## Core-owned verse targets and note commands

ad97fba adds an owned SWORD target and FIFO/C save/remove/cache commands accepting
an exact installed control module plus reading reference. A cloned VerseKey keeps
the live reading position/flags and control canon/locale. Forty safe exact-module
cases compare with the complete frozen/current OSIS helper across Bible,
KJVA Bible, regular commentary and personal verse-key modules. Generic KJVA Tobit
is checked separately. Missing modules/abbreviations and non-verse modules reject;
the native command does not use the retained helper's KJV/first-module guessing or
unsafe non-verse cast. Reference interpretation otherwise follows SWORD's reported
resolution; this is not a new parser or accurate cross-canon equivalence service.

Targets return actual control identity, normalized display reference, stored
label/module/book and checked coordinates. Generic labels have an empty stored
module but always use the explicit control canon. A specific module ID containing
spaces cannot round trip through the legacy first-space encoding and rejects.
Labels are never silently truncated to the old dialog's100-byte buffer. The raw
inspection/removal API remains available for repairing imported identities.

Verse-note save/remove build the label in core before the existing verified
transaction. Reference-based cache reads derive its book in core, then return the
complete retained book cache; frontends need no OSIS/label/packed-key parser.
Missing-file/remove policy, duplicate/cursor diagnostics and source preservation
remain unchanged. Commands do not select/visit a passage or apply highlights.
Tests cover FIFO ordering, copied inputs, exact generic/specific metadata,
options/history/sibling preservation, pre-acceptance observer failure, no-allocation
successful polling, take-once survival and draining accepted writes/cache reads.
