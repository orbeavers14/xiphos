// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private ContentDialog? noteDialog;
    private readonly Button showReferences = new()
    {
        Content = "Reference list", VerticalAlignment = VerticalAlignment.Bottom
    };

    private async Task ShowEnteredReferencesAsync()
    {
        var module = (Module)bible.SelectedItem;
        var preview = await core!.ReadReferencesAsync(module.Id, reference.Text, current?.Reference ?? "John 1:1");
        var selected = await ShowNoteAsync(preview);
        if (selected != null && !closing)
        {
            var passage = await core.VisitAsync(module.Id, (commentary.SelectedItem as Module)?.Id ?? "", selected);
            SetParallelVisible(false);
            SetDictionaryVisible(false);
            await ShowAsync(passage);
        }
    }

    private async Task<string?> ShowNoteAsync(StudyPreview note)
    {
        var view = new WebView2 { Height = 360, MinWidth = 400 };
        var message = new TextBlock { TextWrapping = TextWrapping.Wrap };
        var panel = new StackPanel { Spacing = 8 };
        panel.Children.Add(view);
        panel.Children.Add(message);
        var references = new ComboBox
        {
            Header = "Reference to open", ItemsSource = note.References,
            SelectedIndex = note.References?.Count > 0 ? 0 : -1,
            HorizontalAlignment = HorizontalAlignment.Stretch,
            Visibility = note.References == null ? Visibility.Collapsed : Visibility.Visible
        };
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(references, "Reference to open");
        panel.Children.Add(references);
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(view, "Study note");
        var dialog = new ContentDialog
        {
            Title = $"{note.Module} · {note.Reference} · {note.Label}",
            Content = panel, CloseButtonText = "Close", XamlRoot = ((FrameworkElement)Content).XamlRoot
        };
        if (note.References?.Count > 0)
            dialog.PrimaryButtonText = "Open reference";
        Task? rendering = null;
        async Task RenderNoteAsync()
        {
            try { await RenderAsync(view, Document(note.Module, note.Reference, note.Html)); }
            catch (Exception error) { message.Text = error.Message; }
        }
        dialog.Opened += (_, _) => rendering = RenderNoteAsync();
        noteDialog = dialog;
        try
        {
            var result = await dialog.ShowAsync();
            if (rendering != null)
                await rendering;
            return result == ContentDialogResult.Primary ? references.SelectedItem as string : null;
        }
        finally
        {
            noteDialog = null;
            documents.Remove(view);
            view.Close();
        }
    }
}
