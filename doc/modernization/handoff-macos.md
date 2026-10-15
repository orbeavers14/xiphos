# macOS chat handoff

Updated 2026-10-15. Read [AGENTS.md](../../AGENTS.md) and the
[shared handoff](handoff.md) first.

## Active shared-core task

The macOS chat is continuing shared lexical-rendering corpus/fallback and internal
modernization on `master`, starting from `74a4190`. Scope: `word_blocks` helper,
its tests and associated evidence. No public API or frontend behavior change is
planned for the first checkpoint. Other platform chats should avoid overlapping
edits to these files; platform-local acceptance work can continue independently.
Owner request: continue shared-core work using local checks until usage is exhausted.

Completed/pushed implementation checkpoint: `f2a2c61`, 2026-10-15. Owned lexical
layout now accepts opaque SWORD note markers, preserves their links/labels, checks
OSIS/ThML paragraph and multi-anchor data, and guards uppercase lexical anchors
before unsafe inherited searches. 1,502 legacy-output comparisons; 65/65 macOS
suite, focused ASan/UBSan 1/1 and TSan 1/1, strict C++11 and native-disabled helper
builds passed. No public API/frontend adoption changed.

Completed/pushed follow-up: `f9a8e0e`, 2026-10-15. Checked owned-string annotation
edits retain NASB/class-shim/spacing/morphology behavior over 1,530 oracle cases.
65/65 local suite; focused ASan/UBSan and TSan each 1/1; strict C++11 and legacy
helper builds pass. Current follow-up moves scanner temporaries/recursion to
owned C++ strings and adds structured mutation/style-boundary coverage.

## Existing evidence and boundaries

At `9e288b3`, the macOS preview/shared-core suite had 65/65 passing local checks.
Scoped address/undefined and thread sanitizer checks, C++11 helper builds and
disposable fixture GUI checks are recorded in [status](status.md). These are
historical evidence, not a claim that a fresh checkout was retested today.
AppleClang 21 / Swift 6 and local SWORD 1.9.0.3916 / GLib 2.86.5 were used.
Those local dependency builds require macOS 26, so they do not validate the
requested macOS 13 deployment minimum. No full native release is accepted.

The SwiftUI/AppKit preview already adapts core reading, paired commentary,
histories, options, dictionary and note/reference/hover behavior. Consult the
platform README and tests before rebuilding any of this independently.

## Next bounded work

1. Expand the owned lexical block renderer's real SDK/lemma/note/paragraph corpus
   and fallback evidence; see [word block workflows](word-block-workflows.md).
   The helper exists but is not yet adopted by native snapshots or the old parser.
2. After the supported grammar and fallback policy are checked, integrate owned
   styles/rendering into heading/body snapshots with controls and visual evidence.
   Keep platform-independent behavior in C++, not Swift or a separate SwordKit engine.
3. Preserve main/commentary/dictionary pane composition, draft input, histories,
   unknown option eligibility, busy/closed/stale-result guards and window isolation.
4. Review Linux/Windows requests for shared API adjustments before changing those
   same contracts. Larger search/storage/editor work follows scoped parity evidence.

## Local validation

Follow [the macOS build guide](../../platforms/macos/README.md) for SDK discovery
and deployment settings. From the repository root, with configured dependencies:

```sh
cmake -S platforms/macos -B build/macos
cmake --build build/macos --parallel 4
ctest --test-dir build/macos --output-on-failure
```

Use focused checks while iterating and the appropriate regression suite at a
milestone; do not repeat full builds without a reason. For GUI work, use the
documented disposable fixture launcher outside Documents, inspect the actual
interactions and quit the app when done. Do not weaken macOS privacy protections.
Run sanitizer checks relevant to ownership/concurrency changes; report external
SDK instrumentation limits. Do not claim Linux/MSVC acceptance from these checks.

## Next handoff entry

Replace this section with the [checkpoint format](handoff.md#evidence-to-record-at-each-checkpoint)
when work begins/finishes. Include the tested implementation commit, actual results,
remaining gaps and shared API impact. Commit and push it so other OS chats can read it.
