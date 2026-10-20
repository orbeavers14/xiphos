# Shared resource reuse for HTML export

Updated 2026-10-20. This is a bounded implementation plan, not a claim that search
exports contain images or arbitrary module styles.

## Reuse already extracted

The existing general-book image/stylesheet service now consumes private
`module_resource.hpp` / `module_resource.cpp` helpers for canonical containment,
regular-file checks, bounded binary reads and observed path/size/time changes.
Extraction checkpoint: `ff46f9b`. Book SDK context capture, URI decoding, MIME hints, stylesheet selection and
owned public values remain in their existing service. Error behavior is retained.
Normal Mac, ASan/UBSan, TSan and portable-on-Mac focused checks each pass 4/4
(original fixture, inline images, image-resource/owned C transport, stylesheet).
SDK dependencies remain partly instrumented; ASan leak checks are off.

These helpers accept a directory supplied by an owning core service; they do not
authorize a directory, capture SDK state or interpret a URL. They perform no
network fetch. Observed replacement checks are not a concurrent filesystem
snapshot, interprocess lock or guarantee against all path replacement races.

## Why the book embedding service cannot be adopted unchanged

`prepare_book_inline_images` preserves source HTML and reports per-image issues;
it may embed some images while leaving others unresolved. That behavior belongs
to the existing reading preview. Search export promises a complete bounded
artifact or an explicit failure with empty output. Its current CSP also prohibits
images. Book MIME values are filename hints, not validated decodes, and include
SVG. Arbitrary image bytes/styles cannot become trusted export data by renaming
that service. Its parser can normalize source wrappers and retain style elements;
the passive search policy deliberately rejects unsupported style context.

Book resource context still requires indexed general books. A separate
[owned verse image source service](verse-image-workflows.md) now handles the
characterized Bible/commentary family without weakening that book guard. Do not bypass the module-family gate or accept an arbitrary
frontend filesystem root. Declared modules in legacy image links remain metadata;
resource ownership must stay tied to the exported row's actual module.

## Next bounded slice

1. Characterize original-prose Bible/commentary image output, exact configured
   source directories and URI decoding, including relative/absolute paths, percent
   escapes, symlinks, missing files and explicit image-off preferences.
2. Define an opt-in self-contained document policy with aggregate row/image,
   original-byte and encoded/final-byte budgets, cancellation and no partial
   artifact. Decide permitted image types/decoding, data-URL and CSP behavior
   together. Keep unsupported default behavior until this policy is checked.
3. Reuse the extracted filesystem helper from an owner-side source adapter;
   preserve generation, history, settings and resource provenance. Copy bytes;
   never pass SDK/file handles or resource decisions to platform UIs.
4. Treat module CSS/recursive assets, semantic note/passage actions, external
   destinations, browser/CSP and visual/corpus acceptance as separate gates.

The in-app browser rejected local-file HTML preview by URL policy. Do not use
alternate URLs/surfaces to work around that rejection. Static/core checks do not
establish browser, CSP-runtime or visual acceptance.

The shared path seam additionally rejects embedded NUL in either directory or
filename before filesystem resolution, so native terminated-path calls cannot
silently address a prefix. Original-file checks verify exact resolution and
refusal even in missing-is-absent mode. Normal Mac 4/4, ASan/UBSan 3/3 and TSan
3/3 image/stylesheet checks pass including prerequisite fixtures. No new public
API or module-family/image-export support is implied.

## Verse-source characterization (2026-10-20)

A private original-prose fixture creates RawText and RawCom modules in both ThML
and OSIS, then observes actual SWORD XHTML rendering and `AbsoluteDataPath`.
ThML leaves image source strings unchanged. OSIS prepends `file:` and the exact
SDK data directory to every tested source, including parent traversal, missing
files and a network-looking string. Percent escapes and literal plus signs are
not decoded by either renderer. This is observed SDK behavior, not permission to
fetch a URL or bypass containment. The future adapter must keep row-module origin
and must not percent-decode an SDK-generated native directory prefix as if it
were an encoded URI component.

Configured session source checks establish image presence without learning display
preferences, explicit image-off text retention, stable history and no book
selection. The characterization passes normal, ASan/UBSan and TSan (one focused
fixture test in each; dependencies partly instrumented and leaks off). No resource
bytes or images are added to search exports by this checkpoint. Filesystem,
Unicode-directory, missing/symlink and whole-document policy checks follow.

Owned source implementation now passes 5/5 normal, ASan/UBSan, TSan and portable
on-Mac focused checks. The [contract](verse-image-workflows.md) records exact
bounds, copied ownership and OSIS native-prefix versus URI-suffix handling.
Document image embedding and native/browser acceptance remain pending.


Shared static PNG validation/encoding is now checked in
[its scoped contract](png-export-workflows.md), using libpng/zlib and original
fixtures. It does not yet enable images in search HTML. Next wire an explicit
complete-or-failure per-document policy with aggregate budgets and default reject.
