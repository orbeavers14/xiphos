// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_visit_parallel(nint worker, ulong tab, nint modules, nuint count,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string reference, uint engineFlags, int headings, nuint maxBytes, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_navigate_parallel_chapter(nint worker, ulong tab, int direction, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_parallel_chapter(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_reference(nint chapter);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_title(nint chapter);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern uint xc_parallel_selected_verse(nint chapter);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_parallel_column_count(nint chapter);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_parallel_row_count(nint chapter);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_parallel_chapter_release(nint chapter);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_parallel_column_state(nint chapter, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_column_requested(nint chapter, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_column_module(nint chapter, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_parallel_column_is_rtl(nint chapter, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_row_reference(nint chapter, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern uint xc_parallel_row_ordinal(nint chapter, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_cell_reference(nint chapter, nuint row, nuint column);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_cell_html(nint chapter, nuint row, nuint column);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_parallel_cell_heading_count(nint chapter, nuint row, nuint column);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_parallel_cell_additional_count(nint chapter, nuint row, nuint column);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_cell_heading(nint chapter, nuint row, nuint column, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_cell_additional_reference(nint chapter, nuint row, nuint column, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_cell_additional_html(nint chapter, nuint row, nuint column, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_parallel_cell_additional_heading_count(nint chapter, nuint row, nuint column, nuint index);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_parallel_cell_additional_heading(nint chapter, nuint row, nuint column, nuint part, nuint heading);
}
