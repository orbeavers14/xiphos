# Mac preview: current user guide

This guide describes the native development preview, not the retired GTK app.
Some commands open separate management windows; native navigation improvements
are tracked in the [UI to-do list](modernization/native-ui-todo.md).

## Reading and comparison

Select an installed Bible, enter a canonical passage reference and choose Read.
Book/chapter/verse selectors, history and chapter view help navigation. Reading
options depend on the selected module. Choose a commentary or dictionary for
related study; original-language links use installed lexical metadata.

Use File > Saved Parallel Reading Sets to choose ordered modules, save a set and
read it in the active window. The first module controls chapter references.
Choose a chapter matrix or stacked single-verse comparison and toggle translation
visibility without changing the saved module order. Mapped/split verses retain
their source references; unavailable verses are labeled.
Search uses installed module capabilities; unavailable indexes may fall back or
report unsupported execution. Select a word in the reader or comparison and use selected-word study for literal,
mark-insensitive or available lemma searches. Option-Command-F invokes literal
selected-word search; check which pane owns the selection. Lemma choices require
module metadata. Results can open in the default comparison set.

## Notes, settings and windows

Open Personal Verse Notes, load a target, edit the plain-text note and save it.
Show in Reading applies the note source; expand Personal note in reading to view
it. Bookmarks/import and rich study documents have their own management flows.

| Shortcut | Action |
|---|---|
| Command-comma | Settings |
| Command-N | New reading window |
| Command-T | New reading tab |
| Command-S | Save the active note/editor document or reading session |
| Command-Shift-N | Personal verse notes |
| Command-Shift-P | Saved parallel reading sets |

Use File > Move Reading Tab to New Window to detach a tab. Session save retains
reading/window state and native tab grouping. Settings exposes reading/lexicon
choices, defaults for new windows, startup restoration, recovery interval and
language selection. Appearance offers System/Light/Dark, and Reading Style controls
default and per-module fonts, sizes and colors. Some changes require restart. Non-English translations are
partial; localized selector labels keep canonical navigation references.

## Maps, find and export

Preferences > Biblical Map opens sourced markers and local offline packs; pack
installation/removal and optional checksum-verified HTTPS download are available.
Find in Passage uses Command-F, Command-G and Shift-Command-G within displayed
Bible text. Passage text/HTML export and native printing are available; physical
print pagination still needs acceptance. Selected text also has explicit speech,
Apple Maps and Google Translate actions.

## Library, migration and recovery

Module Library manages repositories, installation and maintenance. Review changes
before applying them. Licensed provider translations beyond available modules
are future work. Legacy migration explicitly reviews settings/session sources
and copies selected assets into a separate destination.

Recovery protects unsaved drafts and reading state. A recovery prompt may appear
after interruption; original documents/modules are not silently overwritten.
Save important work normally. BibleSync shares passage navigation with explicit
connection/consent; it does not synchronize notes or files.

See [status and limitations](modernization/status.md), [build instructions](../INSTALL.md)
and [runtime packaging](modernization/native-runtime-packaging.md) before relying
on a development build. No released installer or full platform acceptance is claimed.
