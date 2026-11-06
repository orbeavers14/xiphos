# Localization

Translation sources for the native application live here. Native builds copy
or compile resources into the layout required by their platform frameworks.

| Directory | Purpose |
|---|---|
| gettext | Preserved .po catalogs and original translator credits; exact-message import source |
| overrides | Reviewed translations for new native wording not covered by old catalogs |
| macos | Generated .lproj/Localizable.strings resources; English plus partial French/German/Spanish |
| sword | GPL SWORD scripture-name catalogs and original credits/license |
| tools | Deterministic native catalog generator, validation and importer tests |
| windows | Future Windows .resw resource guidance |

Run from the repository root:

```sh
python3 localization/tools/build-native-catalogs.py
python3 localization/tools/build-native-catalogs.py --check
python3 localization/tools/test_native_catalogs.py
```

Python3.8+ standard library only; no translation service or paid AI calls.
The generator extracts literal native UI keys and imports exact, unambiguous,
non-fuzzy, singular translations from gettext catalogs, then applies reviewed
JSON overrides. It deliberately skips fuzzy/plural/ambiguous entries. Runtime
messages assembled dynamically and formatting/plural workflows need further
explicit localization; extraction counts are not whole-interface coverage.
coverage.json records the current literal-key counts. Do not edit generated
macos resources by hand: update gettext or overrides, regenerate, reconfigure/
build, lint the resources and verify the bundle. No automatic language creation
or full-translation claim follows from this import. Language selection lists
only currently bundled languages and explicitly marks partial translations.

Gettext POTFILES lists and old source comments refer to retired GTK sources;
they are historical metadata, not the new application's extraction recipe.
Keep all translator credits and required notices. Next steps include stable
message identifiers, complete translated surfaces, plurals/formatting review,
and Windows/Linux adaptation. The existing import is exact-match reuse, not a
complete gettext-to-native migration.

Private SWORD catalogs localize book-selector labels without changing canonical
navigation targets, saved sessions or the SDK's global locale. Typed localized
reference parsing remains separate; canonical reference input stays supported.

Official upstream905762a (2026-11-05) is integrated with its original Chinese
UI/help translations and related corrections. Its PO update lives at
gettext/zh_CN.po after the directory move. Native Chinese activation and broader
message coverage remain separate work; retaining a catalog does not make every
new native screen translated.
