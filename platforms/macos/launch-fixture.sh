#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# GUI test copy avoids requesting access to a Documents-hosted build/fixture.
set -eu
build_dir=$(cd "${1:-build/macos}" && pwd)
app="$build_dir/XiphosNativePreview.app"
fixture="$build_dir/core/fixture"
if [ ! -x "$app/Contents/MacOS/XiphosNativePreview" ] || [ ! -f "$fixture/mods.d/test.conf" ]; then
  printf '%s\n' 'Build the preview and run core_reader tests before launching the fixture.' >&2
  exit 1
fi
preview_dir=$(/usr/bin/mktemp -d /private/tmp/xiphos-preview.XXXXXX)
/usr/bin/ditto "$app" "$preview_dir/XiphosNativePreview.app"
mkdir "$preview_dir/fixture"
# Only the primary fixture is needed: alternate test configs have SDK-test paths.
/usr/bin/ditto "$fixture/mods.d" "$preview_dir/fixture/mods.d"
/usr/bin/ditto "$fixture/modules" "$preview_dir/fixture/modules"
/usr/bin/open -n -F --env "XIPHOS_MODULE_ROOT=$preview_dir/fixture" "$preview_dir/XiphosNativePreview.app"
printf 'Fixture preview launched from %s\n' "$preview_dir"
printf '%s\n' 'Verify the window; launch success alone is not a GUI check. Quit before removing this temporary copy.'
