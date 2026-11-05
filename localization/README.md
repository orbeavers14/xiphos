# Localization

Translation sources for the new application live here. Platform builds copy or
compile these sources into the locations their native frameworks require; the
source layout does not dictate the installed bundle layout.

| Directory | Purpose | Current integration |
|---|---|---|
| gettext | Preserved .po catalogs, translator credits and legacy extraction metadata | Input for reuse/migration; not yet automatically converted into new native UI strings |
| macos | Native .lproj/Localizable.strings resources | Copied into Contents/Resources by the Mac build; English plus partial French |
| windows | Future Windows .resw resource guidance | No Windows translations/frontend integration claimed yet |

Keep language codes, translator attribution and required notices. The gettext
POTFILES lists and old source-location comments describe retired GTK sources;
they are historical metadata, not an active extraction recipe for the new app.
Old translator instructions under doc are marked historical.

For current Mac strings, edit macos/<language>.lproj/Localizable.strings. Keep
English fallback and valid format placeholders. A new language must be added to
the app's supported-language choices only after its resources are available and
checked. Reconfigure/build the Mac target and verify the bundle's copied files.

Next: define stable shared message identifiers and a reproducible import/export
pipeline so reusable gettext translations feed platform resources without manual
duplication. Moving directories alone does not migrate old translations or
translate scripture references. Linux UI localization and broader translation/
RTL acceptance remain open. Do not hand-maintain duplicate translation sources
for the same new message once that pipeline exists.
