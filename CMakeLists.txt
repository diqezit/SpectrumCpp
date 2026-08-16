cmake_minimum_required(VERSION 3.20)

project(SpectrumCpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Resolve GNUInstallDirs at the top level first. CMake 4.2 can recurse
# infinitely if a FetchContent subproject is the first to include it.
include(GNUInstallDirs)

# -------------------------------------------------------------------------
# Options
# -------------------------------------------------------------------------

option(SPECTRUM_SHOW_CONSOLE_IN_RELEASE "Attach a console in RelWithDebInfo" ON)

# -------------------------------------------------------------------------
# Dependencies (Submodules & FetchContent)
# -------------------------------------------------------------------------

# 1. ImGui (from git submodule, compiled manually)
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/imgui/backends
)

# 2. Blend2D (FetchContent). Blend2D compiles asmjit into itself —
#    do NOT add_subdirectory(asmjit) / MakeAvailable(asmjit).
include(FetchContent)
message(STATUS "Fetching AsmJit and Blend2D...")

FetchContent_Declare(
    asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    blend2d
    GIT_REPOSITORY https://github.com/blend2d/blend2d.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# Download asmjit sources only. FetchContent_Populate(<name>) is deprecated
# by CMP0169 in CMake 3.30+, but this is the supported way to skip
# add_subdirectory so Blend2D can embed the sources itself.
FetchContent_GetProperties(asmjit)
if(NOT asmjit_POPULATED)
    if(POLICY CMP0169)
        cmake_policy(PUSH)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Populate(asmjit)
    if(POLICY CMP0169)
        cmake_policy(POP)
    endif()
endif()

set(ASMJIT_DIR "${asmjit_SOURCE_DIR}" CACHE PATH "asmjit source dir for Blend2D" FORCE)
set(BLEND2D_STATIC ON CACHE BOOL "Build Blend2D as a static library" FORCE)
set(BLEND2D_NO_INSTALL ON CACHE BOOL "Do not install Blend2D with the app" FORCE)

FetchContent_MakeAvailable(blend2d)

# Current Blend2D (master):
#   <root>/blend2d/blend2d.h          ->  #include <blend2d/blend2d.h>
# Older Blend2D:
#   <root>/src/blend2d.h              ->  #include <blend2d.h>
#
# Project sources still use #include <blend2d.h>, so for the new layout
# we also add <root>/blend2d as a search path.
set(SPECTRUM_BLEND2D_INCLUDES "")
if(EXISTS "${blend2d_SOURCE_DIR}/blend2d/blend2d.h")
    list(APPEND SPECTRUM_BLEND2D_INCLUDES
        "${blend2d_SOURCE_DIR}"
        "${blend2d_SOURCE_DIR}/blend2d")
    message(STATUS "Blend2D headers: new layout (${blend2d_SOURCE_DIR})")
elseif(EXISTS "${blend2d_SOURCE_DIR}/src/blend2d.h")
    list(APPEND SPECTRUM_BLEND2D_INCLUDES "${blend2d_SOURCE_DIR}/src")
    message(STATUS "Blend2D headers: old layout (${blend2d_SOURCE_DIR}/src)")
else()
    message(FATAL_ERROR
        "blend2d.h not found under '${blend2d_SOURCE_DIR}'. "
        "Expected blend2d/blend2d.h or src/blend2d.h")
endif()

# -------------------------------------------------------------------------
# Resources
# -------------------------------------------------------------------------

configure_file(
    "${CMAKE_SOURCE_DIR}/Resources/resource.rc.in"
    "${CMAKE_BINARY_DIR}/resource.rc"
    @ONLY
)

# -------------------------------------------------------------------------
# Sources
# -------------------------------------------------------------------------

set(SPECTRUM_APP_SOURCES
    App/Application.cpp
    App/Core.h
)

set(SPECTRUM_AUDIO_SOURCES
    Audio/Audio.h
    Audio/Capture/AudioCapture.cpp
    Audio/Capture/AudioCapture.h
    Audio/Capture/AudioCaptureEngine.cpp
    Audio/Capture/AudioCaptureEngine.h
    Audio/Capture/WASAPIHelper.cpp
    Audio/Capture/WASAPIHelper.h
    Audio/Processing/AudioBuffer.cpp
    Audio/Processing/AudioBuffer.h
    Audio/Processing/FFTProcessor.cpp
    Audio/Processing/FFTProcessor.h
    Audio/Processing/FrequencyMapper.cpp
    Audio/Processing/FrequencyMapper.h
    Audio/Processing/GainNormalizer.cpp
    Audio/Processing/GainNormalizer.h
    Audio/Processing/SpectrumAnalyzer.cpp
    Audio/Processing/SpectrumAnalyzer.h
    Audio/Processing/SpectrumPostProcessor.cpp
    Audio/Processing/SpectrumPostProcessor.h
    Audio/Sources/AnimatedAudioSource.cpp
    Audio/Sources/AnimatedAudioSource.h
    Audio/Sources/IAudioSource.h
    Audio/Sources/RealtimeAudioSource.cpp
    Audio/Sources/RealtimeAudioSource.h
)

set(SPECTRUM_COMMON_SOURCES
    Common/Common.h
    Common/EventBus.h
    Common/SpectrumTypes.h
    Common/Types.h
)

set(SPECTRUM_GRAPHICS_SOURCES
    Graphics/RendererManager.h
    Graphics/API/GraphicsHelpers.h
    Graphics/API/Draw.h
    Graphics/API/GraphicsSurface.h
    Graphics/API/D3D11Backend.h
    Graphics/Base/BaseRenderer.h
    Graphics/Base/PeakTracker.h
    Graphics/Base/RenderUtils.h
    Graphics/Visualizers/BarsRenderer.h
    Graphics/Visualizers/CircularWaveRenderer.h
    Graphics/Visualizers/CubesRenderer.h
    Graphics/Visualizers/FireRenderer.h
    Graphics/Visualizers/GaugeRenderer.h
    Graphics/Visualizers/KenwoodBarsRenderer.h
    Graphics/Visualizers/LedPanelRenderer.h
    Graphics/Visualizers/MatrixLedRenderer.h
    Graphics/Visualizers/ParticlesRenderer.h
    Graphics/Visualizers/PolylineWaveRenderer.h
    Graphics/Visualizers/SphereRenderer.h
    Graphics/Visualizers/WaveRenderer.h
    Graphics/Visualizers/Settings/QualityPresets.h
    Graphics/Visualizers/Settings/QualityTraits.h
)

set(SPECTRUM_PLATFORM_SOURCES
    Platform/InputManager.h
    Platform/MainWindow.h
    Platform/Messages.h
    Platform/UIWindow.h
    Platform/WindowBase.h
    Platform/WindowManager.h
)

set(SPECTRUM_UI_SOURCES
    UI/ImGuiContext.h
    UI/UI.h
    UI/Widgets.h
)

set(SPECTRUM_RESOURCE_SOURCES
    Resources/Resource.h
    "${CMAKE_BINARY_DIR}/resource.rc"
)

set(SOURCES
    ${SPECTRUM_APP_SOURCES}
    ${SPECTRUM_AUDIO_SOURCES}
    ${SPECTRUM_COMMON_SOURCES}
    ${SPECTRUM_GRAPHICS_SOURCES}
    ${SPECTRUM_PLATFORM_SOURCES}
    ${SPECTRUM_UI_SOURCES}
    Resources/Resource.h
)

# -------------------------------------------------------------------------
# Executable
# -------------------------------------------------------------------------

add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    "${CMAKE_BINARY_DIR}/resource.rc"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# -------------------------------------------------------------------------
# VS filters
# -------------------------------------------------------------------------

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${SOURCES})
source_group("Resources" FILES
    "${CMAKE_BINARY_DIR}/resource.rc"
    Resources/resource.rc.in
    Resources/Resource.h
)

# -------------------------------------------------------------------------
# Preprocessor
# -------------------------------------------------------------------------

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    BL_STATIC
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    $<$<OR:$<CONFIG:Debug>,$<AND:$<BOOL:${SPECTRUM_SHOW_CONSOLE_IN_RELEASE}>,$<CONFIG:RelWithDebInfo>>>:SHOW_CONSOLE=1>
)

# -------------------------------------------------------------------------
# Compiler / Linker
# -------------------------------------------------------------------------

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /EHsc
        /permissive-
        /utf-8
        /Zc:__cplusplus
        /Zc:inline
        /wd4828
        $<$<CONFIG:Debug>:/Zi /Od /RTC1>
        $<$<CONFIG:Release>:/O2 /Ob2>
        $<$<CONFIG:RelWithDebInfo>:/Zi /O2 /Ob1>
        $<$<CONFIG:MinSizeRel>:/O1 /Ob1>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/DEBUG /INCREMENTAL>
        $<$<CONFIG:Release>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF>
        $<$<CONFIG:MinSizeRel>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -------------------------------------------------------------------------
# Includes
# -------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/Resources"
    ${SPECTRUM_BLEND2D_INCLUDES}
)

# -------------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------------

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    blend2d::blend2d
    d2d1
    d3d11
    dwrite
    dxgi
    ole32
    uuid
    dwmapi
    windowscodecs
    gdi32
    user32
)

# -------------------------------------------------------------------------
# Output
# -------------------------------------------------------------------------

set(SPECTRUM_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin")

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${SPECTRUM_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${SPECTRUM_OUTPUT_DIR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${SPECTRUM_OUTPUT_DIR}/Release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${SPECTRUM_OUTPUT_DIR}/RelWithDebInfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${SPECTRUM_OUTPUT_DIR}/MinSizeRel"
    VS_DEBUGGER_WORKING_DIRECTORY           "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# -------------------------------------------------------------------------
# Install
# -------------------------------------------------------------------------

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# -------------------------------------------------------------------------
# IDE
# -------------------------------------------------------------------------

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# -------------------------------------------------------------------------
# Summary
# -------------------------------------------------------------------------

message(STATUS "=== ${PROJECT_NAME} v${PROJECT_VERSION} ===")
message(STATUS "C++${CMAKE_CXX_STANDARD} | ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "ImGui:     Built from submodule (Static)")
message(STATUS "Blend2D:   Fetched via FetchContent (Static, embedded asmjit)")
message(STATUS "AsmJit:    ${ASMJIT_DIR}")
message(STATUS "Blend2D:   ${blend2d_SOURCE_DIR}")
message(STATUS "Includes:  ${SPECTRUM_BLEND2D_INCLUDES}")
message(STATUS "Output:    ${SPECTRUM_OUTPUT_DIR}")