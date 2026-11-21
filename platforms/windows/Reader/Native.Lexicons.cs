// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal static partial class Native
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct LexiconNames
    {
        [MarshalAs(UnmanagedType.LPUTF8Str)] internal string? Greek;
        [MarshalAs(UnmanagedType.LPUTF8Str)] internal string? Hebrew;
        [MarshalAs(UnmanagedType.LPUTF8Str)] internal string? GreekMorphNt;
        [MarshalAs(UnmanagedType.LPUTF8Str)] internal string? GreekMorphOt;
        [MarshalAs(UnmanagedType.LPUTF8Str)] internal string? HebrewMorph;
    }

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_set_lexicon_preferences(nint worker, ulong tab, in LexiconNames names, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_lexicon_preferences(nint worker, ulong tab, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_import_lexicon_profile(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int xc_worker_save_lexicon_profile(nint worker, ulong tab, [MarshalAs(UnmanagedType.LPUTF8Str)] string path, out nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_lexicons(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexicons_greek(nint lexicons);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexicons_hebrew(nint lexicons);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexicons_greek_morph_nt(nint lexicons);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexicons_greek_morph_ot(nint lexicons);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexicons_hebrew_morph(nint lexicons);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_lexicons_release(nint lexicons);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_request_take_lexical_entry(nint request);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexical_entry_dictionary_module(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexical_entry_dictionary_key(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern nint xc_lexical_entry_dictionary_html(nint entry);

    [DllImport(Library, ExactSpelling = true, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void xc_lexical_entry_release(nint entry);
}
