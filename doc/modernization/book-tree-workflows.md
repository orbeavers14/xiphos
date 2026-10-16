# General-book tree source seam

The first shared F10 slices are read-only C++17 sources and configured entry
rendering, not a complete book reader. `Reader::book_tree(module, offset)` and `book_tree_path(module, path)`
return copied node data and optional parent, first child, previous sibling and
next sibling. They use the Reader's existing manager and private query keys.
No additional engine, frontend tree algorithm, history or persistence is added.

## Reference behavior and deliberate source contracts

- `src/main/module_dialogs.cc` builds lazy tree choices using SDK child/sibling
  order. `src/backend/sword_main.cc` copies TreeKeyIdx values, selects offsets,
  and exposes local names and relationships. The source API preserves that
  order, the SDK's canonical paths, and its root offset 0. Root's display caption
  is the module ID; its SDK path is empty. A top-level node can have root as its
  parent. Structural presence is separate from the presence of entry text.
- `src/main/navbar_book.cc` uses these same relationships for parent/sibling
  navigation. `src/main/sword.cc::main_display_book` clamps numeric reading
  offsets below 4 to 4. Consequently a tree-root query is distinct from reading
  startup. This slice does not change that existing GTK3 behavior or choose a new
  native reading-input policy. The legacy free-text numeric-prefix ambiguity also
  remains a later compatibility decision: typed offsets and canonical slash
  paths keep source discovery unambiguous.
- Only indexed-tree Generic Books are accepted. No unsafe cast of dictionary,
  verse, or unsupported general-book driver keys is permitted. Offset arguments
  are owned unsigned 64-bit values validated against the signed 32-bit, four-byte
  SDK index representation before conversion, including on Windows LLP64.
- Invalid SDK offsets can leave a stale local title/path. Pending path snaps can
  land on a nearby real node when a requested path is missing. Check SDK errors
  before copying and require exact canonical path identity. Failures never publish
  those misleading nodes. Empty path selects root; `/` is not its canonical path.
  Paths reject embedded NUL, relative input and lengths above 4096 bytes.
- Results own all strings and survive later queries and Reader destruction.
  Successful and rejected queries do not select a module key, render text, change
  entry attributes, create a missing path, or call any SDK edit/save operation.
  One-hop snapshots bound their own work; ancestor validation rejects cycles and
  chains of 256 or more ancestors before SDK path assembly. This is a query limit,
  not comprehensive validation of arbitrary corrupt SDK files.

## Local evidence and remaining work

`Reader::read_book(module, offset, options)` pairs exact raw SDK HTML and entry
presence with the validated source tree. It applies/restores the existing shared
engine-option scope and selects the requested live tree offset without replacing
its key object. It does not re-resolve the title/path, so linked entries retain
their selected alias identity. Root stays root; structural-only branches remain
empty rather than showing nearby prose. Source validation happens before live
selection. The common render boundary restores attribute-processing state on
exceptions; failed renders publish no result. Like ordinary entry reads, a
successful selection remains current. It adds no visit/history or preference
write. Unexpected key/source mutation during rendering rejects the result.

`core_book_entry` generates original OSIS/ThML prose and compares exact configured
HTML against frozen live SDK rendering across note/red-letter toggles, root,
branches, leaves and links. It checks real note visibility/red-letter changes,
source/HTML lifetime, missing/wrong-module/offset rejection, injected render and
source-mutation failures, filter/processing recovery and module-data byte identity.
This is raw SDK output: native book-note URI dispatch, display cleanup and image
resource resolution are not enabled or claimed by this API.

`core_book_tree` generates original-prose RawGenBook fixtures, including empty,
structural-only and deep branches, numeric and Unicode titles. It compares all
one-hop relationships with the frozen SDK sequence used by the legacy backend,
checks typed errors/recovery, live key identity/position, attributes, result
lifetime, and byte-for-byte identity of every module data file and filename
before/after successful and rejected queries. Manager-generated configuration
caches are outside this module-data identity assertion.

The next slices are queued worker/C ownership, native tree/navigation controls,
then image/resource routing,
links, history/persistent selection, additional book windows and editors. Preserve
the GTK3 app and all its write paths. No F10 group, native GUI, corpus, platform or
legacy-retirement gate is accepted by source queries alone.
