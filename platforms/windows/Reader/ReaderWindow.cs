// SPDX-License-Identifier: GPL-2.0-or-later
using System.Net;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow : Window
{
    private const string ReadingOrigin = "https://xiphos.invalid/";
    private const string ReadingDocument = ReadingOrigin + "passage";
    private static readonly Module NoCommentary = new("", "None", "", false, false);
    private readonly TextBox library = new() { Header = "SWORD library folder", MinWidth = 300 };
    private readonly Button open = new() { Content = "Open library", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly ComboBox bible = new() { Header = "Bible", MinWidth = 220 };
    private readonly ComboBox commentary = new() { Header = "Commentary", MinWidth = 220 };
    private readonly TextBox reference = new() { Header = "Passage", Text = "John 1:1", MinWidth = 135 };
    private readonly Button go = new() { Content = "Go", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly Button back = new() { Content = "Back" };
    private readonly Button forward = new() { Content = "Forward" };
    private readonly Button previous = new() { Content = "Previous chapter" };
    private readonly Button next = new() { Content = "Next chapter" };
    private readonly TextBlock status = new() { TextWrapping = TextWrapping.Wrap };
    private readonly TextBlock bibleTitle = new() { FontSize = 21 };
    private readonly TextBlock commentaryTitle = new() { FontSize = 21 };
    private readonly WebView2 bibleView = new();
    private readonly WebView2 commentaryView = new();
    private readonly Dictionary<WebView2, string> documents = new();
    private readonly Grid reading = new();
    private readonly Grid commentaryPane = new();
    private readonly ColumnDefinition commentaryColumn = new() { Width = new GridLength(0) };

    private IReadOnlyList<Module> modules = [];
    private CoreSession? core;
    private Passage? current;
    private TaskCompletionSource? active;
    private bool closing;
    private bool closeReady;
    private bool backAvailable;
    private bool forwardAvailable;

    public ReaderWindow()
    {
        Title = "Xiphos";
        AppWindow.Resize(new Windows.Graphics.SizeInt32(1100, 760));
        AppWindow.Closing += OnClosing;

        var layout = new Grid { Padding = new Thickness(20), RowSpacing = 12 };
        for (var i = 0; i < 6; ++i)
            layout.RowDefinitions.Add(new RowDefinition { Height = i == 4 ? new GridLength(1, GridUnitType.Star) : GridLength.Auto });
        AddRow(layout, Horizontal(library, open), 0);
        AddRow(layout, Horizontal(bible, commentary, reference, go), 1);
        AddRow(layout, Horizontal(back, forward, previous, next), 2);
        AddRow(layout, BuildSearchPane(), 3);
        AddRow(layout, reading, 4);
        AddRow(layout, status, 5);
        AutomationProperties.SetLiveSetting(status, AutomationLiveSetting.Polite);
        Content = layout;

        reading.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(2, GridUnitType.Star) });
        reading.ColumnDefinitions.Add(commentaryColumn);
        var biblePane = MakePane(bibleTitle, bibleView, "Bible text");
        reading.Children.Add(biblePane);
        PopulatePane(commentaryPane, commentaryTitle, commentaryView, "Commentary text");
        Grid.SetColumn(commentaryPane, 1);
        reading.Children.Add(commentaryPane);
        commentaryPane.Visibility = Visibility.Collapsed;
        reading.Visibility = Visibility.Collapsed;

        open.Click += async (_, _) => await RunAsync(OpenLibraryAsync);
        go.Click += async (_, _) => await RunAsync(VisitAsync);
        reference.KeyDown += async (_, e) =>
        {
            if (e.Key == Windows.System.VirtualKey.Enter && go.IsEnabled)
            {
                e.Handled = true;
                await RunAsync(VisitAsync);
            }
        };
        bible.SelectionChanged += (_, _) => UpdateControls();
        commentary.SelectionChanged += async (_, _) =>
        {
            if (active == null && current != null && core != null && commentary.SelectedItem is Module selected)
                await RunAsync(async () => await ShowAsync(await core.SelectCommentaryAsync(selected.Id)));
        };
        back.Click += async (_, _) => await RunAsync(async () => await ShowAsync(await core!.MoveHistoryAsync(-1)));
        forward.Click += async (_, _) => await RunAsync(async () => await ShowAsync(await core!.MoveHistoryAsync(1)));
        previous.Click += async (_, _) => await RunAsync(async () => await ShowAsync(await core!.MoveChapterAsync(current!, -1)));
        next.Click += async (_, _) => await RunAsync(async () => await ShowAsync(await core!.MoveChapterAsync(current!, 1)));

        var args = Environment.GetCommandLineArgs();
        var argument = Array.IndexOf(args, "--library");
        var startWithLibrary = argument >= 0 && argument + 1 < args.Length;
        library.Text = startWithLibrary
            ? args[argument + 1] : Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Sword");
        status.Text = "Open a folder containing a SWORD library.";
        UpdateControls();
        if (startWithLibrary)
            DispatcherQueue.TryEnqueue(async () => await RunAsync(async () =>
            {
                await OpenLibraryAsync();
                if (bible.SelectedItem is Module)
                    await VisitAsync();
            }));
    }

    private static StackPanel Horizontal(params UIElement[] elements)
    {
        var panel = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 10 };
        foreach (var element in elements)
            panel.Children.Add(element);
        return panel;
    }

    private static void AddRow(Grid grid, FrameworkElement element, int row)
    {
        Grid.SetRow(element, row);
        grid.Children.Add(element);
    }

    private static Grid MakePane(TextBlock title, WebView2 view, string accessibleName)
    {
        var pane = new Grid();
        PopulatePane(pane, title, view, accessibleName);
        return pane;
    }

    private static void PopulatePane(Grid pane, TextBlock title, WebView2 view, string accessibleName)
    {
        pane.RowSpacing = 8;
        pane.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
        pane.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });
        AutomationProperties.SetName(view, accessibleName);
        AddRow(pane, title, 0);
        AddRow(pane, view, 1);
    }

    private async Task OpenLibraryAsync()
    {
        var folder = library.Text.Trim();
        if (folder.Length == 0)
            throw new InvalidOperationException("Enter a SWORD library folder.");
        var root = Path.GetFullPath(folder);
        if (!Directory.Exists(root))
            throw new InvalidOperationException("Choose an existing SWORD library folder.");

        if (core != null)
        {
            await core.DisposeAsync();
            core = null;
        }

        current = null;
        reading.Visibility = Visibility.Collapsed;
        bible.ItemsSource = null;
        commentary.ItemsSource = null;
        SetSearchModules([]);
        var session = new CoreSession(root);
        try
        {
            modules = await session.OpenAsync();
            core = session;
        }
        catch
        {
            await session.DisposeAsync();
            throw;
        }
        bible.ItemsSource = modules.Where(module => module.Type == "Biblical Texts").ToArray();
        commentary.ItemsSource = new[] { NoCommentary }.Concat(modules.Where(module => module.Commentary)).ToArray();
        SetSearchModules(modules.Where(module => module.Type == "Biblical Texts" || module.Commentary).ToArray());
        commentary.SelectedIndex = 0;
        if (bible.Items.Count == 0)
        {
            status.Text = "No Bible modules were found in this library.";
            return;
        }

        bible.SelectedIndex = 0;
        status.Text = "Choose a passage and select Go.";
    }

    private Task VisitAsync() => VisitSelectedPassageAsync((Module)bible.SelectedItem,
        (Module?)commentary.SelectedItem ?? NoCommentary);

    private async Task VisitSelectedPassageAsync(Module selected, Module selectedCommentary)
    {
        var expression = reference.Text.Trim();
        if (expression.Length == 0)
            throw new InvalidOperationException("Enter a passage reference.");
        await ShowAsync(await core!.VisitAsync(selected.Id, selectedCommentary.Id, expression));
    }

    private async Task ShowAsync(Passage passage)
    {
        status.Text = "";
        current = passage;
        reference.Text = passage.Reference;
        bible.SelectedItem = modules.FirstOrDefault(module => module.Id == passage.Module);
        commentary.SelectedItem = modules.FirstOrDefault(module => module.Id == passage.Commentary) ?? NoCommentary;
        (backAvailable, forwardAvailable) = await core!.HistoryAsync();

        reading.Visibility = Visibility.Visible;
        bibleTitle.Text = $"{passage.Module} · {passage.Reference}";
        var hasCommentary = passage.Commentary.Length != 0;
        commentaryColumn.Width = hasCommentary ? new GridLength(1, GridUnitType.Star) : new GridLength(0);
        reading.ColumnSpacing = hasCommentary ? 16 : 0;
        commentaryPane.Visibility = hasCommentary ? Visibility.Visible : Visibility.Collapsed;
        await RenderAsync(bibleView, Document(passage.Module, passage.Reference, passage.Html));
        if (hasCommentary)
        {
            commentaryTitle.Text = $"{passage.Commentary} · {passage.CommentaryReference}";
            await RenderAsync(commentaryView, Document(passage.Commentary, passage.CommentaryReference, passage.CommentaryHtml));
        }
    }

    private async Task RenderAsync(WebView2 view, string document)
    {
        if (!documents.ContainsKey(view))
            await PrepareViewAsync(view);
        documents[view] = document;
        view.CoreWebView2.Navigate(ReadingDocument);
    }

    private async Task PrepareViewAsync(WebView2 view)
    {
        Exception? initializationError = null;
        void OnInitialized(WebView2 sender, CoreWebView2InitializedEventArgs args)
        {
            initializationError = args.Exception;
        }
        view.CoreWebView2Initialized += OnInitialized;
        try
        {
            await view.EnsureCoreWebView2Async();
        }
        finally
        {
            view.CoreWebView2Initialized -= OnInitialized;
        }
        var browser = view.CoreWebView2 ?? throw new InvalidOperationException(
            initializationError == null ? "WebView2 could not initialize." :
                $"WebView2 initialization failed (0x{initializationError.HResult:X8}): {initializationError.Message}",
            initializationError);
        browser.Settings.IsScriptEnabled = false;
        browser.Settings.IsWebMessageEnabled = false;
        browser.Settings.AreHostObjectsAllowed = false;
        browser.Settings.AreDevToolsEnabled = false;
        browser.Settings.AreDefaultContextMenusEnabled = false;
        browser.Settings.IsStatusBarEnabled = false;
        browser.AddWebResourceRequestedFilter("*", CoreWebView2WebResourceContext.All,
            CoreWebView2WebResourceRequestSourceKinds.All);
        browser.WebResourceRequested += (sender, e) =>
        {
            if (e.ResourceContext == CoreWebView2WebResourceContext.Document &&
                e.Request.Uri == ReadingDocument && documents.TryGetValue(view, out var html))
            {
                var bytes = new MemoryStream(Encoding.UTF8.GetBytes(html), writable: false);
                e.Response = sender.Environment.CreateWebResourceResponse(bytes.AsRandomAccessStream(),
                    200, "OK", "Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store");
            }
            else
                e.Response = sender.Environment.CreateWebResourceResponse(null, 403, "Blocked", "");
        };
        browser.NewWindowRequested += (_, e) => e.Handled = true;
        browser.PermissionRequested += (_, e) => e.State = CoreWebView2PermissionState.Deny;
        browser.NavigationStarting += async (_, e) =>
        {
            if (e.Uri == ReadingDocument)
                return;
            e.Cancel = true;
            if (!e.IsUserInitiated || current == null || core == null || active != null)
                return;
            var uri = e.Uri.StartsWith(ReadingOrigin, StringComparison.OrdinalIgnoreCase)
                ? e.Uri[ReadingOrigin.Length..] : e.Uri;
            if (uri.StartsWith("passagestudy.jsp?", StringComparison.OrdinalIgnoreCase) ||
                uri.StartsWith("sword:", StringComparison.OrdinalIgnoreCase) ||
                uri.StartsWith("bible:", StringComparison.OrdinalIgnoreCase))
                await RunAsync(async () => await ShowAsync(await core.FollowAsync(current, uri)));
        };
        browser.FrameNavigationStarting += (_, e) => e.Cancel = true;
    }

    private string Document(string module, string location, string html)
    {
        var direction = modules.Any(item => item.Id == module && item.RightToLeft) ? "rtl" : "ltr";
        return $$"""
            <!doctype html>
            <html dir="{{direction}}"><head><meta charset="utf-8">
            <meta http-equiv="Content-Security-Policy" content="default-src 'none'; img-src data:; style-src 'unsafe-inline'; form-action 'none'">
            <base href="{{ReadingOrigin}}">
            <title>{{WebUtility.HtmlEncode($"{module} {location}")}}</title>
            <style>
                :root { color-scheme: light dark; }
                body { margin: 0 auto; max-width: 74ch; padding: 1.25rem; color: CanvasText; background: Canvas; font: 1.15rem/1.65 Georgia, serif; overflow-wrap: anywhere; }
                a { color: LinkText; }
                .wordsOfJesus { color: #ac2e2e; }
                @media (prefers-color-scheme: dark) { .wordsOfJesus { color: #ff9b9b; } }
                @media (forced-colors: active) { .wordsOfJesus { color: CanvasText; } }
            </style></head><body><main>{{html}}</main></body></html>
            """;
    }

    private async Task RunAsync(Func<Task> action)
    {
        if (active != null || closing)
            return;
        var operation = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        active = operation;
        status.Text = "";
        UpdateControls();
        try { await action(); }
        catch (Exception error) { status.Text = error.Message; }
        finally
        {
            active = null;
            UpdateControls();
            operation.SetResult();
        }
    }

    private void UpdateControls()
    {
        var ready = active == null && !closing;
        library.IsEnabled = open.IsEnabled = ready;
        bible.IsEnabled = commentary.IsEnabled = reference.IsEnabled = ready && bible.Items.Count != 0;
        go.IsEnabled = ready && core != null && bible.SelectedItem is Module;
        previous.IsEnabled = next.IsEnabled = ready && current != null;
        back.IsEnabled = ready && current != null && backAvailable;
        forward.IsEnabled = ready && current != null && forwardAvailable;
        UpdateSearchControls(ready);
    }

    private async void OnClosing(Microsoft.UI.Windowing.AppWindow sender, Microsoft.UI.Windowing.AppWindowClosingEventArgs args)
    {
        if (closeReady)
            return;
        args.Cancel = true;
        if (closing)
            return;
        closing = true;
        searchCancellation?.Cancel();
        UpdateControls();
        if (active != null)
            await active.Task;
        if (core != null)
            await core.DisposeAsync();
        closeReady = true;
        Close();
    }
}
