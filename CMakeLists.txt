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

# -------------------------------------------------------------------------
# Options
# -------------------------------------------------------------------------

option(SPECTRUM_SHOW_CONSOLE_IN_RELEASE "Attach a console in RelWithDebInfo" ON)

# -------------------------------------------------------------------------
# Paths
# -------------------------------------------------------------------------

set(RAYLIB_ROOT "${CMAKE_SOURCE_DIR}/raylib" CACHE PATH "Path to Raylib root")
set(IMGUI_ROOT "${CMAKE_SOURCE_DIR}/imgui_library" CACHE PATH "Path to ImGui root")
set(BLEND2D_ROOT "${CMAKE_SOURCE_DIR}/../blend2d" CACHE PATH "Path to Blend2D root")

# -------------------------------------------------------------------------
# Helpers
# -------------------------------------------------------------------------

function(spectrum_require_file FILE_PATH HINT)
    if(NOT EXISTS "${FILE_PATH}")
        message(FATAL_ERROR "${HINT}\n  missing: ${FILE_PATH}")
    endif()
endfunction()

function(spectrum_first_existing OUT_VAR)
    set(_found "")
    foreach(_candidate IN LISTS ARGN)
        if(EXISTS "${_candidate}")
            set(_found "${_candidate}")
            break()
        endif()
    endforeach()
    set(${OUT_VAR} "${_found}" PARENT_SCOPE)
endfunction()

# -------------------------------------------------------------------------
# Raylib / ImGui
# -------------------------------------------------------------------------

spectrum_require_file("${RAYLIB_ROOT}/include/raylib.h" "raylib not found. Set -DRAYLIB_ROOT=<path>")
spectrum_require_file("${IMGUI_ROOT}/include/imgui.h" "ImGui not found. Set -DIMGUI_ROOT=<path>")

# -------------------------------------------------------------------------
# Blend2D 0.21.x  (public C++ API: BLContext / BLImage / BLRgba32)
# -------------------------------------------------------------------------

set(BLEND2D_INCLUDE_DIRS "")

if(EXISTS "${BLEND2D_ROOT}/blend2d/blend2d.h")
    # Layout: <root>/blend2d/blend2d.h  and  <root>/blend2d/core/...
    list(APPEND BLEND2D_INCLUDE_DIRS "${BLEND2D_ROOT}/blend2d" "${BLEND2D_ROOT}")
elseif(EXISTS "${BLEND2D_ROOT}/src/blend2d.h")
    list(APPEND BLEND2D_INCLUDE_DIRS "${BLEND2D_ROOT}/src")
elseif(EXISTS "${BLEND2D_ROOT}/include/blend2d.h")
    list(APPEND BLEND2D_INCLUDE_DIRS "${BLEND2D_ROOT}/include")
elseif(EXISTS "${BLEND2D_ROOT}/blend2d.h")
    list(APPEND BLEND2D_INCLUDE_DIRS "${BLEND2D_ROOT}")
endif()

if(NOT BLEND2D_INCLUDE_DIRS)
    message(FATAL_ERROR
        "blend2d.h not found in ${BLEND2D_ROOT}.\n"
        "Use -DBLEND2D_ROOT=<path> to specify the Blend2D tree.")
endif()

spectrum_first_existing(BLEND2D_LIB_RELEASE
    "${BLEND2D_ROOT}/build/Release/blend2d.lib"
    "${BLEND2D_ROOT}/build/release/blend2d.lib"
    "${BLEND2D_ROOT}/lib/Release/blend2d.lib"
    "${BLEND2D_ROOT}/lib/blend2d.lib"
)

spectrum_first_existing(BLEND2D_LIB_DEBUG
    "${BLEND2D_ROOT}/build/Debug/blend2d.lib"
    "${BLEND2D_ROOT}/build/debug/blend2d.lib"
    "${BLEND2D_ROOT}/lib/Debug/blend2d.lib"
    "${BLEND2D_LIB_RELEASE}"
)

if(NOT BLEND2D_LIB_RELEASE)
    message(FATAL_ERROR
        "blend2d.lib (Release) not found under ${BLEND2D_ROOT}.\n"
        "Build Blend2D first, e.g.:\n"
        "  cmake -S \"${BLEND2D_ROOT}\" -B \"${BLEND2D_ROOT}/build\" -G \"Visual Studio 17 2022\" -A x64 -DBLEND2D_STATIC=ON\n"
        "  cmake --build \"${BLEND2D_ROOT}/build\" --config Release")
endif()

if(NOT EXISTS "${BLEND2D_LIB_DEBUG}")
    message(WARNING "Debug blend2d.lib not found, Debug builds will link the Release library")
    set(BLEND2D_LIB_DEBUG "${BLEND2D_LIB_RELEASE}")
endif()

add_library(Blend2D STATIC IMPORTED)
set_target_properties(Blend2D PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${BLEND2D_INCLUDE_DIRS}"
    INTERFACE_COMPILE_DEFINITIONS "BL_STATIC"
    IMPORTED_LOCATION             "${BLEND2D_LIB_RELEASE}"
    IMPORTED_LOCATION_DEBUG       "${BLEND2D_LIB_DEBUG}"
    IMPORTED_LOCATION_RELEASE     "${BLEND2D_LIB_RELEASE}"
    IMPORTED_LOCATION_RELWITHDEBINFO "${BLEND2D_LIB_RELEASE}"
    IMPORTED_LOCATION_MINSIZEREL  "${BLEND2D_LIB_RELEASE}"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
)

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
    IMGUI_USE_DLL
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
# Includes / link dirs
# -------------------------------------------------------------------------

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/Resources"
    "${RAYLIB_ROOT}/include"
    "${IMGUI_ROOT}/include"
)

target_link_directories(${PROJECT_NAME} PRIVATE
    "${RAYLIB_ROOT}/lib"
    "${IMGUI_ROOT}/lib"
)

# -------------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------------

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui.lib
    raylibdll
    Blend2D
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
# Post-build: runtime DLLs
# -------------------------------------------------------------------------

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RAYLIB_ROOT}/lib/raylib.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${IMGUI_ROOT}/lib/imgui.dll"   "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E echo "Build OK -> $<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Copying runtime DLLs"
    VERBATIM
)

# -------------------------------------------------------------------------
# Install
# -------------------------------------------------------------------------

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
    "${RAYLIB_ROOT}/lib/raylib.dll"
    "${IMGUI_ROOT}/lib/imgui.dll"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

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
message(STATUS "raylib:    ${RAYLIB_ROOT}")
message(STATUS "ImGui:     ${IMGUI_ROOT}")
message(STATUS "Blend2D:   ${BLEND2D_ROOT}")
message(STATUS "  includes: ${BLEND2D_INCLUDE_DIRS}")
message(STATUS "  release:  ${BLEND2D_LIB_RELEASE}")
message(STATUS "  debug:    ${BLEND2D_LIB_DEBUG}")
message(STATUS "Output:    ${SPECTRUM_OUTPUT_DIR}")