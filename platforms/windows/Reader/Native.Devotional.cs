// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_read_devotional(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string module,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string date, int todayMonth, int todayDay, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_devotional_entry(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_devotional_entry_module(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_devotional_entry_date(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_devotional_entry_matched_key(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_devotional_entry_html(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_devotional_entry_release(nint entry);
}
