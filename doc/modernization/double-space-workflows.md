# Shared double-spacing workflow

Goal: preserve the legacy per-module preference and style precedence through one
C++ policy. This is a scoped presentation milestone, not whole-feature parity.

## Characterized legacy contract

`src/main/global_ops.cc::main_new_globals` reads `Doublespace` through
`gui_of2tf`: only the decoded exact string `On` enables it. Missing/empty values
are off. SWConfig performs its existing file decoding first; the shared parser
does not add a second normalization rule. The complete frozen and actual legacy
initializers are compared over 2,000 image/font/spacing/module-class combinations,
including missing, empty, case, whitespace, unexpected and UTF-8 values.

`src/gtk/menu_popup.c::on_doublespace_activate` changes the selected module's
setting. `src/main/modulecache.hh` gives it its own cache flag. The retained legacy
CSS chooses 3.5em for both word/lexical layers, 2.7em for one, or 2em only when
annotations are absent and double spacing is enabled. These are established style
rules, not an unconditional multiplication of the frontend's normal line height.

## Shared implementation

Core checkpoint `25cf291` adds `ModulePresentationOptions.double_space`, a queued
per-tab/per-module command, and exact `Doublespace=On/Off` persistence. The legacy
initializer now reuses the shared decoder; its actual/frozen characterization
remains the compatibility gate. Unknown/uninstalled sections and unrelated keys
remain preserved by the existing transactional preference writer.

Opted-in snapshot consumers receive spacing CSS even when engine annotation state
is unconfigured. Matching annotation CSS takes precedence when word layout is
applied. Unsupported documents retain raw HTML and spacing-only CSS, without
absolute annotation positioning. Non-opted-in consumers keep their previous HTML
contract. Entry/paired/chapter/verse snapshots all carry the same owned decision.
`requested/applied` describe annotation word layout, so both may be false while
CSS contains explicit double spacing. Consumers must use the CSS value itself.

The C ABI adds strict queued `xc_worker_set_double_space` and repeatable scalar
`xc_request_double_space` (0/1 for a ready presentation result; -1 otherwise).
Tests cover pending/wrong-kind/failed/null results, invalid booleans/modules,
closed tabs, ownership after worker destruction, history neutrality and isolation.

Mac checkpoint `68562af` and Linux checkpoint `420ae13` copy this state and expose
separate main/commentary controls. Neither frontend decides spacing precedence.
Both host documents include CSS in refresh identity, so a spacing-only change
reloads even when the HTML remains byte-identical. Busy/closed guards and draft,
history, paired-module and window state remain intact.

## Local evidence and remaining gates

- Integrated macOS suite: 70/70, including the new native model mode.
- Portable Linux/core suite on macOS: 54/54; this does not build GTK or certify Linux runtime behavior.
- ASan/UBSan: 4/4 focused core/preference checks and 3/3 focused Linux model checks, including fixture prerequisites. Leak detection off; external SWORD/GLib/libxml2 SDKs are not fully instrumented.
- Strict C++11 presentation helper and C11 header syntax checks pass. Native-disabled legacy helper build passes; full GTK3 runtime is not checked on this Mac.
- Actual temporary Mac window: read John 4:1, show chapter, toggle Double space on/off; spacing changes visibly while passage/selection remain. Paired controls, profile round trips, annotation precedence and fallback are model/core checks, not manually claimed here.

Fedora must compile/run current GTK source and check both controls, refresh,
keyboard/pointer/accessibility and representative modules. MSVC/WinUI remain
unvalidated. No hosted builds were used, no installed modules/profiles changed,
and no legacy source was retired.
