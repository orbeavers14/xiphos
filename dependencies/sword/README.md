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

See the [index hang report](../../doc/modernization/rawgenbook-index-hang.md)
and [move contract](../../doc/modernization/editing-workflows.md#move-contract)
for scope and verification.
