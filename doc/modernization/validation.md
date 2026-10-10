# Initial slice validation

Local checks on 2026-10-10, Apple Silicon, AppleClang 21, installed SWORD
1.9.0.3916 (static SDK):

* Standalone Debug core: engine/C ABI test passed.
* macOS preview: app and Swift adapter compiled; CTest 2/2 passed.
* Address + undefined behavior sanitizer build: core test passed, with leak
  detection disabled because the externally built SWORD library is not
  instrumented and has process-global lifetime state. This is not a leak audit.
* Explicit include/library SDK discovery: core build/test passed (macOS only).
* Generated dictionary fixture: discoverable but rejected as unsupported by
  the verse reader. Empty roots, missing module and empty reference tested.
* Visible app check: selected ParityTest; John 3:16 displayed
  “Original fixture: café & peace.”; changing reference to John 3:17 and
  clicking Read displayed “Second fixture verse.” in WKWebView. Confirmed
  both screenshot and accessibility text. This is not visual parity testing.
* Source inventory: 387 UI labels and 101 manual section titles extracted.

The local SDK requires macOS 26.0, so the preview was tested with
`CMAKE_OSX_DEPLOYMENT_TARGET=26.0` and explicit zlib/bzip2/lzma libraries.
The project defaults to 13.0; compatibility on that OS requires an appropriate
SDK and separate runtime verification.

GTK3/WebKitGTK are absent locally. The legacy desktop application was not
built or launched on this Mac. Its existing Linux/Windows workflow remains
in place; new standalone core/macOS CI is additive. CI results must be read
before merging. Native WinUI 3 and GTK4 builds, broad module corpus tests,
full screenshots/workflows, persistence and packaging remain unvalidated.
