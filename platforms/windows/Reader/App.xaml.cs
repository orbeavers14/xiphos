// SPDX-License-Identifier: GPL-2.0-or-later
using Microsoft.UI.Xaml;

namespace Xiphos.Reader;

public partial class App : Application
{
    private ReaderWindow? window;

    public App() => InitializeComponent();

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        window = new ReaderWindow();
        window.Activate();
    }
}
