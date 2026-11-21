// SPDX-License-Identifier: GPL-2.0-or-later
namespace Xiphos.Reader;

internal sealed record BookNode(ulong Offset, string Path, string Title, bool HasChildren)
{
    public override string ToString() => Title;
}
internal sealed record BookTree(BookNode Current, BookNode? Parent, BookNode? FirstChild,
    BookNode? Previous, BookNode? Next);
internal sealed record BookChildren(BookNode Parent, IReadOnlyList<BookNode> Nodes, ulong? NextAfter);
internal sealed record BookDocument(string Module, BookTree Tree, string Html, string Css,
    uint DisplayLevel, ulong Sections, IReadOnlyList<string> Issues);

internal sealed partial class CoreSession
{
    public Task<BookTree> BookTreeAsync(string module, ulong offset) =>
        RequestAsync((out nint request) => Native.xc_worker_book_tree(worker, module, offset, out request),
            request => ReadOwned(request, Native.xc_request_take_book_tree, Native.xc_book_tree_release, CopyBookTree));

    public Task<BookChildren> BookChildrenAsync(string module, ulong parent, ulong? after = null) =>
        RequestAsync((out nint request) => Native.xc_worker_book_children(worker, module, parent,
            after.HasValue ? 1 : 0, after ?? 0, 64, out request),
            request => ReadOwned(request, Native.xc_request_take_book_children, Native.xc_book_children_release, page =>
            {
                Check(Native.xc_book_children_parent(page, out var source));
                var nodes = new BookNode[checked((int)Native.xc_book_children_count(page))];
                for (var i = 0; i < nodes.Length; i++)
                {
                    Check(Native.xc_book_children_get_node(page, (nuint)i, out var node));
                    nodes[i] = CopyBookNode(node)!;
                }
                var more = Native.xc_book_children_next_after(page, out var next) != 0;
                return new BookChildren(CopyBookNode(source)!, nodes, more ? next : null);
            }));

    public Task<BookDocument> SelectBookAsync(string module, string path) =>
        RequestAsync((out nint request) => Native.xc_worker_select_book_document_path(worker, tab, module, path,
            out request), ReadBookDocument);

    public Task<BookDocument?> NavigateBookAsync(string module, int direction) =>
        RequestAsync((out nint request) => Native.xc_worker_navigate_book_document(worker, tab, direction,
            out request), request => Native.xc_request_book_selection_present(request) == 0 ? null :
                ReadBookDocument(request));

    public Task<BookDocument?> RefreshBookAsync(string module) =>
        RequestAsync((out nint request) => Native.xc_worker_refresh_book_document(worker, tab, out request),
            request => Native.xc_request_book_selection_present(request) == 0 ? null : ReadBookDocument(request));

    public Task<BookDocument?> ImportBookSelectionAsync(string path, bool startupOffset) =>
        RequestAsync((out nint request) => Native.xc_worker_restore_book_document_profile(worker, tab, path,
            startupOffset ? 1 : 0, out request), request => Native.xc_request_book_selection_present(request) == 0
                ? null : ReadBookDocument(request));

    public Task<StudyPreview> BookNoteAsync(string uri) =>
        RequestAsync((out nint request) => Native.xc_worker_book_document_note_link(worker, tab, uri, 1, out request),
            request => ReadOwned(request, Native.xc_request_take_book_note, Native.xc_book_note_release, owner =>
            {
                var note = Native.xc_book_note_content(owner);
                return new StudyPreview(Text(Native.xc_note_module(note)), Text(Native.xc_note_reference(note)),
                    Text(Native.xc_note_label(note)), Text(Native.xc_note_html(note)));
            }));

    private static BookNode? CopyBookNode(Native.BookNode node) => node.Present == 0 ? null :
        new(node.Offset, Text(node.Path), Text(node.Title), node.HasChildren != 0);

    private static BookTree CopyBookTree(nint tree)
    {
        BookNode? Node(int kind)
        {
            Check(Native.xc_book_tree_get_node(tree, kind, out var node));
            return CopyBookNode(node);
        }
        return new BookTree(Node(0)!, Node(1), Node(2), Node(3), Node(4));
    }

    private static BookDocument ReadBookDocument(nint request) =>
        ReadOwned(request, Native.xc_request_take_book_document, Native.xc_book_document_release, document =>
        {
            var source = Native.xc_book_document_tree(document);
            var tree = CopyBookTree(source);
            Check(Native.xc_book_document_lexical_layout(document, out var layout));
            var issues = new List<string>();
            var stylesheetIssue = Text(Native.xc_book_document_stylesheet_issue(document));
            if (stylesheetIssue.Length != 0)
                issues.Add(stylesheetIssue);
            var count = Native.xc_book_document_inline_issue_count(document);
            for (nuint i = 0; i < count; i++)
                issues.Add(Text(Native.xc_book_document_inline_issue_message(document, i)));
            var html = Native.xc_book_document_inline_html(document);
            if (html == 0)
                html = Native.xc_book_document_presentation_html(document);
            return new BookDocument(Text(Native.xc_book_tree_module(source)), tree, Text(html),
                Text(layout.Css) + "\n" + Text(Native.xc_book_document_stylesheet_inline_css(document)),
                Native.xc_book_document_display_level(document),
                (ulong)Native.xc_book_document_count(document), issues);
        });
}
