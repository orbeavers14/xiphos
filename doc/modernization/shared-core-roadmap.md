# Shared core: remaining scope

Updated 2026-10-17. This is a planning map, not an acceptance score.
The full project remains roughly 8% (6–10%), with zero whole parity groups
accepted. There is no evidence-based core-only completion percentage yet.
A tested worker/reading foundation does not mean the application core is almost
finished. Whole workflows and data compatibility remain the acceptance units.

| Core area | Current evidence | Remaining shared work |
|---|---|---|
| Engine ownership and transport | FIFO worker, copied C/C++/Swift results, shutdown draining | Active-operation cancellation, native Windows compiler/SDK validation, stable dynamic packaging |
| Scripture and commentary | Reference choices, chapter/navigation/history, sparse/linked sources and options | Complete display decorations, corpus/locale/versification workflows and restoration |
| Dictionary and lexical content | Lookup/key steps/history, shared casing, lexical HTML/CSS and notes | Complete lookup/hover/search families and full corpus/locale acceptance |
| General books | Tree/navigation/DisplayLevel documents, scoped restore, note/image clicks, bounded inline resources | Image sizing/CSS/formats, complete semantic links/numbering, book windows/tabs and saved workflows |
| Parallel reading | Pending | Verse alignment, ordered translations, named sets, navigation and persistence |
| Search | Pending | Simple/advanced/indexed/attribute searches, bounds, progress/cancel, results/preview/export |
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

Nearest sequence: finish bounded book resource/sizing seams and corpus checks;
complete remaining book link/numbering/session contracts; then extract search
with progress/cancellation before moving into parallel reading and persistence.
Reorder only with evidence or owner steering; do not block all native work until
an imagined final core freeze. Current evidence/active task lives in the
[shared handoff](handoff.md) and [Mac handoff](handoff-macos.md).
