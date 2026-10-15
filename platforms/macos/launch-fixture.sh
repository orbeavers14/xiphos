#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# GUI test copy avoids requesting access to a Documents-hosted build/fixture.
set -eu
build_dir=$(cd "${1:-build/macos}" && pwd)
app="$build_dir/XiphosNativePreview.app"
kind="${2:-reader}"
case "$kind" in
  reader) fixture="$build_dir/core/fixture"; config="test.conf" ;;
  lexicons) fixture="$build_dir/core/dictionary-fixture"; config="bible.conf" ;;
  headings) fixture="$build_dir/core/heading-fixture"; config="notes.conf" ;;
  lexical) fixture="$build_dir/core/lexical-snapshots-fixture"; config="text.conf" ;;
  commentary) fixture="$build_dir/core/study-passage-fixture"; config="Text.conf" ;;
  reading-options) fixture="$build_dir/core/module-preferences-fixture"; config="options.conf" ;;
  *) printf '%s\n' 'Unknown generated fixture kind.' >&2; exit 1 ;;
esac
if [ ! -x "$app/Contents/MacOS/XiphosNativePreview" ] || [ ! -f "$fixture/mods.d/$config" ]; then
  printf '%s\n' 'Build the preview and run the selected generated fixture tests before launching.' >&2
  exit 1
fi
preview_dir=$(/usr/bin/mktemp -d /private/tmp/xiphos-preview.XXXXXX)
/usr/bin/ditto "$app" "$preview_dir/XiphosNativePreview.app"
mkdir "$preview_dir/fixture"
# Copy only the selected generated fixture, never an installed user library.
/usr/bin/ditto "$fixture/mods.d" "$preview_dir/fixture/mods.d"
if [ "$kind" = reader ]; then
  /usr/bin/ditto "$fixture/modules" "$preview_dir/fixture/modules"
elif [ "$kind" = reading-options ]; then
  /usr/bin/ditto "$fixture/data" "$preview_dir/fixture/data"
  /usr/bin/ditto "$fixture/other-data" "$preview_dir/fixture/other-data"
  # The generated legacy preference sample enables native picker checks.
  if [ -f "$fixture/modops.conf" ]; then
    /usr/bin/ditto "$fixture/modops.conf" "$preview_dir/fixture/modops.conf"
  fi
  # Rediscover this generated fixture instead of carrying its SDK catalog cache.
  rm -f "$preview_dir/fixture/mods.d/modules-conf.cache"
else
  /usr/bin/ditto "$fixture" "$preview_dir/fixture"
  rm -f "$preview_dir/fixture/mods.d/modules-conf.cache"
fi
/usr/bin/open -n -F --env "XIPHOS_MODULE_ROOT=$preview_dir/fixture" "$preview_dir/XiphosNativePreview.app"
printf 'Fixture preview launched from %s\n' "$preview_dir"
printf '%s\n' 'Verify the window; launch success alone is not a GUI check. Quit before removing this temporary copy.'
