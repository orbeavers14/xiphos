# Export formatting and remaining workflows

Updated 2026-10-20. F20/F32 remain partial, with no accepted native export workflow.
The shared item formatter below retains GTK3 operation. A separate checked C++
[selected visible-row text service](search-text-export-workflows.md) now resolves
retained identities, reads configured SDK scripture and enforces aggregate output
caps. Native transport/save/copy, full retained export and printing remain pending.

## Shared item contract

C++11 `format_export_item` accepts owned metadata and supplied scripture. C11
`xiphos_export_item` adapts the same implementation; its allocation must be freed
with `xiphos_export_item_free`. Required C description/module/key pointers cannot
be NULL; html accepts only 0/1. C++ strings reject embedded NUL. No helper output
size/time cap is implied; a future export service must impose aggregate limits.

HTML template:

```html
<li>DESCRIPTION<ul><li>MODULE KEY</li>SCRIPTURE_ITEM</ul><br/></li>
```

Description/module/key use exact GLib `g_markup_escape_text` escaping. Supplied
scripture is trusted already-rendered module HTML, wrapped in one `<li>`. Absent
scripture omits that item; requested empty rendered text retains `<li></li>`.
The C facade distinguishes NULL from empty text; the owned C++ helper has an
explicit presence flag. This is item formatting, not sanitization of arbitrary
user HTML or a complete HTML document.

Plain template (literal indentation/newlines):

```text
\tDESCRIPTION\n\t\tMODULE KEY\n\t\tSCRIPTURE\n\n
```

Plain metadata/body is unchanged, including Unicode/markup-like bytes. A missing
body still has the same empty indented line. No whitespace normalization or
reference parsing occurs.

The retained `gui_set_html_item` / `gui_set_plain_text_item` keep their existing
module selection and SDK reads. Module names shorter than two bytes resolve via
the main Bible only if its key predicate succeeds. Text/commentary alone supply
rendered/stripped scripture. HTML NULL rendered text retains GLib's existing
`(null)` formatting; unsupported modules omit scripture. Formatting delegates to
the shared helper, with explicit CoreRender link dependency in the GTK target.
A helper allocation failure is fatal in this legacy path, consistent with its
prior GLib allocation policy, rather than silently writing a missing export row.
Native C++ callers can handle their owned formatter exceptions before file writes.

## Evidence

A verbatim GPL-2.0-or-later copy of the complete pre-extraction helper routines at
`f205722` retains Xiphos Developer Team attribution. Configure-time extraction of
the actual replacements is compared against it for 1,200 combinations of format,
scripture choice, long/Unicode/escaped metadata, normal/empty/NULL bodies, source
family, short-module fallback and invalid-key selection. Output bytes and SDK-call
order both match. A strict C11 object compiles the actual adapter functions without
GTK; a C caller links/runs them through the C++11 library. This is not a full GTK3
desktop build/runtime claim. Focused normal and ASan/UBSan checks, native-disabled
C++11 helper compilation and portable-on-Mac checks cover this seam. Exact final
counts/versions live in the [Mac handoff](handoff-macos.md).

## Advanced orchestration characterization

`core_advanced_search_setup` compiles the actual unchanged dialog setup, export
orchestration and bookmark-preparation functions with UI/storage boundaries
stubbed. No SDK corpus, GUI acceptance, bookmark creation or file write is implied.
It records these existing behaviors for a future shared service:

- Custom range wins over last mode. All selected modules must be legacy Text or
  Commentary. A mixed family disables range without clearing stored keys or
  choosing last. The first selected raw module name provides bounds; abbreviation
  resolution happens later. Missing saved ranges clear keys without selecting
  bounds. Empty custom lists attempt a NULL provider in the stubbed legacy path;
  the new bounded batch service deliberately rejects empty input.
- Advanced export uses the first eligible match only to emit a group header.
  Remaining matches become item rows. A one-match group therefore emits no result
  item. Skipped invalid keys do not count as that first eligible result.
- Group headers interpolate raw query/module text; the shared item helper's
  escaping does not fix those headers. Empty/ineligible groups still close a list.
  The caller in export_bookmarks.c supplies the final document footer.
- Mode labels preserve radio precedence: regex, phrase, multiword, optimized,
  otherwise attribute. They are not a full saved-query serializer.
- Bookmark preparation includes the first key and duplicates, joins eligible keys
  with `; `, and opens one dialog per group using the first eligible module and
  current settings.searchText. Its traversal advances the legacy global list
  pointer to NULL. A new owned service must not inherit this storage side effect.

These quirks are recorded, not endorsed as native parity requirements or silently
fixed. A replacement should preserve selection/module identity, ordering and all
intended matches while making any correction explicit with before/after evidence.
Do not delete the oracle functions until the full workflow gates pass.

## Next shared boundary

Define an owned export request/selection contract before adding native dialogs.
A displayed search prefix can omit results and is insufficient to claim a complete
export. Batch final last scope belongs to one module, so it cannot reconstruct
all earlier groups either. Choose explicit visible-selection export versus a
bounded retained full result/streaming export service; report truncation and never
silently label a prefix as all matches. Preserve module identity and duplicate
order. Do not rerun mutable searches implicitly to recover missing results.

Then add aggregate output bounds, configured history-neutral scripture reads,
header/document policy, text/HTML and bookmark transformations, copied worker/C
transport and appropriate native file/clipboard/print hosts. Compare legacy and
native output with original fixtures, representative permitted modules/profiles
and visual/keyboard/round-trip evidence. Complete passage export, verse lists,
bookmark hierarchy/import, printing/pagination and saved workflows remain open.
