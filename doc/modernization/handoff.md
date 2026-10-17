# Shared project handoff: start here in every chat

Updated 2026-10-16. These files replace missing conversation context with a
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

Shared book restore now includes explicit profile-file import `47fc462`, per-tab
publication `0af4593`, SDK resolution `eb33f2a` and typed restore contexts `929c8d8`.
Latest `106577b` applies the legacy numeric reading minimum at the shared selection
boundary while preserving explicit root-path reads. Failed imports/rendering retain
committed state; read-only calls and absent restore are neutral.
Complete document publication `6cb35cf`, worker/C `99056d7` and native consumption
`e52a497` preserve configured DisplayLevel depth/order, atomic selection and exact
rendered-section note routing. Earlier raw entry/read-only APIs remain stable.
Checks: **109/109 Mac/core**, **85/85 portable Linux/core on Mac**, focused
**ASan/UBSan 9/9** and C11. Shared presentation `86e4ad0` and native controls `614a81d` retain raw sources and
consume matching visible HTML/CSS. The final compact menu adjustment passed
app/model builds and nine focused checks. All checks are local. Full workflow/corpus/visual
acceptance, inline resources, saved tabs and writes remain pending. See
[book workflows](book-tree-workflows.md). Current GTK/MSVC/WinUI remain unvalidated.

| Implemented area | Checkpoints | Contract / limits |
|---|---|---|
| Matching lexical HTML/CSS snapshots and native adapters | `7fd1bce` through `0b40574` | [Layout evidence](word-block-workflows.md); Mac window checked, GTK recheck pending |
| Shared double-spacing preferences and native controls | `25cf291`, `68562af`, `420ae13` | [Spacing evidence](double-space-workflows.md); Mac window checked, GTK recheck pending |
| Owned note counters, scopes and native heading links | `c39b024` through `2390568` | [Note seam](note-numbering-workflows.md); queued core settings and native controls; GTK runtime recheck pending |
| Earlier Fedora preview scope catch-up | PRs #82–89, `a80409b` | [Native Fedora evidence](handoff-linux.md); retain manual/corpus/renderer gates |
| Hosted build conservation | `cf9c853` | Linux manual-only; older workflows administratively paused |

The native separate-image preview is checked and handed off at `1af7207`, using
shared dispatch `57cf632` and bounded bytes. No active edit claim is held.
No reset credit or hosted build was used. No whole parity or
legacy-retirement gate is accepted by scoped checks.

## Platform responsibilities and immediate next tasks

| Execution environment | Responsibility | Next bounded milestone | Handoff |
|---|---|---|---|
| macOS | SwiftUI/AppKit frontend, Swift adapter, existing core characterization | Inline book resources using shared resource policy | [Mac instructions](handoff-macos.md) |
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

## Native book image checkpoint — 2026-10-16

Core dispatch `57cf632` returns an owned note or image result through the same
queued worker/C boundary. Native preview `1af7207` copies bytes before releasing
C ownership, decodes an ImageIO thumbnail off the UI thread (2048-pixel limit),
and presents a native sheet with Close. Failed loads and late completions after
window closure retain committed reading state. A generated original-prose PNG
opened in the Mac window; closing it returned to the same book section. The
subsequent accessibility label compiled and passed focused model checks; full
VoiceOver acceptance remains open.

Local validation: **109/109 Mac/core**, **85/85 portable Linux/core on Mac**,
**9/9 ASan/UBSan**, and strict C11 header compilation. After the image label
adjustment, app/model builds and **8/8 focused checks** passed. No hosted builds
or reset credits were used. Portable checks on Mac do not validate current
Fedora GTK or native Windows/MSVC/WinUI.

Next bounded task: characterize and implement inline book resource routing using
the same shared resource policy. Inline images, sizing/CSS, other formats and
animation, full-size viewing, corpus/visual workflows, note numbering and saved
tabs/writes remain pending. Preserve GTK3 and all retirement gates. Overall
planning estimate remains about **8% (6–10%)**, **0/38** whole feature groups
accepted, **0/3** release-ready native apps. This checkpoint is handed off near
the usage ceiling; no active edit ownership is held.
