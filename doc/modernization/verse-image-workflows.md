# Owned Bible and commentary image resources

Updated 2026-10-20. This checked C++ source service reads copied bytes; it does not
embed images in HTML, decode pixels or add native image controls.

`Reader::read_verse_inline_image` and its FIFO `StudyWorker` command accept an
actual module ID, rendered image source and positive byte cap (default/ceiling
16 MiB). The module must be an installed VerseKey Bible or regular commentary,
the same characterized family used by verse search/text export. General books
keep their existing separate service and family guard. A result owns module ID,
original source, filename-extension MIME hint and binary bytes, including NUL.
Empty files have valid empty data. MIME is not validation of the file's content.
No SDK/file handles escape and accepted worker jobs drain on shutdown.

## Path and ownership policy

The owning core reads the module's actual `AbsoluteDataPath`. Existing URI decoding,
slash/drive normalization, canonical containment/regular-file checks and bounded
file-copy/change-detection helpers are reused. No root comes from a frontend,
declared alternate module, current reading pane or URI authority. Missing files,
parent/cross-module escapes, escaping symlinks, directories, malformed/NUL/non-UTF8
sources, unsupported schemes/authorities and oversized data fail without a result.
Contained symlinks are supported; they were exercised on this Mac. Windows needs
native validation, including environments where symlinks require extra access.

OSIS renders an exact raw native SDK directory prefix followed by the source URI.
When that prefix matches, the adapter removes it before decoding only the URI
suffix. This preserves literal `%20`, spaces and Unicode in native library roots;
ThML relative URLs and explicitly encoded contained absolute file URIs retain
one URI decoding pass. Plus remains literal and `%2520` addresses literal `%20`.
Observed canonical-path/size/time checks are not an interprocess lock or a complete
snapshot against concurrent filesystem replacement.

Resource reads do not render, move module keys, change options, publish learned
image/font preferences, create history visits or select books. Calls are separate
from image-off rendering policy: a caller that intentionally requests source bytes
can read them, while exported documents must first honor their configured source.

## Local evidence

Original-prose RawText/RawCom modules cover ThML and OSIS actual SDK source output.
Their private library directory contains spaces, Unicode and literal percent text.
Exact/one-byte-short caps, binary and empty files, percent/plus paths, missing
files, unsafe/cross-module/directory/symlink cases, unsupported general books,
logical state neutrality, file preservation, copied requests and worker lifetime
are checked. Normal Mac, ASan/UBSan, TSan and portable-on-Mac focused suites each
pass 5/5 including book image/style regressions and fixture setups. External SDK
instrumentation is partial; ASan leaks off. Portable GTK runtime is OFF, not a new
Fedora or MSVC acceptance run. No hosted builds used.

Next: an explicit complete-or-failure document image policy, aggregate resource/
encoded-output limits, cancellation and copied C/native transport where needed.
Do not reuse the reading preview's partial-image fallback as a successful export.
Browser/CSP/visual/corpus checks remain pending; local-file preview was previously
blocked by browser URL policy, with no workaround or visual acceptance.

## Additive C11 transport

A separate opaque image handle exposes the same copied module/source/MIME/bytes.
Submission caps module/source lengths (4096/32768 bytes) before copying/enqueue;
NULL/empty inputs and byte caps outside 1..16 MiB reject and clear the observer.
Semantic/UTF-8/path errors arrive through polling. Results take once after XC_OK,
are distinct from book image handles, and outlive both request and worker. Empty
bytes use NULL plus size0; the owned result itself remains present. All borrowed
fields are valid until that image handle is released. No existing C struct layout
or API version changes.

Focused normal/ASan/UBSan/TSan checks pass 4/4 each, including the source fixture,
C image transport, C++ resource regression and C-reader/pedantic header checks.
Cases cover caller-input mutation, overlong/NULL/invalid input, binary/empty data,
exact/one-short bytes, missing/path errors, wrong/second takes, observer release
and shutdown draining. Strict standalone C11 header syntax passes. This adds no
native UI, image decoding or document/browser acceptance.

## Swift copied values

Swift6 `CoreWorker.readVerseInlineImage` submits through C and copies module,
original source, MIME hint and `Data` before releasing all borrowed C handles.
Signed capacities, empty/overlong and embedded-NUL strings reject before C-string
conversion. No URI decoding, filesystem policy or pixel decoding moves to Swift.
As with existing non-search source reads, accepted work drains to its authoritative
outcome even when its observing task is cancelled; interruption is not promised.

Seven focused Mac checks pass, including the new Swift adapter, book-image Swift
regression, C/C++ verse image tests and prerequisite fixtures. All four original
module types, exact/short capacities, binary/empty values, invalid/unsafe inputs,
state neutrality, cancellation/draining and post-shutdown copied values are
covered. This changes adapters only; native image UI and exported images remain
separate work.

## Actual SDK image commands

`Reader::read_verse_image_link` and its FIFO worker command read actual SDK
showImage links through the same bounded source service. The copied `source`
identity is the original command URI. The existing strict book command parser
performs query decoding/validation (including duplicate/unknown fields and
suffix rules); that command value still contains a separate image URI suffix.
ThML prefixes that value with `file:`; OSIS uses the raw SDK native directory.
The adapter then decodes only the source-URI suffix, preserving the matched OSIS
native prefix. Declared alternate-module fields remain metadata and never rebase
resource ownership. Other commands/external origins are not image dispatch.

This is an explicit SDK-command source contract. Legacy viewer dispatch passes a
raw decoded filename to an external viewer, and the old book command API retains
its characterized filename semantics. The new service intentionally resolves
SDK image URI components under the actual source module so inline and clicked
images agree; it does not silently redefine the old book/manual-filename API.
Legacy hover filename/status-bar presentation and complete native interaction
parity remain separate gates.

Actual generated links for all 28 original rows are characterized. Successful
image commands and inline sources produce the same owned bytes, including Unicode,
literal-percent and plus filenames. Alternate declared modules cannot redirect
them; missing/parent/network-looking sources refuse. Copied FIFO command requests
and source identity survive shutdown. Normal Mac, ASan/UBSan, TSan and portable-on-
Mac focused suites each pass 4/4 including original source/resource tests and
retained book image regression/setup. No image embedding or viewer is added here.

## Typed reading-pane dispatch

The contextual shared link classifier now returns a distinct image target bound
to the committed source module. Contextual follow/resolve and pane navigation APIs
return `VerseImageResource`; the existing C request stores it as the same opaque
`xc_verse_image` kind. No additional C dispatch function or frontend command
parser is needed. Existing note/reference/lexical/passage kinds retain their
behavior, and the older two-kind resolve API is unchanged. Closed tabs or missing
committed context fail; successful image reads add no reading/dictionary history
or book selection.

Image hover returns an explicit no-presentation marker without opening the file,
including when the referenced image is missing. Legacy hover filename/status text
is not implemented by this slice. No image bytes reach a hover preview.

Normal Mac, ASan/UBSan, TSan and portable-on-Mac focused checks each pass 10/10:
verse source/resource/C transport, existing content/note/hover/paired-passage
regressions and prerequisite fixtures. C contextual takes are distinct/take-once;
source-pane ownership differs intentionally from the navigation Bible. Native
consuming image UI and complete visual/workflow acceptance remain next.
