#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

usage() {
  cat <<'HELP'
Usage: platforms/linux/core.sh [--install-deps] [--sanitize] [--build-dir DIR] [--jobs N]
Install dependencies (optional), configure, build and test the standalone shared core.
Supports Debian/Ubuntu and Fedora dependency installation. No GTK is required.
Build outputs and generated test modules stay in the build directory.
HELP
}
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
install=false
sanitize=false
build_dir=
jobs=${CMAKE_BUILD_PARALLEL_LEVEL:-2}
while (($#)); do
  case "$1" in
    --install-deps) install=true; shift ;;
    --sanitize) sanitize=true; shift ;;
    --build-dir|--jobs)
      (($# >= 2)) || { usage >&2; exit 2; }
      if [[ $1 == --build-dir ]]; then build_dir=$2; else jobs=$2; fi
      shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done
[[ $(uname -s) == Linux ]] || { echo 'This entry point requires Linux.' >&2; exit 2; }
[[ $jobs =~ ^[1-9][0-9]*$ ]] || { echo '--jobs must be a positive integer.' >&2; exit 2; }
if $install; then
  elevate=()
  if ((EUID != 0)); then elevate=(sudo); fi
  # Read only the distribution identity; never evaluate os-release as shell code.
  distro=$(sed -n 's/^ID=//p' /etc/os-release | tr -d '"')
  case "$distro" in
    ubuntu|debian)
      "${elevate[@]}" apt-get update
      "${elevate[@]}" apt-get install -y build-essential cmake ninja-build pkg-config libsword-dev libglib2.0-dev libxml2-dev libpng-dev zlib1g-dev ;;
    fedora)
      "${elevate[@]}" dnf install -y gcc gcc-c++ cmake ninja-build pkgconf-pkg-config sword-devel glib2-devel libxml2-devel libpng-devel zlib-devel ;;
    *) echo "Install CMake >=3.16, Ninja, a C++17 compiler, pkg-config, SWORD >=1.8.1, GLib >=2.32, libxml2, libpng >=1.6 and zlib development packages for $distro." >&2; exit 2 ;;
  esac
fi
for tool in cmake ctest ninja pkg-config; do
  command -v "$tool" >/dev/null || { echo "Missing $tool; run with --install-deps." >&2; exit 1; }
done
pkg-config --exists 'sword >= 1.8.1' 'glib-2.0 >= 2.32' libxml-2.0 'libpng >= 1.6' zlib || {
  echo 'Missing core development dependencies; run with --install-deps.' >&2; exit 1;
}
flags=()
mode=Release
suffix=release
if $sanitize; then
  mode=Debug
  suffix=sanitized
  sanitizer_flags='-fsanitize=address,undefined -fno-omit-frame-pointer'
  flags+=("-DCMAKE_C_FLAGS=$sanitizer_flags" "-DCMAKE_CXX_FLAGS=$sanitizer_flags" '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined')
  # System SWORD/libxml2 libraries are not instrumented; match the existing CI policy.
  export ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0}
  export UBSAN_OPTIONS=${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}
fi
build_dir=${build_dir:-"$root/build/linux-core-$suffix"}
cmake -S "$root/src/core" -B "$build_dir" -G Ninja \
  "-DCMAKE_BUILD_TYPE=$mode" -DBUILD_TESTING=ON "${flags[@]}"
cmake --build "$build_dir" --parallel "$jobs"
ctest --test-dir "$build_dir" --output-on-failure --parallel "$jobs" --timeout 120 --no-tests=error
