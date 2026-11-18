// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml.Controls;
using Windows.ApplicationModel.DataTransfer;
using Windows.Storage;
using Windows.Storage.Pickers;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private void AddSearchExportMenu()
    {
        var menu = new MenuFlyout();
        var saveAllText = new MenuFlyoutItem { Text = "Save all as text…" };
        var saveAllHtml = new MenuFlyoutItem { Text = "Save all as HTML…" };
        var copyAllText = new MenuFlyoutItem { Text = "Copy all as text" };
        menu.Items.Add(saveSelectedText);
        menu.Items.Add(saveSelectedHtml);
        menu.Items.Add(copySelectedText);
        menu.Items.Add(new MenuFlyoutSeparator());
        menu.Items.Add(saveAllText);
        menu.Items.Add(saveAllHtml);
        menu.Items.Add(copyAllText);
        saveSelectedText.Click += async (_, _) => await RunAsync(() => ExportSearchAsync(true, false, false));
        saveSelectedHtml.Click += async (_, _) => await RunAsync(() => ExportSearchAsync(true, true, false));
        copySelectedText.Click += async (_, _) => await RunAsync(() => ExportSearchAsync(true, false, true));
        saveAllText.Click += async (_, _) => await RunAsync(() => ExportSearchAsync(false, false, false));
        saveAllHtml.Click += async (_, _) => await RunAsync(() => ExportSearchAsync(false, true, false));
        copyAllText.Click += async (_, _) => await RunAsync(() => ExportSearchAsync(false, false, true));
        exportResults.Flyout = menu;
    }

    private async Task ExportSearchAsync(bool selected, bool html, bool copy)
    {
        if (core == null || latestResults is not { Generation: > 0 } results ||
            (selected && searchHits.SelectedIndex < 0))
            return;

        using var cancellation = new CancellationTokenSource();
        searchCancellation = cancellation;
        searchStatus.Text = "Preparing export…";
        UpdateControls();
        string? content;
        try
        {
            content = await core.ExportSearchAsync(results.Generation, selected ? searchHits.SelectedIndex : null,
                html, cancellation.Token, percent =>
                {
                    if (!cancellation.IsCancellationRequested)
                        searchStatus.Text = $"Preparing export… {percent}%";
                });
        }
        catch (Exception error)
        {
            searchStatus.Text = error.Message;
            return;
        }
        finally
        {
            searchCancellation = null;
            UpdateControls();
        }
        if (content == null || closing)
        {
            searchStatus.Text = "Export stopped.";
            return;
        }
        if (copy)
        {
            var data = new DataPackage();
            data.SetText(content);
            Clipboard.SetContent(data);
            Clipboard.Flush();
            searchStatus.Text = "Results copied to the clipboard.";
            return;
        }

        var picker = new FileSavePicker { SuggestedFileName = "Xiphos search results" };
        WinRT.Interop.InitializeWithWindow.Initialize(picker, WinRT.Interop.WindowNative.GetWindowHandle(this));
        picker.FileTypeChoices.Add(html ? "HTML document" : "Plain text", new[] { html ? ".html" : ".txt" });
        var file = await picker.PickSaveFileAsync();
        if (file == null)
        {
            searchStatus.Text = "Save canceled.";
            return;
        }
        await FileIO.WriteTextAsync(file, content);
        searchStatus.Text = $"Saved {file.Name}.";
    }
}
