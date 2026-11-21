// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Windows.System;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private readonly Button toggleBooks = new() { Content = "Books" };
    private readonly Grid bookPane = new() { Visibility = Visibility.Collapsed, RowSpacing = 8 };
    private readonly Expander bookContentsPane = new()
    {
        Header = "Book contents", HorizontalAlignment = HorizontalAlignment.Stretch,
        HorizontalContentAlignment = HorizontalAlignment.Stretch
    };
    private readonly ComboBox bookModule = new() { Header = "Book module", MinWidth = 180, MaxWidth = 320 };
    private readonly TextBox bookPath = new() { Header = "Book path", MinWidth = 160, PlaceholderText = "Blank for root" };
    private readonly Button openBook = new() { Content = "Open path", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly Button loadBookContents = new() { Content = "Contents", VerticalAlignment = VerticalAlignment.Bottom };
    private readonly Button bookContentsUp = new() { Content = "Up contents" };
    private readonly Button browseBookChildren = new() { Content = "Browse children" };
    private readonly Button readBookSelection = new() { Content = "Read selection" };
    private readonly Button moreBookChildren = new() { Content = "Next page" };
    private readonly ComboBox bookChildren = new() { Header = "Contents", MinWidth = 220, MaxWidth = 360 };
    private readonly TextBlock bookContentsTitle = new() { TextWrapping = TextWrapping.Wrap };
    private readonly Button[] bookNavigation = [new() { Content = "Parent" }, new() { Content = "First child" },
        new() { Content = "Previous section" }, new() { Content = "Next section" }];
    private readonly TextBlock bookTitle = new() { FontSize = 21, TextWrapping = TextWrapping.Wrap };
    private readonly TextBlock bookDetails = new() { TextWrapping = TextWrapping.Wrap };
    private readonly WebView2 bookView = new();
    private BookDocument? bookDocument;
    private BookChildren? bookContents;
    private BookTree? bookContentsTree;
    private string bookContentsModule = "";

    private Grid BuildBookPane()
    {
        for (var i = 0; i < 5; i++)
            bookPane.RowDefinitions.Add(new RowDefinition { Height = i == 4 ?
                new GridLength(1, GridUnitType.Star) : GridLength.Auto });
        AddRow(bookPane, Horizontal(bookModule, bookPath, openBook, loadBookContents), 0);
        var contents = new StackPanel { Spacing = 6 };
        contents.Children.Add(bookContentsTitle);
        contents.Children.Add(Horizontal(bookChildren, bookContentsUp, browseBookChildren, readBookSelection,
            moreBookChildren));
        bookContentsPane.Content = contents;
        AddRow(bookPane, bookContentsPane, 1);
        AddRow(bookPane, Horizontal(bookNavigation), 2);
        AddRow(bookPane, bookDetails, 3);
        AddRow(bookPane, MakePane(bookTitle, bookView, "Book content"), 4);
        bookView.Visibility = Visibility.Collapsed;
        AutomationProperties.SetName(bookChildren, "Book contents");
        toggleBooks.Click += (_, _) => SetBooksVisible(bookPane.Visibility != Visibility.Visible);
        bookModule.SelectionChanged += (_, _) =>
        {
            bookContents = null;
            bookContentsTree = null;
            bookContentsModule = "";
            bookChildren.ItemsSource = null;
            bookContentsTitle.Text = "Choose Contents to browse the selected module.";
            bookPath.Text = "";
            UpdateControls();
        };
        bookChildren.SelectionChanged += (_, _) => UpdateControls();
        openBook.Click += async (_, _) => await RunAsync(OpenBookPathAsync);
        bookPath.KeyDown += async (_, args) =>
        {
            if (args.Key == VirtualKey.Enter && openBook.IsEnabled)
            {
                args.Handled = true;
                await RunAsync(OpenBookPathAsync);
            }
        };
        loadBookContents.Click += async (_, _) => await RunAsync(async () =>
        {
            await LoadBookContentsAsync(0);
            bookContentsPane.IsExpanded = true;
        });
        bookContentsUp.Click += async (_, _) => await RunAsync(() => LoadBookContentsAsync(bookContentsTree!.Parent!.Offset));
        browseBookChildren.Click += async (_, _) => await RunAsync(() =>
            LoadBookContentsAsync(((BookNode)bookChildren.SelectedItem).Offset));
        readBookSelection.Click += async (_, _) => await RunAsync(async () =>
            await ShowBookAsync(await core!.SelectBookAsync(bookContentsModule, ((BookNode)bookChildren.SelectedItem).Path)));
        moreBookChildren.Click += async (_, _) => await RunAsync(() =>
            LoadBookContentsAsync(bookContents!.Parent.Offset, bookContents.NextAfter));
        for (var i = 0; i < bookNavigation.Length; i++)
        {
            var direction = i;
            bookNavigation[i].Click += async (_, _) => await RunAsync(async () =>
            {
                var document = await core!.NavigateBookAsync(bookDocument!.Module, direction);
                if (document != null)
                    await ShowBookAsync(document);
            });
        }
        return bookPane;
    }

    private void ResetBooks(IReadOnlyList<Module> available)
    {
        bookDocument = null;
        bookContentsPane.IsExpanded = false;
        bookView.Visibility = Visibility.Collapsed;
        bookTitle.Text = bookDetails.Text = "";
        SetBooksVisible(false);
        bookModule.ItemsSource = available.Where(module => module.HasBookTree).ToArray();
        bookModule.SelectedIndex = bookModule.Items.Count == 0 ? -1 : 0;
    }

    private void SetBooksVisible(bool visible)
    {
        if (visible)
        {
            SetParallelVisible(false);
            SetDictionaryVisible(false);
        }
        bookPane.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        toggleBooks.Content = visible ? "Close books" : "Books";
        UpdateReadingVisibility();
        UpdateControls();
    }

    private void UpdateBookControls(bool ready)
    {
        var available = ready && core != null && bookModule.SelectedItem is Module;
        toggleBooks.IsEnabled = ready && bookModule.Items.Count > 0;
        bookModule.IsEnabled = bookPath.IsEnabled = openBook.IsEnabled = loadBookContents.IsEnabled = available;
        bookChildren.IsEnabled = available && bookContents != null;
        bookContentsUp.IsEnabled = available && bookContentsTree?.Parent != null;
        browseBookChildren.IsEnabled = available && bookChildren.SelectedItem is BookNode { HasChildren: true };
        readBookSelection.IsEnabled = available && bookChildren.SelectedItem is BookNode;
        moreBookChildren.IsEnabled = available && bookContents?.NextAfter != null;
        BookNode?[] relations = [bookDocument?.Tree.Parent, bookDocument?.Tree.FirstChild,
            bookDocument?.Tree.Previous, bookDocument?.Tree.Next];
        for (var i = 0; i < bookNavigation.Length; i++)
            bookNavigation[i].IsEnabled = ready && core != null && relations[i] != null;
    }

    private Task OpenBookPathAsync() => OpenBookAsync(((Module)bookModule.SelectedItem).Id, bookPath.Text.Trim());

    private async Task OpenBookAsync(string module, string path) =>
        await ShowBookAsync(await core!.SelectBookAsync(module, path));

    private async Task LoadBookContentsAsync(ulong parent, ulong? after = null)
    {
        var module = ((Module)bookModule.SelectedItem).Id;
        var tree = await core!.BookTreeAsync(module, parent);
        var page = await core.BookChildrenAsync(module, parent, after);
        bookContentsTree = tree;
        bookContents = page;
        bookContentsModule = module;
        bookContentsTitle.Text = $"{module} · {(page.Parent.Path.Length == 0 ? "Root" : page.Parent.Path)}";
        bookChildren.ItemsSource = page.Nodes;
        bookChildren.SelectedIndex = page.Nodes.Count == 0 ? -1 : 0;
        if (page.Nodes.Count == 0)
            bookContentsTitle.Text += " · No child sections";
    }

    private async Task ShowBookAsync(BookDocument document, string? anchor = null)
    {
        bookDocument = document;
        bookContentsPane.IsExpanded = false;
        bookModule.SelectedItem = modules.FirstOrDefault(module => module.Id == document.Module);
        bookPath.Text = document.Tree.Current.Path;
        bookTitle.Text = $"{document.Module} · {document.Tree.Current.Title}";
        bookDetails.Text = $"Sections: {document.Sections}";
        if (document.Issues.Count > 0)
            bookDetails.Text += " · " + string.Join("; ", document.Issues);
        await RenderAsync(bookView, Document(document.Module, document.Tree.Current.Path,
            document.Html, stylesheet: document.Css), anchor: anchor);
        bookView.Visibility = Visibility.Visible;
        if (bookPane.Visibility == Visibility.Visible)
            bookView.Focus(FocusState.Programmatic);
    }
}
