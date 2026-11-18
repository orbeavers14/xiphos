// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private sealed record SearchChoice(string Label, SearchMode Mode)
    {
        public override string ToString() => Label;
    }

    private readonly ComboBox searchModule = new() { Header = "Search module", MinWidth = 185 };
    private readonly ComboBox searchMode = new() { Header = "Match", MinWidth = 140 };
    private readonly ComboBox searchScope = new() { Header = "Search scope", MinWidth = 168 };
    private readonly ComboBox rangeBible = new() { Header = "Range Bible", MinWidth = 185 };
    private readonly TextBox searchRange = new() { Header = "Passage range", PlaceholderText = "John 3:1-21", MinWidth = 180 };
    private readonly TextBox searchQuery = new() { Header = "Search terms", MinWidth = 180 };
    private readonly CheckBox matchCase = new() { Content = "Match case" };
    private readonly Button startSearch = new() { Content = "Search" };
    private readonly Button stopSearch = new() { Content = "Stop" };
    private readonly Button previewHit = new() { Content = "Preview result" };
    private readonly Button openHit = new() { Content = "Open result" };
    private readonly DropDownButton exportResults = new() { Content = "Export" };
    private readonly MenuFlyoutItem saveSelectedText = new() { Text = "Save selected as text…" };
    private readonly MenuFlyoutItem saveSelectedHtml = new() { Text = "Save selected as HTML…" };
    private readonly MenuFlyoutItem copySelectedText = new() { Text = "Copy selected as text" };
    private readonly ListView searchHits = new() { MaxHeight = 144, SelectionMode = ListViewSelectionMode.Single };
    private readonly TextBlock searchStatus = new() { TextWrapping = TextWrapping.Wrap };
    private readonly TextBlock searchPreviewTitle = new();
    private readonly WebView2 searchPreviewView = new();
    private readonly Grid searchPreviewPane = new() { Visibility = Visibility.Collapsed };
    private StackPanel? rangeControls;
    private CancellationTokenSource? searchCancellation;
    private SearchResults? latestResults;

    private Expander BuildSearchPane()
    {
        searchMode.ItemsSource = new SearchChoice[]
        {
            new("All words", SearchMode.Words),
            new("Exact phrase", SearchMode.Phrase),
            new("Regular expression", SearchMode.Regex),
            new("Indexed", SearchMode.Indexed),
            new("Lemma / Strong’s", SearchMode.Lemma),
            new("Exact surface form", SearchMode.SurfaceLiteral),
            new("Surface form, ignore marks", SearchMode.SurfaceUnmarked)
        };
        searchMode.SelectedIndex = 0;
        searchScope.ItemsSource = new[] { "Whole module", "Passage range", "Last results" };
        searchScope.SelectedIndex = (int)SearchScope.WholeModule;
        AutomationProperties.SetName(searchHits, "Search results");
        AutomationProperties.SetLiveSetting(searchStatus, AutomationLiveSetting.Polite);

        var panel = new StackPanel { Spacing = 6 };
        panel.Children.Add(Horizontal(searchModule, searchQuery, searchMode));
        panel.Children.Add(Horizontal(searchScope, matchCase));
        rangeControls = Horizontal(rangeBible, searchRange);
        rangeControls.Visibility = Visibility.Collapsed;
        panel.Children.Add(rangeControls);
        AddSearchExportMenu();
        panel.Children.Add(Horizontal(startSearch, stopSearch, previewHit, openHit, exportResults));
        panel.Children.Add(searchStatus);
        var results = new Grid { ColumnSpacing = 12, Height = 144, Visibility = Visibility.Collapsed };
        results.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        results.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(2, GridUnitType.Star) });
        results.Children.Add(searchHits);
        PopulatePane(searchPreviewPane, searchPreviewTitle, searchPreviewView, "Search result preview");
        Grid.SetColumn(searchPreviewPane, 1);
        results.Children.Add(searchPreviewPane);
        panel.Children.Add(results);
        var expander = new Expander { Header = "Search", Content = panel,
            HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };

        startSearch.Click += async (_, _) => await RunAsync(SearchAsync);
        stopSearch.Click += (_, _) =>
        {
            searchCancellation?.Cancel();
            searchStatus.Text = "Stopping search…";
            UpdateControls();
        };
        previewHit.Click += async (_, _) => await RunAsync(PreviewHitAsync);
        openHit.Click += async (_, _) => await RunAsync(OpenHitAsync);
        searchHits.SelectionChanged += (_, _) =>
        {
            searchPreviewPane.Visibility = Visibility.Collapsed;
            UpdateControls();
        };
        searchHits.RegisterPropertyChangedCallback(ItemsControl.ItemsSourceProperty, (_, _) =>
            results.Visibility = latestResults is { References.Count: > 0 } ? Visibility.Visible : Visibility.Collapsed);
        searchModule.SelectionChanged += (_, _) => UpdateControls();
        searchScope.SelectionChanged += (_, _) => UpdateControls();
        rangeBible.SelectionChanged += (_, _) => UpdateControls();
        searchRange.TextChanged += (_, _) => UpdateControls();
        searchQuery.TextChanged += (_, _) => UpdateControls();
        searchQuery.KeyDown += async (_, e) =>
        {
            if (e.Key == Windows.System.VirtualKey.Enter && startSearch.IsEnabled)
            {
                e.Handled = true;
                await RunAsync(SearchAsync);
            }
        };
        return expander;
    }

    private void SetSearchModules(IReadOnlyList<Module> available)
    {
        latestResults = null;
        searchHits.ItemsSource = null;
        searchStatus.Text = "";
        searchModule.ItemsSource = available;
        searchModule.SelectedItem = available.FirstOrDefault(module => !module.Commentary) ?? available.FirstOrDefault();
        rangeBible.ItemsSource = available.Where(module => !module.Commentary).ToArray();
        rangeBible.SelectedIndex = rangeBible.Items.Count == 0 ? -1 : 0;
        searchScope.SelectedIndex = (int)SearchScope.WholeModule;
    }

    private void UpdateSearchControls(bool ready)
    {
        var hasModule = searchModule.SelectedItem is Module;
        searchModule.IsEnabled = ready && searchModule.Items.Count > 0;
        searchQuery.IsEnabled = searchMode.IsEnabled = matchCase.IsEnabled = searchScope.IsEnabled = ready && hasModule;
        var usesRange = searchScope.SelectedIndex == (int)SearchScope.PassageRange;
        if (rangeControls != null)
            rangeControls.Visibility = usesRange ? Visibility.Visible : Visibility.Collapsed;
        rangeBible.IsEnabled = searchRange.IsEnabled = ready && hasModule && usesRange;
        startSearch.IsEnabled = ready && hasModule && !string.IsNullOrWhiteSpace(searchQuery.Text) &&
            (!usesRange || (rangeBible.SelectedItem is Module && !string.IsNullOrWhiteSpace(searchRange.Text)));
        stopSearch.IsEnabled = !closing && searchCancellation is { IsCancellationRequested: false };
        searchHits.IsEnabled = ready && latestResults != null;
        previewHit.IsEnabled = searchHits.IsEnabled && searchHits.SelectedItem is string;
        openHit.IsEnabled = previewHit.IsEnabled && bible.SelectedItem is Module;
        exportResults.IsEnabled = ready && latestResults is { Generation: > 0, References.Count: > 0 };
        saveSelectedText.IsEnabled = saveSelectedHtml.IsEnabled = copySelectedText.IsEnabled = previewHit.IsEnabled;
    }

    private async Task SearchAsync()
    {
        if (core == null || searchModule.SelectedItem is not Module module ||
            searchMode.SelectedItem is not SearchChoice choice)
            return;

        using var cancellation = new CancellationTokenSource();
        searchCancellation = cancellation;
        latestResults = null;
        searchHits.ItemsSource = null;
        searchStatus.Text = "Searching…";
        UpdateControls();
        var scope = (SearchScope)searchScope.SelectedIndex;
        var range = scope == SearchScope.PassageRange && rangeBible.SelectedItem is Module rangeModule
            ? new SearchRange(rangeModule.Id, searchRange.Text.Trim()) : null;
        try
        {
            var result = await core.SearchAsync(module.Id, searchQuery.Text, choice.Mode,
                matchCase.IsChecked == true, cancellation.Token, percent =>
                {
                    if (!cancellation.IsCancellationRequested)
                        searchStatus.Text = $"Searching… {percent}%";
                }, scope, range);
            latestResults = result;
            searchHits.ItemsSource = result.References;
            var count = result.References.Count;
            var summary = result.Limited ? $"Showing {count:N0} of {result.Total:N0} matches" :
                count == 1 ? "1 match" : $"{count:N0} matches";
            searchStatus.Text = (result.Cancelled ? "Search stopped. " : "") + summary + "." +
                (result.IndexFallback ? " This module has no index. The engine used an unindexed search." : "");
        }
        catch (Exception error)
        {
            searchStatus.Text = error.Message;
        }
        finally
        {
            searchCancellation = null;
        }
    }

    private async Task PreviewHitAsync()
    {
        if (core == null || latestResults == null || searchHits.SelectedItem is not string hit)
            return;

        var source = latestResults.Module;
        var preview = await core.ReadEntryAsync(source, hit);
        searchPreviewTitle.Text = $"{source} · {preview.Reference}";
        searchPreviewPane.Visibility = Visibility.Visible;
        await RenderAsync(searchPreviewView, Document(source, preview.Reference, preview.Html));
    }

    private async Task OpenHitAsync()
    {
        if (core == null || latestResults == null || searchHits.SelectedItem is not string hit ||
            bible.SelectedItem is not Module selectedBible)
            return;

        var selectedCommentary = (Module?)commentary.SelectedItem ?? NoCommentary;
        await ShowAsync(await core.VisitLinkAsync(selectedBible.Id, selectedCommentary.Id, latestResults.Module, hit));
    }
}
