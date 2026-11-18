# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.16)
project(xiphos_windows_reader LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(XIPHOS_BUILD_NATIVE_CORE ON)
add_subdirectory(../../src/core core)

add_library(xiphos_reader_core SHARED reader_core.def)
set_target_properties(xiphos_reader_core PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(xiphos_reader_core PRIVATE Xiphos::Core)
