// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct SearchGeneration
    {
        internal ulong Id;
        internal int Retention;
        internal nuint RequestedCount, GroupCount, FailedGroups, VisibleResults;
        internal ulong MatchedElements, RetainedElements;
        internal nuint AccountedKeyBytes;
        internal int Cancelled;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct VisibleRow
    {
        internal nuint Group, Row;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TextExportRequest
    {
        internal ulong Generation;
        internal nint Rows;
        internal nuint RowCount;
        internal nint Title;
        internal int IncludeScripture;
        internal nuint MaxBytes;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct AllTextExportRequest
    {
        internal ulong Generation;
        internal nint Title;
        internal int IncludeScripture;
        internal nuint MaxBytes, MaxRows;
        internal int AllowPartialSource;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct HtmlExportRequest
    {
        internal ulong Generation;
        internal int Selection;
        internal nint Rows;
        internal nuint RowCount;
        internal nint Title;
        internal int IncludeScripture;
        internal nuint MaxBytes, MaxRows;
        internal int AllowPartialSource;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TextExportInfo
    {
        internal int Disposition;
        internal nuint RowCount;
        internal int FailedRowPresent;
        internal nuint FailedRow;
        internal int IncludesScripture;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct HtmlExportInfo
    {
        internal int Disposition;
        internal nuint RowCount;
        internal int FailedRowPresent;
        internal nuint FailedRow;
        internal int IncludesScripture, Selection, Issue;
    }

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_search_generation_updated(nint result);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_search_get_generation(nint result, out SearchGeneration info);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_search_text_export_request_default(out TextExportRequest input);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_search_all_text_export_request_default(out AllTextExportRequest input);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_search_html_export_request_default(out HtmlExportRequest input);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_export_search_visible_text(nint worker, ulong tab,
        in TextExportRequest input, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_export_search_all_text(nint worker, ulong tab,
        in AllTextExportRequest input, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_export_search_html(nint worker, ulong tab,
        in HtmlExportRequest input, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_search_text_export(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_search_text_export_get_info(nint result, out TextExportInfo info);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_search_text_export_text(nint result);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_search_text_export_release(nint result);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_search_html_export(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_search_html_export_get_info(nint result, out HtmlExportInfo info);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_search_html_export_html(nint result);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_search_html_export_release(nint result);
}
