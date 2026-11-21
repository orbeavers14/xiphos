// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private ContentDialog? noteDialog;

    private async Task ShowNoteAsync(StudyPreview note)
    {
        var view = new WebView2 { Height = 360, MinWidth = 400 };
        var message = new TextBlock { TextWrapping = TextWrapping.Wrap };
        var panel = new StackPanel { Spacing = 8 };
        panel.Children.Add(view);
        panel.Children.Add(message);
        Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(view, "Study note");
        var dialog = new ContentDialog
        {
            Title = $"{note.Module} · {note.Reference} · {note.Label}",
            Content = panel, CloseButtonText = "Close", XamlRoot = ((FrameworkElement)Content).XamlRoot
        };
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
            await dialog.ShowAsync();
            if (rendering != null)
                await rendering;
        }
        finally
        {
            noteDialog = null;
            documents.Remove(view);
            view.Close();
        }
    }
}
