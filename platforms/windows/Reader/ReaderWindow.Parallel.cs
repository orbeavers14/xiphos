// SPDX-License-Identifier: GPL-2.0-or-later
using System.Collections.ObjectModel;
using System.Net;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private readonly Button toggleParallel = new() { Content = "Parallel reading" };
    private readonly Grid parallelPane = new() { RowSpacing = 8, Visibility = Visibility.Collapsed };
    private readonly ComboBox parallelModule = new() { Header = "Bible to add", MinWidth = 160 };
    private readonly Button addParallel = new() { Content = "Add" };
    private readonly Button removeParallel = new() { Content = "Remove" };
    private readonly Button raiseParallel = new() { Content = "Move up" };
    private readonly Button lowerParallel = new() { Content = "Move down" };
    private readonly ObservableCollection<Module> orderedModules = [];
    private readonly ListView parallelModules = new() { MaxHeight = 104 };
    private readonly TextBox parallelReference = new() { Header = "Comparison passage", MinWidth = 160 };
    private readonly ComboBox parallelLayout = new() { Header = "Layout", MinWidth = 155 };
    private readonly Button compare = new() { Content = "Compare" };
    private readonly Button previousParallelChapter = new() { Content = "Previous comparison chapter" };
    private readonly Button nextParallelChapter = new() { Content = "Next comparison chapter" };
    private readonly Button previousParallelVerse = new() { Content = "Previous verse" };
    private readonly Button nextParallelVerse = new() { Content = "Next verse" };
    private readonly TextBlock parallelTitle = new() { FontSize = 21 };
    private readonly WebView2 parallelView = new();
    private ParallelChapter? parallelChapter;
    private int parallelRow;

    private Grid BuildParallelPane()
    {
        for (var i = 0; i < 6; i++)
            parallelPane.RowDefinitions.Add(new RowDefinition
            {
                Height = i == 5 ? new GridLength(1, GridUnitType.Star) : GridLength.Auto
            });
        AddRow(parallelPane, Horizontal(parallelModule, addParallel, removeParallel, raiseParallel, lowerParallel), 0);
        AddRow(parallelPane, parallelModules, 1);
        AddRow(parallelPane, Horizontal(parallelReference, parallelLayout, compare), 2);
        AddRow(parallelPane, BuildParallelSetsBar(), 3);
        AddRow(parallelPane, Horizontal(previousParallelChapter, nextParallelChapter, previousParallelVerse,
            nextParallelVerse, translationVisibility), 4);
        AddRow(parallelPane, MakePane(parallelTitle, parallelView, "Parallel passage"), 5);
        AutomationProperties.SetName(parallelModules, "Ordered Bibles");
        parallelModules.ItemsSource = orderedModules;
        parallelLayout.ItemsSource = new[] { "Side-by-side chapter", "Stacked verse" };
        parallelLayout.SelectedIndex = 0;
        foreach (var button in new[] { addParallel, removeParallel, raiseParallel, lowerParallel, compare })
            button.VerticalAlignment = VerticalAlignment.Bottom;

        toggleParallel.Click += (_, _) => SetParallelVisible(parallelPane.Visibility != Visibility.Visible);
        addParallel.Click += (_, _) =>
        {
            if (parallelModule.SelectedItem is Module module && !orderedModules.Contains(module))
            {
                orderedModules.Add(module);
                parallelModules.SelectedItem = module;
                UpdateControls();
            }
        };
        removeParallel.Click += (_, _) =>
        {
            if (parallelModules.SelectedIndex >= 0)
                orderedModules.RemoveAt(parallelModules.SelectedIndex);
            UpdateControls();
        };
        raiseParallel.Click += (_, _) => MoveParallelModule(-1);
        lowerParallel.Click += (_, _) => MoveParallelModule(1);
        parallelModule.SelectionChanged += (_, _) => UpdateControls();
        parallelModules.SelectionChanged += (_, _) => UpdateControls();
        parallelReference.TextChanged += (_, _) => UpdateControls();
        compare.Click += async (_, _) => await RunAsync(CompareAsync);
        parallelLayout.SelectionChanged += async (_, _) =>
        {
            if (parallelChapter != null)
                await RunAsync(RenderParallelAsync);
        };
        previousParallelChapter.Click += async (_, _) => await RunAsync(() => MoveParallelChapterAsync(-1));
        nextParallelChapter.Click += async (_, _) => await RunAsync(() => MoveParallelChapterAsync(1));
        previousParallelVerse.Click += async (_, _) => await RunAsync(() => MoveParallelVerseAsync(-1));
        nextParallelVerse.Click += async (_, _) => await RunAsync(() => MoveParallelVerseAsync(1));
        return parallelPane;
    }

    private void ResetParallel(IReadOnlyList<Module> available)
    {
        parallelChapter = null;
        parallelProfile = null;
        parallelSets.ItemsSource = null;
        parallelTitle.Text = "";
        parallelView.Visibility = Visibility.Collapsed;
        SetParallelVisible(false);
        orderedModules.Clear();
        foreach (var module in available.Take(2))
            orderedModules.Add(module);
        parallelModule.ItemsSource = available;
        parallelModule.SelectedIndex = available.Count == 0 ? -1 : 0;
        parallelReference.Text = "";
    }

    private void SetParallelVisible(bool visible)
    {
        if (visible)
            SetDictionaryVisible(false);
        parallelPane.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        UpdateReadingVisibility();
        toggleParallel.Content = visible ? "Back to reading" : "Parallel reading";
        if (visible && parallelReference.Text.Length == 0)
            parallelReference.Text = current?.Reference ?? reference.Text;
        UpdateControls();
    }

    private void UpdateParallelControls(bool ready)
    {
        var selected = parallelModules.SelectedIndex;
        var hasModules = parallelModule.Items.Count != 0;
        toggleParallel.IsEnabled = ready && hasModules;
        parallelModule.IsEnabled = parallelModules.IsEnabled = parallelReference.IsEnabled =
            parallelLayout.IsEnabled = ready && hasModules;
        addParallel.IsEnabled = ready && orderedModules.Count < 8 && parallelModule.SelectedItem is Module module &&
            !orderedModules.Contains(module);
        removeParallel.IsEnabled = ready && selected >= 0;
        raiseParallel.IsEnabled = ready && selected > 0;
        lowerParallel.IsEnabled = ready && selected >= 0 && selected < orderedModules.Count - 1;
        compare.IsEnabled = ready && orderedModules.Count is >= 2 and <= CoreSession.MaximumParallelColumns &&
            !string.IsNullOrWhiteSpace(parallelReference.Text);
        previousParallelChapter.IsEnabled = nextParallelChapter.IsEnabled = ready && parallelChapter != null;
        var stacked = ready && parallelChapter != null && parallelLayout.SelectedIndex == 1;
        previousParallelVerse.IsEnabled = stacked && parallelRow > 0;
        nextParallelVerse.IsEnabled = stacked && parallelRow + 1 < parallelChapter!.Rows.Count;
        UpdateParallelSetControls(ready, hasModules);
        translationVisibility.IsEnabled = ready && parallelChapter?.Columns.Count > 1;
    }

    private void MoveParallelModule(int direction)
    {
        var index = parallelModules.SelectedIndex;
        if (index < 0 || index + direction < 0 || index + direction >= orderedModules.Count)
            return;
        orderedModules.Move(index, index + direction);
        parallelModules.SelectedIndex = index + direction;
        UpdateControls();
    }

    private async Task CompareAsync()
    {
        var chapter = await core!.VisitParallelAsync(orderedModules.Select(module => module.Id).ToArray(),
            parallelReference.Text.Trim());
        await ShowParallelAsync(chapter);
    }

    private async Task MoveParallelChapterAsync(int direction) =>
        await ShowParallelAsync(await core!.MoveParallelChapterAsync(direction));

    private async Task ShowParallelAsync(ParallelChapter chapter)
    {
        parallelChapter = chapter;
        LoadColumnVisibility(chapter);
        parallelRow = 0;
        for (var i = 0; i < chapter.Rows.Count; i++)
        {
            if (chapter.Rows[i].Ordinal == chapter.SelectedVerse)
            {
                parallelRow = i;
                break;
            }
        }
        parallelReference.Text = chapter.Reference;
        await RenderParallelAsync();
    }

    private async Task MoveParallelVerseAsync(int direction)
    {
        parallelRow += direction;
        await RenderParallelAsync();
    }

    private async Task RenderParallelAsync()
    {
        var chapter = parallelChapter!;
        var stacked = parallelLayout.SelectedIndex == 1;
        var location = stacked && chapter.Rows.Count > 0 ? chapter.Rows[parallelRow].Reference : chapter.Title;
        parallelTitle.Text = location;
        parallelView.Visibility = Visibility.Visible;
        var studyDocument = new WordStudyDocument();
        await RenderAsync(parallelView, Document("Parallel reading", location,
            ParallelMarkup(chapter, stacked, studyDocument), wide: !stacked), studyDocument);
    }

    private string ParallelMarkup(ParallelChapter chapter, bool stacked, WordStudyDocument study)
    {
        var html = new StringBuilder("""
            <style>
              table { width:100%; border-collapse:collapse; }
              th, td { padding:.5rem; border:1px solid GrayText; vertical-align:top; }
              td { min-width:10rem; }
              th { text-align:start; }
              h2 { font-size:1.2em; margin:.7rem 0 .25rem; }
              section { margin-bottom:1rem; }
              tr[aria-current='true'] > th { background:Highlight; color:HighlightText; }
              .reference { margin-inline-end:.4em; font-size:.85em; }
            </style>
            """);
        if (chapter.Rows.Count == 0)
            return html.Append("<p>No verses in this chapter.</p>").ToString();

        if (stacked)
        {
            var row = chapter.Rows[parallelRow];
            foreach (var i in VisibleColumns(chapter))
            {
                var column = chapter.Columns[i];
                html.Append("<section><h2>").Append(WebUtility.HtmlEncode(ColumnTitle(column))).Append("</h2>")
                    .Append("<div dir='").Append(column.RightToLeft ? "rtl" : "ltr").Append("'>");
                AppendParallelCell(html, row.Cells[i], column.Module, study);
                html.Append("</div></section>");
            }
        }
        else
        {
            html.Append("<table><thead><tr><th scope='col'>Verse</th>");
            var visible = VisibleColumns(chapter).ToArray();
            foreach (var i in visible)
                html.Append("<th scope='col'>").Append(WebUtility.HtmlEncode(ColumnTitle(chapter.Columns[i]))).Append("</th>");
            html.Append("</tr></thead><tbody>");
            foreach (var row in chapter.Rows)
            {
                html.Append(row.Ordinal == chapter.SelectedVerse ? "<tr aria-current='true'>" : "<tr>")
                    .Append("<th scope='row'>").Append(WebUtility.HtmlEncode(row.Reference)).Append("</th>");
                foreach (var i in visible)
                {
                    html.Append("<td dir='").Append(chapter.Columns[i].RightToLeft ? "rtl" : "ltr").Append("'>");
                    AppendParallelCell(html, row.Cells[i], chapter.Columns[i].Module, study);
                    html.Append("</td>");
                }
                html.Append("</tr>");
            }
            html.Append("</tbody></table>");
        }
        return html.ToString();
    }

    private static string ColumnTitle(ParallelColumn column) => column.State == 0
        ? column.Module : $"{column.Requested} (unavailable)";

    private static void AppendParallelCell(StringBuilder html, IReadOnlyList<ParallelVerse> verses,
        string module, WordStudyDocument study)
    {
        if (verses.Count == 0)
            html.Append("<span>Unavailable</span>");
        foreach (var verse in verses)
        {
            foreach (var heading in verse.Headings)
                html.Append("<div>").Append(heading).Append("</div>");
            html.Append("<div><span class='reference'>").Append(WebUtility.HtmlEncode(verse.Reference))
                .Append("</span>").Append(study.Mark(module, verse.Reference, verse.Html)).Append("</div>");
        }
    }
}
