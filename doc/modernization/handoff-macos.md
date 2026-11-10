# Mac development handoff

Updated 2026-11-09. Start with [shared instructions](handoff.md) and
[Mac build/setup](../../platforms/macos/README.md).

## Checked environment and behavior

The preview now targets macOS 27.0 or later. The current development host runs
macOS 27.2 with AppleClang 21 and Swift 6.4.2.
See [packaging audit](native-runtime-packaging.md).

Previous native suite:258 passes/2 expected indexing skips out of260,149.12s;
focused sanitizer4/4,14.66s. Exact logs, live checks, provenance and scope:
[localized reading](localized-reading-verification.md), [Settings/tabs](native-settings-workflows.md),
[real corpus](real-corpus-validation.md), [asset migration](legacy-assets-migration.md).
The native Settings screen is functional intermediate UI. Partial languages and
private SWORD book-label catalogs do not imply full translation or localized typing.

## macOS deployment baseline

On `master`, the deployment-floor change following `8ce6f7e` covers the
preview build, its Swift tests, the bundle plist and disposable fixture launch.
Graphify's scoped structural scan covered 560 Mac and core files. The audit graph
shows the plist and Mach-O metadata checks, and the Swift adapter graph shows the
shared session boundary. CMake inspection confirmed that the same deployment
target feeds native compilation, Swift compilation and the bundle plist.

Local Release build on arm64 macOS 27.2 used CMake 4.4.5, Xcode SDK 27.2,
MacPorts SWORD 1.9.0 and curl 8.23.0, Homebrew GLib 2.90.1 and a workspace-built
libarchive 3.8.6. The local libarchive dylib needed an absolute install name
because the Swift link did not supply its `@rpath`. CMake rejected 13.0, 26.6 and
an empty target. The default plist and Mach-O both declared 27.0, and a separate
27.2 configure propagated the override to the plist. The fixture app started
and initialized its disposable profile. The development bundle passed strict
ad-hoc signature verification and all 29 Mach-O image checks at a declared and
observed floor of 27.0. CTest passed 259 of 260 checks in 66.10 seconds,
with the optional BibleSync network pair skipped and zero failures.

## Compatibility cleanup

The Mac preview requires CMake 3.20 because its fixture targets use
`ctest --test-dir`. Six reader and settings callbacks use current SwiftUI
`onChange` overloads. A local rebuild with CMake 4.4.5 and SDK 27.2 produced no
`onChange(of:perform:)` warnings. The app still declares macOS 27.0. CTest passed
259 checks with zero failures and one optional BibleSync network skip.
See the [Linux handoff](handoff-linux.md#maintenance-baseline-on-master) for the
shared toolchain validation that remains.

## Shared core maintenance

After the Fedora integration, shared core CMake now discovers pkg-config once
for SWORD and GLib and retains explicit SDK discovery. On macOS 27.2 with
CMake 4.4.5, the standalone Release suite had 184 passes and one optional
BibleSync skip after the local HTTPS fixture was rerun with loopback access.
Root, Mac preview and Linux portable-model configurations passed on the Mac,
as did standalone core configuration with pkg-config disabled and explicit SDK
paths. Native Windows validation remains open before removing the shared
libarchive compatibility shim.

## Selected-word study and stacked comparisons

The Mac reader and parallel panes send one selected token through the existing
shared search modes. Explicit Greek/Hebrew Strong’s links attached to the
selected rendered word block provide optional lemma terms through the C ABI.
Surface-form search retains the selected Unicode and source module/reference.
The parallel pane stacks one aligned core row at a time and retains the chapter
matrix. Verse search results can open the startup comparison set directly.

On macOS 27.2 the native preview built and all five focused CTest checks passed:
mixed-canon alignment, RTL WebKit presentation, rich module fixture,
parallel/selected-word reader flow and shared attribute search. The WebKit
test uses generated markup. Broader real-module lexical coverage, keyboard and
VoiceOver checks, and native Linux/Windows presentation remain open.

## macOS 26.7 candidate

A separate ARM64 development bundle from `5567843` passed the Mach-O closure
audit at 26.7 and strict ad-hoc signature verification. The core was built with
the macOS 26.5 SDK, and the final app link records SDK 27.2 and minimum OS 26.7.
The candidate used checksum-verified Homebrew macOS 26 builds of SWORD 1.9.0,
GLib 2.90.1, gettext 1.0, PCRE2 10.49 and libpng 1.6.59, plus libarchive 3.8.6
built at 26.7 against Apple system libraries and the Apple SDK's curl.
All six non-system libraries were bundled and the strict audit found no issues
across seven Mach-O images.

On macOS 27.2 the bundle launched with a generated module fixture and disposable
profile. Eight focused tests passed, covering core reading, mixed-canon alignment,
RTL WebKit, selected-word search, stacked comparison, attribute search and local
and HTTPS module installation. The Homebrew SWORD build omits ICU and CLucene.
An actual 26.7 launch and workflow smoke remain required before changing the
main preview deployment floor of 27.0.

## Real word-study corpus and search-to-comparison

The Mac selection menu now reads word metadata through the shared worker and
C ABI. The comparison action passes the actual hit module for SDK versification
mapping. The [real corpus check](real-corpus-validation.md#selected-word-metadata-and-comparison)
passed on macOS 27.2 with MacPorts SWORD 1.9.0 linked to ICU 78.3 and CLucene
2.3.3.4. The separate required-index run passed with a real generated index.
Five focused core checks passed, including C ownership and native session
serialization. The Mac preview built and four focused native/fixture checks
passed, including mapped comparison restoration. These checks used disposable
module data and profiles.

## Surface search modes

The Mac selected-word menu offers literal Unicode surface search and a mode
that ignores Greek, Hebrew and Syriac combining marks. Both use the shared
verse search pipeline and preserve existing phrase and lemma modes. The
disposable real corpus passed on macOS 27.2 with the ICU/CLucene SWORD build.
The Mac preview and focused reader workflow built and passed locally.

## Current next steps

- Follow the [native interface list](native-ui-todo.md): coherent Settings navigation,
  appearance/themes, stacked comparisons and selected-word Hebrew, Greek and Aramaic study.
- Complete broader workflow/VoiceOver/RTL/localization and compatibility acceptance.
- Validate execution on macOS 27.0 and release resources/signing before distribution.
- Evaluate macOS 26.7 as a lower floor after rebuilding or replacing the
  dependencies that require 27.0. Audit the full bundle and launch it on 26.7.

Use disposable profiles/module fixtures for GUI checks. Give each test bundle a
unique identifier and verify the target process: copies sharing a bundle identifier
can cause UI tools to select/reopen the wrong profile. Do not edit a normal user
profile while verifying a disposable one.

[Historical Mac evidence](history/handoff-macos.md) retains earlier logs and
implementation commits; old GTK preservation statements and completion estimates
are superseded by current guidance. Do not start new feature work without a task.

## Documentation refresh

2026-11-05: README and primary status/architecture/roadmap/handoffs now summarize
current native development. Detailed old evidence moved to history with relative
links/anchors preserved. All38 parity requirements remain; affected Mac evidence
is updated. A documentation index, preview guide and explicit historical-help
boundary replace the old GTK release framing.95 Markdown files pass local-target
and heading-anchor checks; no application code changed and no new runtime/test
acceptance is claimed. GitHub About is aligned with the native development fork.

The [carried SWORD SDK procedure](../../dependencies/sword/README.md) supports
staged general-book moves and opt-in leaf deletion. On macOS 27.2 the patched
SWORD 1.9.0/CLucene build passed the focused move, deletion and rebuilt-index
checks. Keep deletion disabled for stock SWORD.

Patched-core verification on macOS 27.2 passed 184 checks with one optional
BibleSync skip. Focused fault injection restored the original book bytes,
index, and section offset after failed move and deletion refreshes. The
patched preview linked libjpeg, passed strict ad-hoc signature and runtime
closure checks, and smoke-launched with a disposable profile. Its loader
used the bundled patched SWORD. This development bundle contains 28 SDK
libraries and has a measured 27.2 floor. It does not establish acceptance
on macOS 27.0 or redistribution readiness.

The Mac module picker resolves a Bible translation switch from the committed
verse through the core mapper. An unavailable mapping restores the picker and
preserves the current passage, draft, and history. The module sidebar offers
new general books and journal/prayer organizers. The book pane offers section
rename, move and confirmed deletion. Move and deletion require the carried
SWORD SDK. Search HTML has separate opt-ins for JPEG and scoped module styles.
Stacked and chapter comparisons label unavailable and differently numbered
verses. The selected-word menu offers surface search and resolved lemma choices.

The native development package was audited on macOS 27.2. Physical macOS 27.0
execution and a manual keyboard/VoiceOver pass remain separate acceptance gates.

The Settings sidebar now includes General, Appearance, Reading, Lexicons and
BibleSync. Profile-specific System, Light and Dark settings apply immediately.
On macOS 27.2, the native preference smoke verified persistence and profile
isolation, and the WebKit smoke verified live appearance changes in a mixed
RTL/LTR comparison. Manual keyboard and VoiceOver checks remain deferred.
