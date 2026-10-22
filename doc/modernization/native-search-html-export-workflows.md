# Native macOS HTML search destinations

Updated 2026-10-21. Implementation `f89c585`, consuming the shared
[HTML/image service](html-image-export-workflows.md). This is scoped native
save/copy support, not full export, print or cross-platform parity.

The existing native Export menu now offers Plain text or HTML, selected passage
or all returned matches, and Include scripture. HTML additionally offers Embed
PNG images, enabled when scripture is included. Plain text remains the default;
PNG embedding requires an explicit choice. Include search details additionally
opts into [committed core context headers](export-context-workflows.md) in both
formats; it defaults off. These are native SwiftUI/AppKit
controls. Search, scripture rendering, selection, resource validation, budgets
and complete document assembly remain in the shared core.

The model captures the displayed generation, original submitted query, selection,
format, scripture and image choice before queuing. Editable drafts or later
format changes cannot change that request. Text and HTML share one native busy/
progress/Stop/stale/closed-window lifecycle. Both refuse a destination unless the
core returns a complete successful document. A stopped native action suppresses
its destination even if core completion wins a cancellation race. Resource
failures explain that images/resources are not enabled or supported; they do not
silently drop content or open a save dialog.

Save uses NSSavePanel after document preparation, the corresponding plain-text
or HTML content type/extension, and an atomic background UTF-8 write of the
captured payload. It does not render or reload the document after the picker.
Copy uses the owned plain-text or HTML NSPasteboard representation. HTML Copy explicitly requests a bounded plain-text companion from the shared
core and advertises both representations on one pasteboard item. Save continues
to request HTML alone. The frontend does not strip HTML or re-render scripture
to invent the fallback. See the [rich clipboard contract](rich-clipboard-workflows.md). A failed payload is rejected before
changing existing file or pasteboard contents.

## Executed checks

The preview, export model/destination executable and existing search model
compile locally in Swift6 mode. The focused selection passes **10/10** including
original fixture setup, both native destination formats, existing search behavior
and shared Swift HTML/image transport. Logs:
`build/native-html-export-search-regression-{build,tests}.log`.
Initial destination checks pass 9/9 in `build/native-html-export-{build,tests}.log`.

Tests verify captured original query/format/options, selected/all output,
unchanged generation/history, exact UTF-8 atomic file replacement, exact isolated
named HTML clipboard representation, unavailable/unsupported resources,
non-file destinations and unchanged destination contents on failed payloads.
Stale, Stop and close suppression are exercised. Only named test pasteboards and
temporary original-prose files were used; the user's general clipboard was not
changed.

A disposable generated OSISText window was opened through the existing temporary
fixture launcher. Native search for PNG returned John1:8. The Export format and
scripture/image controls produced an NSSavePanel with the HTML extension; saving
into that disposable folder reported HTML document saved and retained the search
selection/main reading draft. Static inspection found a complete 791-byte UTF-8
HTML document with one 75-byte original PNG payload. With embedding disabled,
Save selected passage reported the resource failure without a save dialog.
The app was quit and only that disposable app/fixture/export directory removed.
Static evidence: `build/native-html-export-window-check.log`.

This checks native controls/destination behavior, not browser-rendered HTML.
The recorded browser local-file policy block remains in effect; no alternate URL
or viewing surface was used. Full browser/CSP, corpus/visual, accessibility,
passage/print, saved preferences and GTK/WinUI destination
acceptance remain pending. No whole parity group or legacy retirement gate passed.
The earlier full 164-pass + 1-skip Mac checkpoint belongs to `374147d`; this UI
slice has focused/model/window evidence rather than a new full-suite run.

Current local SDK discovery and older-macOS compatibility limits remain in the
[dependency inventory](../../dependencies/README.md). No dependency, language
standard or hosted workflow changed. No GitHub build credits or usage reset used.
The owner requested stopping after this slice; code and this handoff are pushed.
