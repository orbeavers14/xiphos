# Shared HTTPS transfer and repository extraction

> Service contract and scoped evidence. Use [current status](status.md) and the
> [roadmap](shared-core-roadmap.md) for present priorities. Retired GTK paths and
> dated “pending/next” statements below describe historical baselines/checkpoints.


## Service boundary (2026-10-31, af9a859)

The shared C++/FIFO/C service fetches SWORD catalog archives and installs selected
module ZIP packages through the existing fingerprint-checked local transaction.
No GTK widget, global application settings object, second SDK owner or platform
installer engine participates. Network and archive code itself uses no SWORD
manager; SDK metadata validation and install/reload run on the existing FIFO owner.
A long transfer occupies that queue; native callers remain asynchronous and can
poll/cancel through independent atomic controls. Broader download scheduling is
not introduced here.

`fetch_remote_catalog` returns copied metadata, its revision, an informational
archive SHA256 and the observed destination fingerprint. It uses an existing
explicit workspace outside the live module library; temporary data is private and
removed after success, failure or cancellation. Catalog snapshots are owned in
memory, not a new persistent repository/cache database.

`install_remote_module` downloads the selected ZIP, extracts it, checks exactly one
module against the selected catalog's advertised shared metadata/version, prepares
local installation, and rejects a changed destination before commit. The installed
configuration's unknown fields/notices and module data are preserved by the local
transaction. Source/destination checks, rollback, fresh SDK/session/book caches and
search invalidation reuse that service. Successful results include a copied
catalog with the new destination fingerprint, permitting another selection from
the same offer without an unnecessary catalog download.

HTTPS authenticates the selected server, not a publisher license or signed module
artifact. The archive digest is informational; SWORD repository metadata does not
supply a universal trusted package signature. This is the SWORD repository adapter;
licensed non-SWORD provider entitlements remain separate from transfer capability.

## Network and cancellation contracts

Remote access is explicit/acknowledged in request options. libcurl enforces peer
and hostname certificate checks, TLS1.2 or newer, HTTPS-only initial/redirect
protocols, at most5 redirects, no URL userinfo, and a complete HTTP200 response.
There is no insecure-cert switch, FTP fallback, auto retry, resume or partial206
installation. A caller-selected CA bundle is optional; absence uses transport
system trust. No passwords, API keys or cookie/session store are added.

Actual received body bytes, declared size, cumulative response headers (64KiB),
connection time (15s) and total transfer time (1..300s) are bounded. Default body
limit128MiB, absolute cap512MiB; catalog jobs additionally cap compressed size8MiB,
expanded tar size32MiB and entries4096. Module defaults expanded512MiB and16384
entries, including implicit directory paths; callers may lower limits. OS/network
stalls and parser-internal allocation are not claimed to be preemptively bounded.

One atomic gate arbitrates cancellation against commit. A cancellation that wins
prevents installed-library changes and returns `XC_CANCELLED`; once the local
transaction begins its commit, cancellation returns false and completion/failure
wins. Cancel remains cooperative during IO/SDK work. Progress is copied from
atomics: phase, received/declared bytes, expanded payload bytes and entries.
Accepted jobs drain on worker shutdown.

C observer, result and control envelopes allocate before acceptance. The control
can be taken immediately while a request is pending and survives request/worker
teardown. Committed result polling moves preallocated owned values without
allocation. Results take once; nested catalog/local-result handles and strings
borrow from the parent and must not be separately released. C inputs and selected
catalog data copy before return, including when a borrowed next-catalog handle is
submitted and its original result released immediately afterward.

## Extraction policy

libarchive reads ZIP and plain tar only; zlib first expands catalog gzip into a
bounded private tar file, including gzip footer/trailing-stream checks. ZIP EOCD/
central-directory preflight bounds entry counts and declared expansion before
parser allocation. ZIP64, split/encrypted ZIPs, unsupported compression, corrupt
CRC/header data and truncated archives reject. This first slice is below4GiB and
does not claim generic ZIP compatibility.

A scoped thread-local UTF-8 locale supports valid Unicode archive names even
when the process starts in the default C locale; global application language
settings are unchanged. The extractor writes regular files/directories itself. It does not use bulk
archive-to-disk restoration or preserve archive owners, executable permissions,
ACLs or xattrs. Links, devices, FIFOs and sparse entries reject. Paths must be
bounded UTF-8, relative, at most4096 bytes and64 components, with no traversal,
backslashes, drive/stream colons, control bytes, trailing spaces/dots or Windows
reserved component names. Case-folded Unicode-normalized aliases, duplicate files
and file/directory conflicts reject. Catalog payload is restricted to `mods.d`
configuration files; module payload stays under `mods.d`, `modules` or `data`.
Mac metadata folders/arbitrary top-level files are not accepted implicitly.

Untrusted configuration is parsed with the existing read-only SDK loader, which
rejects escaping data paths before constructing module drivers and rejects
implicit AutoInstall. A module cannot write to live storage merely because its
ZIP downloaded successfully. Unsupported/failed extraction removes its private
output; no installed library has changed at that point. Final local installation
still has the documented two-rename/crash-durability/interprocess-lock limits.

## Dependencies, provenance and validation

Native builds now require libcurl>=7.62, libarchive>=3.6 and zlib in addition to
existing dependencies. Legacy native-off C++11 helpers do not acquire them.
Use maintained SDK/distro builds; actual local validation uses curl8.23.0 and
libarchive3.8.11. FindLibArchive's imported-target fallback preserves CMake3.16.
Static SDKs can supply transitive dependencies with
`XIPHOS_TRANSFER_EXTRA_LIBRARIES`; native Swift custom links receive those too.
Runtime platform trust/proxy behavior and redistribution of TLS dependencies still
require platform packaging checks.

The local libarchive SDK was built under the ignored build directory from the
[official v3.8.11 release](https://github.com/libarchive/libarchive/releases/tag/v3.8.11),
SHA256 `a3dc0872b1b39da34dc18ef1b944f0f11ffd37425d70ae04be679279f1cf43d2`
for its tar.xz artifact. No vendored library implementation was added.
The upstream [libarchive notice](licenses/libarchive-COPYING) and
[curl notice](licenses/curl-COPYING) are retained for packaging/provenance.
Relevant transport contracts are in [curl's redirect protocol documentation](https://curl.se/libcurl/c/CURLOPT_REDIR_PROTOCOLS_STR.html)
and [libarchive's API examples](https://github.com/libarchive/libarchive/wiki/Examples).
SwordKit's native HTTPS implementation informed separation of transfer and local
installation; no SwordKit bridge or source code was imported.

`core_remote_install` starts loopback-only TLS/HTTP servers and generates original
SWORD prose, certificates and hostile archives inside a named disposable fixture.
It checks trusted/untrusted TLS, a working HTTPS redirect, downgrade to a working
HTTP endpoint, HTTP errors, declared/unknown-length limits, bounded gzip expansion,
traversal/absolute/backslash/device paths, case/Unicode aliases, conflicts, links,
CRC damage, truncation, cancellation, stale destinations, metadata mismatch,
unchanged reading/history, end-to-end install and explicit replacement, fresh SDK
reopen, C admission failures and allocation-free committed publication. No licensed
Bible content or public-repository network access is used by automated tests.
Tests require Python3, the openssl CLI and permission to bind loopback ports.

Native source configuration/installer UI, persistent catalog refresh/offline
browsing, account credentials, publisher APIs, indexes/removal and physical
Windows/Fedora validation remain open. These core checks do not accept full F27/F28
or minimum-macOS/release packaging parity.

## Why the old GTK integration was difficult to migrate

SWORD was a separately linked engine and the original app could install/update
modules; neither was intrinsically unupgradable. The Xiphos wrapper mixed layers:
`GSStatusReporter` directly called `update_install_status/progress`, the backend
used global `mgr/list_mgr/installMgr` objects and application settings, and GTK
callbacks coordinated module-tree rebuilding and rereading. That made toolkit
replacement and cross-platform reuse require application refactoring. The retained
GTK3 application is unchanged; new shared services isolate those policies from UI.
