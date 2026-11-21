// SPDX-License-Identifier: GPL-2.0-or-later
using System.Globalization;

namespace Xiphos.Reader;

internal sealed record DevotionalEntry(string Module, string Date, string MatchedKey, string Html);

internal sealed partial class CoreSession
{
    public Task<DevotionalEntry> ReadDevotionalAsync(string module, DateTimeOffset date)
    {
        var today = DateTime.Now;
        return RequestAsync((out nint request) => Native.xc_worker_read_devotional(worker, tab, module,
            date.ToString("MM.dd", CultureInfo.InvariantCulture), today.Month, today.Day, out request),
            request => ReadOwned(request, Native.xc_request_take_devotional_entry,
                Native.xc_devotional_entry_release, value => new DevotionalEntry(
                    Text(Native.xc_devotional_entry_module(value)), Text(Native.xc_devotional_entry_date(value)),
                    Text(Native.xc_devotional_entry_matched_key(value)), Text(Native.xc_devotional_entry_html(value)))));
    }
}
