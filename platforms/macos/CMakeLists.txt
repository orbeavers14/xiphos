# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version" FORCE)
endif()
project(xiphos_macos_preview LANGUAGES C CXX)
if(NOT APPLE)
  message(FATAL_ERROR "This preview requires macOS")
endif()
include(CTest)
find_program(SWIFTC swiftc REQUIRED)
set(XIPHOS_BUILD_NATIVE_CORE ON)
add_subdirectory(../../src/core core)
if(TARGET PkgConfig::Sword)
  get_target_property(sword_link_flags PkgConfig::Sword INTERFACE_LINK_LIBRARIES)
  get_target_property(sword_link_options PkgConfig::Sword INTERFACE_LINK_OPTIONS)
  if(sword_link_options)
    list(APPEND sword_link_flags ${sword_link_options})
  endif()
else()
  set(sword_link_flags ${SWORD_LIBRARY})
endif()
foreach(extra IN LISTS XIPHOS_SWORD_EXTRA_LIBRARIES)
  if(IS_ABSOLUTE "${extra}")
    list(APPEND sword_link_flags "${extra}")
  else()
    list(APPEND sword_link_flags "-l${extra}")
  endif()
endforeach()
set(bridge "${CMAKE_CURRENT_SOURCE_DIR}/Bridge")
set(adapter "${CMAKE_CURRENT_SOURCE_DIR}/CoreReader.swift" "${CMAKE_CURRENT_SOURCE_DIR}/CoreWorker.swift")
set(swift_cache "${CMAKE_CURRENT_BINARY_DIR}/swift-cache")
set(swift_target "${CMAKE_SYSTEM_PROCESSOR}-apple-macosx${CMAKE_OSX_DEPLOYMENT_TARGET}")
set(app "${CMAKE_CURRENT_BINARY_DIR}/XiphosNativePreview.app")
file(MAKE_DIRECTORY "${app}/Contents/MacOS" "${swift_cache}")
configure_file(Info.plist "${app}/Contents/Info.plist" @ONLY)
add_custom_command(OUTPUT "${app}/Contents/MacOS/XiphosNativePreview"
  COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
    -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/ReaderApp.swift"
    "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
    ${sword_link_flags} -lc++ -framework SwiftUI -framework AppKit -framework WebKit
    -o "${app}/Contents/MacOS/XiphosNativePreview"
  DEPENDS xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift ReaderApp.swift Bridge/module.modulemap
    ../../src/core/include/xiphos/core/c_api.h
  VERBATIM)
add_custom_target(macos_preview ALL DEPENDS "${app}/Contents/MacOS/XiphosNativePreview")
if(BUILD_TESTING)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/AdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/adapter_smoke"
    DEPENDS xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/AdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/adapter_smoke")
  add_test(NAME swift_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/fixture")
  set_tests_properties(swift_adapter PROPERTIES FIXTURES_REQUIRED sword_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/async_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/AsyncAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/async_adapter_smoke"
    DEPENDS xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/AsyncAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_async_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/async_adapter_smoke")
  add_test(NAME swift_async_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/async_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/fixture")
  set_tests_properties(swift_async_adapter PROPERTIES FIXTURES_REQUIRED sword_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -D XIPHOS_PREVIEW_TEST -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/ReaderApp.swift" "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ReaderModelSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -framework SwiftUI -framework AppKit -framework WebKit
      -o "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke"
    DEPENDS xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift ReaderApp.swift Tests/ReaderModelSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_reader_model_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke")
  add_test(NAME reader_model COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/fixture")
  set_tests_properties(reader_model PROPERTIES FIXTURES_REQUIRED sword_fixture)
  add_test(NAME reader_model_options COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --options "${CMAKE_CURRENT_BINARY_DIR}/core/module-preferences-fixture")
  set_tests_properties(reader_model_options PROPERTIES FIXTURES_REQUIRED module_preferences_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/OptionAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke"
    DEPENDS xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/OptionAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_option_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke")
  add_test(NAME swift_option_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/options-fixture")
  set_tests_properties(swift_option_adapter PROPERTIES FIXTURES_REQUIRED options_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/DictionaryAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke"
    DEPENDS xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/DictionaryAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_dictionary_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke")
  add_test(NAME swift_dictionary_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture")
  set_tests_properties(swift_dictionary_adapter PROPERTIES FIXTURES_REQUIRED dictionary_fixture)
  # Opt-in GUI target, never part of default builds/CI. Generate/verify the
  # fixture before copying it and the app out of protected Documents storage.
  add_custom_target(preview_fixture
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
      -R "^core_reader$" --output-on-failure
    COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/launch-fixture.sh" "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS macos_preview core_reader_test
    VERBATIM)
  add_custom_target(preview_reading_options_fixture
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
      -R "^reader_model_options$" --output-on-failure
    COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/launch-fixture.sh" "${CMAKE_CURRENT_BINARY_DIR}" reading-options
    DEPENDS macos_preview macos_reader_model_test core_module_preferences_test core_scoped_options_test
    VERBATIM)
endif()
