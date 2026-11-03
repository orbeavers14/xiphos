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

FIFO/C transport returns owned snapshots and copies inputs before admission on
the existing worker. The macOS Module Library presents explicit Favorite, Hide in
module list and Group controls, with revision-checked Save and Reload on conflict.
Hidden modules remain available in installed-module management and search/pickers.
Reading sidebars show favorites first within their sections; a custom group
replaces the default type section. Preferences update all reading windows without
changing selections and reload from the explicit profile metadata path at restart.
Swift identities compare/hash UTF-8 bytes, matching the shared repository rather
than merging canonically equivalent Unicode identifiers.

Generated checks cover provider separation, preference reset, unknown fields,
revision conflict/create-only protection, exact rereads, copied C ownership,
native cross-window filtering, stale-save rejection and fresh-controller reload.
A disposable native app/profile also verified hide/unhide, favorites/custom groups
and real quit/restart while retaining the displayed passage. Broader migration,
real libraries and physical platform acceptance remain open.
