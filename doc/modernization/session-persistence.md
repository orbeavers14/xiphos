# Session persistence contracts and remaining work

The owner prioritized complete core session persistence while deferring further
devotional work. The retained `gui_save_tabs`/`gui_load_tabs` format is
`Xiphos_Tabs` XML, separate from the main `Xiphos` preferences file. This is a
compatibility source, not a complete representation of the new core's state.

## Retained session fields

Each `tabs/tab` contains raw text/commentary/dictionary/book module names,
`text_commentary_key`, `dictlex_key`, `book_offset`, `comm_showing`, four pane
visibility attributes and `showparallel`. All `tabs` sections contribute in
source order. `Version` is retained as metadata. Missing and explicit empty
attributes are distinct. Reading raw offsets does not parse them with `atol`
or assume a valid book-tree node. Module identifiers are not resolved at import.

Only the exact lowercase `yes` token is true. The four pane visibility flags
inherit the global defaults together only when `showtexts` is absent. An empty
`showtexts` is present and false, and missing sibling flags become false rather
than individually inheriting defaults. Commentary visibility and parallel flags
always use the exact yes-token policy.

Legacy loading creates native widgets, resets history to an initial visit,
selects the first tab and may rewrite old files when pane flags are missing.
Those effects are separate from the new explicit read-only repository. The new
reader never creates fallback tabs, renders modules, chooses a selected tab,
rewrites a source file or searches the home directory. It accepts up to256 tabs,
32768 bytes per retained attribute and16MiB total retained text, with the shared
secure XML parser rejecting malformed/DTD-bearing/NUL-hidden documents.

## Complete new-core restoration still required

A complete native snapshot needs per-tab main and dictionary selections/history
with cursors; book raw/display selection; parallel raw intent, resolved control,
reference and explicit options; tab-wide and per-module engine/presentation
preferences; lexical/preview preferences; and tab order/active tab intent.
Search input policy and a deliberate policy for retained results must be defined.
Window geometry, detach ownership and native UI selection belong at the host seam.
SDK pointers, rendered DOM objects and active jobs are never persistence objects.

Restore must validate a complete replacement before publishing it, retain the old
session on failure, define unavailable-module behavior and avoid silently trimming
bounded lists. Saved metadata and actual restored module provenance must remain
separate. Storage-only inspection must remain possible without installed modules.
Export/replacement must preserve unrelated data, verify a staged owned snapshot
before atomic replacement and use explicit selected paths. Observed external-change
checks do not replace an interprocess locking/sync protocol.

This groundwork does not accept the full tabs/detach/session parity group. Broader
Windows/Fedora and real native restart workflows remain required alongside portable
core validation.
