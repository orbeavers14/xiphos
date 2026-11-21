// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage.Pickers;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private readonly Button configureLexicons = new()
    {
        Content = "Lexicon settings", VerticalAlignment = VerticalAlignment.Bottom
    };
    private ContentDialog? lexiconDialog;

    private async Task ConfigureLexiconsAsync()
    {
        var names = await core!.LexiconsAsync();
        var panel = new StackPanel { Spacing = 8 };
        var fields = new[] { "Greek Strong’s", "Hebrew Strong’s", "Greek morphology, New Testament",
            "Greek morphology, Old Testament", "Hebrew morphology" }
            .Select(label => new ComboBox { Header = label, IsEditable = true,
                HorizontalAlignment = HorizontalAlignment.Stretch }).ToArray();
        var available = modules.Where(module => module.IsDictionary).Select(module => module.Id).ToArray();
        foreach (var field in fields)
        {
            field.ItemsSource = available;
            panel.Children.Add(field);
        }
        void SetFields(Native.LexiconNames value)
        {
            var values = new[] { value.Greek, value.Hebrew, value.GreekMorphNt, value.GreekMorphOt, value.HebrewMorph };
            for (var i = 0; i < fields.Length; i++)
                fields[i].Text = values[i] ?? "";
        }
        Native.LexiconNames ReadFields() => new()
        {
            Greek = EmptyToNull(fields[0].Text), Hebrew = EmptyToNull(fields[1].Text),
            GreekMorphNt = EmptyToNull(fields[2].Text), GreekMorphOt = EmptyToNull(fields[3].Text),
            HebrewMorph = EmptyToNull(fields[4].Text)
        };
        SetFields(names);
        var message = new TextBlock { TextWrapping = TextWrapping.Wrap };
        var import = new Button { Content = "Import profile…" };
        var save = new Button { Content = "Save profile…" };
        panel.Children.Add(Horizontal(import, save));
        panel.Children.Add(message);
        var dialog = new ContentDialog
        {
            Title = "Lexicon settings", Content = panel, PrimaryButtonText = "Apply",
            CloseButtonText = "Close", XamlRoot = dictionaryPane.XamlRoot
        };
        var busy = false;
        Task? profileOperation = null;
        async Task ProfileAsync(bool importing)
        {
            if (busy)
                return;
            busy = true;
            import.IsEnabled = save.IsEnabled = dialog.IsPrimaryButtonEnabled = false;
            try
            {
                var handle = WinRT.Interop.WindowNative.GetWindowHandle(this);
                if (importing)
                {
                    var picker = new FileOpenPicker();
                    WinRT.Interop.InitializeWithWindow.Initialize(picker, handle);
                    picker.FileTypeFilter.Add(".xml");
                    var file = await picker.PickSingleFileAsync();
                    if (file != null)
                    {
                        SetFields(await core.ImportLexiconsAsync(file.Path));
                        message.Text = "Profile imported and applied.";
                    }
                }
                else
                {
                    var picker = new FileSavePicker { SuggestedFileName = "Xiphos lexicons" };
                    WinRT.Interop.InitializeWithWindow.Initialize(picker, handle);
                    picker.FileTypeChoices.Add("XML profile", new[] { ".xml" });
                    var file = await picker.PickSaveFileAsync();
                    if (file != null)
                    {
                        await core.SetLexiconsAsync(ReadFields());
                        await core.SaveLexiconsAsync(file.Path);
                        message.Text = "Lexicon choices applied and saved.";
                    }
                }
            }
            catch (Exception error) { message.Text = error.Message; }
            finally
            {
                busy = false;
                import.IsEnabled = save.IsEnabled = dialog.IsPrimaryButtonEnabled = true;
            }
        }
        import.Click += async (_, _) => await (profileOperation = ProfileAsync(true));
        save.Click += async (_, _) => await (profileOperation = ProfileAsync(false));
        dialog.Closing += (_, args) => args.Cancel = busy && !closing;
        dialog.PrimaryButtonClick += async (_, args) =>
        {
            var deferral = args.GetDeferral();
            try { await core.SetLexiconsAsync(ReadFields()); }
            catch (Exception error) { args.Cancel = true; message.Text = error.Message; }
            finally { deferral.Complete(); }
        };
        lexiconDialog = dialog;
        try
        {
            await dialog.ShowAsync();
            if (profileOperation != null)
                await profileOperation;
        }
        finally { lexiconDialog = null; }
    }

    private static string? EmptyToNull(string value) => string.IsNullOrWhiteSpace(value) ? null : value.Trim();
}
