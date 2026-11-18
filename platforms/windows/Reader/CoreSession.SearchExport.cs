// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal sealed partial class CoreSession
{
    public Task<string?> ExportSearchAsync(ulong generation, int? selectedRow, bool html,
        CancellationToken cancellation, Action<uint> progress)
    {
        return RequestAsync<string?>((out nint request) =>
        {
            // The export APIs copy the row before returning from submission.
            var row = selectedRow.HasValue ? Marshal.AllocHGlobal(Marshal.SizeOf<Native.VisibleRow>()) : 0;
            try
            {
                if (selectedRow is int index)
                    Marshal.StructureToPtr(new Native.VisibleRow { Group = 0, Row = checked((nuint)index) }, row, false);
                if (html)
                {
                    Native.xc_search_html_export_request_default(out var input);
                    input.Generation = generation;
                    input.IncludeScripture = 1;
                    input.Selection = selectedRow.HasValue ? 0 : 1;
                    input.Rows = row;
                    input.RowCount = selectedRow.HasValue ? 1u : 0u;
                    return Native.xc_worker_export_search_html(worker, tab, in input, out request);
                }
                if (selectedRow.HasValue)
                {
                    Native.xc_search_text_export_request_default(out var input);
                    input.Generation = generation;
                    input.IncludeScripture = 1;
                    input.Rows = row;
                    input.RowCount = 1;
                    return Native.xc_worker_export_search_visible_text(worker, tab, in input, out request);
                }

                Native.xc_search_all_text_export_request_default(out var all);
                all.Generation = generation;
                all.IncludeScripture = 1;
                return Native.xc_worker_export_search_all_text(worker, tab, in all, out request);
            }
            finally
            {
                if (row != 0)
                    Marshal.FreeHGlobal(row);
            }
        }, html ? ReadHtmlExport : ReadTextExport, ObserveSearch(cancellation, progress));
    }

    private static string? ReadTextExport(nint request) =>
        ReadOwned(request, Native.xc_request_take_search_text_export, Native.xc_search_text_export_release, result =>
        {
            Check(Native.xc_search_text_export_get_info(result, out var info));
            return ExportContent(info.Disposition, Native.xc_search_text_export_text(result));
        });

    private static string? ReadHtmlExport(nint request) =>
        ReadOwned(request, Native.xc_request_take_search_html_export, Native.xc_search_html_export_release, result =>
        {
            Check(Native.xc_search_html_export_get_info(result, out var info));
            return ExportContent(info.Disposition, Native.xc_search_html_export_html(result));
        });

    private static string? ExportContent(int disposition, nint content) => disposition switch
    {
        0 => Text(content),
        4 => null,
        1 => throw new InvalidOperationException("Run the search again to export its results."),
        3 or 7 => throw new InvalidOperationException("The export is too large. Select one result or narrow the search."),
        8 => throw new InvalidOperationException("Run the search to completion before exporting all results."),
        9 => throw new InvalidOperationException("These results contain content that cannot be exported as HTML. Try plain text."),
        _ => throw new InvalidOperationException($"The shared core could not export the results ({disposition}).")
    };
}
