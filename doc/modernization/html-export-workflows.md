# Initial shared HTML search documents

Updated 2026-10-20. Checked C++ service for a restricted passive HTML subset.
C/Swift HTML transport, native HTML save/clipboard, visual/browser acceptance,
images/module CSS, full advanced headers and printing remain gates. See
[text export](search-text-export-workflows.md), [configured sources](html-source-workflows.md)
and [remaining work](search-export-design.md). No whole parity group is accepted.

## One selection and source engine

`export_search_html` accepts the originating live tab/generation, explicit visible
ordinals or all retained results, title, scripture choice and positive aggregate
rows/bytes. Visible selection preserves duplicates/order; all mode requires empty
explicit rows. All-source cancellation/failures/unattempted groups require the
same explicit partial-source choice as text. Stale/capacity/clipping/invalid-row
outcomes never substitute newer results, visible prefixes or a rerun.

Text and HTML share the private resolver for authoritative module/key pairs and
source disposition. Common C++ metadata is factored without changing existing C
struct layouts. Text item bytes remain unchanged. HTML uses the checked shared
item formatter with escaped title/description/module/key, including every first
and duplicate row. It emits a complete UTF-8 document with selected/all labels;
explicitly accepted partial sources are labelled inside the document as well as
in copied generation metadata.

Optional scripture uses bounded configured body/CSS capture on the same owner,
without publishing automatic image/font preferences. Every row keeps its current
module settings. Known core lexical/spacing styles are scoped to reserved wrapper
classes and deduplicated; a raw/plain fallback cannot inherit a neighboring
module's annotation rules. Arbitrary module/host styles are not mistaken for the
core styles. Body-only source follows legacy item export; advanced grouping/mode
headers and separate passage/heading workflows are not complete.

## Initial markup/resource policy

The core parses UTF-8 fragments locally with libxml2 NONET and no external DTD
loading, checks a restricted passive tag/attribute/link subset, then serializes
that structured body. Known SDK bare ampersands in internal links become canonical
HTML entities without changing URI values. Requiring strict XML-style wellFormed
would reject real SDK output; the initial rich-source test caught and corrected
that before acceptance. This is not a general-purpose HTML sanitizer or a claim
of byte-identical browser repair across arbitrary module markup.

Unsupported images/resources, inline styles, active/unknown markup, unrecognized
stylesheets, unsafe/unsupported link schemes and reserved-class collisions reject
the whole document with a typed issue. They are never silently dropped or fetched.
Existing explicit image-off preferences still apply in the shared renderer. Core
note/passage links may remain application/context dependent; self-contained note
bodies, resource rewriting and semantic action export are separate gates.

The document sets a restrictive CSP: no default resource/script loading, no images,
base or form actions; only its core-generated inline styles are allowed. This does
not replace full module/corpus/browser acceptance. The fragment traversal also
has node/depth guards (100,000/128). SDK/libxml/presentation temporary allocation
and execution time are outside the logical copied-output limits.

## Bounds, ownership and evidence

Defaults are 1 MiB/10,000 rows; ceilings 8 MiB/10,000. Header, escaped metadata,
scoped styles and body/footer all count toward the exact final byte cap. Any
limit, cancellation or unsupported source returns empty HTML and zero output rows.
An optional failed ordinal identifies the resolved output/request row. Only a
fully allocated accounted document is published. Requests/metadata/payload are
copied; worker shutdown drains accepted work. Histories, book selection, search
scope/generation/context and logical preferences remain unchanged.

Original OSIS/ThML fixtures and pure policy tests establish complete results beyond
prefixes, first/duplicate rows, escaped malicious-looking titles, exact/one-byte-
short byte caps, clipped all-result rejection, partial-source labels, malformed
selection, stale/cancelled jobs, no publication on unsupported images, configured
mixed-module stylesheet isolation, valid SDK link canonicalization and owned
shutdown lifetime. Existing text/C export regressions pass alongside HTML.
Eight focused normal checks pass, with 8/8 ASan/UBSan and 8/8 TSan including fixture
setup. Full portable core/model suite on Mac passes 117 + 1 index skip (118
registered), 88.79 s, GTK runtime OFF. Dependencies remain partly instrumented
and ASan leaks off.

An attempted in-app browser preview of the generated original-prose local file was
rejected by its URL security gate (file protocol disallowed). No workaround was
attempted. Browser pixels, CSP runtime behavior, physical input and visual HTML
parity remain unvalidated; DOM/stylesheet structural checks are the accepted scope.
No hosted jobs/reset credit or legacy source retirement.

## Owned C11 transport

The additive C descriptor copies generation/selection, visible ordinals, plain
UTF-8 title, scripture, row/byte caps and partial-source choice before acceptance.
All-retained mode requires NULL rows/zero count. A dedicated take-once owned HTML
handle exposes disposition, issue, failed ordinal, source generation and complete
HTML/byte size. It remains independent of the request and worker. Text and other
request takes cannot consume it; invalid getters clear stale output values.
Existing C structs/layouts remain unchanged and strict C11/pedantic compilation
passes. Existing per-job progress/cancel controls observe this job only.

Original-fixture C checks establish copied-input mutation, omitted-prefix recovery,
duplicate visible rows, exact/one-byte-short cap, stale/unknown tab, invalid/NULL
shapes, typed unsupported images, cooperative cancellation, detached observer and
shutdown lifetime. Normal C+fixtures pass 4/4; C/C++ HTML plus C-text regressions
and fixtures pass 6/6 ASan/UBSan and 6/6 TSan. Dependencies remain partly
instrumented and ASan leaks off. This adds no native HTML/browser acceptance.
