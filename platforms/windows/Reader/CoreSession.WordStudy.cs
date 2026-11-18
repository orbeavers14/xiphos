// SPDX-License-Identifier: GPL-2.0-or-later
namespace Xiphos.Reader;

internal sealed record WordStudy(string Module, string Reference, string Surface, IReadOnlyList<string> Lemmas);

internal sealed partial class CoreSession
{
    public Task<WordStudy> WordStudyAsync(string module, string reference, string surface) =>
        RequestAsync((out nint request) => Native.xc_worker_word_study(worker, tab, module, reference, surface, out request),
            request => ReadOwned(request, Native.xc_request_take_word_study, Native.xc_word_study_release, result =>
            {
                var terms = new string[checked((int)Native.xc_word_study_lemma_count(result))];
                for (var i = 0; i < terms.Length; i++)
                    terms[i] = Text(Native.xc_word_study_lemma(result, (nuint)i));
                return new WordStudy(Text(Native.xc_word_study_module(result)), Text(Native.xc_word_study_reference(result)),
                    Text(Native.xc_word_study_surface(result)), terms);
            }));
}
