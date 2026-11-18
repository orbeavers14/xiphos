// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private readonly ComboBox parallelSets = new() { Header = "Saved sets", MinWidth = 160 };
    private readonly TextBox parallelSetName = new() { Header = "Set name", MinWidth = 155, MaxLength = 120 };
    private readonly Button openParallelSet = new() { Content = "Open set", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly DropDownButton manageSets = new() { Content = "Manage sets", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly MenuFlyoutItem createSet = new() { Text = "Save as new set" };
    private readonly MenuFlyoutItem replaceSet = new() { Text = "Update set order" };
    private readonly MenuFlyoutItem renameSet = new() { Text = "Rename set" };
    private readonly MenuFlyoutItem defaultSet = new() { Text = "Use as default" };
    private readonly MenuFlyoutItem deleteSet = new() { Text = "Delete set" };
    private readonly TextBlock setStatus = new() { TextWrapping = TextWrapping.Wrap };
    private ParallelProfile? parallelProfile;

    private static string ParallelProfilePath()
    {
        var args = Environment.GetCommandLineArgs();
        var index = Array.IndexOf(args, "--parallel-profile");
        return index >= 0 && index + 1 < args.Length
            ? Path.GetFullPath(args[index + 1])
            : Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "Xiphos", "preferences.xml");
    }

    private StackPanel BuildParallelSetsBar()
    {
        var reload = new MenuFlyoutItem { Text = "Reload sets" };
        var menu = new MenuFlyout();
        foreach (var item in new[] { createSet, replaceSet, renameSet, defaultSet, deleteSet, reload })
            menu.Items.Add(item);
        manageSets.Flyout = menu;
        createSet.Click += async (_, _) => await RunAsync(() => EditParallelSetAsync(ParallelSetAction.Create));
        replaceSet.Click += async (_, _) => await RunAsync(() => EditParallelSetAsync(ParallelSetAction.Save));
        renameSet.Click += async (_, _) => await RunAsync(() => EditParallelSetAsync(ParallelSetAction.Rename));
        defaultSet.Click += async (_, _) => await RunAsync(() => EditParallelSetAsync(ParallelSetAction.Activate));
        deleteSet.Click += async (_, _) => await RunAsync(() => EditParallelSetAsync(ParallelSetAction.Remove));
        reload.Click += async (_, _) => await RunAsync(() => LoadParallelSetsAsync(useDefault: false));
        openParallelSet.Click += async (_, _) => await RunAsync(OpenParallelSetAsync);
        parallelSetName.TextChanged += (_, _) => UpdateControls();
        parallelSets.SelectionChanged += (_, _) => UpdateControls();
        AutomationProperties.SetLiveSetting(setStatus, AutomationLiveSetting.Polite);
        var panel = new StackPanel { Spacing = 4 };
        panel.Children.Add(Horizontal(parallelSets, parallelSetName, openParallelSet, manageSets));
        panel.Children.Add(setStatus);
        return panel;
    }

    private async Task LoadParallelSetsAsync(bool useDefault)
    {
        if (core == null)
            return;
        try
        {
            var path = ParallelProfilePath();
            var profile = File.Exists(path) ? await core.ReadParallelProfileAsync(path) : new ParallelProfile(null, "", []);
            ApplyParallelProfile(profile, profile.CurrentKey);
            if (useDefault && parallelSets.SelectedItem is NamedParallelSet selected)
                SetOrderedModules(selected.Modules);
            setStatus.Text = "";
        }
        catch (Exception error)
        {
            setStatus.Text = $"Could not load comparison sets. {error.Message}";
        }
    }

    private void ApplyParallelProfile(ParallelProfile profile, string selectedKey)
    {
        parallelProfile = profile;
        parallelSets.ItemsSource = profile.Sets.ToArray();
        parallelSets.SelectedItem = profile.Sets.FirstOrDefault(set => set.Key == selectedKey);
    }

    private void SetOrderedModules(IEnumerable<string> ids)
    {
        orderedModules.Clear();
        foreach (var id in ids)
            orderedModules.Add(modules.FirstOrDefault(module => module.Type == "Biblical Texts" && module.Id == id)
                ?? new Module(id, "Unavailable", "", false, false));
    }

    private async Task OpenParallelSetAsync()
    {
        var selected = (NamedParallelSet)parallelSets.SelectedItem;
        var chapter = await core!.VisitNamedParallelSetAsync(ParallelProfilePath(), selected.Key,
            parallelReference.Text.Trim());
        SetOrderedModules(chapter.Columns.Select(column => column.Requested));
        await ShowParallelAsync(chapter);
    }

    private async Task EditParallelSetAsync(ParallelSetAction action)
    {
        var profile = parallelProfile!;
        var path = ParallelProfilePath();
        var selected = parallelSets.SelectedItem as NamedParallelSet;
        try
        {
            if (action == ParallelSetAction.Create)
                Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            var ids = action is ParallelSetAction.Create or ParallelSetAction.Save
                ? orderedModules.Select(module => module.Id).ToArray() : null;
            var updated = await core!.EditParallelProfileAsync(path, profile.Revision, action, selected?.Key ?? "",
                parallelSetName.Text.Trim(), ids);
            var key = action is ParallelSetAction.Create or ParallelSetAction.Rename
                ? updated.Sets.FirstOrDefault(set => profile.Sets.All(prior => prior.Key != set.Key))?.Key ?? selected?.Key
                : action == ParallelSetAction.Remove ? null : selected?.Key;
            ApplyParallelProfile(updated, key ?? updated.CurrentKey);
            setStatus.Text = action == ParallelSetAction.Activate ? "Default comparison set updated." : "Comparison sets updated.";
        }
        catch (Exception error)
        {
            setStatus.Text = $"Could not update comparison sets. {error.Message} Reload sets before retrying a concurrent edit.";
        }
    }

    private void UpdateParallelSetControls(bool ready, bool hasModules)
    {
        var selected = parallelSets.SelectedItem as NamedParallelSet;
        var named = !string.IsNullOrWhiteSpace(parallelSetName.Text);
        var canSave = orderedModules.Count is >= 2 and <= CoreSession.MaximumParallelColumns;
        var profileReady = ready && parallelProfile != null;
        manageSets.IsEnabled = ready && hasModules;
        parallelSetName.IsEnabled = profileReady;
        parallelSets.IsEnabled = profileReady && parallelSets.Items.Count > 0;
        openParallelSet.IsEnabled = profileReady && selected?.Modules.Count is >= 2 and <= CoreSession.MaximumParallelColumns &&
            !string.IsNullOrWhiteSpace(parallelReference.Text);
        createSet.IsEnabled = profileReady && named && canSave;
        replaceSet.IsEnabled = profileReady && selected != null && canSave;
        renameSet.IsEnabled = profileReady && selected != null && named;
        defaultSet.IsEnabled = profileReady && selected != null && selected.Key != parallelProfile!.CurrentKey;
        deleteSet.IsEnabled = profileReady && selected != null;
    }
}
