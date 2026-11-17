# Carried SWORD 1.9.0 patches

Apply the carried patches to a clean SWORD 1.9.0 release tree before building
the SDK. Use an isolated prefix. The script checks all patches before applying
any of them.

```sh
svn export --ignore-externals https://crosswire.org/svn/sword/tags/sword-1-9-0 sword-1.9.0
dependencies/sword/apply-reviewed-patches.sh sword-1.9.0
cmake -S sword-1.9.0 -B build/sword \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/sword-sdk"
cmake --build build/sword --parallel
cmake --install build/sword
```

Select this SDK's headers and library together when configuring Xiphos, and
set `-DXIPHOS_SWORD_SAFE_BOOK_DELETE=ON` only for an SDK carrying the indexing
fix. Enable and verify CLucene when validating index rebuild and search.
The move API is selected by the SDK headers.

The Windows directory-removal patch uses `_wrmdir` after clearing the entries.
This lets SWORD delete a CLucene index directory during a staged edit.

### Windows ARM64 indexed SDK

The checked toolchain used CLucene 2.3.3.4 from the
[MacPorts distfile mirror](https://distfiles.macports.org/clucene/clucene-core-2.3.3.4.tar.gz)
with SHA-256 `ddfdc433dd8ad31b5c5819cc4404a8d2127472a3b720d3e744e8c51d79732eab`.
Apply [the ARM64 MSVC patch](clucene-msvc-arm64.patch) from the CLucene source
directory. Configure it with an ARM64 developer shell, Ninja, Release,
`CMAKE_POLICY_VERSION_MINIMUM=3.5`, `CMAKE_REQUIRED_LIBRARIES=legacy_stdio_definitions.lib`,
and `CMAKE_CXX_FLAGS=/D_HAS_AUTO_PTR_ETC=1`. Point `CMAKE_PREFIX_PATH` at the
ARM64 zlib SDK. Build `clucene-core` and `clucene-shared`, then install both.

Build SWORD as a Release static library with the `/MD` runtime and explicit
`/O2` in the C and C++ flags, since SWORD's older CMake setup uses `/O3`.
With `SWORD_NO_CURL=Yes`, use `/DSWORD_STATIC_FTPLIB` for both languages and
link `ws2_32` for the bundled FTP code. Include `/D_HAS_AUTO_PTR_ETC=1` in the
SWORD C++ flags. Set both
`CLUCENE_INCLUDE_DIR` and `CLUCENE_LIBRARY_DIR` to the installed CLucene
include directory, and set `CLUCENE_LIBRARY` to the two import libraries.
Use the ARM64 ICU SDK described in the [Windows handoff](../../doc/modernization/handoff-windows.md).
Pass CLucene's import libraries through `XIPHOS_SWORD_EXTRA_LIBRARIES` and
add its DLL directory to the validation process's `PATH`.

See the [index hang report](../../doc/modernization/rawgenbook-index-hang.md)
and [move contract](../../doc/modernization/editing-workflows.md#move-contract)
for scope and verification.

The memory fixes use overlap-safe copying when an attribute key is assigned
from within its own buffer and match array allocation with array deletion
for general-book node data. The Ubuntu sanitizer jobs build this SDK in
an isolated prefix. The release jobs still test the distribution SDK.

For the native Mac app, configure SWORD with `-DSWORD_NO_CURL=Yes`.
Xiphos downloads remote modules through URLSession. The carried patch
omits the legacy FTP fallback on Mac when SWORD is built without curl
and returns an error if a legacy installer transport is unavailable.
