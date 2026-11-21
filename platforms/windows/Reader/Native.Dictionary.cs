// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_catalog_is_devotional(nint catalog, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_visit_dictionary(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string key, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_dictionary_choices(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string key, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_navigate_dictionary(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string key, int direction, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_navigate_dictionary_history(nint worker, ulong tab, int direction, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_dictionary_history(nint worker, ulong tab, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_dictionary_entry(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_dictionary_history(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_dictionary_choices(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_dictionary_entry_module(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_dictionary_entry_key(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_dictionary_entry_html(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_dictionary_entry_release(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_dictionary_history_can_back(nint history);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_dictionary_history_can_forward(nint history);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_dictionary_history_release(nint history);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_dictionary_choices_count(nint choices);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_dictionary_choices_release(nint choices);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_dictionary_choices_at(nint choices, nuint index);
}
