# This file is for CMake users. If you are using Visual Studio directly,
# ensure all .cpp files are included in your project.

cmake_minimum_required(VERSION 3.20)
project(SpectrumCpp)

# ==============================================================================
# Project-wide Settings
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# Source Files (Grouped by Module)
# ==============================================================================
set(SOURCES
    # Application Core
    App/Application.cpp
    App/ControllerCore.cpp

    # Audio Pipeline
    Audio/AudioManager.cpp
    Audio/Capture/AudioCapture.cpp
    Audio/Capture/AudioCaptureEngine.cpp
    Audio/Capture/WASAPIHelper.cpp
    Audio/Processing/AudioBuffer.cpp
    Audio/Processing/FFTProcessor.cpp
    Audio/Processing/FrequencyMapper.cpp
    Audio/Processing/GainNormalizer.cpp
    Audio/Processing/SpectrumAnalyzer.cpp
    Audio/Processing/SpectrumPostProcessor.cpp
    Audio/Sources/AnimatedAudioSource.cpp
    Audio/Sources/RealtimeAudioSource.cpp

    # Common Utilities
    Common/ColorUtils.cpp
    Common/Random.cpp
    Common/StringUtils.cpp
    Common/Timer.cpp

    # Graphics API
    Graphics/API/Core/GeometryBuilder.cpp
    Graphics/API/Core/GraphicsContext.cpp
    Graphics/API/Core/ResourceCache.cpp
    Graphics/API/Core/TransformManager.cpp
    Graphics/API/Renderers/EffectsRenderer.cpp
    Graphics/API/Renderers/GradientRenderer.cpp
    Graphics/API/Renderers/PrimitiveRenderer.cpp
    Graphics/API/Renderers/SpectrumRenderer.cpp
    Graphics/API/Renderers/TextRenderer.cpp

    # Graphics Foundation & Visualizers
    Graphics/Base/BaseRenderer.cpp
    Graphics/Base/RenderUtils.cpp
    Graphics/RendererManager.cpp
    Graphics/Visualizers/BarsRenderer.cpp
    Graphics/Visualizers/CircularWaveRenderer.cpp
    Graphics/Visualizers/CubesRenderer.cpp
    Graphics/Visualizers/FireRenderer.cpp
    Graphics/Visualizers/GaugeRenderer.cpp
    Graphics/Visualizers/KenwoodBarsRenderer.cpp
    Graphics/Visualizers/LedPanelRenderer.cpp
    Graphics/Visualizers/WaveRenderer.cpp

    # Platform (Windowing & Input)
    Platform/InputManager.cpp
    Platform/MainWindow.cpp
    Platform/WindowHelper.cpp
    Platform/WindowManager.cpp

    # User Interface
    UI/ColorPicker.cpp
    UI/UIManager.cpp
)

# ==============================================================================
# Executable Target
# ==============================================================================
# Add the source files and the resource file directly to the executable.
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    Resources/resource.rc.in
)

# ==============================================================================
# Compile Definitions and Options
# ==============================================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /EHsc)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_RESOURCE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/Resources"
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ==============================================================================
# Link Libraries
# ==============================================================================
# Link necessary Windows libraries for Direct2D, DirectWrite, etc.
target_link_libraries(${PROJECT_NAME} PRIVATE
    d2d1
    dwrite
    ole32
    uuid
    dwmapi
)

# ==============================================================================
# Include Directories
# ==============================================================================
# Allows for simpler #include "Module/File.h" syntax from anywhere.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
)