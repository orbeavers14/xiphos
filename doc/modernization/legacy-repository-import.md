# Read-only legacy repository review

2026-11-01 checkpoint, implementation `9a78747`. This begins legacy migration with module source entries;
it is not complete user-profile/settings/session migration.

The retained `src/backend/module_manager.cc` reads/writes SWORD's
InstallMgr/InstallMgr.conf. The selected SDK's InstallSource::getConfEnt() uses
`caption|source|directory|u|p|uid`. The new shared parser characterizes these fields
against the installed SDK; it does not instantiate the retained GTK manager or
call its initialization/reset paths, which can replace configuration files.

`legacy_repositories.hpp/.cpp` accepts an explicit regular-file path or UTF-8
bytes. Maximum input1MiB and256 source rows. It supports the strict single-line
three-to-six-field source format in `[Sources]`, preserves repeated-key order and
source line numbers, and ignores unrelated sections/keys. Unknown syntax is not
rewritten. This does not claim compatibility with every historical INI dialect,
custom port syntax, continuation or escaping extension.

Only credential-free HTTPSSource entries with valid existing HTTPS host/catalog
rules expose candidate fields. HTTP/FTP, local DIRSource, credentials, unsupported
types (including SFTP) and malformed entries receive explicit dispositions with
empty returned fields. Raw rejected strings, credentials and legacy UIDs are not
returned or serialized. No FTP/HTTP endpoint is silently upgraded to HTTPS.

A caller must select a supported entry, supply a new noncolliding identity and
explicitly specify its HTTPS package directory. The old catalog directory does
not establish where packages live. Shared acceptance returns a disabled/uncached
repository value and preserves the loaded profile revision; it does not save or
contact a server. The existing revision-checked store handles explicit saving.
FIFO reads and take-once owned C results preserve thread and lifetime contracts.

Mac Module Library > Import Legacy Sources selects a file and shows the shared
preview. Review Selected HTTPS Source fills a new source editor with an empty
package folder and Enabled off. Save Source requires the package location and an
explicit user action; subsequent enabling and catalog refresh are independent.
The old file remains unchanged, including credentials and unknown sections. No
implicit home-directory discovery, credential migration or background refresh.

## Actual checks and next migration scope

Core tests cover actual SDK field characterization, duplicate INI keys/order,
UTF-8, credentials, insecure/unsupported entries, bounds, invalid paths/directories,
unchanged source bytes, explicit disabled acceptance, duplicate-ID refusal,
no destination/staging creation, FIFO input copying and C result ownership after
worker teardown. Scoped native tests also load the preview, prefill the editor,
require the package directory and save a disabled uncached source through the
shared repository store. Test input uses original prose and example.invalid hosts.
Live isolated Mac UI also checked file selection, safe disposition rows, disabled
review prefill, required package folder and explicit disabled save without network
access. See [the Mac handoff](handoff-macos.md) for final checked results.

The broader [settings/session conversion and exact backup/restore workflow](legacy-settings-session-migration.md)
now exists. Next: representative real-profile/cross-host acceptance and supported
local-source workflows. Do not interpret
this source review as complete profile migration or F27/F37 parity acceptance.
