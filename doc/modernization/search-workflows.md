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

## Initial owned Reader execution

`Reader::search_module` now executes whole-module Bible and regular-commentary
phrase/regex/word searches. It copies requested/effective query, actual SDK mode,
SDK result count and complete ordered references. Indexed word selection uses
the legacy capability probe and shared AND preparation; external results sort
as the backend does. Actual indexed execution remains unvalidated here. Output
is at most 10,000 copied references/8 MiB, with smaller defaults (1,000/1 MiB).
When a bound is reached, only complete prefix references are returned and the
limited flag is set; the SDK's own execution/internal results are not bounded.
Queries are nonempty UTF-8 without NUL, at most 32 KiB. No range/last-search,
active cancellation, progress exposure, index creation or UI is added yet.

The three temporary accent/vowel settings restore per-filter values, including
strip failures. Attribute processing and skip-link flags restore as well. The
checked SDK restores the key value but can replace its internal key object;
SDK pointers must never escape or be held across core operations. Search results
are owned values. Original-prose tests compare modes, case, ordered keys and
queries with an extraction of actual BackEnd::do_module_search, check copied
limits/no matches and unchanged data, and verify settings/recovery.

SWORD's case-insensitive flag depends on its compiled regex backend and is not
exported as a public constant. Native-core CMake exposes
`XIPHOS_SWORD_SEARCH_ICASE_FLAG` (GNU/ICU 2, C++11-regex 1). The local SDK is
checked with 2 and explicit upper/lower phrase-hit expectations. Run core_search
against each actual SDK before accepting case behavior; do not guess another
OS's dependency build settings. The initial failed fixture lacked its RawText
data directories; fixture reads now verify stored text before comparing search
results. This was corrected before source acceptance.
