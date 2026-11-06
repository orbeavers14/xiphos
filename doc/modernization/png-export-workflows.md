# Shared static PNG export preparation

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


Updated 2026-11-03. This private C++ service prepares owned PNG data URLs for
the explicit complete-document image export policy. Static PNG behavior is
described below; optional BMP conversion is documented at the end.

## Content validation and bounds

`prepare_export_png` accepts copied resource bytes/MIME plus explicit original,
encoded and pixel budgets. Positive byte ceilings are 8 MiB; the pixel ceiling
is 16,777,216 and each side is at most 16,384. It checks a complete static PNG
container, bounded chunk sizes/count (4096), ordering, checksums and terminal
trailer with no trailing payload. Animation chunks and unknown critical chunks
reject. Actual libpng RGBA pixel decoding must succeed without warnings before
encoding the original file bytes. The temporary decoded pixels are discarded;
metadata/colors/source bytes are not replaced with a lossy re-encoding.

The resulting MIME prefix is core-owned `data:image/png;base64,`. Its full prefix,
padding and encoded bytes count toward the output budget. Failure/cancellation
returns no partial URI or successful image metadata. Cancellation is checked
between stages/chunks, not interruption inside a codec call. The budgets do not
bound every library temporary allocation or execution time; this is not a general
PNG-conformance or decoder-safety certification. Other formats, animations,
whole-document resource transactions and browser/visual acceptance remain gates.

The implementation uses the [W3C PNG specification](https://www.w3.org/TR/2025/REC-png-3-20250624/)
for container/checksum rules and libpng's maintained simplified read API for
pixel decoding. No upstream codec source is copied into the repository.

## Dependency and local SDK provenance

Native core builds require libpng >=1.6 with simplified reading/writing and zlib, using
CMake `PNG::PNG`/`ZLIB::ZLIB`. The legacy native-disabled C++11 helper build still
returns before these dependency checks. Linux's explicit dependency installer
now lists PNG/zlib development packages; Windows and Mac guides list discovery
inputs. Existing disabled hosted workflows remain disabled; no job was started.

Project minimum and tested SDK pin are distinct: local validation uses official
**libpng1.6.59**, source archive
[libpng-1.6.59.tar.xz](https://downloads.sourceforge.net/project/libpng/libpng16/1.6.59/libpng-1.6.59.tar.xz),
SHA-256 `d80dd2a38a37f803cb9b6ac7b14bd6e74ddc3b654780a8380bdf93523fdb4389`,
verified against official Homebrew package metadata. Its inspected license is the
PNG Reference Library License version2 (`libpng-2.0` package identifier). Preserve
external SDK notices/material in release packaging; the overall distribution
license audit remains open.

The inspected Homebrew bottle of the same version targets macOS27. Local target26
checks instead use a static official-source build, with object minimum26 verified
by inspection. No codec source/binary/cache is committed. The existing SWORD/GLib
SDKs also target26; macOS13 and target Windows/Linux SDK acceptance remain open.
Configured zlib headers/discovery report1.2.12; system dependency instrumentation
is partial. This does not change the shared C++17/C11/Swift6 language decisions.

After downloading and verifying the archive under ignored build storage, local
SDK commands are:

```sh
cmake -S build/dependencies/libpng-1.6.59 -B build/dependencies/libpng-build \
  -DPNG_SHARED=OFF -DPNG_STATIC=ON -DPNG_FRAMEWORK=OFF -DPNG_TESTS=OFF \
  -DPNG_TOOLS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0 \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/libpng-sdk"
cmake --build build/dependencies/libpng-build --parallel 2
cmake --install build/dependencies/libpng-build
cmake -S platforms/macos -B build/macos \
  -DPNG_PNG_INCLUDE_DIR="$PWD/build/libpng-sdk/include" \
  -DPNG_LIBRARY_RELEASE="$PWD/build/libpng-sdk/lib/libpng16.a"
```

These are local validation inputs, not a portable release SDK or permission to
raise another platform's deployment target. On other targets use a coherent SDK
prefix/toolchain and record actual versions/configuration, never Mac archives.

## Checked evidence

Normal Mac focused checks pass3/3 (PNG codec, existing Swift verse adapter and
source fixture); ASan/UBSan, TSan and portable-on-Mac PNG checks pass1/1 each.
Native-disabled C++11 helper configure/build passes with PNG discovery explicitly
disabled. Linux shell syntax and strict C11 headers pass. SDK/codec libraries are
not fully sanitizer-instrumented; ASan leak checks are off. These are scoped
checks, not a newly run full suite or Fedora/MSVC acceptance.

Original fixtures cover actual pixel decoding and exact source encoding, byte/
encoded/pixel boundaries, every truncation/single-byte corruption, CRC-correct
invalid compressed data, bad headers/order/critical/animation/trailing chunks,
work limits, original grayscale/palette/16-bit/alpha data, cancellation and eight
concurrent independent codec calls. No module content or user image is imported.
Next: consume this service in a bounded per-document resource policy with no
partial output, then copy options/results through existing platform adapters.
The previously blocked local export-HTML preview remains unvalidated; do not work
around browser URL policy to claim visual/CSP runtime acceptance.

## Optional BMP conversion — 2026-11-03

Policy2 (`embedded_static_png_bmp` / `XC_HTML_IMAGES_EMBEDDED_STATIC_PNG_BMP`)
adds uncompressed24-bit Windows BMP with an exact40-byte DIB header and54-byte
pixel offset. Policy1 remains PNG-only and policy0 still rejects images. Existing
C structs/ABI are unchanged. The native explicit toggle is now Embed PNG/BMP
images; previewing other formats does not grant export permission.

The core validates the complete BMP container, reserved fields, dimensions,
planes/bit depth, compression, palette metadata, row padding and declared/actual
sizes before allocating pixels. Top-down/bottom-up orientation and BGR channels
are preserved. Unsupported BMP variants, appended/truncated data and other media
formats reject. Source byte/pixel/output limits and cooperative cancellation apply.
The existing libpng simplified write API creates a PNG data URL; original BMP
metadata/payload is not copied into the export. No new dependency is introduced.

Per-document cache identity remains actual module plus exact source URI. Statistics
count original BMP bytes and decoded pixels once per unique resource; repeated
occurrences still count toward the occurrence cap. Aggregate failure publishes no
partial HTML or successful-resource metadata. Existing PNG validation/preservation
semantics remain unchanged. Original fixture tests decode resulting pixels and
check orientation, exact/short budgets, malformed inputs, cancellation, cache
identity, mixed keyed export and the Swift/C path. JPEG/GIF/SVG and other BMP
variants remain outside passive export scope.
