cmake_minimum_required(VERSION 3.22)
project(gp16-editor-qt
  VERSION 0.1.0
  DESCRIPTION "Roland GP-16 editor (Qt + libremidi)"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Qt 6
# ---------------------------------------------------------------------------
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

# ---------------------------------------------------------------------------
# libremidi (FetchContent — recommended MIDI backend)
# ---------------------------------------------------------------------------
include(FetchContent)
set(LIBREMIDI_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBREMIDI_TESTS OFF CACHE BOOL "" FORCE)
set(LIBREMIDI_NO_BOOST ON CACHE BOOL "" FORCE)

# Optional offline: cmake -Dlibremidi_ROOT=/path/to/libremidi-checkout
if(libremidi_ROOT)
  message(STATUS "Using local libremidi at ${libremidi_ROOT}")
  add_subdirectory("${libremidi_ROOT}" "${CMAKE_BINARY_DIR}/libremidi-build")
else()
  FetchContent_Declare(
    libremidi
    GIT_REPOSITORY https://github.com/celtera/libremidi.git
    GIT_TAG        v5.3.1
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(libremidi)
endif()

# ---------------------------------------------------------------------------
# Shared Roland SysEx helpers
# ---------------------------------------------------------------------------
add_library(gp16_sysex STATIC
  src/RolandSysex.cpp
  src/RolandSysex.h
  src/Patch.cpp
  src/Patch.h
  src/PatchBank.cpp
  src/PatchBank.h
)
target_include_directories(gp16_sysex PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# Qt GUI application
# ---------------------------------------------------------------------------
add_executable(gp16-editor-qt
  src/main.cpp
  src/MainWindow.h
  src/MainWindow.cpp
  src/PatchListPanel.h
  src/PatchListPanel.cpp
  src/MidiService.h
  src/MidiService.cpp
)

target_include_directories(gp16-editor-qt PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gp16-editor-qt PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  libremidi
  gp16_sysex
)

# ---------------------------------------------------------------------------
# CLI full dump tool (no Qt)
# ---------------------------------------------------------------------------
add_executable(gp16-dump
  src/cli/gp16_dump.cpp
)
target_link_libraries(gp16-dump PRIVATE
  libremidi
  gp16_sysex
)

if(UNIX AND NOT APPLE)
  find_package(ALSA)
  if(ALSA_FOUND)
    target_link_libraries(gp16-editor-qt PRIVATE ALSA::ALSA)
    target_link_libraries(gp16-dump PRIVATE ALSA::ALSA)
  endif()
endif()

install(TARGETS gp16-editor-qt gp16-dump RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# Unit tests for gp16_sysex (patch parsing), run against the committed captures
# ---------------------------------------------------------------------------
enable_testing()

add_executable(gp16_sysex_tests
  tests/test_patch_parsing.cpp
)
target_link_libraries(gp16_sysex_tests PRIVATE gp16_sysex)
target_compile_definitions(gp16_sysex_tests PRIVATE
  GP16_TEST_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_test(NAME gp16_sysex_tests COMMAND gp16_sysex_tests)

add_executable(gp16_librarian_tests
  tests/test_librarian.cpp
  src/PatchListPanel.h
  src/PatchListPanel.cpp
)
target_link_libraries(gp16_librarian_tests PRIVATE
  Qt6::Widgets
  gp16_sysex
)
target_compile_definitions(gp16_librarian_tests PRIVATE
  GP16_TEST_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)
add_test(NAME gp16_librarian_tests COMMAND gp16_librarian_tests)
