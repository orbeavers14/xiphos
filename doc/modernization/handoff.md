# Shared project handoff: start here in every chat

Updated 2026-10-15. These files replace missing conversation context with a
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

Known pushed checkpoints:

- `9e288b3`: local macOS/shared-core checkpoint, including the owned lexical
  block renderer and explicit threading linkage for its concurrent checks.
- `f190c49` / PR #82: Linux shared-core setup/build/test script, Fedora dependency
  support, explicit standalone C++17 and CMake extraction compatibility fixes.
- `cf9c853`: Linux hosted workflow changed to manual-only to conserve allowance.
- `7986d20` concludes the current shared lexical helper checkpoints: owned scanner/
  annotation state, SWORD note-marker preservation, combined HTML/CSS policy and
  real note/reference URL integration evidence. See the Mac handoff for exact
  tests and commits. Later snapshot/adapter checkpoints below add scoped native consumption; no full UI parity is implied.
- Linux catch-up PRs #83–89 and the final scope audit expose the existing macOS
  preview capability set. Fedora ARM64 passes 51 integrated tests, four sanitized
  model executables and six software-rendered desktop smoke modes. Exact source,
  environment, commands and unaccepted manual gates are in the Linux handoff.

Latest lexical checkpoints: `7fd1bce` core snapshots, `8c0f4f2` C metadata,
`0af3689` Swift copies, `1393edd` nesting guard/captured headings, `d953adc` Mac
view consumption and `0b40574` Linux adapter/headless model support. Mac passes
68/68 plus scoped GUI checks; Linux portable/core checks pass 53/53 on Mac with
4/4 focused ASan/UBSan checks. New GTK view source still needs native Fedora
build/runtime checks; the earlier 51-test Fedora result does not cover it.

Double-spacing follow-up: `25cf291` shared decoder/persistence/session/worker/C ABI,
`68562af` Mac controls, `420ae13` Linux controls. Current local results: 70/70 Mac,
54/54 portable Linux/core on Mac, focused ASan/UBSan 4/4 core and 3/3 Linux model.
See [workflow evidence](double-space-workflows.md). Fedora must rebuild current
GTK source; previous native counts are historical. Next shared presentation seam:
characterize note-number rendering and its SDK filter/state boundaries.

## Platform responsibilities and immediate next tasks

| Execution environment | Responsibility | Next bounded milestone | Handoff |
|---|---|---|---|
| macOS | SwiftUI/AppKit frontend, Swift adapter, existing core characterization | Characterize remaining presentation preferences and broader lexical corpus; preserve shared HTML/CSS and existing preview behavior | [Mac instructions](handoff-macos.md) |
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
