// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal sealed record Module(string Id, string Title, string Type, bool Commentary, bool RightToLeft)
{
    public override string ToString() => Title == Id ? Id : $"{Title} ({Id})";
}

internal sealed record Passage(string Module, string Reference, string Html,
    string Commentary, string CommentaryReference, string CommentaryHtml);

internal enum SearchMode
{
    Regex = 0, Phrase = 1, Words = 2, Lemma = 3, Indexed = 7,
    SurfaceLiteral = 8, SurfaceUnmarked = 9
}

internal sealed record SearchResults(string Module, IReadOnlyList<string> References, ulong Total,
    bool Limited, bool Cancelled, bool IndexFallback, ulong Generation);

internal enum SearchScope { WholeModule, PassageRange, LastResults }

internal sealed record SearchRange(string Module, string Expression);

internal sealed partial class CoreSession : IAsyncDisposable
{
    private delegate int Submit(out nint request);
    private nint worker;
    private ulong tab;

    public CoreSession(string root)
    {
        Check(Native.xc_worker_open(root, out worker));
    }

    public async Task<IReadOnlyList<Module>> OpenAsync()
    {
        tab = await RequestAsync((out nint r) => Native.xc_worker_create_tab(worker, out r),
            Native.xc_request_tab);
        return await RequestAsync((out nint r) => Native.xc_worker_modules(worker, out r),
            request => ReadOwned(request, Native.xc_request_take_catalog, Native.xc_catalog_release, catalog =>
            {
                var modules = new List<Module>();
                for (nuint i = 0; i < Native.xc_catalog_count(catalog); ++i)
                {
                    var id = Text(Native.xc_catalog_id(catalog, i));
                    var title = Text(Native.xc_catalog_description(catalog, i));
                    modules.Add(new Module(id, title.Length == 0 ? id : title,
                        Text(Native.xc_catalog_type(catalog, i)),
                        Native.xc_catalog_is_regular_commentary(catalog, i) != 0,
                        Native.xc_catalog_is_rtl(catalog, i) != 0));
                }
                return modules;
            }));
    }

    public Task<Passage> VisitAsync(string module, string commentary, string reference) =>
        RequestAsync((out nint r) => Native.xc_worker_visit_passage(worker, tab, module, commentary, reference, out r), ReadPassage);

    public Task<(string Reference, string Html)> ReadEntryAsync(string module, string reference) =>
        RequestAsync((out nint r) => Native.xc_worker_read_tab_entry(worker, tab, module, reference, out r),
            request => ReadOwned(request, Native.xc_request_take_entry, Native.xc_entry_release,
                entry => (Text(Native.xc_entry_reference(entry)), Text(Native.xc_entry_html(entry)))));

    public Task<Passage> MoveHistoryAsync(int direction) =>
        RequestAsync((out nint r) => Native.xc_worker_navigate_passage(worker, tab, direction, out r), ReadPassage);

    public Task<Passage> MoveChapterAsync(Passage current, int direction) =>
        RequestAsync((out nint r) => Native.xc_worker_navigate_passage_chapter(worker, tab, current.Module,
            current.Commentary, current.Reference, direction, out r), ReadPassage);

    public Task<Passage> SelectCommentaryAsync(string commentary) =>
        RequestAsync((out nint r) => Native.xc_worker_select_commentary(worker, tab, commentary, out r), ReadPassage);

    public Task<SearchResults> SearchAsync(string module, string query, SearchMode mode,
        bool caseSensitive, CancellationToken cancellation, Action<uint> progress,
        SearchScope scope = SearchScope.WholeModule, SearchRange? range = null)
    {
        Native.xc_search_options_default(out var options);
        options.Mode = (int)mode;
        options.CaseSensitive = caseSensitive ? 1 : 0;
        options.MaxResults = 500;
        return RequestAsync((out nint r) => scope switch
            {
                SearchScope.PassageRange when range != null => Native.xc_worker_search_module_in_range(
                    worker, tab, module, query, range.Module, range.Expression, in options, out r),
                SearchScope.LastResults => Native.xc_worker_search_module_in_last_results(
                    worker, tab, module, query, in options, out r),
                SearchScope.WholeModule => Native.xc_worker_search_module(worker, tab, module, query, in options, out r),
                _ => throw new ArgumentException("A passage range is required.", nameof(range))
            },
            request => ReadOwned(request, Native.xc_request_take_search, Native.xc_search_release, result =>
            {
                Check(Native.xc_search_get_info(result, out var info));
                var references = new string[checked((int)info.ResultCount)];
                for (var i = 0; i < references.Length; i++)
                    references[i] = Text(Native.xc_search_reference(result, (nuint)i));
                var generation = Native.xc_search_generation_updated(result) == 1 &&
                    Native.xc_search_get_generation(result, out var published) == 0 ? published.Id : 0;
                return new SearchResults(Text(Native.xc_search_source_module(result)), references,
                    info.TotalResults, info.Limited != 0, info.Cancelled != 0,
                    Native.xc_search_index_fallback(result) == 1, generation);
            }),
            ObserveSearch(cancellation, progress));
    }

    private static Action<nint> ObserveSearch(CancellationToken cancellation, Action<uint> progress)
    {
        var cancelSent = false;
        return request =>
        {
            if (cancellation.IsCancellationRequested && !cancelSent)
            {
                Native.xc_request_cancel_search(request);
                cancelSent = true;
            }
            if (Native.xc_request_search_progress(request, out var state) == 0)
                progress(state.Percent);
        };
    }

    public Task<(bool Back, bool Forward)> HistoryAsync() =>
        RequestAsync((out nint r) => Native.xc_worker_history(worker, tab, out r),
            request => ReadOwned(request, Native.xc_request_take_history, Native.xc_history_release,
                history => (Native.xc_history_can_back(history) != 0, Native.xc_history_can_forward(history) != 0)));

    public async Task<Passage> FollowAsync(Passage current, string uri)
    {
        var target = await RequestAsync((out nint r) => Native.xc_worker_resolve_study_link_at(
            worker, tab, current.Module, current.Reference, uri, out r), ReadLink);
        return await VisitLinkAsync(current.Module, current.Commentary, target.Module, target.Reference);
    }

    public Task<Passage> VisitLinkAsync(string bible, string commentary, string module, string reference) =>
        RequestAsync((out nint r) => Native.xc_worker_visit_linked_passage(
            worker, tab, bible, commentary, module, reference, out r), ReadPassage);

    private static (string Module, string Reference) ReadLink(nint request)
    {
        var link = Native.xc_request_take_link(request);
        if (link != 0)
        {
            try { return (Text(Native.xc_link_module(link)), Text(Native.xc_link_reference(link))); }
            finally { Native.xc_link_release(link); }
        }

        var references = Native.xc_request_take_reference_list(request);
        if (references != 0)
        {
            try
            {
                if (Native.xc_reference_list_count(references) == 1 &&
                    Native.xc_reference_list_limited(references) == 0 &&
                    Native.xc_reference_list_skipped(references) == 0)
                    return (Text(Native.xc_reference_list_module(references)),
                        Text(Native.xc_reference_list_entry_reference(references, 0)));
            }
            finally { Native.xc_reference_list_release(references); }
        }

        throw new InvalidOperationException("This link needs a study view outside the reading preview.");
    }

    private static Passage ReadPassage(nint request) =>
        ReadOwned(request, Native.xc_request_take_passage, Native.xc_passage_release,
            passage => new Passage(Text(Native.xc_passage_module(passage)),
                Text(Native.xc_passage_reference(passage)), Text(Native.xc_passage_html(passage)),
                Text(Native.xc_passage_commentary_module(passage)),
                Text(Native.xc_passage_commentary_reference(passage)),
                Text(Native.xc_passage_commentary_heading_html(passage)) + Text(Native.xc_passage_commentary_html(passage))));

    private static T ReadOwned<T>(nint request, Func<nint, nint> take, Action<nint> release, Func<nint, T> read)
    {
        var result = take(request);
        if (result == 0)
            throw new InvalidOperationException("The requested result is unavailable in the reading preview.");
        try { return read(result); }
        finally { release(result); }
    }

    private static string Text(nint value) => Marshal.PtrToStringUTF8(value) ?? "";

    private static async Task<T> RequestAsync<T>(Submit submit, Func<nint, T> read, Action<nint>? pending = null)
    {
        Check(submit(out var request));
        if (request == 0)
            throw new InvalidOperationException("The core did not return a request.");

        try
        {
            int status;
            while ((status = Native.xc_request_poll(request)) == 4)
            {
                pending?.Invoke(request);
                await Task.Delay(pending == null ? 15 : 75);
            }
            Check(status);
            return read(request);
        }
        finally { Native.xc_request_release(request); }
    }

    private static void Check(int status)
    {
        if (status != 0)
            throw new InvalidOperationException(status switch
            {
                1 => "The shared core rejected the request.",
                2 => "The selected module is unavailable.",
                _ => $"The shared core could not complete the request ({status})."
            });
    }

    public async ValueTask DisposeAsync()
    {
        var handle = Interlocked.Exchange(ref worker, 0);
        if (handle != 0)
            await Task.Run(() => Native.xc_worker_close(handle));
    }
}
