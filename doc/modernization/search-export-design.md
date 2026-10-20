# Next shared slice: retained search results and export

Updated 2026-10-19. The first C++ retention/session/worker step is implemented in
[retained-result contracts](search-retention-workflows.md). The remaining export,
C/Swift transport and native host sections below are a proposal, **not implemented
public APIs**. Historical proposed wording is superseded by that current contract
where the retention service now exists.
See [current search](search-workflows.md), [export evidence](export-workflows.md)
and [remaining core scope](shared-core-roadmap.md). No native export is accepted.

## Problem established by current code

Public single/batch search output is a bounded prefix. The private last scope
retains complete typed keys, but a batch replaces it after each execution, leaving
only the final module. Neither a truncated prefix nor final scope can reproduce
all search groups for export. Rerunning a search can change results/options and
must not be an implicit recovery mechanism. Legacy export/bookmark code also has
characterized first-row/header and global-list traversal quirks.

## Proposed retention contract

Keep one current search-result generation per tab on the existing engine owner,
separately from last-search scope and annotation context. A generation owns module
identity, ordered group/status metadata, query disposition and, when capacity
allows, complete SDK keys for each successful group in display/export order.
Scope retains its characterized pre-index-sort snapshot; export order must use
post-sort keys instead. Preserve duplicate groups/rows and module identity.

Return a copied nonzero generation ID with public output. Export requests must
name that ID; a stale ID rejects rather than exporting a newer invisible query.
No SDK object/owning SDK handle leaves the FIFO worker. Replacing a generation or
closing a tab destroys SDK snapshots on that owner. A later C/Swift presentation
failure cannot accidentally associate old visible rows with new retained results;
the host re-queries metadata or shows the stale-result error.

Publication follows visible-result disposition:

| Disposition | Proposed current generation |
|---|---|
| Returned successful or SDK-partial executed search | Replace with returned groups/disposition |
| Successful empty last-scope short-circuit | Replace with an explicit empty result generation |
| Cancel before execution, with old visible results retained | Keep previous generation |
| Single-search validation/preparation failure | Keep previous generation |
| Completed batch with ordered per-group failures | Replace with attempted groups/statuses; no silent old result reuse |
| Fatal batch failure before owned publication | Keep previous generation |
| Clear last scope / reset annotation context | Keep visible result generation independent |

Use an aggregate retention cap across all groups, not the existing per-module
scope cap multiplied by 32. Initial proposed ceilings: 100,000 SDK elements and
16 MiB accounted key/range/versification bytes across the generation, with separately
bounded owned metadata/prefix storage. Validate and test the actual accounting.
These are logical caps, not exact SDK heap/time limits.

If complete export retention exceeds capacity while search itself can succeed,
return the usable bounded display result with a **new** generation whose complete
export capability is explicitly unavailable (`capacity_exceeded`). Do not leave
an old generation masquerading as the new query. Explicit visible-row export can
remain possible from independently retained bounded public rows. This is a new
capability/disposition contract and needs tests before implementation acceptance.

## Proposed export request and result

Two explicit selections:

- All retained matches: require complete retained groups and the matching current
  generation. A missing/incomplete generation rejects with an owned reason.
- Visible rows: identify generation/group/row ordinal, bounded in count, retaining
  requested order and duplicates. The core resolves authoritative module/key
  metadata; the host does not synthesize an all-results claim from a prefix.

Rows need ordinal identity because duplicate keys/groups are legitimate. Establish
selection ordering against the retained UI before adding native selection actions.
A returned stopped search can export its explicitly identified partial generation;
it must retain that cancelled/partial disposition in metadata or presentation.

The request chooses text/HTML, include-scripture and a positive aggregate output
byte cap. Build a complete owned payload or return a typed failure/cancellation;
do not return an unlabeled half-document as a successful complete export. Validate
selection and estimate/consume the output budget before publication. SDK rendering
and temporary allocation/time are not automatically bounded by the final byte cap.

Scripture reads use existing configured, history-neutral core reading on that tab.
Use settings captured at export-job start, as legacy export reads the current
reading backend, rather than assuming search-only annotation choices are display
preferences. One worker job prevents application commands from interleaving those
reads; it does not make externally modified module files transactional. Reading,
book selection and both histories must remain unchanged, including failure/cancel.

Reuse the checked item formatter. Header/document assembly belongs in one shared
policy. Include every intended match; handle the characterized first-row omission,
raw header escaping and unmatched list closes as explicit corrections with
before/after fixtures. Do not claim byte-identical legacy orchestration when
correcting these quirks. HTML assets/styles, localized labels, whole module
families and saved workflows require separate acceptance evidence.

The worker/C boundary transports only copied statuses, metadata and a complete
owned payload. Swift/Windows/GTK adapters provide native file/clipboard/print UI;
they do not recreate result selection, scripture rendering or document assembly.
Prepare payload fully before any destination write. Reuse verified replacement
primitives where their ownership/path contracts fit; profile persistence is not
an automatic generic export-file API. Clipboard/printing/pagination remain
independent host and acceptance gates.

## Bounded implementation order

1. Add private retained generation/capability metadata; test single/batch/duplicate
   ordering, index sorting versus scope, aggregate retention capacity, empty/clear,
   errors, cancellation, stale IDs, tab isolation and owner-thread destruction.
2. Add bounded explicit visible-row text export with the existing formatter and
   history-neutral configured reads. Test exact output/limits and no publication
   on failure before broadening to complete retained results and HTML headers.
3. Add copied C requests/results, take-once/borrowed lifetime and shutdown tests;
   consume a native save/copy action only after the same shared contract is checked.
4. Complete all-results export, bookmark transformation/storage and print/HTML asset
   behavior with representative permitted corpus/profiles and visual/input evidence.

This plan does not retire legacy source, complete bookmarks/passage export or
recompute the overall/core completion estimate. Keep each slice reviewable and
locally tested; no hosted build is required to begin it.
