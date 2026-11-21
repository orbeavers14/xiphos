// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_note(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_note_module(nint note);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_note_reference(nint note);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_note_label(nint note);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_note_html(nint note);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_note_release(nint note);
}
