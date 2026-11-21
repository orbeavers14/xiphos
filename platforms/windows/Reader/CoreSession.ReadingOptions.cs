// SPDX-License-Identifier: GPL-2.0-or-later
namespace Xiphos.Reader;

internal sealed record ReadingOptions(uint Flags, bool Configured);

internal sealed partial class CoreSession
{
    public Task<DictionaryEntry> RefreshDictionaryAsync(DictionaryEntry entry) =>
        RequestAsync((out nint request) => Native.xc_worker_read_dictionary(worker, tab, entry.Module,
            entry.Key, out request), ReadDictionary);
    public Task<ReadingOptions> ReadingOptionsAsync(string module) =>
        RequestAsync((out nint request) => Native.xc_worker_module_engine_options(worker, tab, module, out request),
            request =>
            {
                Check(Native.xc_request_engine_options(request, out var flags, out var configured));
                return new ReadingOptions(flags, configured != 0);
            });

    public Task EnableReadingDefaultsAsync() =>
        RequestAsync((out nint request) => Native.xc_worker_enable_module_preferences(worker, tab, out request), _ => true);

    public Task SetReadingOptionsAsync(string module, uint flags) =>
        RequestAsync((out nint request) => Native.xc_worker_set_module_engine_options(worker, tab, module, flags,
            out request), _ => true);

    public Task ResetReadingOptionsAsync(string module) =>
        RequestAsync((out nint request) => Native.xc_worker_clear_module_engine_options(worker, tab, module,
            out request), _ => true);

    public Task ImportReadingOptionsAsync(string path) =>
        RequestAsync((out nint request) => Native.xc_worker_import_module_preferences(worker, tab, path, out request), _ => true);

    public Task SaveReadingOptionsAsync(string path) =>
        RequestAsync((out nint request) => Native.xc_worker_save_module_preferences(worker, tab, path, out request), _ => true);

    public Task<Passage> RefreshPassageAsync(Passage passage) =>
        RequestAsync((out nint request) => Native.xc_worker_read_passage(worker, tab, passage.Module,
            passage.Commentary, passage.Reference, out request), ReadPassage);
}
