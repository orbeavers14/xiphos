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
