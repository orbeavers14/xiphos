# Shared static PNG export preparation

Updated 2026-10-20. This checked private C++ service prepares an owned PNG data
URL. It is not yet image support in HTML exports: those still reject resources
until the complete-document policy consumes this service.

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

Native core builds require libpng >=1.6 with simplified reading and zlib, using
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
