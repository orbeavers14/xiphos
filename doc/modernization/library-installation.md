# Provider-neutral library and installation boundary

## Delivered catalog and planning slice (2026-10-31)

Implementation `48467f3`: `library.hpp` defines copied, platform-independent catalog data and pure installation
decisions. `LibraryProvider` is a catalog acquisition boundary, not a second
navigation/history/rendering engine. No GTK headers, widgets, network client,
credential storage or new provider execution thread enter this service.

An item identity is the pair **provider + item**, compared exactly. A SWORD module
uses provider `sword` and its existing exact module ID; aliases and display names
never substitute for installation identities. Different providers' editions can
coexist even when names/item IDs coincide. Translation/edition labels, version,
language, type, versification, attribution and license notices are separate
metadata. Unknown SWORD translation/edition identifiers stay empty rather than
being inferred from a description or version string.

Technical capabilities distinguish online/offline reading, local/provider search,
annotations, cross-references, export, local import and offline packages. Unknown
capabilities remain off. Access can require sign-in, a license or an unlock key,
or be unavailable. Installed state is separate from delivery. Download,
**full-package offline storage**, and export permissions are independent,
unknown/allowed/denied values. A limited online cache permission must not be mapped
to a full-package offline-storage grant. Arbitrary license text is retained as a
notice; it is not interpreted as a publisher entitlement. No credentials or cipher
keys are returned in catalog DTOs.

Catalogs bound items to4096, provider IDs to128 lowercase identifier bytes, item
IDs/source IDs to4096 bytes, each metadata field to32768 UTF-8/NUL-free bytes and
aggregate owned text to16MiB. Duplicate provider/item identities and unknown enum
or capability values reject. SHA256 metadata revisions are deterministic across
item enumeration order and include notices, permissions, availability and source
identity. They are **not package checksums, executable plans or license proofs**.

## SWORD adapter and ownership

The installed snapshot uses the live Reader manager. An explicit local-source
snapshot uses a temporary SWORD manager on the same FIFO owner, requires an
existing root containing `mods.d`, and disables standard/home augmentation.
It does not install, edit, remove, index, render or search modules, alter tab
history, or fetch a remote source. Returned snapshots own their metadata; the
existing module catalog and reading APIs remain compatible.

The adapter preserves source copyright/license notices, actual version and
versification. Available installed content advertises offline reading and the
existing supported verse-search capability. A local source advertises local
installation capability; a local file is not an online-download permission.

SDK builds differ in whether an explicitly empty `CipherKey=` survives config
loading. A bounded read-only marker scan of the manager configuration and module
`mods.d` roots preserves only the declaration, never a copied key value. It
recognizes section/key declarations, without replacing the SDK's full configuration
parser or writing configuration. Declared cipher content without a configured key
is marked locked; a configured key is not independently verified for correctness.
Configuration scanning bounds file count to4096 and aggregate bytes to16MiB;
individual source files must pass the shared4MiB UTF-8 text bound. A failed or
unsupported inspection rejects this catalog request, leaving ordinary reading
available. This classifier does not implement unlocking or whole F28 acceptance.

Worker requests copy paths before return. C result envelopes allocate before FIFO
acceptance; polling moves owned values without allocation. Results take once and
survive source/request/worker teardown. C item pointers borrow until catalog
release. Pure C installation plans own their strings after input catalogs release.

## Shared installation decisions

`plan_library_install` accepts a source catalog, installed inventory and exact
provider/item identity. Destination entries must actually be installed. A missing
item, sign-in requirement, license requirement, unavailable source, unsupported
delivery and unknown/denied permissions produce distinct decisions.

A local SWORD source can propose import, including encrypted content that still
needs unlocking before reading. A provider package requires its advertised package
capability plus explicit download AND full-package offline-storage grants. Online
reading alone does not imply installation. No grant implies export permission.

An absent destination item proposes installation. Exact nonempty declared version
equality reports already installed; different or missing versions require explicit
replacement confirmation. Versions are opaque: neither lexical comparison nor a
provider-specific ordering rule is applied to another provider. This is not an
automatic upgrade/downgrade decision or a content-integrity check. Plan results
include both metadata revisions and declared versions for later revalidation.

`core_library_catalog` exercises an original-prose non-SWORD fixture provider,
permission/access gates, provider identity collisions, opaque versions, metadata
revision changes/order independence, duplicates/bounds, real-SDK installed/local
catalogs, empty cipher declarations, unchanged reading/history and C copied-input,
preallocation, allocation-free polling and owned-handle lifetimes. This proves the
catalog/planning boundary, **not non-SWORD passage reading or downloads**.

## Next execution slice and remaining gates

Next: characterize retained local SWORD installation and introduce verified staged
local-package execution, including data/config consistency, source/destination
revalidation, failure rollback and SDK/tab/search cache refresh. Do not call the
legacy direct installer against live data and label that operation atomic.

Then add source configuration/remote catalog refresh, bounded download execution,
progress/cancellation, update/removal and index/cipher maintenance. These require
native presentation and physical Windows/Fedora validation. No current installer
UI, remote network consent behavior or full F27/F28 parity is accepted here.

Actual licensed providers still need publisher authorization, secure authentication,
provider-specific entitlement/cache limits, verified packages, shared reference
mapping and passage dispatch, availability/errors, versioned session identities,
and attribution/export enforcement. Old sessions retain SWORD module identities;
no new provider identity is silently routed to a SWORD module or fallback text.
No ESV/NASB account, license agreement or integration is introduced by this slice.
