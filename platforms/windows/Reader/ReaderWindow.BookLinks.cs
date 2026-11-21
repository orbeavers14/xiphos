// SPDX-License-Identifier: GPL-2.0-or-later
using System.Net;
using Microsoft.UI.Xaml;

namespace Xiphos.Reader;

internal sealed partial class ReaderWindow
{
    private async Task FollowBookLinkAsync(string uri)
    {
        var target = await core!.FollowBookAsync(current, uri);
        if (target.Book != null)
        {
            await ShowBookAsync(await core.SelectBookAsync(target.Book.Module, target.Book.Path), target.Book.Anchor);
        }
        else if (target.Image != null)
        {
            var image = target.Image;
            var html = $"<figure><img alt=\"{WebUtility.HtmlEncode(image.Filename)}\" " +
                $"style=\"max-width:100%;max-height:300px;object-fit:contain\" " +
                $"src=\"data:{WebUtility.HtmlEncode(image.MediaType)};base64,{Convert.ToBase64String(image.Bytes)}\">" +
                $"<figcaption>{WebUtility.HtmlEncode(image.Filename)}</figcaption></figure>";
            await ShowNoteAsync(new StudyPreview(image.Module, image.Filename, "Image", html));
        }
        else if (target.Dictionary != null)
        {
            SetDictionaryVisible(true);
            await ShowDictionaryAsync(target.Dictionary);
        }
        else if (target.Preview != null)
        {
            var selected = await ShowNoteAsync(target.Preview);
            if (selected != null && !closing)
                await OpenBookScriptureTargetAsync(target.Preview.Module, selected);
        }
        else
            await OpenBookScriptureTargetAsync(target.Module, target.Reference);
    }

    private async Task OpenBookScriptureTargetAsync(string module, string reference)
    {
        var bibleModule = current?.Module ?? (bible.SelectedItem as Module)?.Id ?? module;
        var passage = await core!.VisitLinkAsync(bibleModule, current?.Commentary ?? "", module, reference);
        SetBooksVisible(false);
        await ShowAsync(passage);
    }
}
