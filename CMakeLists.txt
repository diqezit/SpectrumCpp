cmake_minimum_required(VERSION 3.20)
project(SpectrumCpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# External libraries
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

set(RAYLIB_ROOT "${CMAKE_SOURCE_DIR}/raylib")
set(IMGUI_ROOT  "${CMAKE_SOURCE_DIR}/imgui_library")

if(NOT EXISTS "${RAYLIB_ROOT}/include/raylib.h" OR
   NOT EXISTS "${RAYLIB_ROOT}/lib/raylib.dll" OR
   NOT EXISTS "${RAYLIB_ROOT}/lib/raylibdll.lib")
    message(FATAL_ERROR "raylib not found at ${RAYLIB_ROOT}")
endif()

if(NOT EXISTS "${IMGUI_ROOT}/include/imgui.h" OR
   NOT EXISTS "${IMGUI_ROOT}/lib/imgui.dll" OR
   NOT EXISTS "${IMGUI_ROOT}/lib/imgui.lib")
    message(FATAL_ERROR "ImGui not found at ${IMGUI_ROOT}")
endif()

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Resource file
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

configure_file(
    "${CMAKE_SOURCE_DIR}/Resources/resource.rc.in"
    "${CMAKE_BINARY_DIR}/resource.rc"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Sources
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

set(SOURCES
    App/Application.cpp
    App/ControllerCore.cpp
    App/ControllerCore.h

    Audio/AudioManager.cpp
    Audio/AudioManager.h
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

    Common/Common.h
    Common/EventBus.h
    Common/SpectrumTypes.h
    Common/Types.h

    Graphics/IRenderer.h
    Graphics/RendererManager.h
    Graphics/API/GraphicsAPI.cpp
    Graphics/API/GraphicsAPI.h
    Graphics/API/GraphicsHelpers.h
    Graphics/Base/BaseRenderer.h
    Graphics/Base/PeakTracker.h
    Graphics/Base/RenderUtils.h
    Graphics/Visualizers/BarsRenderer.cpp
    Graphics/Visualizers/BarsRenderer.h
    Graphics/Visualizers/CircularWaveRenderer.cpp
    Graphics/Visualizers/CircularWaveRenderer.h
    Graphics/Visualizers/CubesRenderer.cpp
    Graphics/Visualizers/CubesRenderer.h
    Graphics/Visualizers/FireRenderer.cpp
    Graphics/Visualizers/FireRenderer.h
    Graphics/Visualizers/GaugeRenderer.cpp
    Graphics/Visualizers/GaugeRenderer.h
    Graphics/Visualizers/KenwoodBarsRenderer.cpp
    Graphics/Visualizers/KenwoodBarsRenderer.h
    Graphics/Visualizers/LedPanelRenderer.cpp
    Graphics/Visualizers/LedPanelRenderer.h
    Graphics/Visualizers/MatrixLedRenderer.cpp
    Graphics/Visualizers/MatrixLedRenderer.h
    Graphics/Visualizers/ParticlesRenderer.cpp
    Graphics/Visualizers/ParticlesRenderer.h
    Graphics/Visualizers/PolylineWaveRenderer.cpp
    Graphics/Visualizers/PolylineWaveRenderer.h
    Graphics/Visualizers/SphereRenderer.cpp
    Graphics/Visualizers/SphereRenderer.h
    Graphics/Visualizers/WaveRenderer.cpp
    Graphics/Visualizers/WaveRenderer.h
    Graphics/Visualizers/Settings/QualityPresets.h
    Graphics/Visualizers/Settings/QualityTraits.h

    Platform/InputManager.h
    Platform/MainWindow.h
    Platform/MessageHandler.h
    Platform/MessageHandlerBase.h
    Platform/UIMessageHandler.h
    Platform/UIWindow.h
    Platform/WindowBase.h
    Platform/WindowManager.cpp
    Platform/WindowManager.h

    UI/ImGuiContext.h
    UI/UIManager.h
    UI/UIManager.cpp

    Resources/Resource.h
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Executable
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    "${CMAKE_BINARY_DIR}/resource.rc"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# VS source tree
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${SOURCES})
source_group("Resources" FILES
    "${CMAKE_BINARY_DIR}/resource.rc"
    Resources/resource.rc.in
    Resources/Resource.h
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Preprocessor
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN
    IMGUI_USE_DLL
    $<$<CONFIG:Debug>:_DEBUG;SHOW_CONSOLE=1>
    $<$<CONFIG:RelWithDebInfo>:NDEBUG;SHOW_CONSOLE=1>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:MinSizeRel>:NDEBUG>
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Compiler / linker
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /EHsc /permissive- /wd4828
        $<$<CONFIG:Debug>:/MDd /Zi /Od /RTC1>
        $<$<CONFIG:Release>:/MD /O2 /Ob2>
        $<$<CONFIG:RelWithDebInfo>:/MD /Zi /O2 /Ob1>
        $<$<CONFIG:MinSizeRel>:/MD /O1 /Ob1>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/DEBUG /INCREMENTAL>
        $<$<CONFIG:Release>:/INCREMENTAL:NO /OPT:REF /OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Include / link
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

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

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui.lib
    raylibdll
    d2d1 d3d11 dwrite dxgi
    ole32 uuid dwmapi windowscodecs
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Output
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

set(OUT "${CMAKE_BINARY_DIR}/bin")

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${OUT}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${OUT}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${OUT}/Release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUT}/RelWithDebInfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${OUT}/MinSizeRel"
    VS_DEBUGGER_WORKING_DIRECTORY           "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Copy DLLs
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RAYLIB_ROOT}/lib/raylib.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${IMGUI_ROOT}/lib/imgui.dll"   "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E echo "Build OK -> $<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Copying DLLs"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Install
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES
    "${RAYLIB_ROOT}/lib/raylib.dll"
    "${IMGUI_ROOT}/lib/imgui.dll"
    DESTINATION bin
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# IDE
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Summary
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

message(STATUS "=== ${PROJECT_NAME} v${PROJECT_VERSION} ===")
message(STATUS "C++${CMAKE_CXX_STANDARD} | ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "raylib: ${RAYLIB_ROOT}")
message(STATUS "ImGui:  ${IMGUI_ROOT}")
message(STATUS "Output: ${OUT}")