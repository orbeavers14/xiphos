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
find_package(LibXml2 REQUIRED)
find_package(PNG 1.6 REQUIRED)
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
if(TARGET PkgConfig::Glib)
  get_target_property(glib_link_flags PkgConfig::Glib INTERFACE_LINK_LIBRARIES)
  get_target_property(glib_link_options PkgConfig::Glib INTERFACE_LINK_OPTIONS)
  if(glib_link_options)
    list(APPEND glib_link_flags ${glib_link_options})
  endif()
else()
  set(glib_link_flags ${GLIB_LIBRARY})
endif()
list(APPEND sword_link_flags ${glib_link_flags} ${LIBXML2_LIBRARIES} ${PNG_LIBRARIES})
foreach(extra IN LISTS XIPHOS_GLIB_EXTRA_LIBRARIES)
  if(IS_ABSOLUTE "${extra}")
    list(APPEND sword_link_flags "${extra}")
  elseif(extra MATCHES "^-")
    separate_arguments(extra_flags NATIVE_COMMAND "${extra}")
    list(APPEND sword_link_flags ${extra_flags})
  else()
    list(APPEND sword_link_flags "-l${extra}")
  endif()
endforeach()
foreach(extra IN LISTS XIPHOS_SWORD_EXTRA_LIBRARIES)
  if(IS_ABSOLUTE "${extra}")
    list(APPEND sword_link_flags "${extra}")
  else()
    list(APPEND sword_link_flags "-l${extra}")
  endif()
endforeach()
set(bridge "${CMAKE_CURRENT_SOURCE_DIR}/Bridge")
set(sdk_link_dependencies)
foreach(library IN LISTS sword_link_flags)
  if(IS_ABSOLUTE "${library}" AND EXISTS "${library}" AND NOT IS_DIRECTORY "${library}")
    list(APPEND sdk_link_dependencies "${library}")
  endif()
endforeach()
set(adapter "${CMAKE_CURRENT_SOURCE_DIR}/CoreReader.swift" "${CMAKE_CURRENT_SOURCE_DIR}/CoreWorker.swift")
set(preview_sources
  "${CMAKE_CURRENT_SOURCE_DIR}/ReaderApp.swift" "${CMAKE_CURRENT_SOURCE_DIR}/BookPane.swift"
  "${CMAKE_CURRENT_SOURCE_DIR}/ImagePreview.swift" "${CMAKE_CURRENT_SOURCE_DIR}/SearchModel.swift"
  "${CMAKE_CURRENT_SOURCE_DIR}/SearchPane.swift" "${CMAKE_CURRENT_SOURCE_DIR}/SearchExportDestination.swift")
set(swift_cache "${CMAKE_CURRENT_BINARY_DIR}/swift-cache")
set(swift_target "${CMAKE_SYSTEM_PROCESSOR}-apple-macosx${CMAKE_OSX_DEPLOYMENT_TARGET}")
set(app "${CMAKE_CURRENT_BINARY_DIR}/XiphosNativePreview.app")
file(MAKE_DIRECTORY "${app}/Contents/MacOS" "${swift_cache}")
configure_file(Info.plist "${app}/Contents/Info.plist" @ONLY)
add_custom_command(OUTPUT "${app}/Contents/MacOS/XiphosNativePreview"
  COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
    -I "${bridge}" ${adapter} ${preview_sources}
    "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
    ${sword_link_flags} -lc++ -framework SwiftUI -framework AppKit -framework WebKit
    -o "${app}/Contents/MacOS/XiphosNativePreview"
  DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift ${preview_sources} Bridge/module.modulemap
    ../../src/core/include/xiphos/core/c_api.h
  VERBATIM)
add_custom_target(macos_preview ALL DEPENDS "${app}/Contents/MacOS/XiphosNativePreview")
if(BUILD_TESTING)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/SearchAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/SearchAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h VERBATIM)
  add_custom_target(macos_search_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke")
  add_test(NAME swift_search_html_export COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" --html "${CMAKE_CURRENT_BINARY_DIR}/core/attribute-search-fixture" "${CMAKE_CURRENT_BINARY_DIR}/core/lexical-snapshots-fixture")
  set_tests_properties(swift_search_html_export PROPERTIES FIXTURES_REQUIRED "attribute_search_fixture;lexical_snapshots_fixture" TIMEOUT 60)
  add_test(NAME swift_search_all_text_export COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" --all-export "${CMAKE_CURRENT_BINARY_DIR}/core/attribute-search-fixture")
  set_tests_properties(swift_search_all_text_export PROPERTIES FIXTURES_REQUIRED attribute_search_fixture TIMEOUT 60)
  add_test(NAME swift_search_text_export COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" --export "${CMAKE_CURRENT_BINARY_DIR}/core/attribute-search-fixture")
  set_tests_properties(swift_search_text_export PROPERTIES FIXTURES_REQUIRED attribute_search_fixture TIMEOUT 60)
  add_test(NAME swift_search_generation COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" --generation "${CMAKE_CURRENT_BINARY_DIR}/core/attribute-search-fixture")
  set_tests_properties(swift_search_generation PROPERTIES FIXTURES_REQUIRED attribute_search_fixture TIMEOUT 60)
  add_test(NAME swift_search_batch COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" --batch "${CMAKE_CURRENT_BINARY_DIR}/core/attribute-search-fixture")
  set_tests_properties(swift_search_batch PROPERTIES FIXTURES_REQUIRED attribute_search_fixture TIMEOUT 60)
  add_test(NAME swift_search_text_options COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" --text-options "${CMAKE_CURRENT_BINARY_DIR}/core/attribute-search-fixture")
  set_tests_properties(swift_search_text_options PROPERTIES FIXTURES_REQUIRED attribute_search_fixture TIMEOUT 30)
  add_test(NAME swift_attribute_search_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" --attributes "${CMAKE_CURRENT_BINARY_DIR}/core/attribute-search-fixture")
  set_tests_properties(swift_attribute_search_adapter PROPERTIES FIXTURES_REQUIRED attribute_search_fixture TIMEOUT 30)
  add_test(NAME swift_search_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/search-fixture")
  set_tests_properties(swift_search_adapter PROPERTIES FIXTURES_REQUIRED search_fixture TIMEOUT 30)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/AdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/AdapterSmoke.swift
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
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/AsyncAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_async_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/async_adapter_smoke")
  add_test(NAME swift_async_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/async_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/fixture")
  set_tests_properties(swift_async_adapter PROPERTIES FIXTURES_REQUIRED sword_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -D XIPHOS_PREVIEW_TEST -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} ${preview_sources} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ReaderModelSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -framework SwiftUI -framework AppKit -framework WebKit
      -o "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift ${preview_sources} Tests/ReaderModelSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_reader_model_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/search_export_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -D XIPHOS_PREVIEW_TEST -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} ${preview_sources} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/SearchExportSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -framework SwiftUI -framework AppKit -framework WebKit
      -o "${CMAKE_CURRENT_BINARY_DIR}/search_export_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift ${preview_sources} Tests/SearchExportSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h VERBATIM)
  add_custom_target(macos_search_export_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/search_export_smoke")
  add_test(NAME reader_model_search_export COMMAND "${CMAKE_CURRENT_BINARY_DIR}/search_export_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/search-fixture")
  set_tests_properties(reader_model_search_export PROPERTIES FIXTURES_REQUIRED search_fixture TIMEOUT 60)
  add_test(NAME reader_model_search COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --search "${CMAKE_CURRENT_BINARY_DIR}/core/search-fixture")
  set_tests_properties(reader_model_search PROPERTIES FIXTURES_REQUIRED search_fixture TIMEOUT 30)
  add_test(NAME reader_model COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/fixture")
  set_tests_properties(reader_model PROPERTIES FIXTURES_REQUIRED sword_fixture)
  add_test(NAME reader_model_book_note_numbers COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --book-note-numbers "${CMAKE_CURRENT_BINARY_DIR}/core/book-note-document-fixture")
  set_tests_properties(reader_model_book_note_numbers PROPERTIES FIXTURES_REQUIRED book_note_document_fixture TIMEOUT 30)
  add_test(NAME reader_model_book_stylesheets COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --book-stylesheets "${CMAKE_CURRENT_BINARY_DIR}/core/book-stylesheet-fixture")
  set_tests_properties(reader_model_book_stylesheets PROPERTIES FIXTURES_REQUIRED book_stylesheet_fixture TIMEOUT 30)
  add_test(NAME reader_model_verse_images COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --verse-images "${CMAKE_CURRENT_BINARY_DIR}/core/verse-image-source-fixture")
  set_tests_properties(reader_model_verse_images PROPERTIES FIXTURES_REQUIRED verse_image_fixture TIMEOUT 30)
  add_test(NAME reader_model_book_images COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --book-images "${CMAKE_CURRENT_BINARY_DIR}/core/book-image-fixture")
  set_tests_properties(reader_model_book_images PROPERTIES FIXTURES_REQUIRED book_image_resource_fixture TIMEOUT 30)
  add_test(NAME reader_model_book_presentation COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --book-presentation "${CMAKE_CURRENT_BINARY_DIR}/core/book-document-presentation-fixture")
  set_tests_properties(reader_model_book_presentation PROPERTIES FIXTURES_REQUIRED book_document_presentation_fixture TIMEOUT 30)
  add_test(NAME reader_model_book_documents COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --book-documents "${CMAKE_CURRENT_BINARY_DIR}/core/book-document-fixture")
  set_tests_properties(reader_model_book_documents PROPERTIES FIXTURES_REQUIRED book_document_fixture TIMEOUT 30)
  add_test(NAME reader_model_books COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --books "${CMAKE_CURRENT_BINARY_DIR}/core/book-entry-fixture")
  set_tests_properties(reader_model_books PROPERTIES FIXTURES_REQUIRED book_entry_fixture TIMEOUT 30)
  add_test(NAME reader_model_content COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --content "${CMAKE_CURRENT_BINARY_DIR}/core/heading-fixture")
  set_tests_properties(reader_model_content PROPERTIES FIXTURES_REQUIRED heading_fixture)
  add_test(NAME reader_model_heading_notes COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --heading-notes "${CMAKE_CURRENT_BINARY_DIR}/core/heading-snapshots-fixture")
  set_tests_properties(reader_model_heading_notes PROPERTIES FIXTURES_REQUIRED heading_snapshots_fixture TIMEOUT 30)
  add_test(NAME reader_model_note_numbers COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --note-numbers "${CMAKE_CURRENT_BINARY_DIR}/core/note-snapshots-fixture")
  set_tests_properties(reader_model_note_numbers PROPERTIES FIXTURES_REQUIRED note_snapshots_fixture TIMEOUT 30)
  add_test(NAME reader_model_double_space COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --double-space "${CMAKE_CURRENT_BINARY_DIR}/core/lexical-snapshots-fixture")
  set_tests_properties(reader_model_double_space PROPERTIES FIXTURES_REQUIRED lexical_snapshots_fixture TIMEOUT 30)
  add_test(NAME reader_model_lexical_layout COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --lexical-layout "${CMAKE_CURRENT_BINARY_DIR}/core/lexical-snapshots-fixture")
  set_tests_properties(reader_model_lexical_layout PROPERTIES FIXTURES_REQUIRED lexical_snapshots_fixture TIMEOUT 30)
  add_test(NAME reader_model_headings COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --headings "${CMAKE_CURRENT_BINARY_DIR}/core/heading-fixture")
  set_tests_properties(reader_model_headings PROPERTIES FIXTURES_REQUIRED heading_fixture)
  add_test(NAME reader_model_commentary COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --commentary "${CMAKE_CURRENT_BINARY_DIR}/core/study-passage-fixture")
  set_tests_properties(reader_model_commentary PROPERTIES FIXTURES_REQUIRED study_passage_fixture)
  add_test(NAME reader_model_options COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --options "${CMAKE_CURRENT_BINARY_DIR}/core/module-preferences-fixture")
  set_tests_properties(reader_model_options PROPERTIES FIXTURES_REQUIRED module_preferences_fixture)
  add_test(NAME reader_model_lexicons COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --lexicons "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture")
  set_tests_properties(reader_model_lexicons PROPERTIES FIXTURES_REQUIRED dictionary_fixture)
  add_test(NAME reader_model_profile COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --profile
    "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture" "${CMAKE_CURRENT_BINARY_DIR}/profile-model-fixture")
  set_tests_properties(reader_model_profile PROPERTIES FIXTURES_REQUIRED dictionary_fixture)
  add_test(NAME reader_model_hover COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --hover "${CMAKE_CURRENT_BINARY_DIR}/core/module-preferences-fixture")
  set_tests_properties(reader_model_hover PROPERTIES FIXTURES_REQUIRED module_preferences_fixture)
  add_test(NAME reader_model_locale COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --locale "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture")
  set_tests_properties(reader_model_locale PROPERTIES FIXTURES_REQUIRED dictionary_fixture ENVIRONMENT "LC_ALL=tr_TR.UTF-8")
  add_test(NAME reader_model_invalid_locale COMMAND "${CMAKE_CURRENT_BINARY_DIR}/reader_model_smoke" --invalid-locale "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture")
  set_tests_properties(reader_model_invalid_locale PROPERTIES FIXTURES_REQUIRED dictionary_fixture ENVIRONMENT "LC_ALL=xiphos-invalid-locale")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/OptionAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/OptionAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_option_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke")
  add_test(NAME swift_option_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/option_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/options-fixture")
  set_tests_properties(swift_option_adapter PROPERTIES FIXTURES_REQUIRED options_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/verse_image_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/VerseImageAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/verse_image_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/VerseImageAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_verse_image_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/verse_image_adapter_smoke")
  add_test(NAME swift_verse_image_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/verse_image_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/verse-image-source-fixture")
  set_tests_properties(swift_verse_image_adapter PROPERTIES FIXTURES_REQUIRED verse_image_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/book_image_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/BookImageAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/book_image_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/BookImageAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_book_image_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/book_image_adapter_smoke")
  add_test(NAME swift_book_image_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/book_image_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/book-image-fixture")
  set_tests_properties(swift_book_image_adapter PROPERTIES FIXTURES_REQUIRED book_image_resource_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/book_presentation_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/BookPresentationAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/book_presentation_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/BookPresentationAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_book_presentation_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/book_presentation_adapter_smoke")
  add_test(NAME swift_book_presentation_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/book_presentation_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/book-document-presentation-fixture")
  set_tests_properties(swift_book_presentation_adapter PROPERTIES FIXTURES_REQUIRED book_document_presentation_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/book_document_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/BookDocumentAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/book_document_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/BookDocumentAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_book_document_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/book_document_adapter_smoke")
  add_test(NAME swift_book_document_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/book_document_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/book-document-fixture")
  set_tests_properties(swift_book_document_adapter PROPERTIES FIXTURES_REQUIRED book_document_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/book_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/BookAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/book_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/BookAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_book_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/book_adapter_smoke")
  add_test(NAME swift_book_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/book_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/book-entry-fixture")
  set_tests_properties(swift_book_adapter PROPERTIES FIXTURES_REQUIRED book_entry_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/DictionaryAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/DictionaryAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_dictionary_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke")
  add_test(NAME swift_dictionary_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/dictionary_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture")
  set_tests_properties(swift_dictionary_adapter PROPERTIES FIXTURES_REQUIRED dictionary_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lexical_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/LexicalAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/lexical_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/LexicalAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_lexical_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lexical_adapter_smoke")
  add_test(NAME swift_lexical_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/lexical_adapter_smoke" "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture")
  set_tests_properties(swift_lexical_adapter PROPERTIES FIXTURES_REQUIRED dictionary_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/profile_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/ProfileAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/profile_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/ProfileAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_profile_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/profile_adapter_smoke")
  add_test(NAME swift_profile_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/profile_adapter_smoke"
    "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture" "${CMAKE_CURRENT_BINARY_DIR}/profile-adapter-fixture")
  set_tests_properties(swift_profile_adapter PROPERTIES FIXTURES_REQUIRED dictionary_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hover_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/HoverAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/hover_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/HoverAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_hover_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/hover_adapter_smoke")
  add_test(NAME swift_hover_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/hover_adapter_smoke"
    "${CMAKE_CURRENT_BINARY_DIR}/core/module-preferences-fixture" "${CMAKE_CURRENT_BINARY_DIR}/core/dictionary-fixture")
  set_tests_properties(swift_hover_adapter PROPERTIES FIXTURES_REQUIRED "module_preferences_fixture;dictionary_fixture")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/passage_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PassageAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/passage_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/PassageAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_passage_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/passage_adapter_smoke")
  add_test(NAME swift_passage_adapter COMMAND "${CMAKE_CURRENT_BINARY_DIR}/passage_adapter_smoke"
    "${CMAKE_CURRENT_BINARY_DIR}/core/study-passage-fixture")
  set_tests_properties(swift_passage_adapter PROPERTIES FIXTURES_REQUIRED study_passage_fixture)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lexical_snapshot_adapter_smoke"
    COMMAND ${SWIFTC} -swift-version 6 -parse-as-library -target "${swift_target}" -module-cache-path "${swift_cache}"
      -I "${bridge}" ${adapter} "${CMAKE_CURRENT_SOURCE_DIR}/Tests/LexicalSnapshotAdapterSmoke.swift"
      "$<TARGET_FILE:xiphos_core>" "$<TARGET_FILE:xiphos_core_render>" "$<TARGET_FILE:xiphos_core_history>" "$<TARGET_FILE:xiphos_core_options>"
      ${sword_link_flags} -lc++ -o "${CMAKE_CURRENT_BINARY_DIR}/lexical_snapshot_adapter_smoke"
    DEPENDS ${sdk_link_dependencies} xiphos_core xiphos_core_render xiphos_core_history xiphos_core_options CoreReader.swift CoreWorker.swift Tests/LexicalSnapshotAdapterSmoke.swift
      Bridge/module.modulemap ../../src/core/include/xiphos/core/c_api.h
    VERBATIM)
  add_custom_target(macos_lexical_snapshot_adapter_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/lexical_snapshot_adapter_smoke")
  add_test(NAME swift_lexical_snapshots COMMAND "${CMAKE_CURRENT_BINARY_DIR}/lexical_snapshot_adapter_smoke"
    "${CMAKE_CURRENT_BINARY_DIR}/core/lexical-snapshots-fixture")
  set_tests_properties(swift_lexical_snapshots PROPERTIES FIXTURES_REQUIRED lexical_snapshots_fixture TIMEOUT 30)
  # Opt-in GUI target, never part of default builds/CI. Generate/verify the
  # fixture before copying it and the app out of protected Documents storage.
  add_custom_target(preview_fixture
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
      -R "^core_reader$" --output-on-failure
    COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/launch-fixture.sh" "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS macos_preview core_reader_test
    VERBATIM)
  add_custom_target(preview_books_fixture
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
      -R "^(reader_model_books|swift_book_adapter)$" --no-tests=error --output-on-failure
    COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/launch-fixture.sh" "${CMAKE_CURRENT_BINARY_DIR}" books
    DEPENDS macos_preview macos_reader_model_test macos_book_adapter_test core_book_entry_test
    VERBATIM)
  add_custom_target(preview_reading_options_fixture
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_CURRENT_BINARY_DIR}"
      -R "^reader_model_options$" --output-on-failure
    COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/launch-fixture.sh" "${CMAKE_CURRENT_BINARY_DIR}" reading-options
    DEPENDS macos_preview macos_reader_model_test core_module_preferences_test core_scoped_options_test
    VERBATIM)
endif()
