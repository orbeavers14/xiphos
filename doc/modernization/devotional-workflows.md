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

Next: use the date policy in an owned devotional service on the existing FIFO
worker, then copy its result through the C ABI. Preserve separate devotional
selection and dictionary history. Native dated-pane presentation, date-picker
behavior, localized button formatting, current GTK/MSVC and broad corpus
acceptance remain open. The existing frozen tests do not exercise GTK widgets or
publisher content.
