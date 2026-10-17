# Search workflow characterization and seams

Updated 2026-10-17. This is scoped source evidence, not accepted F18–F20 parity.
Retain the GTK search interfaces/backend until complete replacements pass gates.

## Existing workflows

The sidebar chooses Bible/commentary, regex/phrase/word, case sensitivity,
unbounded/range/last-search scope. Search text is saved in a 255-byte setting
while the engine receives the complete entry text. Word search selects external
index mode only when the module reports a usable search framework; otherwise
it remains multiword. Indexed sidebar searches prefix + per ASCII-space word
and append a trailing-space workaround, preserving repeated/leading/trailing
spaces and tabs exactly. Query text then passes through the module's strip
filters. Before search, Greek accents, Hebrew vowel points and Arabic vowel
points are disabled in its dedicated legacy manager.

BackEnd::do_module_search clears current results, short-circuits an empty
last-search scope, invokes SDK search with progress callbacks, copies results
to the next last-search scope, and sorts external-index results. The legacy
result iterator copies keys to a 128-byte buffer before advancing. A native
owned result should preserve complete key text with explicit output bounds.
The advanced dialog handles multiple modules, abbreviations, attribute queries,
explicit index fallback messages, and result/preview/export interactions.
Cancellation currently changes the legacy search module's terminateSearch flag.
Native cancellation must respect one engine owner and avoid an unsynchronized
UI write to SDK state. No second search manager belongs in a native frontend.

| Requirement | Source | Native status |
|---|---|---|
| Sidebar mode/case/bounds/last scope | src/main/search_sidebar.cc | Indexed text policy extracted; service work active |
| Backend results/index/scopes/cancel | src/backend/sword_main.cc | Extraction pending |
| Multi-module/index/attribute/result handling | src/main/search_dialog.cc | Pending |
| Progress/events/native preview/export | src/gtk/search_dialog.c; src/main/search_sidebar.cc | Pending |

## First shared policy

`sidebar_search_text` is C++11, used by the retained sidebar and available to
the native core. It matches a frozen copy of the actual pre-extraction transform
for Unicode/punctuation/space cases and a deterministic 1,000-input byte corpus.
Non-indexed text remains unchanged. Inputs are NUL-free; this helper does not
validate search modes, execute SDK work, tokenize language, open files or manage
history. Removing the old early GString allocation also avoids retaining it on
the sidebar's empty-input return. Focused normal, sanitizer and native-disabled
legacy-library evidence is recorded with the implementation checkpoint.

The first execution seam should copy bounded results under the existing FIFO
worker, retain requested/effective query/mode/case context, restore temporary
filter settings, and keep reading/history/session state independent. Result
limits alone do not bound SDK search execution or internal result memory.
Bounds/last-search, complete cancellation/progress, advanced/indexed execution,
all module families, corpus/locale and native workflow acceptance remain gates.

The [official SDK source](https://www.crosswire.org/sword/apiref/swmodule_8cpp_source.html)
helps identify search ownership and backend-dependent case flags, but is an older
revision than the installed 1.9.0.3916 SDK. Validate actual behavior with local
original-prose fixtures; do not infer platform acceptance from that source page.
