// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal enum ParallelSetAction { Save, Rename, Remove, Activate, Create }

internal sealed record NamedParallelSet(string Key, string Name, IReadOnlyList<string> Modules)
{
    public override string ToString() => Name;
}

internal sealed record ParallelProfile(string? Revision, string CurrentKey, IReadOnlyList<NamedParallelSet> Sets);

internal sealed partial class CoreSession
{
    public Task<ParallelProfile> ReadParallelProfileAsync(string path) =>
        RequestAsync((out nint r) => Native.xc_worker_read_parallel_profile(worker, tab, path, out r), ReadParallelProfile);

    public Task<ParallelProfile> EditParallelProfileAsync(string path, string? revision, ParallelSetAction action,
        string key, string? name = null, IReadOnlyList<string>? modules = null) =>
        RequestAsync((out nint request) => WithModulePointers(modules ?? [],
            (nint pointers, nuint count, out nint result) =>
            {
                var edit = new Native.ParallelProfileEdit
                {
                    Action = (int)action,
                    Key = action == ParallelSetAction.Create ? EncodeSetName(name!) : key,
                    Replacement = action == ParallelSetAction.Rename ? EncodeSetName(name!) : null,
                    Modules = pointers,
                    ModuleCount = count
                };
                return Native.xc_worker_edit_parallel_profile_checked(worker, tab, path, revision, in edit, out result);
            }, out request), ReadParallelProfile);

    public Task<ParallelChapter> VisitNamedParallelSetAsync(string path, string key, string reference) =>
        RequestAsync((out nint r) => Native.xc_worker_visit_parallel_profile(worker, tab, path, key,
            reference, 0, 1, ParallelSnapshotBytes, out r), ReadParallel);

    private static string EncodeSetName(string name)
    {
        Check(Native.xc_parallel_set_encode_name(name, 0, 0, out var size));
        var buffer = Marshal.AllocHGlobal(checked((nint)size));
        try
        {
            Check(Native.xc_parallel_set_encode_name(name, buffer, size, out _));
            return Text(buffer);
        }
        finally { Marshal.FreeHGlobal(buffer); }
    }

    private static ParallelProfile ReadParallelProfile(nint request) =>
        ReadOwned(request, Native.xc_request_take_parallel_profile, Native.xc_parallel_profile_release, profile =>
        {
            var sets = new NamedParallelSet[checked((int)Native.xc_parallel_profile_count(profile))];
            for (var i = 0; i < sets.Length; i++)
            {
                var row = (nuint)i;
                var modules = new string[checked((int)Native.xc_parallel_profile_module_count(profile, row))];
                for (var j = 0; j < modules.Length; j++)
                    modules[j] = Text(Native.xc_parallel_profile_module(profile, row, (nuint)j));
                sets[i] = new NamedParallelSet(Text(Native.xc_parallel_profile_key(profile, row)),
                    Text(Native.xc_parallel_profile_display_name(profile, row)), modules);
            }
            return new ParallelProfile(Marshal.PtrToStringUTF8(Native.xc_parallel_profile_revision(profile)),
                Text(Native.xc_parallel_profile_current_key(profile)), sets);
        });
}
