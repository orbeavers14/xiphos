// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private readonly ComboBox devotionalModule = new() { Header = "Daily devotional module", MinWidth = 170 };
    private readonly CalendarDatePicker devotionalDate = new() { Date = DateTimeOffset.Now };
    private readonly Button readDevotional = new() { Content = "Read devotional" };
    private readonly Button previousDevotional = new() { Content = "Previous day" };
    private readonly Button nextDevotional = new() { Content = "Next day" };
    private bool devotionalActive;

    private Expander BuildDevotionalControls()
    {
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(devotionalDate, "Devotional date");
        readDevotional.Click += async (_, _) => await RunAsync(ReadDevotionalAsync);
        previousDevotional.Click += async (_, _) => await RunAsync(() => MoveDevotionalDayAsync(-1));
        nextDevotional.Click += async (_, _) => await RunAsync(() => MoveDevotionalDayAsync(1));
        devotionalModule.SelectionChanged += (_, _) => UpdateControls();
        return new Expander
        {
            Header = "Daily devotional", HorizontalAlignment = HorizontalAlignment.Stretch,
            Content = Horizontal(devotionalModule, devotionalDate, readDevotional, previousDevotional, nextDevotional)
        };
    }

    private void ResetDevotionals(IReadOnlyList<Module> available)
    {
        devotionalActive = false;
        devotionalModule.ItemsSource = available.Where(module => module.Devotional).ToArray();
        devotionalModule.SelectedIndex = devotionalModule.Items.Count == 0 ? -1 : 0;
    }

    private void UpdateDevotionalControls(bool ready)
    {
        var available = devotionalModule.SelectedItem is Module;
        devotionalModule.IsEnabled = ready && devotionalModule.Items.Count > 0;
        devotionalDate.IsEnabled = readDevotional.IsEnabled = previousDevotional.IsEnabled =
            nextDevotional.IsEnabled = ready && available;
    }

    private async Task MoveDevotionalDayAsync(int direction)
    {
        devotionalDate.Date = (devotionalDate.Date ?? DateTimeOffset.Now).AddDays(direction);
        await ReadDevotionalAsync();
    }

    private async Task ReadDevotionalAsync()
    {
        if (core == null || devotionalModule.SelectedItem is not Module module)
            return;
        var date = devotionalDate.Date ?? DateTimeOffset.Now;
        var entry = await core.ReadDevotionalAsync(module.Id, date);
        devotionalActive = true;
        displayedDictionarySource = entry.Module;
        ClearNearbyDictionaryKeys();
        dictionaryTitle.Text = $"{module.Id} · {date:d}";
        dictionaryView.Visibility = Visibility.Visible;
        await RenderAsync(dictionaryView, Document(entry.Module, entry.Date, entry.Html));
        status.Text = entry.Date == entry.MatchedKey ? "" : $"Selected {entry.Date}. Showing the available entry {entry.MatchedKey}.";
    }
}
