// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct BookNode
    {
        internal ulong Offset;
        internal nint Path, Title;
        internal int Present, HasChildren;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LexicalPresentation
    {
        internal nint Css;
        internal int Requested, Applied;
    }

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_catalog_has_book_tree(nint catalog, nuint index);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_book_children(nint worker, [MarshalAs(UnmanagedType.LPUTF8Str)] string module,
        ulong parent, int hasAfter, ulong after, uint limit, out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_book_children(nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_book_children_count(nint page);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_book_children_parent(nint page, out BookNode node);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_book_children_get_node(nint page, nuint index, out BookNode node);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_book_children_next_after(nint page, out ulong offset);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_book_children_release(nint page);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_book_tree(nint worker, [MarshalAs(UnmanagedType.LPUTF8Str)] string module,
        ulong offset, out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_book_tree(nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_book_tree_get_node(nint tree, int kind, out BookNode node);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_book_tree_release(nint tree);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_select_book_document_path(nint worker, ulong tab,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string module, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_navigate_book_document(nint worker, ulong tab, int direction, out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_refresh_book_document(nint worker, ulong tab, out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_request_book_selection_present(nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_book_document(nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_document_tree(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_document_inline_html(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_document_presentation_html(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_document_stylesheet_inline_css(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_document_stylesheet_issue(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_book_document_lexical_layout(nint document, out LexicalPresentation presentation);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_book_document_inline_issue_count(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_document_inline_issue_message(nint document, nuint index);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern uint xc_book_document_display_level(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_book_document_count(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_book_document_release(nint document);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_book_document_note_link(nint worker, ulong tab,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string uri, int renderBody, out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_book_note(nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_note_content(nint note);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_book_note_release(nint note);
}
