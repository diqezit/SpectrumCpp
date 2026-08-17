cmake_minimum_required(VERSION 3.20)

project(SpectrumCpp VERSION 1.0.0 LANGUAGES C CXX)

# -------------------------------------------------------------------------
# Global Policies
# -------------------------------------------------------------------------
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

include(GNUInstallDirs)
include(CheckIPOSupported)

# -------------------------------------------------------------------------
# Options
# -------------------------------------------------------------------------
option(
    SPECTRUM_SHOW_CONSOLE_IN_RELEASE 
    "Attach a console in RelWithDebInfo" 
    ON
)

# -------------------------------------------------------------------------
# Dependencies (Submodules & FetchContent)
# -------------------------------------------------------------------------

# 1. ImGui (Submodule)
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
    ${PROJECT_SOURCE_DIR}/imgui
    ${PROJECT_SOURCE_DIR}/imgui/backends
)

# 2. Blend2D & AsmJit (FetchContent)
include(FetchContent)
message(STATUS "Fetching AsmJit and Blend2D...")

FetchContent_Declare(
    asmjit
    GIT_REPOSITORY https://github.com/asmjit/asmjit.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    blend2d
    GIT_REPOSITORY https://github.com/blend2d/blend2d.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)

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

set(ASMJIT_DIR "${asmjit_SOURCE_DIR}" CACHE PATH "asmjit source dir" FORCE)
set(BLEND2D_STATIC ON CACHE BOOL "Build Blend2D as static" FORCE)
set(BLEND2D_NO_INSTALL ON CACHE BOOL "No Blend2D install" FORCE)

FetchContent_MakeAvailable(blend2d)

set(SPECTRUM_BLEND2D_INCLUDES "")
if(EXISTS "${blend2d_SOURCE_DIR}/blend2d/blend2d.h")
    list(APPEND SPECTRUM_BLEND2D_INCLUDES 
        "${blend2d_SOURCE_DIR}" 
        "${blend2d_SOURCE_DIR}/blend2d"
    )
elseif(EXISTS "${blend2d_SOURCE_DIR}/src/blend2d.h")
    list(APPEND SPECTRUM_BLEND2D_INCLUDES "${blend2d_SOURCE_DIR}/src")
else()
    message(FATAL_ERROR "blend2d.h not found under '${blend2d_SOURCE_DIR}'.")
endif()

# 3. KissFFT
message(STATUS "Fetching KissFFT...")
set(KISSFFT_TEST OFF CACHE BOOL "" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "" FORCE)
set(KISSFFT_STATIC ON CACHE BOOL "" FORCE)
set(KISSFFT_PKGCONFIG OFF CACHE BOOL "" FORCE)
set(KISSFFT_OPENMP OFF CACHE BOOL "" FORCE)
set(KISSFFT_DATATYPE "float" CACHE STRING "" FORCE)

FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG        131.1.0
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(kissfft)

# 4. miniaudio
message(STATUS "Fetching miniaudio...")
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.22
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
    if(POLICY CMP0169)
        cmake_policy(PUSH)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Populate(miniaudio)
    if(POLICY CMP0169)
        cmake_policy(POP)
    endif()
endif()

set(MINIAUDIO_IMPL "${CMAKE_BINARY_DIR}/miniaudio_impl.c")
file(WRITE "${MINIAUDIO_IMPL}"
    "#define MA_NO_DECODING\n"
    "#define MA_NO_ENCODING\n"
    "#define MA_NO_GENERATION\n"
    "#define MA_NO_ENGINE\n"
    "#define MA_NO_RESOURCE_MANAGER\n"
    "#define MA_NO_NODE_GRAPH\n"
    "#define MA_NO_FLAC\n"
    "#define MA_NO_MP3\n"
    "#define MA_NO_WAV\n"
    "#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS\n"
    "#define MA_ENABLE_WASAPI\n"
    "#define MINIAUDIO_IMPLEMENTATION\n"
    "#include \"miniaudio.h\"\n"
)

add_library(miniaudio STATIC "${MINIAUDIO_IMPL}")
target_include_directories(miniaudio PUBLIC "${miniaudio_SOURCE_DIR}")
target_compile_definitions(miniaudio PUBLIC
    MA_NO_DECODING 
    MA_NO_ENCODING 
    MA_NO_GENERATION 
    MA_NO_ENGINE
    MA_NO_RESOURCE_MANAGER 
    MA_NO_NODE_GRAPH 
    MA_ENABLE_ONLY_SPECIFIC_BACKENDS 
    MA_ENABLE_WASAPI
)

# -------------------------------------------------------------------------
# Resources
# -------------------------------------------------------------------------
configure_file(
    "${PROJECT_SOURCE_DIR}/Resources/resource.rc.in"
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
    Audio/Capture/AudioCapture.hpp 
    Audio/Processing/Spectrum.hpp
    Audio/Sources/AnimatedAudioSource.hpp 
    Audio/Sources/IAudioSource.hpp 
    Audio/Sources/RealtimeAudioSource.hpp
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

# -------------------------------------------------------------------------
# VS Filters
# -------------------------------------------------------------------------
source_group(TREE "${PROJECT_SOURCE_DIR}" FILES ${SOURCES})
source_group("Resources" FILES 
    "${CMAKE_BINARY_DIR}/resource.rc" 
    Resources/resource.rc.in 
    Resources/Resource.h
)

# -------------------------------------------------------------------------
# Preprocessor
# -------------------------------------------------------------------------
set(SPECTRUM_CONSOLE_DEF "$<$<OR:$<CONFIG:Debug>,$<AND:$<BOOL:${SPECTRUM_SHOW_CONSOLE_IN_RELEASE}>,$<CONFIG:RelWithDebInfo>>>:SHOW_CONSOLE=1>")

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE 
    _UNICODE 
    NOMINMAX 
    WIN32_LEAN_AND_MEAN 
    BL_STATIC
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    ${SPECTRUM_CONSOLE_DEF}
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
        /MP
        $<$<CONFIG:Debug>:/Zi /Od /RTC1>
        $<$<CONFIG:Release>:/O2 /Ob2 /fp:fast>
        $<$<CONFIG:RelWithDebInfo>:/Zi /O2 /Ob1 /fp:fast>
        $<$<CONFIG:MinSizeRel>:/O1 /Ob1>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/DEBUG /INCREMENTAL>
        $<$<CONFIG:Release>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF>
        $<$<CONFIG:MinSizeRel>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall 
        -Wextra 
        -Wpedantic
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -march=native -ffast-math>
    )
endif()

# -------------------------------------------------------------------------
# LTO (Link Time Optimization)
# -------------------------------------------------------------------------
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(ipo_supported)
    set_property(TARGET ${PROJECT_NAME} 
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
    set_property(TARGET ${PROJECT_NAME} 
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE
    )
    set_property(TARGET miniaudio 
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
    set_property(TARGET kissfft 
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
else()
    message(WARNING "LTO is not supported: ${error}")
endif()

# -------------------------------------------------------------------------
# Includes
# -------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/Resources"
    ${SPECTRUM_BLEND2D_INCLUDES}
)

# -------------------------------------------------------------------------
# Dependencies Linking
# -------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui 
    blend2d::blend2d 
    kissfft::kissfft 
    miniaudio
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
# Output & Install
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

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# -------------------------------------------------------------------------
# IDE Settings & Summary
# -------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} 
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}
)

message(STATUS "=== ${PROJECT_NAME} v${PROJECT_VERSION} ===")
message(STATUS "C++${CMAKE_CXX_STANDARD} | ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "LTO (Release): ${ipo_supported}")
message(STATUS "Output: ${SPECTRUM_OUTPUT_DIR}")