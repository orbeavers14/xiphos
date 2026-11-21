// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage.Pickers;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private readonly ComboBox readingOptionsModule = new() { Header = "Module to configure", MinWidth = 210 };
    private readonly Button loadReadingOptions = new() { Content = "Load options", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly Button enableReadingDefaults = new() { Content = "Use Xiphos defaults" };
    private readonly Button applyReadingOptions = new() { Content = "Apply to module" };
    private readonly Button resetReadingOptions = new() { Content = "Reset module" };
    private readonly Button importReadingOptions = new() { Content = "Import settings…" };
    private readonly Button saveReadingOptions = new() { Content = "Save settings…" };
    private readonly TextBlock readingOptionsStatus = new() { TextWrapping = TextWrapping.Wrap };
    private readonly StackPanel readingFilters = new() { Spacing = 4 };
    private readonly ComboBox readingVariants = new() { Header = "Textual variants", MinWidth = 180 };
    private readonly List<(uint Flag, CheckBox Control)> readingToggles = [];
    private ReadingOptions? loadedReadingOptions;
    private bool readingDefaultsEnabled;

    private Expander BuildReadingOptionsPane()
    {
        var labels = new[]
        {
            "Strong’s numbers", "Morphology", "Footnotes", "Greek accents", "Lemmas", "Cross-references",
            "Hebrew vowel points", "Hebrew cantillation", "Italic headings", "Words of Jesus in red",
            "Transliterated forms", "Enumerations", "Glosses", "Morpheme segmentation", "Transliteration"
        };
        for (var i = 0; i < labels.Length; i++)
        {
            var toggle = new CheckBox { Content = labels[i] };
            readingToggles.Add((1u << i, toggle));
            readingFilters.Children.Add(toggle);
        }
        readingVariants.ItemsSource = new[] { "Primary", "Secondary", "All" };
        readingFilters.Children.Add(readingVariants);
        var panel = new StackPanel { Spacing = 6 };
        panel.Children.Add(Horizontal(readingOptionsModule, loadReadingOptions));
        panel.Children.Add(Horizontal(enableReadingDefaults, applyReadingOptions, resetReadingOptions));
        panel.Children.Add(Horizontal(importReadingOptions, saveReadingOptions));
        panel.Children.Add(readingOptionsStatus);
        panel.Children.Add(new ScrollViewer { Content = readingFilters, MaxHeight = 210 });
        var expander = new Expander
        {
            Header = "Reading options", Content = panel, HorizontalAlignment = HorizontalAlignment.Stretch,
            HorizontalContentAlignment = HorizontalAlignment.Stretch
        };
        AutomationProperties.SetLiveSetting(readingOptionsStatus, AutomationLiveSetting.Polite);
        readingOptionsModule.SelectionChanged += (_, _) =>
        {
            loadedReadingOptions = null;
            readingOptionsStatus.Text = "Load options to view this module’s settings.";
            UpdateControls();
        };
        loadReadingOptions.Click += async (_, _) => await RunAsync(LoadReadingOptionsAsync);
        enableReadingDefaults.Click += async (_, _) => await RunAsync(async () =>
        {
            await core!.EnableReadingDefaultsAsync();
            readingDefaultsEnabled = true;
            await LoadReadingOptionsAsync();
            await RefreshReadingViewsAsync();
        });
        applyReadingOptions.Click += async (_, _) => await RunAsync(ApplyReadingOptionsAsync);
        resetReadingOptions.Click += async (_, _) => await RunAsync(async () =>
        {
            await core!.ResetReadingOptionsAsync(((Module)readingOptionsModule.SelectedItem).Id);
            await LoadReadingOptionsAsync();
            await RefreshReadingViewsAsync();
        });
        importReadingOptions.Click += async (_, _) => await RunAsync(() => ReadingOptionsFileAsync(importing: true));
        saveReadingOptions.Click += async (_, _) => await RunAsync(() => ReadingOptionsFileAsync(importing: false));
        return expander;
    }

    private void ResetReadingOptions(IReadOnlyList<Module> available)
    {
        loadedReadingOptions = null;
        readingDefaultsEnabled = false;
        readingOptionsModule.ItemsSource = available;
        readingOptionsModule.SelectedItem = available.FirstOrDefault(module => module.Type == "Biblical Texts") ??
            available.FirstOrDefault();
    }

    private void UpdateReadingOptionsControls(bool ready)
    {
        var module = readingOptionsModule.SelectedItem as Module;
        readingOptionsModule.IsEnabled = ready && core != null;
        loadReadingOptions.IsEnabled = enableReadingDefaults.IsEnabled = resetReadingOptions.IsEnabled =
            ready && core != null && module != null;
        applyReadingOptions.IsEnabled = ready && loadedReadingOptions?.Configured == true;
        importReadingOptions.IsEnabled = ready && core != null;
        saveReadingOptions.IsEnabled = ready && core != null && readingDefaultsEnabled;
        foreach (var (flag, control) in readingToggles)
        {
            control.Visibility = module != null && (module.DeclaredOptions & flag) != 0 ? Visibility.Visible : Visibility.Collapsed;
            control.IsEnabled = applyReadingOptions.IsEnabled;
        }
        readingVariants.Visibility = module != null && (module.DeclaredOptions & (7u << 15)) != 0
            ? Visibility.Visible : Visibility.Collapsed;
        readingVariants.IsEnabled = applyReadingOptions.IsEnabled;
    }

    private async Task LoadReadingOptionsAsync()
    {
        var module = (Module)readingOptionsModule.SelectedItem;
        loadedReadingOptions = await core!.ReadingOptionsAsync(module.Id);
        foreach (var (flag, control) in readingToggles)
            control.IsChecked = loadedReadingOptions.Configured && (loadedReadingOptions.Flags & flag) != 0;
        readingVariants.SelectedIndex = (loadedReadingOptions.Flags & (1u << 17)) != 0 ? 2 :
            (loadedReadingOptions.Flags & (1u << 16)) != 0 ? 1 : 0;
        readingOptionsStatus.Text = loadedReadingOptions.Configured ? "Options loaded. Apply changes to update the reading views." :
            "Using module defaults. Choose Use Xiphos defaults or import settings to customize reading.";
    }

    private async Task ApplyReadingOptionsAsync()
    {
        var module = (Module)readingOptionsModule.SelectedItem;
        var flags = loadedReadingOptions!.Flags;
        foreach (var (flag, control) in readingToggles)
        {
            if ((module.DeclaredOptions & flag) != 0)
                flags = control.IsChecked == true ? flags | flag : flags & ~flag;
        }
        if ((module.DeclaredOptions & (7u << 15)) != 0)
            flags = (flags & ~(7u << 15)) | (1u << (15 + readingVariants.SelectedIndex));
        await core!.SetReadingOptionsAsync(module.Id, flags);
        await LoadReadingOptionsAsync();
        await RefreshReadingViewsAsync();
        readingOptionsStatus.Text = "Reading options applied.";
    }

    private async Task RefreshReadingViewsAsync()
    {
        if (current != null)
            await ShowAsync(await core!.RefreshPassageAsync(current));
        if (dictionaryEntry != null && !devotionalActive)
            await ShowDictionaryAsync(await core!.RefreshDictionaryAsync(dictionaryEntry));
        if (devotionalActive)
            await ReadDevotionalAsync();
        if (bookDocument != null)
        {
            var refreshed = await core!.RefreshBookAsync(bookDocument.Module);
            if (refreshed != null)
                await ShowBookAsync(refreshed);
        }
        searchPreviewPane.Visibility = Visibility.Collapsed;
    }

    private async Task ReadingOptionsFileAsync(bool importing)
    {
        var handle = WinRT.Interop.WindowNative.GetWindowHandle(this);
        if (importing)
        {
            var picker = new FileOpenPicker();
            WinRT.Interop.InitializeWithWindow.Initialize(picker, handle);
            picker.FileTypeFilter.Add(".conf");
            var file = await picker.PickSingleFileAsync();
            if (file == null)
                return;
            await core!.ImportReadingOptionsAsync(file.Path);
            readingDefaultsEnabled = true;
            await RefreshReadingViewsAsync();
        }
        else
        {
            var picker = new FileSavePicker { SuggestedFileName = "modops" };
            WinRT.Interop.InitializeWithWindow.Initialize(picker, handle);
            picker.FileTypeChoices.Add("Module reading settings", new[] { ".conf" });
            var file = await picker.PickSaveFileAsync();
            if (file == null)
                return;
            await core!.SaveReadingOptionsAsync(file.Path);
        }
        await LoadReadingOptionsAsync();
        readingOptionsStatus.Text = importing ? "Reading settings imported." : "Reading settings saved.";
    }
}
