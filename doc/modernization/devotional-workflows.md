# Shared devotional date selection

Updated 2026-10-30. This is a scoped core contract, not whole devotional acceptance.

## Retained source and characterization

The complete main_display_dictionary dispatcher in src/main/sword.cc has a
frozen/current strict C++11 oracle (12,887 traces). A changed DailyDevotion module
redirects to its pane and returns before dictionary history; absent/current
module selection follows the ordinary dictionary route. The shared
DictionaryDisplayRoute preserves that decision and the dispatcher's NN.NN
shape-only validation. It does not itself validate month ranges.

Complete main_display_devotional is frozen from `595e127` with its GPL notice.
Its host oracle covers installed fallback, clock, target/preview/sidebar rendering,
entry/button publication and temporary display restoration. The frozen month-name
lookup is guarded: it logs out-of-range indexing without actually accessing invalid
memory. The original renderer accepts any five-byte entry; 99.99 requests month
index98, while 00.00 and ab.cd request -1. These unsafe accesses are evidence of a
bug, not a compatibility requirement.

## Shared policy and explicit correction

Implementation `f0cd37d` adds devotional_date in the shared C++11 rendering
library. The host supplies local today's one-based month and day. The helper
returns an owned key, month, day and used_today flag with no clock, SDK or UI
access. It rejects embedded NUL and host month/day outside 1..12 / 1..31.

For a five-byte entry whose legacy atoi month falls in 1..12, preserve the exact
key and legacy atoi day. Other input uses today's zero-padded MM.DD key and title
components. This intentionally corrects only unsafe month indexing; safe existing
separator/day behavior remains, including 07x09, 02.29, 02.30 and 07.99. Full
calendar validation is a separate compatibility decision.

The retained renderer now delegates date selection to this helper, using the
returned key consistently for rendering, module positioning, title and successful
entry/button publication. Invalid-month input such as 99.99 falls back to local
today instead of reading outside month_names. A missing rendered result still
preserves the prior entry and button state. Clock acquisition, installed-module
fallback and display restoration order remain as characterized. Ordinary
dictionary dispatch/history policy is unchanged.

## Verification and remaining scope

The renderer oracle now compares 11,536 complete-function cases: the earlier
1,536 combinations plus all10,000 numeric NN.NN keys. Safe cases compare directly
to the frozen renderer; unsafe-month cases compare to the frozen today branch,
with prior entry retention checked for missing rendered text. No actual renderer
trace may request an invalid month index. Direct helper checks cover all372
bounded month/day combinations, owned keys, host bounds, embedded NUL, leap and
noncalendar dates, and nonstandard separators.

On Mac: focused normal2/2 (2.37s), portable2/2 (2.27s), ASan/UBSan2/2 (3.25s),
TSan2/2 (4.57s). Native-disabled C++11 libraries build and all6/6 tests pass
(3.51s). These are scoped checks, not new full native-suite or GTK application
runs. SDK/framework sanitizer coverage remains partial. No C ABI change.

The owned explicit read service and C transport are described below. Preserve
separate devotional selection and dictionary history when adding publication.
Native dated-pane presentation, date-picker
behavior, localized button formatting, current GTK/MSVC and broad corpus
acceptance remain open. The existing frozen tests do not exercise GTK widgets or
publisher content.

## Owned explicit devotional read service

Reader, StudySession and StudyWorker now expose read_devotional for an exact
installed DailyDevotion SWLD module. It applies the shared date policy using
host-supplied local today, scopes configured tab/module engine options and
restores the SDK module key after rendering. It returns a copied DevotionalEntry:
actual module ID, selected date/month/day/used_today, SDK matched key and HTML.
The selected date is kept separate from a nearest matching entry (for example,
requesting an absent 07.10 must not silently relabel the selected date). No
dictionary uppercase conversion, Bible/dictionary history visit, search generation
change or committed pane selection occurs.

The first service accepts explicit installed IDs only. Ordinary dictionaries,
verse modules, aliases and missing modules are rejected; it does not implement
first-installed fallback, saved devotional selection or automatic URI dispatch.
Those need separate policy/state integration before native presentation.

Module IDs are bounded to4096 bytes and entered date text to64 bytes. Empty date
uses today; embedded NUL, oversized arguments and invalid host-local month/day
reject. Worker validation returns an already completed exceptional future before
FIFO admission. The C wrapper bounds its scan before copying arguments or
allocating an observer; invalid C input clears the output request. Limits are byte
counts, not Unicode character counts. The renderer's selected day still follows
the existing safe-month atoi behavior, so it is not necessarily a valid calendar
day even though supplied local today is bounded.

xc_worker_read_devotional returns a distinct take-once xc_devotional_entry. Its
strings and date metadata survive request/worker close until result release.
NULL date means empty/today; NULL result getters return NULL/zero. It is a
read-only snapshot API, not devotional pane publication. Existing C result kinds
and dictionary/history contracts are unchanged. Native consumers must copy the
result before releasing it and format localized titles from its date metadata.

Original-prose Daily fixture entries now cover January1, February29, July9 and
December31. Service checks compare rendered output and matched key with a
separate SDK baseline, distinguish selected date from match, check strict module
type/feature selection, exact input boundaries, queued copies, closed tabs,
accepted-work draining, take-once/null getters and result lifetime. Allocation
interposition in the shared admission test checks that invalid C arguments
allocate neither observer nor owned string on the submitting thread.

Implementation `941a7de`. Verification after the API addition: full Mac197 passes
plus one explicit index skip (198 registered,160.32s); full Mac-hosted portable
core/Linux models150 passes plus one index skip (151 registered,132.58s; GTK
runtime OFF). Focused service/admission and fixtures4/4 (3.76s); ASan/UBSan7/7
(12.59s), TSan7/7 (27.14s); native-disabled C++11 all6/6 (12.40s). Strict C11
public-header syntax and whitespace checks pass. Native Swift importers and real
WebKit regressions rebuild/run in the full host suite. SDK/framework sanitizer
coverage remains partial and ASan leak detection is OFF. No Windows/MSVC or
current GTK application acceptance follows.
