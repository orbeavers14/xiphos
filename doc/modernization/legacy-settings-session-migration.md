# Legacy settings and session migration

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


Checked workflow, 2026-11-01, implementation `4bfb38c`. The shared core now connects the existing legacy
readers to a reviewable, reversible native-session conversion. The original GTK3
app and original user files are not modified. This covers selected settings,
saved-tab XML and module-option files, not every file in an old user directory.

## Explicit inputs and conversion

`legacy_migration.hpp/.cpp` accepts an explicitly selected Xiphos settings XML,
optional Xiphos_Tabs XML and optional modops.conf. No home-directory discovery,
module download, locale change, sync opt-in or installed-library change occurs.
Reads use bounded existing regular files. Source snapshots are copied once;
module INI decoding uses the same SWORD decoder on a private temporary copy of
those bytes. It does not reread a moving original during decoding.

The first-effective legacy XML values and existing profile parsers provide:

- Five lexicon choices and the cross-reference preview preference.
- Exact saved-tab order, commentary/parallel intent and four pane flags; absent
  showtexts inherits the four global flags together. Exact lowercase yes applies
  to legacy per-tab flags. The first tab is active, matching the retained loader.
- Current main/commentary, dictionary/key and resolved book selections. Core
  session services normalize/validate references and book offsets/paths, and
  establish the initial histories. No SDK pointer or rendered HTML is persisted.
- Per-module engine options and presentation preferences through the existing
  decoder, including dormant engine entries. Installed module type determines
  presentation classification. Reopening a package replans against the current
  SDK/library, including modules installed since the first preview.
- Effective startup parallel ordering, explicit legacy parallel options/defaults,
  headings and canonical resolved control/reference when a tab requests parallel.

Omitting a tabs file creates one candidate from the saved global selections.
An explicitly selected zero-tab file remains zero tabs. No arbitrary installed
Bible is substituted for missing settings. Required absent modules, incompatible
types, invalid selections or unrenderable parallel intent produce per-tab issues
and block the entire native session. Explicit old-ID to installed-ID mappings may
resolve them; colliding module-preference mappings reject instead of overwriting.
Preview stages temporary sessions on the existing FIFO owner and leaves all live
tabs/history unchanged. Native export also revalidates the complete saved state.

## Preserved data and format

A migration package contains exact original bytes, per-source SHA256 revisions,
conversion issues and an optional complete versioned native snapshot. Selected
machine paths are deliberately not serialized. Comments, unknown XML nodes and
attributes, duplicate/shadowed settings, named parallel/search collections,
locale, fonts, colors, layout, special-module preferences and unknown INI options
remain recoverable in the originals. Supported values are converted; unsupported
values are disclosed as preserved rather than claimed as applied. This is not
implementation of missing appearance, collection-management or devotional UI.
The package can be saved even if conversion is blocked by missing content.

The version-1 package has a checksummed length-framed binary payload. Each source
is bounded to16MiB; the whole package to80MiB, XML setting inventory to16,384 rows
and conversion issues to1,024. Existing tab/native limits still apply (256 tabs,
bounded module/preferences/text collections). XML uses the existing secure parser:
DTD/external resources, malformed/hidden-NUL inputs and wrong roots reject;
XML-declared/detected encodings such as UTF-16 retain exact original bytes while
extracted values are UTF-8. INI NUL bytes reject; non-UTF-8 comments remain original
bytes, while session module identities must satisfy native text validation.
Unknown package versions, checksum failures, invalid/truncated lengths and trailing
payloads reject. SHA256 detects corruption; it is not a signature or encryption.

## Saving, restoration and native use

Package and native-session output filenames must be new. The core verifies
staged bytes and publishes new files atomically without replacing a destination
created during staging. A live preview rechecks every selected source revision
before output. A changed file requires a new preview. A loaded package is
independent of old machine paths and replans from its own original copies.

Restore Original Copies writes settings.xml, tabs.xml and modops.conf into a
private sibling directory, verifies their exact bytes, then publishes the
complete folder. A competing destination causes the operation to fail. Staging
is cleaned up on failure. These checks protect publication from overwriting
another writer's destination. They provide no power-loss guarantee.

Mac File > Import Legacy Settings and Session provides explicit file selection,
package reopening, optional module mappings, a source/converted-window summary,
key-name inventory and blocking issues. Raw setting values are not displayed.
Changing selections invalidates the preview. Save Backup Package, Export Native
Session and Restore Original Copies use the same shared service through owned C
handles. Immutable copied results may cross the Swift actor boundary; no SDK
handle or second persistence engine does so.

Use exported session on next launch is optional and starts off. After successful
export, the host saves the current reading session separately before changing the
startup selection. Current windows/editors stay active until normal quit/restart;
normal recovery/dirty-editor confirmation still applies. A failure after export
reports that the file exists but startup selection was not changed. The current
reading-session path is protected even if it has not yet been created. A developer
session-path override requires explicit selection outside that override.

## Evidence and remaining acceptance

Original-prose generated module/profile fixtures cover full two-tab conversion,
multiple tabs sections, inherited/explicit pane flags, initial histories,
dictionary/book/parallel state, module settings, unknown/duplicate fields, exact
backup restoration, changed sources, existing destinations, missing modules and
explicit mappings, empty versus omitted tabs, UTF-16 little/big-endian and declared Latin-1 XML, Latin-1 INI comments,
sparse duplicate-section lookup with first-empty-value precedence,
large unknown values, DTD/overflow rejection, C admission copying and ownership after worker teardown.
Fresh core and native processes restore converted sessions. Native checks cover
preview invalidation, protected current-session paths, explicit activation after
export and package reopening without consulting changed originals. Actual final
build/test and live-UI results are recorded in [the Mac handoff](handoff-macos.md).

Physical Fedora/MSVC/native Windows/Linux UI, representative real-user profiles,
full display/visual parity, platform geometry translation and representative
whole-directory workflows remain acceptance work. The shared
[complete-folder asset copy](legacy-assets-migration.md) now preserves a selected
legacy tree; external references require separate explicit selection. No whole F05/F29/F37
parity group is accepted by this scoped settings/session workflow.
