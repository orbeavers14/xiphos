#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 /path/to/clean/sword-1.9.0-source" >&2
    exit 2
fi
patches=$(CDPATH='' cd -- "$(dirname -- "$0")/../../doc/modernization" && pwd)
cd -- "$1"

check_source() {
    checksum=$(cmake -E sha256sum "$1")
    if [ "${checksum%% *}" != "$2" ] && [ "${checksum%% *}" != "${3:-}" ]; then
        echo "Unexpected SWORD 1.9.0 source: $1" >&2
        exit 1
    fi
}
check_source src/modules/swmodule.cpp 7f7bd12f48112d209697cf34c87be5c19ae8f865adacd6e41759932ba0db1e16 062b9e121a722a0556ab6a0c64043efc178175acb10095efdf2a5f86f1002b75
check_source src/keys/treekeyidx.cpp cf249e65cc413c08188cd1e94967273b311db2dcdf51f36818079d6b42212133
check_source include/treekeyidx.h 5561bd9292a5238e14fa292df9d38917a2de3a2c4984f9c2c16ef2b34ed71b90

check_source include/swbuf.h 8ad13abd686f8024565274f1364b2e9908a7cca3432155f585497316d6a3c43b

# Check all patches before modifying the source tree.
for name in rawgenbook-index-candidate.patch sword-tree-array-delete.patch sword-buffer-overlap.patch sword-macos-no-legacy-ftp.patch rawgenbook-move-candidate.patch; do
    git apply --check --unidiff-zero "$patches/$name"
done
for name in rawgenbook-index-candidate.patch sword-tree-array-delete.patch sword-buffer-overlap.patch sword-macos-no-legacy-ftp.patch rawgenbook-move-candidate.patch; do
    git apply --unidiff-zero "$patches/$name"
done
