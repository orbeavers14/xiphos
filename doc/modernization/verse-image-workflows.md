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
