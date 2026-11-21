// SPDX-License-Identifier: GPL-2.0-or-later
using System.Runtime.InteropServices;

namespace Xiphos.Reader;

internal sealed record BookTarget(string Module, string Path, string? Anchor);
internal sealed record BookImage(string Module, string Filename, string MediaType, byte[] Bytes);
internal sealed record BookLink(BookTarget? Book = null, BookImage? Image = null,
    DictionaryEntry? Dictionary = null, StudyPreview? Preview = null, string Module = "", string Reference = "");

internal sealed partial class CoreSession
{
    public Task<BookLink> FollowBookAsync(Passage? main, string uri) =>
        RequestAsync((out nint request) => Native.xc_worker_follow_book_pane_link_at(worker, tab,
            main?.Module ?? "", main?.Reference ?? "", uri, out request), ReadBookLink);

    private static BookLink ReadBookLink(nint request)
    {
        var noteOwner = Native.xc_request_take_book_note(request);
        if (noteOwner != 0)
        {
            try
            {
                var note = Native.xc_book_note_content(noteOwner);
                return new BookLink(Preview: new StudyPreview(Text(Native.xc_note_module(note)),
                    Text(Native.xc_note_reference(note)), Text(Native.xc_note_label(note)), Text(Native.xc_note_html(note))));
            }
            finally { Native.xc_book_note_release(noteOwner); }
        }
        var image = Native.xc_request_take_book_image_data(request);
        if (image != 0)
        {
            try
            {
                var size = Native.xc_book_image_data_size(image);
                var data = Native.xc_book_image_data_bytes(image);
                if (size == 0 || size > 16 * 1024 * 1024 || data == 0)
                    throw new InvalidOperationException("The book image is empty or exceeds the preview limit.");
                var bytes = new byte[checked((int)size)];
                Marshal.Copy(data, bytes, 0, bytes.Length);
                var source = Native.xc_book_image_data_source(image);
                return new BookLink(Image: new BookImage(Text(Native.xc_book_image_module(source)),
                    Text(Native.xc_book_image_filename(source)), Text(Native.xc_book_image_media_type(source)), bytes));
            }
            finally { Native.xc_book_image_data_release(image); }
        }
        var book = Native.xc_request_take_book_uri_target(request);
        if (book != 0)
        {
            try
            {
                var tree = Native.xc_book_uri_target_tree(book);
                var source = CopyBookTree(tree).Current;
                var anchor = Native.xc_book_uri_target_anchor(book);
                return new BookLink(Book: new BookTarget(Text(Native.xc_book_tree_module(tree)), source.Path,
                    anchor == 0 ? null : Text(anchor)));
            }
            finally { Native.xc_book_uri_target_release(book); }
        }
        var target = ReadPaneTarget(request);
        return new BookLink(Dictionary: target.Dictionary, Preview: target.Preview,
            Module: target.Module, Reference: target.Reference);
    }
}
