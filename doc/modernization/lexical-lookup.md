# Lexical lookup seams

Inspected 2026-10-11 against fork source at
51e01935fc18211cda8199e4fc2c03f0b17c22ca. This is a source inventory and
implementation contract; source observations are not executable parity proof.

## Commands and shared parsing

`lexical_link` accepts local `passagestudy.jsp` / `xiphos.url` and the native
preview's fixed internal origin. It owns kind, source module, type, raw value
and optional passage. Parameters decode individually, preserving encoded plus,
ampersand and Unicode. Strong's requires a value; morphology requires type and
value. An empty source module can remain empty because Strong's selection need
not use a Bible source. Explicit module overrides the fallback source.

No key padding, uppercasing, namespace-to-dictionary mapping or selection occurs
in this parser. Custom lemma namespaces remain data. Unknown actions, combined
showStrongsMorph, external origins, duplicate/unknown parameters, malformed
escapes, fragments and control characters reject. Existing contextual native
dispatch is unchanged; lexical commands are not yet actionable in the preview.

## Existing selection differences

| Trigger | Main reader (`src/main/url.cc`) | Detached dialog (`src/main/module_dialogs.cc`) |
|---|---|---|
| Strong's exact Greek | settings.lex_greek | settings.lex_greek |
| Strong's exact Hebrew | settings.lex_hebrew | settings.lex_hebrew |
| Strong's other nonempty type | settings.lex_hebrew | no selected module |
| Strong's empty type | InvStrongsRealGreek | no selected module; direct helper assumes nonnull type |
| Greek morphology recognition | exact Greek or substring x-Robinson, robinson, packard, Robinson | exact Greek or substring x-Robinson, robinson, packard |
| Greek morphology context | source module with settings.currentverse | dialog module with dialog key |
| Greek morphology NT | configured installed NT lexicon; otherwise Robinson name even if absent | configured installed NT lexicon; otherwise installed Robinson; otherwise no result |
| Greek morphology OT/non-NT | configured installed OT lexicon; otherwise Packard name even if absent | configured installed OT lexicon; otherwise installed Packard; otherwise no result |
| Hebrew morphology | substring oshm, requires OSHM installed, then uses configured Hebrew name without checking that name | exact Hebrew or substring x-HebMorph, requires configured Hebrew name installed |
| Unrecognized morphology | no module; helper still calls display/render paths | returns 0 before display/render |

Main morphology's Hebrew check is a separate `if`, so a type containing both a
Greek-recognized substring and oshm can override Greek selection when OSHM is
installed. Dialog Hebrew selection is an `else if`. Matching is case-sensitive.
Do not merge these paths into an undocumented heuristic. First extract helpers
with an explicit main/dialog context and compare them against frozen functions,
including absent modules, overlapping tags, empty settings and both testaments.
Any deliberate behavior correction needs separate evidence and documentation.

## Preferences and presentation

`src/main/settings.c` lexicons defaults: Greek StrongsGreek, Hebrew
StrongsHebrew, default dictionary WebstersDict; missing/empty Greek morphology
NT Robinson, OT Packard and Hebrew OSHM. Greek/Hebrew Strong's defaults only
replace null values in this source, unlike morphology's empty-string handling.
`src/gtk/preferences_dialog.c` filters choices by Feature GreekDef, HebrewDef,
GreekParse and HebrewParse; default dictionaries use a separate image-map filter.
Persisting these lexicon preferences is outside the current 18 engine fields.

Main clicks call `main_display_dictionary` (`src/main/sword.cc`), which checks
availability, switches the dictionary pane, uppercases the key, updates its own
dictionary history and handles daily devotionals separately. Hover renders the
raw key in the information preview. Dialog clicks use the information dialog;
hovers use the dialog preview. Therefore the new no-history dictionary snapshot
service is a rendering seam, not the complete click/browse workflow. Bible
history and dictionary history are distinct contracts.

The existing `BackEnd::get_key_testament` mutates the source VerseKey and casts
without verifying its type. The extracted service should validate verse-based
source context and work on a cloned key without silently replacing the user's
committed location with unfinished input. Characterize canonical/versification
behavior before claiming equivalent selection.

## Next implementation and acceptance gates

1. Characterize actual main/dialog selection branches with installed-module
   stubs, frozen originals and source-context cases; include custom lemma tags.
2. Extract shared selection/preferences with explicit origin and typed failures.
   Keep rendering through the existing worker and SWLD service.
3. Add C/Swift bindings and native dictionary/preview presentation. Preserve
   source context, clicked versus hovered behavior, independent dictionary
   history, selected module, navigation, labels, pinning and keyboard behavior.
4. Validate representative Greek/Hebrew lexicons, morphology modules and custom
   namespaces against the legacy app, then repeat on GTK4 and WinUI 3.

No F16 acceptance or legacy-file retirement is justified by this parser.


## Executable characterization added 2026-10-11

`core_lexical_selection` compiles selection prefixes extracted from the actual
main/dialog source without GTK display code and compares them against frozen
prefixes from 96068973. It covers 11 type tags, 2 testaments, all 64 combinations
of six installed lexicon names, configured versus null morphology settings and
four handlers: 11,264 comparisons plus explicit missing-fallback/context and
custom-lemma differences. It records source module/key calls. C++11 strict
compilation and the 24-test local suite pass. This validates selection prefixes,
not GUI effects, dictionary rendering, null dialog types or full lexical parity.
The extraction guards fail configuration if required source markers disappear.
No production selection logic has been changed yet.

The selection characterization also passes address/undefined instrumentation
on its GTK-free harness. A negative-control build deliberately replaced the
main NT fallback name in an ignored temporary copy; the comparison failed at
the expected morphology divergence. Production/generated source was not
modified by that control. This confirms the harness detects a real selection
regression; it does not extend GUI/module-corpus acceptance.
