# Native Mac runtime audit and development bundling

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


These explicit tools inspect the new application, not the retained GTK3 package.
They do not run automatically with ordinary builds or dispatch hosted jobs.

`platforms/macos/runtime-audit.py` walks the Mach-O dependency closure with Apple
`otool`/`lipo`, resolves loader/executable/rpath references, checks architectures
and minimum-OS load commands, and reports JSON. It refuses unresolved dependencies,
external SDK linkage, unknown floors and non-macOS binaries. System libraries may
live only in Apple's dyld cache. The explicit `macos_runtime_audit` CMake target
runs the same audit after building the native preview when Python is available.
JSON includes local SDK paths: retain reports privately under ignored build/.

```sh
python3 -B platforms/macos/runtime-audit.py build/macos/XiphosNativePreview.app
cmake --build build/macos --target macos_runtime_audit
```

Exit0 means the audited Mach-O closure is self-contained and consistent with its
declared floor. Exit1 reports readiness issues; exit2 is an inspection failure.
This is not proof of notices/source/relink material, runtime-loaded plugins,
TLS trust/locale resources, signing/notarization, or execution on an older OS.
Static SDK provenance and API availability still require their own validation.

The inspected development app declares26.0 but16 of its17 external libraries
require27.0. Those current bottles do not establish macOS26 or the project's
requested13.0 floor. The tools report this instead of silently accepting it.
This is historical evidence from before the project raised the preview floor to 27.0.

`bundle-runtime.py` produces a NEW development-only app. It copies the selected
app and its non-Apple dependencies into private staging, uses short
content-hash filenames to avoid SDK basename collisions/header-size problems,
rewrites references and IDs, applies local ad-hoc signatures, verifies signatures
and re-audits the complete closure before publication. Original app/SDK binaries
and existing destinations are not changed. Context-dependent resolutions,
external source-bundle symlinks, missing architectures/dependencies and excessive
closure sizes reject. The destination must have an existing parent and be separate
from the source. Bounds are256 images/1GiB; audit traversal is also bounded.

```sh
python3 -B platforms/macos/bundle-runtime.py \
  build/macos/XiphosNativePreview.app build/RuntimeClosureCheck.app \
  --development-only
python3 -B platforms/macos/runtime-audit.py build/RuntimeClosureCheck.app
```

The bundler rejects a dependency that requires a higher OS than the source app
declares. For a development-only experiment, `--allow-development-os-floor`
allows the new artifact to declare the observed floor.
`Contents/Resources/DevelopmentRuntime.json` marks the artifact development-only
and not redistribution-ready, with remaining release gates. No module content is
bundled. It is not a signed/notarized release or an older-macOS compatibility claim.

An inspected local artifact passes strict signatures and all18 Mach-O image
checks. A private-profile live launch discovers the six test modules; dyld logs
show17 bundled SDK loads and zero Homebrew/MacPorts/usr-local library loads.
Existing-destination and unacknowledged-floor refusal checks pass. Final artifact
and complete regression evidence are recorded in [the Mac handoff](handoff-macos.md).
Native Windows/Linux packaging and execution on macOS 27.0 remain open.
