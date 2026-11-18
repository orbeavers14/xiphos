// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal sealed record ParallelColumn(string Requested, string Module, bool RightToLeft, int State);
internal sealed record ParallelVerse(string Reference, string Html, IReadOnlyList<string> Headings);
internal sealed record ParallelRow(string Reference, uint Ordinal, IReadOnlyList<ParallelVerse>[] Cells);
internal sealed record ParallelChapter(string Reference, string Title, uint SelectedVerse,
    IReadOnlyList<ParallelColumn> Columns, IReadOnlyList<ParallelRow> Rows);

internal sealed partial class CoreSession
{
    public const int MaximumParallelColumns = 32;
    private const int ParallelSnapshotBytes = 16 * 1024 * 1024;
    private delegate int SubmitModules(nint modules, nuint count, out nint request);

    public Task<ParallelChapter> VisitParallelAsync(IReadOnlyList<string> modules, string reference) =>
        RequestAsync((out nint request) => WithModulePointers(modules,
            (nint pointers, nuint count, out nint result) => Native.xc_worker_visit_parallel(
                worker, tab, pointers, count, reference, 0, 1, ParallelSnapshotBytes, out result),
            out request), ReadParallel);

    private static int WithModulePointers(IReadOnlyList<string> modules, SubmitModules submit, out nint request)
    {
        if (modules.Count == 0)
            return submit(0, 0, out request);

        var strings = new nint[modules.Count];
        var pointers = Marshal.AllocHGlobal(checked(IntPtr.Size * strings.Length));
        try
        {
            for (var i = 0; i < strings.Length; i++)
                strings[i] = Marshal.StringToCoTaskMemUTF8(modules[i]);
            Marshal.Copy(strings, 0, pointers, strings.Length);
            return submit(pointers, (nuint)strings.Length, out request);
        }
        finally
        {
            foreach (var value in strings)
                Marshal.FreeCoTaskMem(value);
            Marshal.FreeHGlobal(pointers);
        }
    }

    public Task<ParallelChapter> MoveParallelChapterAsync(int direction) =>
        RequestAsync((out nint request) => Native.xc_worker_navigate_parallel_chapter(
            worker, tab, direction, out request), ReadParallel);

    private static ParallelChapter ReadParallel(nint request) =>
        ReadOwned(request, Native.xc_request_take_parallel_chapter, Native.xc_parallel_chapter_release, chapter =>
        {
            var columns = new ParallelColumn[checked((int)Native.xc_parallel_column_count(chapter))];
            for (var i = 0; i < columns.Length; i++)
            {
                var index = (nuint)i;
                columns[i] = new ParallelColumn(Text(Native.xc_parallel_column_requested(chapter, index)),
                    Text(Native.xc_parallel_column_module(chapter, index)),
                    Native.xc_parallel_column_is_rtl(chapter, index) != 0,
                    Native.xc_parallel_column_state(chapter, index));
            }

            var rows = new ParallelRow[checked((int)Native.xc_parallel_row_count(chapter))];
            for (var i = 0; i < rows.Length; i++)
            {
                var row = (nuint)i;
                var cells = new IReadOnlyList<ParallelVerse>[columns.Length];
                for (var j = 0; j < columns.Length; j++)
                    cells[j] = ReadParallelCell(chapter, row, (nuint)j);
                rows[i] = new ParallelRow(Text(Native.xc_parallel_row_reference(chapter, row)),
                    Native.xc_parallel_row_ordinal(chapter, row), cells);
            }

            return new ParallelChapter(Text(Native.xc_parallel_reference(chapter)),
                Text(Native.xc_parallel_title(chapter)), Native.xc_parallel_selected_verse(chapter), columns, rows);
        });

    private static IReadOnlyList<ParallelVerse> ReadParallelCell(nint chapter, nuint row, nuint column)
    {
        var verses = new List<ParallelVerse>();
        var reference = Text(Native.xc_parallel_cell_reference(chapter, row, column));
        if (reference.Length != 0)
        {
            var headings = new List<string>();
            var count = Native.xc_parallel_cell_heading_count(chapter, row, column);
            for (nuint i = 0; i < count; i++)
                headings.Add(Text(Native.xc_parallel_cell_heading(chapter, row, column, i)));
            verses.Add(new ParallelVerse(reference, Text(Native.xc_parallel_cell_html(chapter, row, column)), headings));
        }

        var additional = Native.xc_parallel_cell_additional_count(chapter, row, column);
        for (nuint part = 0; part < additional; part++)
        {
            var headings = new List<string>();
            var count = Native.xc_parallel_cell_additional_heading_count(chapter, row, column, part);
            for (nuint i = 0; i < count; i++)
                headings.Add(Text(Native.xc_parallel_cell_additional_heading(chapter, row, column, part, i)));
            verses.Add(new ParallelVerse(Text(Native.xc_parallel_cell_additional_reference(chapter, row, column, part)),
                Text(Native.xc_parallel_cell_additional_html(chapter, row, column, part)), headings));
        }
        return verses;
    }
}
