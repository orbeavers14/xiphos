# Shared core: initial reader slice

Build/test independently of the GTK application:

```sh
cmake -S src/core -B build/core -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core --parallel
ctest --test-dir build/core --output-on-failure
```

Requires CMake >=3.16, C++17 and SWORD >=1.8.1. The legacy rendering seam uses
C++11. pkg-config discovery is preferred; SDKs can instead supply
`SWORD_INCLUDE_DIR` and `SWORD_LIBRARY`, with
`XIPHOS_SWORD_EXTRA_LIBRARIES` for static SDK transitive libraries. The fallback
SDK version and architecture must be checked by the SDK provider. Windows
SDK/MSVC linking has not yet been tested.

Tests generate a small on-disk RawText OSIS module with original prose.
They validate discovery isolation, UTF-8 XHTML, legacy setKey/renderText
comparison, C header consumption, handle/result ownership, missing modules,
empty libraries and invalid arguments. Fixtures remain in the build directory
for the Swift smoke test and manual preview; no live user data is modified.

The default legacy application still builds normally; its get_render_text
calls the same rendering function. The standalone core build avoids all GTK,
WebKit, help and packaging dependencies. There is no search/history/storage
implementation here yet. The C ABI is experimental and documented in c_api.h.
Follow the [architecture](../../doc/modernization/architecture.md) and
[parity ledger](../../doc/modernization/parity-matrix.md) before expanding it.
