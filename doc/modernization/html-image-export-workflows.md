# Shared static PNG images in HTML search export

Updated 2026-10-21. This is a scoped shared-core document feature with copied
C/Swift adapters, not full Xiphos export parity or a native HTML save/print UI.
See [HTML documents](html-export-workflows.md), [source image ownership](verse-image-workflows.md),
[PNG validation](png-export-workflows.md) and [resource extraction](resource-export-design.md).

## Explicit document policy

`SearchHtmlExportRequest.images` defaults to `SearchHtmlImagePolicy::reject`.
Existing C calls and default Swift calls retain the resource-rejecting output.
Opting into `embedded_static_png` permits only the shared validator's complete,
decoded static PNG images. Unsupported image formats, corrupt/empty images,
missing/outside-module paths and external source URIs fail with no document.
An explicit existing image-off module preference still removes images in the
configured source renderer; export opt-in does not change that preference.
Reference-only exports do not load images.

Selection, row ordering/duplicates, retained-generation identity and explicit
partial-search-source acceptance use the same existing shared selection engine.
The resulting document keeps source disposition labels. A failure on any row,
resource, output limit or cancellation returns empty HTML, zero rows and zero
image statistics. A failed row ordinal identifies the selected output row where
available. No save, clipboard write, visit or logical profile change occurs.

## Ownership, cache and limits

One per-job cache loads bytes through `Reader::read_verse_inline_image` on the
existing SWORD-owning worker. Each exported row supplies its actual module.
Frontend paths and modules declared in image commands cannot supply resource
ownership. ThML and OSIS Bible/commentary source forms reuse the characterized
URI/native-prefix handling and canonical bounded filesystem helper.

Cache identity is the pair of actual module and exact parsed source URI. Repeated
occurrences reuse the first validated copy during that document. The same URI in
two modules is a distinct resource; URI aliases are conservatively distinct too.
No persistent cache or network loading is introduced. Observed filesystem change
guards do not provide an interprocess filesystem snapshot.

All image limits must be positive, including requests using reject policy:

| Limit | Default and ceiling | Accounting |
|---|---|---|
| Image count | 64 | Every occurrence, including cache hits, across all rows |
| Original resource bytes | 8 MiB | Unique module/URI assets |
| Decoded pixels | 16 Mi pixels | Unique module/URI assets |
| Encoded cache bytes | Existing requested document byte cap | Unique PNG data URLs |
| Final HTML bytes | Existing 1 MiB default / 8 MiB ceiling | All markup, metadata, CSS and every repeated encoded image |

The existing maximum 10,000 export rows remains independent. The PNG validator
also retains its per-image dimension/chunk limits. Cache statistics (`count`,
`unique_count`, `original_bytes`, `pixels`) publish only after complete successful
assembly. They describe included assets, not all resources present in a module.

The complete passive structure of each fragment passes before its images load.
Image sources are replaced with validated original-byte PNG data URLs. Existing
passive alt/dimension attributes survive serialization. Scripts, events, srcset,
arbitrary inline/module styles and other unsupported resources still reject.
Default documents keep `img-src 'none'`; explicit PNG-policy documents use
`img-src data:` while retaining default resource/script denial and base/form
restrictions. These are generated-policy checks, not browser/CSP acceptance.

Cancellation is cooperative before/after bounded file reads, between PNG chunks
and decoding/encoding stages, and during row assembly. It does not interrupt a
codec call or freeze SDK/filesystem work. Logical output/resource bounds do not
bound every parser/codec allocation or wall-clock duration.

## C11 and Swift 6 transport

The additive C API uses `xc_search_html_image_options_default` and
`xc_worker_export_search_html_with_images` with a required separate options value.
It does not extend the existing request/info struct layouts. Inputs copy before
submission; invalid shapes clear the observer. Existing polling, progress/cancel,
take-once document ownership and release rules apply. Image failure is a typed
export disposition on a successfully polled request. Unknown/closed tab errors
still arrive at poll time.

`xc_search_html_export_get_images` copies four counters from the owned result.
NULL argument rejection clears the supplied output. HTML and counters survive
request/worker teardown until document release. The experimental C API version
remains 1; no old C layout changed.

Swift `CoreHTMLImageOptions` gets its defaults from C. `CoreWorker.exportSearchHTML`
accepts those options and copies the resulting HTML and typed image statistics
before releasing C ownership. Swift rejects nonpositive signed limits before
submission; the C/core boundary enforces ceilings. There is no Swift URI parser,
image loader, cache or PNG validator. Native HTML save/copy/print controls remain
separate work.

## Validation limits

Original-prose generated RawText/RawCom ThML/OSIS fixtures cover actual inline
image source forms, valid original PNG bytes and missing/outside/external/corrupt
resources. Tests check selected duplicates, all-retained and labelled partial
sources, exact/one-short byte/pixel/count bounds, state neutrality, copied request
options, C ownership, Swift lifetimes and cooperative cancellation outcomes.

The browser previously rejected local-file export preview by URL policy. Do not
use an alternate URL or viewing surface to bypass that restriction. No browser,
CSP-runtime, visual/corpus, current Fedora/MSVC, minimum-macOS or whole-feature
acceptance follows from static parsing and local Mac tests. Full module CSS,
other image formats, semantic-action expansion and print/passage workflows remain
open. The status ledger and platform handoff record executed checks.

Focused local checks pass 11/11 normal, 11/11 ASan/UBSan, 11/11 TSan and 11/11
portable/core-on-Mac, with GTK runtime OFF for the portable configuration.
The Swift image/default HTML selection passes 8/8 including original fixture
setup and C++/C image transport. Strict C11 header syntax passes. Dependencies
are partly instrumented; ASan leak detection is off. Local log prefixes are
`build/search-html-images-c-final`, `search-html-images-swift-final`,
`search-html-images-sanitized`, `search-html-images-thread-sanitized` and
`search-html-images-linux-models`. See the
[dependency discovery record](../../dependencies/README.md) for current GLib
selection and deployment-target limitations. No hosted build was run.

Implementation checkpoints: core/C `4fecce8`, Swift `374147d`. The complete
local macOS build/suite passes **164 tests + 1 explicit index skip** (165
registered, 137.38 s); `build/search-html-images-full-{build,tests}.log`.
The unavailable index framework is not counted as execution acceptance. No
new full portable/model suite was run; the portable result above is focused
core coverage. Native HTML destination controls are the next bounded slice.
