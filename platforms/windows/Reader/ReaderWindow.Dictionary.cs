// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.System;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private readonly Button toggleDictionary = new() { Content = "Dictionary" };
    private readonly Grid dictionaryPane = new() { RowSpacing = 8, Visibility = Visibility.Collapsed };
    private readonly ComboBox dictionaryModule = new() { Header = "Dictionary module", MinWidth = 170 };
    private readonly TextBox dictionaryKey = new() { Header = "Entry key", MinWidth = 160 };
    private readonly Button dictionaryLookup = new() { Content = "Look up", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly Button dictionaryNearby = new() { Content = "Nearby keys", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly ComboBox nearbyKeys = new() { Header = "Nearby entries", MinWidth = 170 };
    private readonly Button openNearbyKey = new() { Content = "Open key", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly Button previousDictionaryKey = new() { Content = "Previous key" };
    private readonly Button nextDictionaryKey = new() { Content = "Next key" };
    private readonly Button dictionaryBack = new() { Content = "Dictionary back" };
    private readonly Button dictionaryForward = new() { Content = "Dictionary forward" };
    private readonly TextBlock dictionaryTitle = new() { FontSize = 21 };
    private readonly WebView2 dictionaryView = new();
    private StackPanel nearbyRow = null!;
    private DictionaryEntry? dictionaryEntry;
    private (bool Back, bool Forward) dictionaryHistory;

    private Grid BuildDictionaryPane()
    {
        for (var i = 0; i < 4; i++)
            dictionaryPane.RowDefinitions.Add(new RowDefinition
            {
                Height = i == 3 ? new GridLength(1, GridUnitType.Star) : GridLength.Auto
            });
        AddRow(dictionaryPane, Horizontal(dictionaryModule, dictionaryKey, dictionaryLookup, dictionaryNearby), 0);
        nearbyRow = Horizontal(nearbyKeys, openNearbyKey);
        nearbyRow.Visibility = Visibility.Collapsed;
        AddRow(dictionaryPane, nearbyRow, 1);
        AddRow(dictionaryPane, Horizontal(previousDictionaryKey, nextDictionaryKey, dictionaryBack, dictionaryForward), 2);
        AddRow(dictionaryPane, MakePane(dictionaryTitle, dictionaryView, "Dictionary entry"), 3);
        dictionaryView.Visibility = Visibility.Collapsed;

        toggleDictionary.Click += (_, _) => SetDictionaryVisible(dictionaryPane.Visibility != Visibility.Visible);
        dictionaryModule.SelectionChanged += (_, _) => ClearNearbyDictionaryKeys();
        dictionaryKey.TextChanged += (_, _) => ClearNearbyDictionaryKeys();
        nearbyKeys.SelectionChanged += (_, _) => UpdateControls();
        dictionaryLookup.Click += async (_, _) => await RunAsync(() => VisitDictionaryAsync(dictionaryKey.Text));
        dictionaryKey.KeyDown += async (_, args) =>
        {
            if (args.Key == VirtualKey.Enter && dictionaryLookup.IsEnabled)
            {
                args.Handled = true;
                await RunAsync(() => VisitDictionaryAsync(dictionaryKey.Text));
            }
        };
        dictionaryNearby.Click += async (_, _) => await RunAsync(LoadNearbyDictionaryKeysAsync);
        openNearbyKey.Click += async (_, _) => await RunAsync(() => VisitDictionaryAsync((string)nearbyKeys.SelectedItem));
        previousDictionaryKey.Click += async (_, _) => await RunAsync(async () =>
            await ShowDictionaryAsync(await core!.MoveDictionaryKeyAsync(dictionaryEntry!, -1)));
        nextDictionaryKey.Click += async (_, _) => await RunAsync(async () =>
            await ShowDictionaryAsync(await core!.MoveDictionaryKeyAsync(dictionaryEntry!, 1)));
        dictionaryBack.Click += async (_, _) => await RunAsync(async () =>
            await ShowDictionaryAsync(await core!.MoveDictionaryHistoryAsync(-1)));
        dictionaryForward.Click += async (_, _) => await RunAsync(async () =>
            await ShowDictionaryAsync(await core!.MoveDictionaryHistoryAsync(1)));
        return dictionaryPane;
    }

    private void ResetDictionaries(IReadOnlyList<Module> available)
    {
        dictionaryEntry = null;
        dictionaryHistory = default;
        dictionaryTitle.Text = "";
        dictionaryView.Visibility = Visibility.Collapsed;
        SetDictionaryVisible(false);
        dictionaryModule.ItemsSource = available;
        dictionaryModule.SelectedIndex = available.Count == 0 ? -1 : 0;
        dictionaryKey.Text = "";
        ClearNearbyDictionaryKeys();
    }

    private void SetDictionaryVisible(bool visible)
    {
        if (visible)
            SetParallelVisible(false);
        dictionaryPane.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        toggleDictionary.Content = visible ? "Close dictionary" : "Dictionary";
        UpdateReadingVisibility();
        UpdateControls();
    }

    private void UpdateReadingVisibility() =>
        reading.Visibility = current != null && parallelPane.Visibility != Visibility.Visible &&
            dictionaryPane.Visibility != Visibility.Visible ? Visibility.Visible : Visibility.Collapsed;

    private void ClearNearbyDictionaryKeys()
    {
        nearbyKeys.ItemsSource = null;
        nearbyRow.Visibility = Visibility.Collapsed;
        UpdateControls();
    }

    private void UpdateDictionaryControls(bool ready)
    {
        var module = dictionaryModule.SelectedItem as Module;
        toggleDictionary.IsEnabled = ready && dictionaryModule.Items.Count > 0;
        dictionaryModule.IsEnabled = toggleDictionary.IsEnabled;
        dictionaryKey.IsEnabled = ready && module != null;
        dictionaryLookup.IsEnabled = dictionaryNearby.IsEnabled = dictionaryKey.IsEnabled &&
            !string.IsNullOrWhiteSpace(dictionaryKey.Text);
        nearbyKeys.IsEnabled = ready && nearbyKeys.Items.Count > 0;
        openNearbyKey.IsEnabled = nearbyKeys.IsEnabled && nearbyKeys.SelectedItem is string;
        previousDictionaryKey.IsEnabled = nextDictionaryKey.IsEnabled = ready && dictionaryEntry != null &&
            module?.Id == dictionaryEntry.Module;
        dictionaryBack.IsEnabled = ready && dictionaryHistory.Back;
        dictionaryForward.IsEnabled = ready && dictionaryHistory.Forward;
    }

    private async Task LoadNearbyDictionaryKeysAsync()
    {
        var module = (Module)dictionaryModule.SelectedItem;
        var keys = (await core!.DictionaryChoicesAsync(module.Id, dictionaryKey.Text)).ToArray();
        nearbyKeys.ItemsSource = keys;
        nearbyKeys.SelectedIndex = Array.IndexOf(keys, dictionaryKey.Text);
        if (nearbyKeys.SelectedIndex < 0 && keys.Length > 0)
            nearbyKeys.SelectedIndex = 0;
        nearbyRow.Visibility = keys.Length == 0 ? Visibility.Collapsed : Visibility.Visible;
    }

    private async Task VisitDictionaryAsync(string key)
    {
        var module = (Module)dictionaryModule.SelectedItem;
        await ShowDictionaryAsync(await core!.VisitDictionaryAsync(module.Id, key));
    }

    private async Task ShowDictionaryAsync(DictionaryEntry entry)
    {
        dictionaryEntry = entry;
        dictionaryModule.SelectedItem = modules.FirstOrDefault(module => module.Id == entry.Module);
        dictionaryKey.Text = entry.Key;
        ClearNearbyDictionaryKeys();
        dictionaryHistory = await core!.DictionaryHistoryAsync();
        dictionaryTitle.Text = $"{entry.Module} · {entry.Key}";
        dictionaryView.Visibility = Visibility.Visible;
        await RenderAsync(dictionaryView, Document(entry.Module, entry.Key, entry.Html));
    }
}
