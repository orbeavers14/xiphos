# Optional safe BibleSync SDK

The native core's networking adapter is optional, OFF by default. Its base sync
policy and the rest of the application work without this SDK. Enable networking
only with a reviewed SDK that supplies the safety extensions below. No GTK or
second SWORD owner is introduced.

Inspected source: [Karl Kleinpaste's BibleSync](https://github.com/karlkleinpaste/biblesync),
commit `e1d35362351e4eef9da71fe49caf709efd05c179`, declaring version2.2.0.
The implementation/header/public-domain dedication is reproduced in [COPYING](COPYING).
Only the small [patch](native-safety.patch) and dedication are stored here;
the dependency source and local SDK products remain outside versioned app source.
This is a downstream SDK patch, not an upstream release or an upstream submission.

## Required changes

The inspected `setMode` calls `Setup`, which sends initial beacon/presence messages
before the caller can invoke `setPrivate`. Applying personal privacy after mode
activation therefore does not protect the first messages. The adapter requires
`setModeWithPrivacy`, which applies TTL before those messages and fails setup when
TTL configuration fails. Existing `setMode` retains its original public behavior.

The patch also supplies `ReceiveBounded`: at most64 packets per call, with a
256-speaker SDK bound. Normal upstream `Receive` retains its existing entry point.
Outgoing payloads that exceed the wire limit fail instead of silently truncating
references. Initial beacon/presence failures fail setup. Wire formats, protocol
versions and regular mode semantics remain unchanged.

The application adapter polls effectively once per second, preserving the SDK's
beacon/expiry cadence. It limits published batches to32 copied events, exposes
skipped/overflow state, and never publishes raw packet dumps/session phrases.
The configure-time API/link probe rejects an unpatched SDK. This patch does not
add encryption or cryptographic authentication: the protocol transmits the session
phrase and identity metadata in plaintext. Personal privacy is host-only TTL.

## Local build

Use the pinned source in an ignored build directory, apply the patch, and build a
private SDK prefix. No global installation or automatic network download occurs
in application configuration. For example, from the repository root:

```sh
git clone https://github.com/karlkleinpaste/biblesync.git build/biblesync-source
git -C build/biblesync-source checkout e1d35362351e4eef9da71fe49caf709efd05c179
git -C build/biblesync-source apply ../../dependencies/biblesync/native-safety.patch
cmake -S build/biblesync-source -B build/biblesync-build \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/biblesync-sdk" -DCMAKE_CXX_STANDARD=17
cmake --build build/biblesync-build --parallel 2
cmake --install build/biblesync-build
cmake -S platforms/macos -B build/macos -DXIPHOS_ENABLE_BIBLESYNC=ON \
  -DXIPHOS_BIBLESYNC_INCLUDE_DIR="$PWD/build/biblesync-sdk/include/biblesync" \
  -DXIPHOS_BIBLESYNC_LIBRARY="$PWD/build/biblesync-sdk/lib/libbiblesync.a"
```

When running `git -C`, the patch argument resolves from that source directory;
`../../dependencies/...` is the repository's patch. Unix builds require UUID
headers/library; on macOS the system UUID implementation comes from libSystem.
The upstream UUID finder may need explicit `UUID_INCLUDE_DIRS` pointing to the
selected Apple SDK's `usr/include` and `UUID_LIBRARIES` to its `usr/lib/libSystem.tbd`.
Set `CMAKE_OSX_DEPLOYMENT_TARGET` to the intended deployment floor; the inspected
local SDK uses26.0. This does not prove the whole app runs on26.0 or older systems.
Use `XIPHOS_BIBLESYNC_EXTRA_LIBRARIES` for static SDK dependencies as needed.
Windows SDK/compiler compatibility remains unverified; do not equate MinGW support
with native MSVC support. Keep platform notices and dependencies in release audits.

## Local fixture isolation

`core_sync_transport` links the actual SDK. Its executable interposes SDK `sendto`
and redirects all such messages to an ephemeral loopback sink using `sendmsg`.
It checks that the very first personal-mode messages have TTL0. A synthetic UDP
peer sends original fixture beacons/navigation to the SDK's real receiver; C/FIFO
results, session mismatch and teardown ownership are checked. This is actual UDP
and SDK parsing, not a mock transport, but is not a physical two-device/LAN test.
The fixture refuses to share an already occupied BibleSync receiver port and
reports an explicit skip. No fixture identity, session phrase or navigation is
sent to the LAN. Normal application transport has no fixture redirect hooks.
