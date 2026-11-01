# Native BibleSync integration

Implementation `526cd1c`, 2026-11-01. File > BibleSync opens the Mac controls.
The Swift adapter transports owned C/FIFO values. Shared C++ policy remains
responsible for modes, sender/group/domain filtering, module resolution, direct
versus review routing and received-navigation echo suppression.

Startup creates no connection. Display name, session phrase, network consent and
an explicit Connect are required; receive defaults to Nobody. Personal mode
initially restricts multicast traffic to this Mac. This is a transport TTL setting,
not encryption. The protocol shares identity/device metadata and a plaintext
session phrase. No consent or phrase is written to preferences or recovery files.
Disconnect clears the in-memory phrase and consent. Builds without the optional
reviewed SDK show unavailable controls and never silently enable networking.

Settings edits take effect only on Connect/Reconnect. The displayed applied mode
remains authoritative while fields are edited. Disconnect uses the shared
policy-only command and needs no credentials. Failed connection attempts stop any
prior connection, including when C admission rejects malformed input. Transfer or
poll failures stop sync and require explicit reconnection; automatic network
reconnection is not implemented. Peer discovery uses periodic SDK beacons, so a
new listener may wait for the next beacon.

A single native ticker schedules bounded core polling. Connection changes cancel
and drain old polls and sends; generations discard stale results. Outgoing local
passage changes are coalesced with one accepted send and one pending latest target.
Committed reading navigation broadcasts in Personal/Speaker modes; Audience cannot
send. Received navigation uses the existing reader/history/presentation services
with an explicit received origin, so it cannot cause retransmission. Quit blocks
connection changes, suppresses navigation callbacks during session capture and
drains/stops sync after the user completes quit/save confirmation.

Ranges, missing modules, disabled automatic following and busy readers leave
reviewable received entries. Review uses the shared reference-list service;
missing modules are explained rather than replaced with another Bible. The list
retains the latest32 entries and reports drops. Speaker IDs support explicit
allow-list configuration. Group input is empty or one digit1–9; automatic group
to window/tab mapping is not exposed in this native slice.

## Actual checks and limits

Two independent processes use the real SDK and FIFO workers with separate empty
module roots. Test-executable socket interposition redirects every send and fixed
receive bind to isolated loopback ports. A local relay fans out actual SDK packets;
no fabricated peer packets or public Bible text are needed. Tests wait for normal
periodic peer beacons. Bidirectional Personal sharing, Speaker/Audience rules,
wrong sessions, listen-none, echo suppression, disconnect/reconnect and injected
send-failure/repair pass. Unknown module IDs correctly remain reviewable.

The pair test passed78.08s initially and is included in the full native regression
checkpoint:235 passes plus one existing index skip,236 registered,284.00s.
Final native layout/refinement scoped checks4/4,6.36s. Native bridge/model checks
exercise owned values, selected-speaker filtering, direct navigation origin,
range review, missing modules and credential-free disconnect without opening
network sockets. Live isolated Mac UI checked readable controls, selected-speaker
fields and disconnected launch/restart; it did not enable a real LAN connection.

These are not physical two-device discovery, LAN/multicast routing, firewall,
sleep/interface-change or unrelated-client interoperability acceptance. Windows/
Fedora native controls, friendly presence/chat, automatic group routing and full
F31 parity remain open. Personal data sync and Apple Handoff are separate future
features in the [roadmap](shared-core-roadmap.md).
