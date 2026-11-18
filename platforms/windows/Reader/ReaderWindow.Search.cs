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
    private readonly TextBox searchQuery = new() { Header = "Search terms", MinWidth = 180 };
    private readonly CheckBox matchCase = new() { Content = "Match case" };
    private readonly Button startSearch = new() { Content = "Search" };
    private readonly Button stopSearch = new() { Content = "Stop" };
    private readonly Button openHit = new() { Content = "Open result" };
    private readonly ListView searchHits = new() { MaxHeight = 144, SelectionMode = ListViewSelectionMode.Single };
    private readonly TextBlock searchStatus = new() { TextWrapping = TextWrapping.Wrap };
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
        AutomationProperties.SetName(searchHits, "Search results");
        AutomationProperties.SetLiveSetting(searchStatus, AutomationLiveSetting.Polite);

        var panel = new StackPanel { Spacing = 6 };
        panel.Children.Add(Horizontal(searchModule, searchQuery, searchMode));
        panel.Children.Add(Horizontal(matchCase, startSearch, stopSearch, openHit));
        panel.Children.Add(searchStatus);
        panel.Children.Add(searchHits);
        var expander = new Expander { Header = "Search", Content = panel,
            HorizontalAlignment = HorizontalAlignment.Stretch, HorizontalContentAlignment = HorizontalAlignment.Stretch };

        startSearch.Click += async (_, _) => await RunAsync(SearchAsync);
        stopSearch.Click += (_, _) =>
        {
            searchCancellation?.Cancel();
            searchStatus.Text = "Stopping search…";
            UpdateControls();
        };
        openHit.Click += async (_, _) => await RunAsync(OpenHitAsync);
        searchHits.SelectionChanged += (_, _) => UpdateControls();
        searchModule.SelectionChanged += (_, _) => UpdateControls();
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
    }

    private void UpdateSearchControls(bool ready)
    {
        var hasModule = searchModule.SelectedItem is Module;
        searchModule.IsEnabled = ready && searchModule.Items.Count > 0;
        searchQuery.IsEnabled = searchMode.IsEnabled = matchCase.IsEnabled = ready && hasModule;
        startSearch.IsEnabled = ready && hasModule && !string.IsNullOrWhiteSpace(searchQuery.Text);
        stopSearch.IsEnabled = !closing && searchCancellation is { IsCancellationRequested: false };
        searchHits.IsEnabled = ready && latestResults != null;
        openHit.IsEnabled = searchHits.IsEnabled && searchHits.SelectedItem is string && bible.SelectedItem is Module;
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
        try
        {
            var result = await core.SearchAsync(module.Id, searchQuery.Text, choice.Mode,
                matchCase.IsChecked == true, cancellation.Token, percent =>
                {
                    if (!cancellation.IsCancellationRequested)
                        searchStatus.Text = $"Searching… {percent}%";
                });
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

    private async Task OpenHitAsync()
    {
        if (core == null || latestResults == null || searchHits.SelectedItem is not string hit ||
            bible.SelectedItem is not Module selectedBible)
            return;

        var selectedCommentary = (Module?)commentary.SelectedItem ?? NoCommentary;
        await ShowAsync(await core.VisitLinkAsync(selectedBible.Id, selectedCommentary.Id, latestResults.Module, hit));
    }
}
