// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_follow_book_pane_link_at(nint worker, ulong tab,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string mainModule, [MarshalAs(UnmanagedType.LPUTF8Str)] string mainReference,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string uri, out nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_book_uri_target(nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_uri_target_tree(nint target);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_uri_target_anchor(nint target);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_book_uri_target_release(nint target);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_tree_module(nint tree);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_book_image_data(nint request);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_image_data_source(nint image);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_image_data_bytes(nint image);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint xc_book_image_data_size(nint image);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_book_image_data_release(nint image);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_image_module(nint source);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_image_filename(nint source);
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_book_image_media_type(nint source);
}
