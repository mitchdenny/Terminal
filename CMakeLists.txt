cmake_minimum_required(VERSION 3.20)
project(WindowsTerminalLinux CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Platform check ---
if(NOT UNIX)
    message(FATAL_ERROR "This CMake build is for Linux only")
endif()

# --- Find packages ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
find_package(OpenGL REQUIRED)
find_package(fmt REQUIRED)

# --- Global includes ---
# The codebase uses #include "LibraryIncludes.h" and #include <til.h> etc.
# We need to set up include paths so these resolve correctly on Linux.
set(TERMINAL_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(TERMINAL_SRC ${TERMINAL_ROOT}/src)

# Global include directories
# IMPORTANT: platform and stubs must come BEFORE src/inc to override Windows headers
include_directories(BEFORE
    ${TERMINAL_SRC}/platform         # Linux LibraryIncludes.h (overrides src/inc one)
    ${TERMINAL_SRC}/platform/stubs   # Stubs for windows.h, wincon.h, etc.
)
include_directories(
    ${TERMINAL_SRC}/inc              # til.h, conattrs.hpp, DefaultSettings.h
    ${TERMINAL_ROOT}/oss/chromium    # base/numerics/safe_math.h
    ${TERMINAL_ROOT}/oss             # interval_tree, pcg, etc.
    ${TERMINAL_ROOT}/oss/interval_tree # IntervalTree.h
    ${TERMINAL_ROOT}/dep/Console     # Console headers (conmsgl1.h, etc.)
)

# Global compile definitions
add_compile_definitions(
    __linux__
    NOMINMAX
)

# Global compiler flags
add_compile_options(
    -Wno-unknown-pragmas       # #pragma warning is MSVC-specific
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -Wno-switch
    -Wno-reorder
    -Wno-ignored-qualifiers
    -Wno-changes-meaning       # til::rect::size() vs std::size
    -Wno-deprecated-declarations  # std::codecvt_utf8_utf16
    -fPIC
    -mavx2                         # Required for SIMD in Row.cpp
)

# --- Static library: terminal_types ---
# Note: UIA, ThemeUtils, and some utils functions are Windows-specific.
# We exclude them and compile only the portable subset.
add_library(terminal_types STATIC
    src/types/CodepointWidthDetector.cpp
    src/types/ColorFix.cpp
    src/types/colorTable.cpp
    src/types/GlyphWidth.cpp
    src/types/sgrStack.cpp
    src/types/viewport.cpp
)
target_include_directories(terminal_types PUBLIC
    ${TERMINAL_SRC}/types/inc
    ${TERMINAL_SRC}/types
)

# --- Static library: terminal_buffer ---
add_library(terminal_buffer STATIC
    src/buffer/out/cursor.cpp
    src/buffer/out/ImageSlice.cpp
    src/buffer/out/OutputCell.cpp
    src/buffer/out/OutputCellIterator.cpp
    src/buffer/out/OutputCellRect.cpp
    src/buffer/out/OutputCellView.cpp
    src/buffer/out/Row.cpp
    src/buffer/out/search.cpp
    src/buffer/out/TextAttribute.cpp
    src/buffer/out/textBuffer.cpp
    src/buffer/out/textBufferCellIterator.cpp
    src/buffer/out/textBufferTextIterator.cpp
    src/buffer/out/TextColor.cpp
    # UTextAdapter.cpp excluded - requires ICU library
)
target_include_directories(terminal_buffer PUBLIC
    ${TERMINAL_SRC}/buffer/out
    ${TERMINAL_SRC}/types/inc
    ${TERMINAL_SRC}/types
)
target_link_libraries(terminal_buffer PUBLIC terminal_types)

# --- Static library: vt_parser ---
add_library(vt_parser STATIC
    src/terminal/parser/base64.cpp
    src/terminal/parser/InputStateMachineEngine.cpp
    src/terminal/parser/OutputStateMachineEngine.cpp
    src/terminal/parser/stateMachine.cpp
    src/terminal/parser/tracing.cpp
)
target_include_directories(vt_parser PUBLIC
    ${TERMINAL_SRC}/terminal/parser
    ${TERMINAL_SRC}/terminal/parser/lib
    ${TERMINAL_SRC}/terminal/input
    ${TERMINAL_SRC}/terminal/input/lib
    ${TERMINAL_SRC}/types/inc
)
target_link_libraries(vt_parser PUBLIC terminal_types)

# --- Static library: vt_adapter ---
add_library(vt_adapter STATIC
    src/terminal/adapter/adaptDispatch.cpp
    src/terminal/adapter/adaptDispatchGraphics.cpp
    src/terminal/adapter/FontBuffer.cpp
    # InteractDispatch.cpp excluded - depends on console host ServiceLocator
    src/terminal/adapter/MacroBuffer.cpp
    src/terminal/adapter/PageManager.cpp
    src/terminal/adapter/SixelParser.cpp
    src/terminal/adapter/terminalOutput.cpp
)
target_include_directories(vt_adapter PUBLIC
    ${TERMINAL_SRC}/terminal/adapter
    ${TERMINAL_SRC}/terminal/adapter/lib
    ${TERMINAL_SRC}/terminal/input
    ${TERMINAL_SRC}/terminal/input/lib
    ${TERMINAL_SRC}/host      # some headers reference host types
)
target_link_libraries(vt_adapter PUBLIC vt_parser terminal_buffer)

# --- Static library: renderer_base ---
add_library(renderer_base STATIC
    src/renderer/base/CSSLengthPercentage.cpp
    src/renderer/base/FontInfoBase.cpp
    src/renderer/base/fontinfo.cpp
    src/renderer/base/FontInfoDesired.cpp
    # FontResource.cpp excluded - uses Win32 font APIs (AddFontMemResourceEx)
    src/renderer/base/RenderEngineBase.cpp
    src/renderer/base/renderer.cpp
    src/renderer/base/RenderSettings.cpp
)
target_include_directories(renderer_base PUBLIC
    ${TERMINAL_SRC}/renderer/inc
    ${TERMINAL_SRC}/renderer/base
)
target_link_libraries(renderer_base PUBLIC terminal_buffer terminal_types)

# --- Static library: terminal_core ---
add_library(terminal_core STATIC
    src/cascadia/TerminalCore/Terminal.cpp
    src/cascadia/TerminalCore/TerminalApi.cpp
    src/cascadia/TerminalCore/terminalrenderdata.cpp
    src/cascadia/TerminalCore/TerminalSelection.cpp
)
target_include_directories(terminal_core PUBLIC
    ${TERMINAL_SRC}/cascadia/TerminalCore
    ${TERMINAL_SRC}/cascadia/inc
)
target_link_libraries(terminal_core PUBLIC
    vt_parser
    vt_adapter
    terminal_buffer
    renderer_base
    terminal_types
)

# TODO: Phase 2 - OpenGL renderer library
# TODO: Phase 3 - GTK4 terminal widget
# TODO: Phase 4 - Main executable

message(STATUS "Configured WindowsTerminalLinux for ${CMAKE_SYSTEM_NAME}")
