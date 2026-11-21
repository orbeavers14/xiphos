// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_read_dictionary(nint worker, ulong tab,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string module, [MarshalAs(UnmanagedType.LPUTF8Str)] string key,
        out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_module_engine_options(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_request_engine_options(nint request, out uint flags, out int configured);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_enable_module_preferences(nint worker, ulong tab, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_set_module_engine_options(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module, uint flags, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_clear_module_engine_options(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_import_module_preferences(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_save_module_preferences(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_read_passage(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string commentary, [MarshalAs(UnmanagedType.LPUTF8Str)] string reference, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern uint xc_catalog_declared_engine_flags(nint catalog, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_catalog_transliteration_available(nint catalog, nuint index);
}
