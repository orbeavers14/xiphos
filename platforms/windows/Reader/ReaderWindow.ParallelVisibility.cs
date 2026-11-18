// SPDX-License-Identifier: GPL-2.0-or-later
using System.Security;
using System.Security.Cryptography;
using System.Text.Json;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Win32;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private const string ParallelVisibilityKey = @"Software\Xiphos\ParallelVisibility";
    private readonly DropDownButton translationVisibility = new() { Content = "Translations" };
    private int hiddenTranslations;

    private static string VisibilityValueName(ParallelChapter chapter) =>
        Convert.ToHexString(SHA256.HashData(JsonSerializer.SerializeToUtf8Bytes(
            chapter.Columns.Select(column => column.Requested).ToArray())));

    private void LoadColumnVisibility(ParallelChapter chapter)
    {
        hiddenTranslations = 0;
        try
        {
            using var key = Registry.CurrentUser.OpenSubKey(ParallelVisibilityKey);
            if (key?.GetValue(VisibilityValueName(chapter)) is int stored)
            {
                var all = -1 >>> (CoreSession.MaximumParallelColumns - chapter.Columns.Count);
                hiddenTranslations = (stored & all) == all ? 0 : stored & all;
            }
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or SecurityException)
        {
            setStatus.Text = "Windows could not read saved translation visibility.";
        }
        UpdateTranslationMenu(chapter);
    }

    private IEnumerable<int> VisibleColumns(ParallelChapter chapter) =>
        Enumerable.Range(0, chapter.Columns.Count).Where(index => (hiddenTranslations & (1 << index)) == 0);

    private void UpdateTranslationMenu(ParallelChapter chapter)
    {
        var menu = new MenuFlyout();
        var visibleCount = VisibleColumns(chapter).Count();
        for (var i = 0; i < chapter.Columns.Count; i++)
        {
            var index = i;
            var visible = (hiddenTranslations & (1 << index)) == 0;
            var toggle = new ToggleMenuFlyoutItem
            {
                Text = ColumnTitle(chapter.Columns[index]),
                IsChecked = visible,
                IsEnabled = !visible || visibleCount > 1
            };
            toggle.Click += async (_, _) => await RunAsync(() => SetTranslationVisibleAsync(chapter, index, toggle.IsChecked));
            menu.Items.Add(toggle);
        }
        translationVisibility.Flyout = menu;
    }

    private async Task SetTranslationVisibleAsync(ParallelChapter chapter, int index, bool visible)
    {
        hiddenTranslations = visible ? hiddenTranslations & ~(1 << index) : hiddenTranslations | (1 << index);
        try
        {
            using var key = Registry.CurrentUser.CreateSubKey(ParallelVisibilityKey);
            if (hiddenTranslations == 0)
                key.DeleteValue(VisibilityValueName(chapter), throwOnMissingValue: false);
            else
                key.SetValue(VisibilityValueName(chapter), hiddenTranslations, RegistryValueKind.DWord);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or SecurityException)
        {
            setStatus.Text = "Windows could not save translation visibility.";
        }
        UpdateTranslationMenu(chapter);
        await RenderParallelAsync();
    }
}
