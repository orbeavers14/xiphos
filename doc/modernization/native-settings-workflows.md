# Native Settings, localization and reading windows

2026-11-04 scoped implementation. Settings is no longer an empty SwiftUI scene.
It observes the active reader/session, forwards supported reading/display options
and lexicon choices to existing core-backed model commands, and saves through
native session persistence. Unsupported module flags are disabled. Reading
settings remain per window; the Save Settings and Session action captures the
whole session through the existing FIFO. Startup preferences use a separate
profile-scoped native UserDefaults namespace: default Bible/commentary, restore
saved session and recovery interval (5/15/30/60s). New windows use installed
configured modules, falling back safely when absent. Restored sessions retain
saved choices; explicit recovery still takes precedence over fresh startup.
Recovery interval changes apply next launch; autosave is not disabled.

BibleSync stores its successful non-secret configuration and remains explicitly
disconnected on startup. Identity, session phrase, acknowledgment and active
connection state are not saved. Core configuration validation still runs when
connecting; saving a preference does not initiate network activity.

## Localization boundary

Native en/fr lproj resources are copied into the bundle. System/English/French
interface selection persists; restart is required. French is explicitly a
partial translation. Common stable core error statuses are mapped through native
localized strings; arbitrary SDK/internal messages remain fallback detail.
Legacy gettext catalogs remain independent preserved translator input, not
silently claimed as integrated. No process locale is changed while engines run.
Scripture reference localization, resource closure for SWORD locale data, broader
translated surfaces and complete RTL acceptance remain open.

## Reading windows and tabs

Command-T creates a native AppKit tab sharing the existing one-worker study
session. File > Move Reading Tab to New Window detaches it. Session host metadata
adds optional tab group/selected-tab fields, preserving older files and other
hosts. Grouping is restored only among windows in the same saved session;
bounded group keys are used solely as presentation identifiers. Core navigation,
parallel/annotation services and document ownership are unchanged.

## Evidence

Mac27.2/AppleClang21/Swift6.4.2. Full build and suite:
build/settings-workflow-full-{build,tests}.log:257 passed/2 expected index skips
out of259,142.53s. Expanded native restart checks run new processes for preference
round trips/profile isolation/fresh startup, native tab grouping restoration,
detaching and subsequent separate-window restoration. Focused final restart
and prerequisite2/2,9.67s. Existing parallel, annotations, model and recovery
checks pass as part of the full suite. en/fr string resources pass plutil lint.

Isolated development-bundle live checks: Command-comma opens populated Settings;
chapter option changes and Save confirms success. Command-T exposes two native
accessible tabs; the detach command produces a separate reading window. A
process-only French override shows translated Settings labels and English
fallback, without changing the system language. The isolated app quits normally.
No real user profile opened and no live sync connected. Graphify refreshed from
local code only:585 files/10141 nodes/27745 edges/364 communities, zero paid calls.

## Remaining acceptance

- Native in-window Settings navigation and other screen review are explicitly
  tracked for Mac/Windows in native-ui-todo.md. Current management launch buttons
  and lexicon sheet are functional intermediate UI, not the final native design.
- Complete translation coverage, localized references, RTL visual checks and
  broader keyboard/VoiceOver/workflow acceptance remain open. Existing scoped
  tests do not establish entire feature parity or physical-platform acceptance.
