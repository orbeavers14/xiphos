# Shared project handoff: start here in every chat

Updated 2026-10-20. These files replace missing conversation context with a
versioned record. They do not transfer chat messages or synchronize themselves:
each chat must fetch/pull Git and read them before working.

## Goal and current checkpoint

Fork: `orbeavers14/xiphos`; integration branch: `master`. Preserve Xiphos's full
functionality, design composition, data compatibility and workflows while
modernizing into one platform-independent C++ application core and three native
frontends. Keep GTK3 working as the reference implementation throughout migration.

The [status ledger](status.md) is the detailed scope/evidence record; the
[parity matrix](parity-matrix.md) controls acceptance. The planning estimate is
about 8% (range 6–10%), with 0/38 feature groups fully accepted and 0/3 native
release-ready applications. Do not turn passed build counts into parity scores.

Current search checkpoint: shared attribute execution `572f539` / Swift `a83d3c4`,
annotation context `8ebeddc` / probe ordering `92bacfc`, canonical copied C request
`b71980b` / Swift `f4934b2`, explicit advanced multiword/raw-index dialects `69dea11`,
and ordered bounded batches C++ `f86293b` / C `ad28750` / Swift `cf80541`. Basic
native controls remain `0ca0bab`; advanced controls are not yet implemented.
Complete typed last scope is independent of copied result prefixes. Search-only
settings are per tab and physical SDK filters restore on every job. One FIFO
owner handles batches, progress/Stop, scope and copied values. See the consolidated
[current search contract](search-workflows.md); chronology remains in Git.

Latest integrated native image checkpoint: **159 Mac passes + 1 index skip**
(160 registered, 134.30 s), with desktop access for the isolated pasteboard test;
**121 portable/core model passes + 1 skip on Mac** (122 registered, 92.46 s),
GTK runtime OFF. Current Fedora/MSVC acceptance remains separate.
Detailed normal/sanitizer/C11 evidence lives in the contracts and platform handoff.
Actual-index diagnostic `a3dcae8` still has no usable SDK framework; optional probe
skips, required-index negative control fails as expected. No actual indexed
execution or failed required probe is counted as acceptance.

Private HTML image infrastructure at `63ce703` now includes a module/URI cache with aggregate
occurrence/raw-byte/pixel/encoded-byte bounds, typed file-limit reporting and
complete-fragment validation before loading. Public HTML remains resource-rejecting.
See [the resource checkpoint and next integration step](resource-export-design.md).

Book tree/restore/navigation/DisplayLevel, resources/styles and combined note
counters remain checked partial services with native Mac consumption; detailed
contracts and earlier hashes live in [book workflows](book-tree-workflows.md).
These search advances accept no whole parity group or retirement gate and do not
create a reliable core-only completion percentage. Keep [core scope](shared-core-roadmap.md)
and the independent platform evidence in view.

| Implemented area | Checkpoints | Contract / limits |
|---|---|---|
| Matching lexical HTML/CSS snapshots and native adapters | `7fd1bce` through `0b40574` | [Layout evidence](word-block-workflows.md); Mac window checked, GTK recheck pending |
| Shared double-spacing preferences and native controls | `25cf291`, `68562af`, `420ae13` | [Spacing evidence](double-space-workflows.md); Mac window checked, GTK recheck pending |
| Owned note counters, scopes and native heading links | `c39b024` through `2390568` | [Note seam](note-numbering-workflows.md); queued core settings and native controls; GTK runtime recheck pending |
| Earlier Fedora preview scope catch-up | PRs #82–89, `a80409b` | [Native Fedora evidence](handoff-linux.md); retain manual/corpus/renderer gates |
| Hosted build conservation | `cf9c853` | Linux manual-only; older workflows administratively paused |

Retained generations are pushed: C++ `37d9004`, C `155e0cb`, Swift `d1a15df`.
See [the contract](search-retention-workflows.md) for capacity, identity and copied
stale-safe reads. Selected text export: C++ `3faadea`, C `6f0e3c1`, Swift `ec26889`;
complete returned text: C++ `b7dafd4`, C `1bb2615`, Swift `80589a4`; native Mac
selected/all text save/copy: `5d52a11`. See [the text contract](search-text-export-workflows.md).
Output guards, first-row inclusion, order/duplicates, configured SDK text reads,
partial-source choice, cancellation, state neutrality and owned lifetime are
checked. Native window/save evidence is scoped; the user's general clipboard was
untouched by tests. No complete HTML/print or whole feature group is accepted.

SDK reference/range guards are pushed at `c3218e6` (six focused checks each in
normal/ASan/UBSan/TSan). Configured C++ HTML/CSS source snapshots now share live
rendering without publishing auto-discovered image/font choices. Real ThML
regressions corrected the earlier query/scripture guard's terminal-NUL convention;
[the source contract](html-source-workflows.md) records the correction and 10/10
ASan/UBSan plus 10/10 TSan checks. Bounded body/CSS capture additionally passes 7 focused checks each on normal Mac,
portable-on-Mac, ASan/UBSan and TSan. Initial restricted passive HTML documents
are pushed: C++ `1b327d9`, C `e7c141e`, Swift `e8a8ec8`. Selection stays shared
with text, matching core CSS is scoped, unsupported resources/markup reject
without partial output. See [HTML contract](html-export-workflows.md).
Native HTML destinations, resource/link policy expansion and browser/visual
acceptance remain pending. Local-file browser preview was blocked by URL policy;
no workaround or visual acceptance.
Subsequent wrapper/traversal guards are checked at `395c834`. Shared resource
filesystem policy is extracted at `ff46f9b` with NUL-path guards at `7e9ff93`;
[reuse audit](resource-export-design.md) defines the next bounded source contract.
Keep actual indexed execution, advanced controls/mixed families/settings and other
core services separate. No hosted jobs or reset credit used.

Shared verse image source/command services, C/Swift values and typed reading-pane
dispatch are checked through `c4336f6`. Native Mac image preview now passes scoped
models/window checks using the same source owner and a reused native viewer.
See [image contract](verse-image-workflows.md). HTML exported images and complete
inline-image/hover/format/corpus/platform workflows remain pending.

## Platform responsibilities and immediate next tasks

| Execution environment | Responsibility | Next bounded milestone | Handoff |
|---|---|---|---|
| macOS | SwiftUI/AppKit frontend, Swift adapter, existing core characterization | Shared resource/link export characterization, then native HTML consumption and advanced search workflows | [Mac instructions](handoff-macos.md) |
| Fedora / Linux / WSL2 | Linux core validation and GTK4 frontend | Rebuild current GTK lexical integration and new model test, then physical-input, picker, accessibility, corpus and accelerated-rendering gates | [Linux instructions](handoff-linux.md) |
| Native Windows | MSVC/dependency validation, Windows adapter and WinUI 3 frontend | Build/test the existing core with native Windows dependencies before creating the matching reading slice | [Windows instructions](handoff-windows.md) |

These are task lanes, not claims that sessions are running or leases are held.
The platform directories are separate frontends in one repository; permanent
diverging OS branches or copies of core behavior are not the architecture.

## Start, coordinate, integrate

1. Check `git status`, current branch and `git remote -v`. Fetch `origin` and
   inspect changes since your last checkpoint. On a clean integration checkout,
   use `git pull --ff-only origin master`. Do not reset/stash/delete another
   person's work just to get a clean tree; preserve it and resolve ownership.
2. Read your platform handoff and the shared API contracts before picking a task.
   Record your bounded task and working branch in your platform handoff. A pushed
   task note advertises intent; it is not an atomic lock or proof a chat is active.
3. For simultaneous chats, use a short-lived branch with an OS/task name and
   separate checkout/build directory per environment. Keep edits in your platform
   directory where possible. Do not have multiple chats edit the same shared-core,
   C ABI, build or central status files at once. Confirm the shared task's owner
   with the user when ownership is unclear; continue independent platform work.
   Sequential checked `master` commits remain authorized.
4. A shared API change needs a compatible implementation, ownership/error/lifetime
   tests, and assessment of every existing consumer. Avoid parallel incompatible
   API designs. Write down new contracts and the platform checks still pending.
5. Run meaningful local checks for the change, then inspect the diff and commit.
   Fetch again before integrating/pushing. If `master` moved, review/reconcile and
   rerun affected checks. Never force-push or blindly resolve conflicts by taking
   your own version. PRs are useful for overlapping work; hosted CI is not required
   for every integration. Do not bypass branch protections.
6. Update your platform handoff and any affected central status/parity rows, then
   push the checked checkpoint. The next chat pulls that commit. A local-only note
   cannot align another computer.

## Evidence to record at each checkpoint

Use this short format in the relevant platform handoff; retain older substantive
evidence in the status ledger or a linked PR rather than endlessly duplicating it.

```text
Date / platform:
Task and branch (active / handed off / blocked):
Implementation commit tested:
OS, architecture, compiler and dependency versions:
Changed behavior:
Exact local commands and results (include test count):
GUI interactions checked, if any:
Not checked / known failures:
Shared API changes and other platforms affected:
Next bounded task:
```

Record the implementation commit first and add the handoff in a subsequent commit
when necessary; do not invent a self-referential commit hash. Logs must be scrubbed
of credentials/private data. Record exact commands even when another machine needs
different SDK discovery settings. A successful launch is not an interaction check;
a WSL test is not native Windows or full Linux desktop acceptance.

## Budget, data and acceptance rules

Keep builds on the available computers. Linux `core.sh` supports release and
address/undefined sanitizers without GitHub. Hosted `linux-core` is manual-only
and would run four jobs if dispatched. Older workflows were administratively
paused; their trigger configuration still matters if re-enabled. Do not spend
hosted build allowance without explicit owner authorization.

The core is C++17, reusable legacy seams C++11, the public C interface C11, and
the current Swift adapter Swift 6. SwordKit is only a possible audited adapter;
application logic must stay shared. Native embedded web views present core-owned
HTML; URI routing and study behavior remain in C++. Do not enable arbitrary module
scripts/network content to make a preview work.

All legacy source stays until its complete replacement has validated functional,
visual, workflow, data and relevant platform evidence under the retirement rules.
An empty/unavailable dependency environment is a reported limitation, not a reason
to delete tests or claim parity. Preserve GPL notices and audit actual imports.

## Starter message for a new chat

Paste this into each new OS chat after opening the fork checkout:

```text
Continue the Xiphos modernization in this fork. First fetch the latest repository
state without overwriting local work, read AGENTS.md and
doc/modernization/handoff.md, then read the handoff for your actual OS.
Use those files as shared project memory. Follow the platform's next bounded task,
coordinate ownership before changing shared-core APIs, run checks locally, and
update/push the handoff with the tested commit and evidence. Preserve the GTK3 app
and one shared C++ core. Do not trigger or enable hosted GitHub builds.
```

## Hosted-build conservation verification (2026-10-20)

Read-only GitHub inspection after the checked pushes confirms `builds` and
`native-core` remain disabled_manually. `linux-core` remains active with only
workflow_dispatch in its YAML. The latest repository runs returned by GitHub are
from 2026-10-15; checked pushes through `7e9ff93` created no hosted jobs. No enable/dispatch
or allowance reset was performed. Recheck these states before any future hosted
run; an active manual workflow is not permission to dispatch it.

At this checked handoff, source/docs are pushed sequentially to the fork's master;
there are no shared source edits left in flight. Preserve unrelated local files
and fetch before the next slice. The next shared service and explicit unresolved
gates are recorded above and in the remaining export/resource plans.
