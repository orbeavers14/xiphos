# Preverse heading collection: initial seam

Updated 2026-10-12. This supports the F03/F08 heading work but does not establish
complete rendering, caching or visual parity. Core commentary snapshots now carry
headings; typed C/Swift adapters and native verse/chapter controls use the shared
per-module preference. Bible heading integration remains pending.

## Source and scope

`src/main/display.cc:CacheHeader` clears a cached header, reads sequential
`Heading/Preverse` attributes starting at zero, renders each through SWORD,
postprocesses it and appends it. It stops at the first empty/missing index.
`src/backend/sword_main.cc:BackEnd::get_entry_attribute` defaults to refreshing
the current entry's attributes before each lookup and converts the attribute
through SWORD's UTF8HTML filter before returning it.

The C++11 `render_preverse_headings` helper preserves this SDK collection/dataflow
and returns owned strings. It refreshes attributes after each previous buffer
render, preserves the current module key and stops at the first empty index.
Callers establish engine options and serialize the module first. It does not
implement Xiphos block rendering, CleanupContent transformations or GTK caching.

The test includes complete frozen functions from CrossWire c9976a8 and extracts
the complete current functions at configure time. It compares their output with
the helper using disabled block flags and an identity CleanupContent shim that
checks the non-reset header call. This validates collection/dataflow under that
scope, not the shimmed transformations or the complete cached display path.
The original license notices remain with the frozen functions.

## Evidence

`core_heading_collection` creates original-prose RawCom OSIS and ThML data with composite
preverse headings, Unicode text, an entry without headings and mixed interverse/
preverse markup. It compares frozen/current/helper output, key identity and
selection, body exclusion and absence of stale headings after navigation. Where
the local SDK numbers an interverse heading before a preverse heading, it checks
legacy termination at the resulting numbering gap. ThML adds two sequential
headers, a linked verse, no-heading and heading-only entries. Frozen/current
collection agrees with all of these on the local SDK; sparse chapter snapshots
render linked heading content once while retaining the missing-selection anchor.

The fixture's preverse container follows the vendor's
[OSIS heading filter source](https://www.crosswire.org/svn/sword/trunk/src/modules/filters/osisheadings.cpp).
The [ThML filter source](https://www.crosswire.org/svn/sword/trunk/src/modules/filters/thmlheadings.cpp)
informs original `sechead`/`title` fixtures. The installed SDK and local test oracle determine behavior; the test does not
assume that every OSIS title becomes a separate preverse attribute on every SDK.

Normal and address/undefined checks pass; strict C++11 syntax also passes. SWORD
and GLib are not fully instrumented, and leak detection is disabled. The shared
helper supplies owned commentary entry/chapter snapshots. Chapter HTML places
headings before the selection separator and verse body. Ordinary body-only reads
remain compatible. Shared per-module Headings preferences default on, decode
exact Off, and participate in combined import/save. Commentary chapter capability
controls selective mode persistence; unrelated and unavailable settings survive.

The full local suite passes 62/62. Focused address/undefined checks pass 2/2
and thread checks 2/2, including heading-only entries, body exclusion, preference
round trips, FIFO tab isolation and unchanged history.

The C ABI exposes borrowed heading strings from the existing owned entry, chapter
and paired-passage handles, with bounds/null guards and unchanged transfer rules.
Swift copies these strings; native controls fetch/update the shared preference.
Heading extraction also scopes/restores every SDK Headings filter when no engine
profile is active, leaving ordinary body-only reads unchanged. Native model tests
cover busy/closed commands, drafts/history, window isolation, heading-only entries
and explicit import/save. A temporary preview verified headings above the verse
body, chapter toggling and retained draft input. The control governs preverse
attributes; interverse titles remain part of SDK body rendering. ThML heading
scripture links produce SDK reference-preview commands. Core and native model
checks preserve the commentary source module, copied preview text, draft input
and history; this is not proof of other heading note/lexical link families.

## Next acceptance work

- Bible chapter/entry heading integration and complete placement/visual acceptance.
- Representative module data, canonical headings, broader numbering gaps,
  linked entries/selection across SDKs and heading note/lexical links.
- Block/cleanup transformations, cache invalidation, personal commentaries,
  fonts/images/colors and legacy/native runtime/visual comparison.

No existing renderer or source file is retired by this seam.
