# Passage URI dispatch boundaries

Updated 2026-10-22. This records retained behavior and partial native coverage;
no whole workflow or new parity group is accepted.

## Complete retained callbacks

The C++11 oracle freezes complete `show_in_previewer` and `sword_uri` callbacks
from `e0e9a86`, with their GPL notice, and compiles current callbacks extracted
from `main/url.cc`. Deterministic host callbacks compare18,436 traces and final
state across explicit/missing/unknown modules, aliases (including Unicode and
uninstalled alias targets), every retained module family, punctuation/ranges,
anchors, commentary visibility, notebook page, bookmark tags, preview render
failure, null navigation results and recursive entry. Each hover gets a mutable
copy of the URI and must restore its temporary slash change. This characterizes
callback dispatch, not actual GTK/SWORD/WebKit rendering or malformed legacy URL
safety. Native parsers retain their stricter supported-origin/syntax boundary.

Observed click behavior:

- Resolve a nonempty declared module or the main-module default through the
  abbreviation map first.
- A Bible module plus any `-;,` reference marker opens the sidebar before the
  final history flag. Other families do not take this range branch.
- Known Bible/commentary/dictionary/book/prayer modules use their family route.
  Commentary/book routes change the notebook page as needed and preserve the
  anchor pointer around that page change.
- Unknown modules navigate the main reader through its navigation controls, then
  update Bible/commentary/sync callbacks. They do not simply return an error.
- The normal click tail sets addhistoryitem; recursive URI entry returns zero.

Observed hover behavior: dictionary targets render the information viewer;
other families update status text, preferring bookmark tag information. Hover
performs no click navigation/history dispatch.

The frozen Bible-range early return retains a split-string allocation. The
oracle records that allocation behavior and reclaims buffers after every trace,
so the matrix itself remains bounded. This is recorded evidence, not a claim
that this retained resource defect has been repaired.

## Current native scope

Typed single-passage sword/bible resolution consumes the bounded owner alias
catalog and returns owned actual module IDs. Command showRef module fallback is
separate and shared with GTK. Unknown single-passage URI modules now fall back to the exact installed main
module after alias lookup reports ordinary absence; invalid alias metadata is
still an error. The owner validates the copied actual target without navigating.
Native pane dispatch preserves original URI and main/source roles, so fallback
does not accidentally use an unknown offered name or the commentary source.
An unavailable fallback or known non-verse target still fails.

Contextual native sword/bible range/list URIs now use the extracted retained
Bible-only sidebar decision. Literal URI tokens and decoded expressions stage
before owner alias/type resolution. Installed Bible targets return bounded owned
reference snapshots, preserving supplied source reference, ordered duplicates,
per-tab rendering options, actual source identity and limited disclosure. Missing
URI modules use main navigation context. Unknown/non-Bible range targets remain
errors; retained fallback/navigation for those cases is still a separate gap.
Contextual URI input is capped at32768 bytes and rendered previews at2000 total
entries. Older two-kind/Reader single-target APIs keep strict single references.
No main/history/search mutation occurs when the preview is returned.

The retained GTK callback now calls the same C++11 Bible/range decision; complete
18,436-trace frozen/current comparison preserves type-lookup callback order.
Native anchors,
dictionary hover/keyed-module navigation and status/bookmark behavior must be
implemented and checked independently against the retained callback evidence.
See [alias policy](module-abbreviations-workflows.md),
[hover policy](hover-workflows.md) and [platform evidence](handoff-macos.md).
