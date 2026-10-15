# Instructions for coding agents

This repository is the durable project memory across operating systems and chats.
Do not rely on another chat's history or assume your checkout is current.

## Read at the beginning of every task

1. Read [the shared handoff](doc/modernization/handoff.md).
2. Read [current status](doc/modernization/status.md), the relevant rows of the
   [parity matrix](doc/modernization/parity-matrix.md), and
   [architecture](doc/modernization/architecture.md).
3. Read the handoff for your execution environment:
   [macOS](doc/modernization/handoff-macos.md),
   [Linux/Fedora/WSL2](doc/modernization/handoff-linux.md), or
   [native Windows](doc/modernization/handoff-windows.md).
   WSL2 is a Linux validation environment, not Windows/MSVC validation.
4. Inspect the working tree, branch, remote and recent commits; fetch the fork
   before choosing work. Preserve unrelated local changes. If network access is
   unavailable, state that your handoff information may be stale.

## Project constraints

- Work on `orbeavers14/xiphos`, not upstream `crosswire/xiphos`.
- Preserve the goal of 1:1 functionality, visual composition and established
  workflows, with one shared C++ core and SwiftUI/AppKit, WinUI 3 and GTK4 UIs.
- Application logic belongs in `src/core`; platform code adapts/presents it.
  Do not create separate navigation, search, history or persistence engines.
- Native core: C++17. Legacy-linked helpers: C++11. C ABI: C11. Swift: Swift 6.
  A language-standard increase needs a separate compatibility decision.
- Preserve the existing GTK3 app, unrelated changes, tests, licenses and notices.
  Delete legacy source only after the documented parity/retirement gates pass.
- Keep routine builds/tests local. Do not dispatch, enable or add automatic
  GitHub builds without the owner's explicit authorization. `linux-core` is
  manual-only; older `builds`/`native-core` workflows were paused administratively.
  Their YAML still contains automatic triggers: do not assume they are safe to enable.
- The fork is public. Do not commit private project material, credentials, user
  profiles, installed Bible modules, machine paths or generated build artifacts.
  Use original-prose fixtures and audit provenance/licenses before reusing code.
- Preserve one SWORD-owning FIFO worker per process, copied results, explicit
  handle ownership and nonblocking native UI behavior. Read `c_api.h` contracts.

## Coordination and completion

Follow the shared handoff's branch/ownership rules before editing shared files.
Use small reviewable commits; push checked work when authorized. The owner has
authorized commits/pushes to this fork and sequential work directly on `master`.
That does not authorize overwriting another chat's changes or force-pushing.

Before a handoff, update your platform handoff with the task, branch, implementation
commit, actual checks/environment/results, limitations and next step. Update the
status/parity ledger only to the extent supported by evidence. Passing tests are
not complete feature parity. Distinguish owner-reported results from inspected
logs and tests you actually ran. Documentation-only changes need link/diff checks,
not hosted builds. Do not send messages to other chats without user authorization.
