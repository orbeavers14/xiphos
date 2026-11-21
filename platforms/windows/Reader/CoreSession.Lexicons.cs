// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal sealed partial class CoreSession
{
    public Task<Native.LexiconNames> LexiconsAsync() =>
        RequestAsync((out nint r) => Native.xc_worker_lexicon_preferences(worker, tab, out r), ReadLexicons);

    public async Task<Native.LexiconNames> SetLexiconsAsync(Native.LexiconNames names)
    {
        await RequestAsync((out nint r) => Native.xc_worker_set_lexicon_preferences(worker, tab, in names, out r), _ => true);
        return await LexiconsAsync();
    }

    public Task<Native.LexiconNames> ImportLexiconsAsync(string path) =>
        RequestAsync((out nint r) => Native.xc_worker_import_lexicon_profile(worker, tab, path, out r), ReadLexicons);

    public Task SaveLexiconsAsync(string path) =>
        RequestAsync((out nint r) => Native.xc_worker_save_lexicon_profile(worker, tab, path, out r), _ => true);

    private static Native.LexiconNames ReadLexicons(nint request) =>
        ReadOwned(request, Native.xc_request_take_lexicons, Native.xc_lexicons_release, value => new Native.LexiconNames
        {
            Greek = Marshal.PtrToStringUTF8(Native.xc_lexicons_greek(value)),
            Hebrew = Marshal.PtrToStringUTF8(Native.xc_lexicons_hebrew(value)),
            GreekMorphNt = Marshal.PtrToStringUTF8(Native.xc_lexicons_greek_morph_nt(value)),
            GreekMorphOt = Marshal.PtrToStringUTF8(Native.xc_lexicons_greek_morph_ot(value)),
            HebrewMorph = Marshal.PtrToStringUTF8(Native.xc_lexicons_hebrew_morph(value))
        });
}
