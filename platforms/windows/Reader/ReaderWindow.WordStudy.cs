// SPDX-License-Identifier: GPL-2.0-or-later
using System.Text;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.Web.WebView2.Core;
using Windows.System;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private sealed record WordSource(string Module, string Reference);
    private sealed record WordSearchChoice(string Label, SearchMode Mode, string Query)
    {
        public override string ToString() => Label;
    }

    private sealed class WordStudyDocument
    {
        public readonly string Attribute = $"data-xiphos-{Guid.NewGuid():N}";
        public readonly List<WordSource> Sources = [];

        public string Mark(string module, string reference, string html)
        {
            var index = Sources.Count;
            Sources.Add(new WordSource(module, reference));
            return FormattableString.Invariant($"<div {Attribute}=\"{index}\">{html}</div>");
        }
    }

    private readonly Dictionary<WebView2, WordStudyDocument> studyDocuments = new();
    private readonly Button studySelectedWord = new() { Content = "Study selected word" };
    private ContentDialog? wordStudyDialog;

    private void ConfigureWordStudy(FrameworkElement layout)
    {
        studySelectedWord.Click += async (_, _) => await RunAsync(StudyVisibleSelectionAsync);
        var accelerator = new KeyboardAccelerator
        {
            Key = VirtualKey.F,
            Modifiers = VirtualKeyModifiers.Control | VirtualKeyModifiers.Menu
        };
        accelerator.Invoked += async (_, args) =>
        {
            if (!studySelectedWord.IsEnabled)
                return;
            args.Handled = true;
            await RunAsync(StudyVisibleSelectionAsync);
        };
        layout.KeyboardAccelerators.Add(accelerator);
    }

    private void ConfigureWordStudyMenu(WebView2 view, CoreWebView2 browser)
    {
        browser.ContextMenuRequested += (_, args) =>
        {
            args.Handled = true;
            if (!studySelectedWord.IsEnabled || !args.ContextMenuTarget.HasSelection)
                return;

            var item = new MenuFlyoutItem { Text = "Study selected word" };
            item.Click += async (_, _) => await RunAsync(() => StudySelectionAsync(view));
            var menu = new MenuFlyout();
            menu.Items.Add(item);
            var deferral = args.GetDeferral();
            menu.Closed += (_, _) => deferral.Complete();
            menu.ShowAt(view, new FlyoutShowOptions { Position = args.Location });
        };
    }

    private Task StudyVisibleSelectionAsync() =>
        StudySelectionAsync(parallelPane.Visibility == Visibility.Visible ? parallelView : bibleView);

    private async Task StudySelectionAsync(WebView2 view)
    {
        if (core == null)
            return;

        if (view.CoreWebView2?.Source != ReadingDocument || !studyDocuments.TryGetValue(view, out var context))
            throw new InvalidOperationException("Select a word in a loaded Bible passage or comparison verse.");

        // The browser runs this fixed host query while the module document remains passive.
        var raw = await view.CoreWebView2.ExecuteScriptAsync($$"""
            (() => {
                const selection = window.getSelection();
                if (!selection || selection.rangeCount !== 1 || selection.isCollapsed) return null;
                const source = node => (node?.nodeType === 1 ? node : node?.parentElement)
                    ?.closest('[{{context.Attribute}}]');
                const start = source(selection.anchorNode);
                if (!start || start !== source(selection.focusNode)) return null;
                const text = selection.toString().trim();
                if (!text || text.length > 1024) return null;
                return { index: Number(start.getAttribute('{{context.Attribute}}')), text };
            })()
            """);
        using var selection = JsonDocument.Parse(raw);
        var value = selection.RootElement;
        if (value.ValueKind != JsonValueKind.Object ||
            !value.TryGetProperty("index", out var indexValue) || !indexValue.TryGetInt32(out var index) ||
            index < 0 || index >= context.Sources.Count || !value.TryGetProperty("text", out var textValue) ||
            textValue.ValueKind != JsonValueKind.String || textValue.GetString() is not { Length: > 0 } text ||
            text.Any(char.IsWhiteSpace) || Encoding.UTF8.GetByteCount(text) > 1024)
            throw new InvalidOperationException("Select one word within a single verse.");

        var source = context.Sources[index];
        var study = await core.WordStudyAsync(source.Module, source.Reference, text);
        var choices = new List<WordSearchChoice>
        {
            new("Exact surface form", SearchMode.SurfaceLiteral, study.Surface),
            new("Surface form, ignore marks", SearchMode.SurfaceUnmarked, study.Surface)
        };
        choices.AddRange(study.Lemmas.Select(term => new WordSearchChoice($"Lemma / Strong’s {term}", SearchMode.Lemma, term)));
        var selector = new ComboBox
        {
            Header = $"{study.Module} · {study.Reference}", ItemsSource = choices, SelectedIndex = 0,
            HorizontalAlignment = HorizontalAlignment.Stretch
        };
        var dialog = new ContentDialog
        {
            Title = $"Study “{study.Surface}”", Content = selector, PrimaryButtonText = "Search",
            CloseButtonText = "Cancel", DefaultButton = ContentDialogButton.Primary, XamlRoot = view.XamlRoot
        };
        if (closing)
            return;
        ContentDialogResult response;
        wordStudyDialog = dialog;
        try { response = await dialog.ShowAsync(); }
        finally { wordStudyDialog = null; }
        if (closing || response != ContentDialogResult.Primary || selector.SelectedItem is not WordSearchChoice choice)
            return;

        searchModule.SelectedItem = modules.Single(module => module.Id == study.Module);
        searchMode.SelectedItem = ((SearchChoice[])searchMode.ItemsSource).Single(mode => mode.Mode == choice.Mode);
        searchScope.SelectedIndex = (int)SearchScope.WholeModule;
        searchQuery.Text = choice.Query;
        matchCase.IsChecked = false;
        searchPane.IsExpanded = true;
        await SearchAsync();
    }
}
