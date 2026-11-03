# Library organization metadata

The shared C++ organization repository stores favorites, hidden state and one
optional group for exact provider/item identities. Equal display/module IDs from
different providers stay separate. It does not change installed modules, catalog
order, licenses, permissions or provider grants.

Callers select an explicit XML metadata path. Existing files require an exact
source revision; null revision creates only a missing file. Duplicate identities,
invalid flags/UTF-8, unsupported versions and capacity failures reject. Up to
10,000 entries and a 4MiB document use existing bounded editable-document storage
and verified replacement. Observed changes reject; this is not an interprocess lock. Unknown XML and comments survive edits. Setting flags
false and an empty group resets preferences without deleting unknown metadata.
No profile discovery or user-library scanning occurs.

Generated checks cover provider separation, preference reset, unknown fields,
revision conflict/create-only protection and exact rereads. FIFO/C transport,
native filtering/group controls and broader migration/platform acceptance remain
next; the storage foundation alone is not a completed library-management workflow.
