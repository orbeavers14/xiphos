# Reading annotations and bookmark styling

The shared `read_decorated_reading` FIFO/C service presents a single verse or a
chapter using the tab's existing render options. Explicit optional bookmark and
annotation paths enable each source independently; empty paths disable it. No
profile discovery, note writes, history/search visit or reading-selection change
occurs. Owned results contain bounded HTML/CSS plus incomplete/invalid-color
counts and survive request/worker teardown.

The service reuses canonical annotation targets, bookmark maps, escaped personal
note caches and `compose_verse_decoration`. The first tag retains priority and
personal-note highlighting retains the existing inner-color precedence. It
preserves canonical chapter IDs, selected anchors and verse-number links. Saved
plain notes appear in expandable details. Unknown/skipped metadata remains
reported rather than silently treated as complete.

Only exact RGB hex forms (#RGB, #RRGGBB and GDK16 #RRRRGGGGBBBB) can enter the
emitted style attributes; they normalize to RGB. An invalid first color is omitted
with diagnostics, without selecting a later tag or injecting raw CSS. Host-owned
CSS variables supply contrasting inner highlight colors, and a tag-colored edge
remains visible beneath note precedence. The returned HTML/CSS budget is16MiB;
limit/source failures return no partial reading. SDK internal rendering allocations
are outside that returned-output limit. No arbitrary theme/CSS metadata is applied.

## macOS integration

Bookmarks and Imports and Personal Verse Notes offer Show in Reading for their
explicitly selected files. A note form can also choose a bookmark XML file. These
commands merge selected sources into the open reading windows. Note saves and
bookmark imports refresh those windows through the same worker. Preferences has
Reload Reading Notes and Clear Reading Notes; Clear changes presentation only.
There is no external-file watcher: use Reload for changes made outside the app.

Selected paths are optional fields in each window's existing host metadata, with
older-session compatibility. Restore reapplies those sources without changing the
main reference draft. New windows inherit the active window's choices. A missing,
unreadable or invalid source leaves normal reading available with a notice; it
never displays stale decorated content as current. Invalid colors and incomplete
metadata are disclosed. Source selections do not export personal notes into
unrelated search/export workflows.

Checks cover first-tag/inner-note priority, safe color forms and malicious color
rejection, bounds, canonical anchors, C ownership/history isolation, real WebKit
computed theme styles, missing-source fallback, clearing and window restoration.
Disposable live checks include Hebrew/Arabic reading, single/chapter modes,
expandable notes and real quit/restart with selected sources.

This is a main-reading presentation slice. Commentary-side-pane, dictionary and
parallel annotation styling, all legacy color/theme forms, complete corpus/canon
and physical Windows/Fedora acceptance remain open. No full feature-parity claim.
