# Shared document editing foundation

The next owner priority after session restoration is editing. The retained
GtkTextView file path serializes its buffer and calls `g_file_replace_contents`
(`src/editor/gtktextview_editor.c`, `_save_file`). Module note/book writes take
separate SWORD paths. Graphify identified the file serializer and shared
same-directory replacement transaction; these boundaries were verified in source.

## Exact source and revision-checked saves

`EditableDocument` owns serialized UTF-8 source and its exact-byte SHA256 revision.
Explicit-path open/save is available through shared C++, the FIFO worker and C11
handles. Empty files, BOM, CRLF/LF, markup and literal percent sequences retain
their bytes. There is no implicit formatting, HTML rendering, percent decoding,
profile discovery, recent-file registration or module write. Non-UTF-8/NUL-bearing
sources and files over4MiB reject. Sources/destinations must be regular files;
existing symlinks retain their alias and update the selected target.

Creating requires an absent expected revision and a still-missing file. Replacing
requires the revision from the last open/save; missing/stale/external-change
mismatches reject before replacement. The owned output is prepared first, staged
bytes are verified, and the existing same-directory transaction commits atomically
while retaining permissions. These are observed content-change checks, not an
interprocess lock, collaborative merge or crash-durable fsync protocol.

Worker commands validate the calling tab before IO and leave selections, histories,
search and preferences alone. C arguments copy before return; observer/result
storage precedes acceptance. Successful polling only moves owned data; results are
take-once and survive source/request/worker teardown. Accepted dropped writes drain
at shutdown. Public C bounds validate source/revision structure before observer/copy
allocation. Paths are explicit and bounded to4096 bytes.

The disposable fixture checks exact source, independently known empty SHA256,
external/stale changes, create-versus-replace, invalid text/size/directory inputs,
the exact byte limit, permissions/symlinks where supported, closed tabs, C admission
allocation failures, copied inputs, no-allocation committed polling and ownership.

## Remaining editing work

Next: characterize the retained rich-text serialization/formatting/link contracts,
then provide native editor presentation over the shared document service. Shared
RawFiles personal-commentary and RawGenBook writes still need explicit revision,
transaction/error and SDK cache-refresh contracts; the legacy direct `setEntry`
helpers are not an atomic module-editing service. Dirty-buffer close/quit handling,
unsaved-document recovery, journal/prayer-list organization and native editor
restart remain open. This groundwork does not accept F25/F26 editor parity.
