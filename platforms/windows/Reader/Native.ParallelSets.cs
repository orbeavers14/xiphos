// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct ParallelProfileEdit
    {
        internal int Action;
        [MarshalAs(UnmanagedType.LPUTF8Str)] internal string Key;
        [MarshalAs(UnmanagedType.LPUTF8Str)] internal string? Replacement;
        internal nint Modules;
        internal nuint ModuleCount;
        internal int MakeCurrent;
    }

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_read_parallel_profile(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_edit_parallel_profile_checked(nint worker, ulong tab,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string path, [MarshalAs(UnmanagedType.LPUTF8Str)] string? revision,
        in ParallelProfileEdit edit, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_visit_parallel_profile(nint worker, ulong tab,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string path, [MarshalAs(UnmanagedType.LPUTF8Str)] string key,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string reference, uint flags, int headings, nuint maxBytes, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_parallel_set_encode_name([MarshalAs(UnmanagedType.LPUTF8Str)] string name, nint buffer, nuint capacity, out nuint required);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_parallel_profile(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_profile_revision(nint profile);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_profile_current_key(nint profile);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_parallel_profile_count(nint profile);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_parallel_profile_release(nint profile);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_profile_key(nint profile, nuint row);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_profile_display_name(nint profile, nuint row);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_parallel_profile_module_count(nint profile, nuint row);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_profile_module(nint profile, nuint row, nuint column);
}
