#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 /path/to/clean/sword-1.9.0-source" >&2
    exit 2
fi
patches=$(CDPATH='' cd -- "$(dirname -- "$0")/../../doc/modernization" && pwd)
cd -- "$1"

# Check both patches before modifying the source tree. They affect distinct files.
for name in rawgenbook-index-candidate.patch rawgenbook-move-candidate.patch; do
    git apply --check --unidiff-zero "$patches/$name"
done
for name in rawgenbook-index-candidate.patch rawgenbook-move-candidate.patch; do
    git apply --unidiff-zero "$patches/$name"
done
