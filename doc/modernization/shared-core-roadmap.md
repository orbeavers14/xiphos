# Shared core: remaining scope

Updated 2026-10-20. This is a planning map, not an acceptance score.
The full project remains roughly 8% (6–10%), with zero whole parity groups
accepted. There is no evidence-based core-only completion percentage yet.
A tested worker/reading foundation does not mean the application core is almost
finished. Whole workflows and data compatibility remain the acceptance units.

| Core area | Current evidence | Remaining shared work |
|---|---|---|
| Engine ownership and transport | FIFO worker, copied C/C++/Swift results, shutdown draining | Cancellation for other active operations, native Windows compiler/SDK validation, stable dynamic packaging |
| Scripture and commentary | Reference choices, chapter/navigation/history, sparse/linked sources and options | Complete display decorations, corpus/locale/versification workflows and restoration |
| Dictionary and lexical content | Lookup/key steps/history, shared casing, lexical HTML/CSS and notes | Complete lookup/hover/search families and full corpus/locale acceptance |
| General books | Tree/navigation/DisplayLevel documents, scoped restore, note/image clicks, bounded inline resources/module CSS, shared geometry and combined note numbering | Native sizing/global CSS/assets/formats, complete semantic links/corpus acceptance, book windows/tabs and saved workflows |
| Parallel reading | Pending | Verse alignment, ordered translations, named sets, navigation and persistence |
| Search | Bible/commentary basic + exact attribute modes, explicit intervals/complete last scope, private annotation context, advanced word dialects, bounded ordered batches, complete per-group result generations, stale-safe C/Swift reads, bounded selected/all text and restricted HTML export with C/Swift adapters, native Mac text save/copy, progress/cancel and basic controls | Advanced module/range setup, actual indexed execution, mixed families, advanced controls/settings, complete preview/export and full backend/corpus acceptance |
| Library and installation | Module metadata/discovery subset | Favorites/hiding/grouping, sources/install/uninstall/index/cipher workflows |
| Bookmarks and annotations | Pending | Hierarchy/order/colors, XML round trips, AndBible import, rendering precedence |
| Writing and editing | Pending | Personal commentary, study pads, journals/prayer lists, compatible document storage |
| Settings and sessions | Explicit scoped option import/save and book restore subsets | Complete profile/session/tab restoration, lossless migration/backup and unknown-field preservation |
| Sync and external actions | Internal URI routing subset | BibleSync modes/privacy, IPC and full semantic action/export policies |

Native controls, accessibility, clipboard/print dialogs, window layout and platform
packaging also remain, but are frontend responsibilities rather than a second
implementation of these services. The detailed requirements remain in the
[parity matrix](parity-matrix.md). Implement shared services in vertical slices
with a consuming frontend to expose boundary problems early. Existing GTK3 is
the behavior/design reference; keep it operational and retain retirement gates.

Current checkpoint: retained result generations C++ `37d9004`, C `155e0cb`, Swift
`d1a15df`; selected visible text export C++ `3faadea`, C `6f0e3c1`, Swift `ec26889`.
See [retention](search-retention-workflows.md) and [text export](search-text-export-workflows.md)
for actual bounds/ownership. Complete returned text is checked at C++ `b7dafd4`, C `1bb2615`, Swift `80589a4`;
native Mac text actions at `5d52a11`. Restricted HTML C++ `1b327d9`, C `e7c141e`, Swift `e8a8ec8` share configured
sources/selection and reject unsupported resources. Next resource/link policy,
native HTML destinations and browser/visual acceptance remain distinct gates.
The [resource reuse audit](resource-export-design.md) records shared private
filesystem extraction `ff46f9b` and NUL-path guard `7e9ff93`; this adds no
search-image support. Exact [item formatting](export-workflows.md)
is shared with GTK; advanced header/first-row quirks are characterized. Actual
indexed execution, mixed families, advanced controls/settings and saved workflows
remain partial work.
Book link/session, native
sizing and corpus acceptance remain tracked work alongside the existing reading
seams. Parallel reading and complete persistence remain later core services.
Reorder only with evidence or owner steering; do not block all native work until
an imagined final core freeze. Current evidence/active task lives in the
[shared handoff](handoff.md) and [Mac handoff](handoff-macos.md).
