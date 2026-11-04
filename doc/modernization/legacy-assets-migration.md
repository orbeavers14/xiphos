# Complete-folder legacy asset migration

The shared core can review and copy a selected legacy directory, including
bookmarks, annotations, study pads, templates, auxiliary settings, hidden files,
binary assets and empty directories. Select the legacy application folder first;
external study-pad or SWORD module folders can be selected separately. The
operation does not discover or follow arbitrary absolute paths in settings.

`inspect_legacy_assets` returns an owned sorted manifest: relative path, file or
directory kind, file byte count, SHA256 and a complete-tree revision. The existing
FIFO worker, additive C handles and copied Swift adapter carry this same result.
The native Legacy Migration screen provides folder selection, review and an
explicit Copy Verified Assets action. Editing the selection invalidates review;
changed contents are rejected by the shared revision check at copy time.

Copy requires a new destination outside the source. It preserves every reviewed
relative path and exact file byte, stages in a private sibling directory, verifies
the destination tree and rechecks the source before publishing. Failure cleanup is limited to the operation's private staging tree; an interruption
or cleanup failure can leave unpublished private staging. Existing destinations are never merged;
originals are never deleted or rewritten. The returned destination manifest has
the same revision as the reviewed source. File ownership, original access modes,
timestamps, ACLs, extended attributes and filesystem links are not migrated;
copied files use private owner read/write permissions and directories owner access.

Symlinks (including dangling links), special files, unreadable files, paths that
collide after Unicode normalization/case folding, and nonportable names reject
the whole review. No incomplete copy is presented as successful. Select a linked
target itself rather than a symlink. Bounds are 10000 entries, depth64,
128MiB per file and1GiB total; a larger library needs a separately scoped transfer.
The optimistic source/destination rechecks are not an interprocess lock or a
power-loss guarantee. Interrupted staging directories are not published copies.

Copying assets does not activate installed modules, execute study pads, change
current reading windows, rewrite unknown XML, or relocate saved absolute paths.
Use the existing bookmark/annotation import, document Open and explicit module
library selection with files in the copied folder. The separate
[settings/session conversion](legacy-settings-session-migration.md) remains the
way to convert supported state; its exact-original three-file package is unchanged.
This closes the missing whole-directory preservation path, not automatic
application of every legacy preference or arbitrary external reference.

Original-prose tests cover complete binary/Unicode/hidden/empty content, source
revision drift, source/destination containment, existing/dangling destinations,
symlink refusal, oversized files, source retention, and C admission-copy/owned
results after worker teardown. One-shot allocation failures across every counted
copy allocation boundary verify that failure never follows publication and that
staging is rolled back. Result paths allocate before publication; returned values
then move without allocation. Native model checks exercise review, verified
copy, stale rejection and selection invalidation. See [Mac evidence](handoff-macos.md).
