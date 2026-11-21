// SPDX-License-Identifier: GPL-2.0-or-later
namespace Xiphos.Reader;

internal sealed record DictionaryEntry(string Module, string Key, string Html);

internal sealed partial class CoreSession
{
    public Task<DictionaryEntry> VisitDictionaryAsync(string module, string key) =>
        RequestAsync((out nint r) => Native.xc_worker_visit_dictionary(worker, tab, module, key, out r), ReadDictionary);

    public Task<DictionaryEntry> MoveDictionaryKeyAsync(DictionaryEntry entry, int direction) =>
        RequestAsync((out nint r) => Native.xc_worker_navigate_dictionary(worker, tab, entry.Module, entry.Key,
            direction, out r), ReadDictionary);

    public Task<DictionaryEntry> MoveDictionaryHistoryAsync(int direction) =>
        RequestAsync((out nint r) => Native.xc_worker_navigate_dictionary_history(worker, tab, direction, out r), ReadDictionary);

    public Task<(bool Back, bool Forward)> DictionaryHistoryAsync() =>
        RequestAsync((out nint r) => Native.xc_worker_dictionary_history(worker, tab, out r),
            r => ReadOwned(r, Native.xc_request_take_dictionary_history, Native.xc_dictionary_history_release,
                history => (Native.xc_dictionary_history_can_back(history) != 0,
                    Native.xc_dictionary_history_can_forward(history) != 0)));

    public Task<IReadOnlyList<string>> DictionaryChoicesAsync(string module, string key) =>
        RequestAsync<IReadOnlyList<string>>((out nint r) => Native.xc_worker_dictionary_choices(worker, tab, module, key, out r),
            r => ReadOwned(r, Native.xc_request_take_dictionary_choices, Native.xc_dictionary_choices_release, choices =>
            {
                var keys = new string[checked((int)Native.xc_dictionary_choices_count(choices))];
                for (var i = 0; i < keys.Length; i++)
                    keys[i] = Text(Native.xc_dictionary_choices_at(choices, (nuint)i));
                return keys;
            }));

    private static DictionaryEntry ReadDictionary(nint request) =>
        ReadOwned(request, Native.xc_request_take_dictionary_entry, Native.xc_dictionary_entry_release, entry =>
            CopyDictionary(entry));

    private static DictionaryEntry CopyDictionary(nint entry) =>
        new(Text(Native.xc_dictionary_entry_module(entry)),
            Text(Native.xc_dictionary_entry_key(entry)), Text(Native.xc_dictionary_entry_html(entry)));
}
