# Localized reading and parallel/annotation verification

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


Completed scoped batch,2026-11-05. Core implementation e2ef279.

## Implementation

- Additive Reader/worker/C selectors accept a bounded UTF-8 SWORD catalog file.
  A private SWLocale translates book labels; original navigation targets and
  module/global locale remain unchanged. French/German/Spanish catalog sources
  and GPL notices are recorded in localization/sword. The Mac selects a bundled
  catalog at startup. Canonical reference input and persisted references remain
  unchanged; full typed-localized-reference parsing is still open.
- Exact-match gettext import plus reviewed native overrides generates Mac string
  resources. It excludes fuzzy/plural/ambiguous entries.376 literal keys extracted:
  English376, French73, German38, Spanish37; all non-English choices are explicitly
  partial and use fallback. Counts are not complete UI coverage. Python3.8+
  standard library only; two importer tests, regeneration check and plist lint pass.
- Shared actual-module RTL metadata crosses owned C/Swift catalogs into Bible,
  commentary and dictionary WebKit bodies; unknown sources use auto direction.
  Parallel cells preserve individual RTL/LTR direction and gain accessible table
  names/column/row headers. Inline verse spacing uses logical padding.
- Command-Shift-N opens personal notes; Command-Shift-P opens parallel sets.
  Command-S now saves the active personal note rather than the reading session.
  Annotation module/reference/editor fields have explicit accessible names.
- A live ASV/OSHB comparison exposed failed annotation projection for an unavailable
  canon. The worker now skips unresolved annotation targets, preserves valid
  decorated columns and returns incomplete metadata. Invalid profiles and output
  budget failures still fail normally. Original MT-only regression added.

## Checks performed

Mac27.2/AppleClang21/Swift6.4.2. Final full build/suite:
build/localization-reading-completion-{build,tests}.log:258 passes/two expected
index skips out of260,149.12s. Includes complete parallel/annotation model,
recovery/restart and new real WebKit mixed-direction/localized-selector checks.
Focused primary selector/C ownership2/2,2.37s; parallel/annotation/native direction
and prerequisites5/5,9.48s; mixed-canon regression/prerequisite2/2,3.55s.
Final ASan/UBSan focused reader/async/AndBible plus prerequisite4/4,14.66s,
detect_leaks=0; SDK dependencies are not fully instrumented. C11 syntax passes.
Final resource build also includes locale license/credits in the app bundle.
No hosted jobs, external translation provider or paid Graphify calls.

Live disposable-profile checks: French book menus show Genèse/Jean; the note
shortcut opens named editable fields, Unicode input saves with Command-S and
appears under Personal note in reading. Parallel-set creation, ordered ASV/OSHB
choices, saved default and application to Genesis1 work; accessible table exposes
both scripts. Hebrew single-verse view is right aligned. The unavailable-canon
failure was observed live and corrected/verified with the original MT fixture;
no post-fix full live NT comparison acceptance is claimed. While checking exit,
the UI tool switched to a normal-profile window; no edits were made there.

## Remaining scope

Full translated surfaces, plural/formatting review, typed localized reference
input, additional book/preview RTL cases and complete VoiceOver acceptance are
still open. Mac/Windows native screen navigation and stacked verse comparison
are in native-ui-todo.md, along with the new selected-word Hebrew search flow.
These checks are scoped acceptance, not complete platform/feature parity.
