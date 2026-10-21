# Remaining search export work

Updated 2026-10-21. This is the remaining plan, not an implemented API contract.
Checked behavior is defined by [retention](search-retention-workflows.md),
[text export](search-text-export-workflows.md) and [shared item formatting](export-workflows.md).
No whole search/export parity group or legacy retirement gate is accepted.

## Checked foundation

One engine-owner FIFO retains immutable per-tab result generations, ordered
queries/groups/failures and bounded complete typed keys, separately from final
last scope. Copied generation IDs reject stale reads/exports. Aggregate capacity
failures explicitly disable complete retention while visible rows remain usable.
C++/C/Swift transport is checked; no SDK handle crosses to a frontend.

Selected visible and complete returned text share one document/SDK-reading policy.
They preserve order/duplicates, include every intended first row, enforce aggregate
row/byte guards and publish no partial failure document. Complete export rejects
cancelled/failed/unattempted source by default; explicit partial-source acceptance
preserves its metadata. Native Mac text save/copy consumes the same services with
Stop/stale/close protection, isolated clipboard/file tests and scoped window/save
evidence. Exact hashes/environment/counts live in the handoff and contracts.

Initial C++/C/Swift selected/all passive HTML is now checked in [its scoped contract](html-export-workflows.md).
Explicit static PNG images and native Mac HTML save/copy now have scoped
[shared](html-image-export-workflows.md) and [native](native-search-html-export-workflows.md)
evidence. Other formats/module styles, semantic links and browser/visual acceptance
remain open. Rich clipboard plain-text fallback is a remaining shared task.

## Next shared seams

1. Follow the [resource reuse audit](resource-export-design.md) to characterize self-contained versus externally referenced images/assets and
   semantic note/passage links across module families. The restricted service
   rejects unsupported requirements; do not silently strip them or enable fetching.
2. Expand shared advanced query/mode headers and module sections with workflow
   comparisons. Keep first-row inclusion and explicit escaping corrections.
3. Native Mac HTML save/copy now consumes copied complete documents. Expand shared
   rich-clipboard/plain-text fallback and native target acceptance without frontend
   HTML stripping or a second render/selection policy.
4. Validate browser/CSP, representative corpus, native destinations and full
   workflows on each target. SDK temporaries/time and external file mutation are
   not bounded/transactional merely because final output bytes are bounded.

## Remaining workflow acceptance

The current text document has one caller-localizable title and item sequence.
Full advanced query/mode headers, module sections, multi-selection ordering,
legacy/native dialog choices and saved export settings still need workflow and
visual acceptance. Native hosts should adapt those shared policies rather than
recreate them. Core and output capability must remain distinct from all requested
modules successfully completing.

Bookmarks require a shared ordered transformation/repository, compatible hierarchy,
XML import/export round trips and existing first-row/header quirk decisions.
Passage/selected-text export is a separate source contract. Printing, pagination,
clipboard rich formats, accessibility and native dialogs on all three platforms
remain independent gates. Keep actual indexed execution, mixed module families,
full permitted corpus/locales and saved session compatibility in view.

Continue small reviewable slices with local tests and existing GTK3 operation.
No hosted builds are required. Profile replacement is not a generic export-file
API; reuse utilities only when their ownership/path/format contracts fit.
