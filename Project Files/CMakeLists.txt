cmake_minimum_required(VERSION 3.20)
project(SpectrumCpp)

# ==============================================================================
# Project-wide Settings
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==============================================================================
# Source Files
# ==============================================================================
set(SOURCES
    App/Application.cpp
    App/ControllerCore.cpp
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
    Common/ColorUtils.cpp
    Common/Random.cpp
    Common/StringUtils.cpp
    Common/Timer.cpp
    Graphics/RendererManager.cpp
    Graphics/API/Core/GeometryBuilder.cpp
    Graphics/API/Core/GraphicsContext.cpp
    Graphics/API/Core/ResourceCache.cpp
    Graphics/API/Core/TransformManager.cpp
    Graphics/API/Renderers/EffectsRenderer.cpp
    Graphics/API/Renderers/GradientRenderer.cpp
    Graphics/API/Renderers/PrimitiveRenderer.cpp
    Graphics/API/Renderers/SpectrumRenderer.cpp
    Graphics/API/Renderers/TextRenderer.cpp
    Graphics/Base/BaseRenderer.cpp
    Graphics/Base/RenderUtils.cpp
    Graphics/Visualizers/BarsRenderer.cpp
    Graphics/Visualizers/CircularWaveRenderer.cpp
    Graphics/Visualizers/CubesRenderer.cpp
    Graphics/Visualizers/FireRenderer.cpp
    Graphics/Visualizers/GaugeRenderer.cpp
    Graphics/Visualizers/KenwoodBarsRenderer.cpp
    Graphics/Visualizers/LedPanelRenderer.cpp
    Graphics/Visualizers/WaveRenderer.cpp
    Platform/InputManager.cpp
    Platform/MainWindow.cpp
    Platform/WindowHelper.cpp
    Platform/WindowManager.cpp
    UI/Core/UIManager.cpp
    UI/Components/UIButton.cpp
    UI/Components/UISlider.cpp
    UI/Panels/AudioSettingsPanel/AudioSettingsPanel.cpp
    UI/Panels/ColorPicker/ColorPicker.cpp
    UI/Panels/ControlPanel/ControlPanel.cpp
)

# ==============================================================================
# Executable Target
# ==============================================================================
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    Resources/resource.rc.in
)

# ==============================================================================
# Group sources into filters for Visual Studio
# ==============================================================================
source_group("App" FILES
    App/Application.cpp
    App/ControllerCore.cpp
    App/ControllerCore.h
)
source_group("Audio" FILES
    Audio/AudioManager.cpp
    Audio/AudioManager.h
)
source_group("Audio\\Capture" FILES
    Audio/Capture/AudioCapture.cpp
    Audio/Capture/AudioCapture.h
    Audio/Capture/AudioCaptureEngine.cpp
    Audio/Capture/AudioCaptureEngine.h
    Audio/Capture/WASAPIHelper.cpp
    Audio/Capture/WASAPIHelper.h
)
source_group("Audio\\Processing" FILES
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
)
source_group("Audio\\Sources" FILES
    Audio/Sources/AnimatedAudioSource.cpp
    Audio/Sources/AnimatedAudioSource.h
    Audio/Sources/IAudioSource.h
    Audio/Sources/RealtimeAudioSource.cpp
    Audio/Sources/RealtimeAudioSource.h
)
source_group("Common" FILES
    Common/ColorUtils.cpp
    Common/ColorUtils.h
    Common/Common.h
    Common/EventBus.h
    Common/Logger.h
    Common/MathUtils.h
    Common/PlatformUtils.h
    Common/Random.cpp
    Common/Random.h
    Common/SpectrumTypes.h
    Common/StringUtils.cpp
    Common/StringUtils.h
    Common/TemplateUtils.h
    Common/Timer.cpp
    Common/Timer.h
    Common/Types.h
)
source_group("Graphics" FILES
    Graphics/IRenderer.h
    Graphics/RendererManager.cpp
    Graphics/RendererManager.h
)
source_group("Graphics\\API" FILES
    Graphics/API/D2DHelpers.h
)
source_group("Graphics\\API\\Core" FILES
    Graphics/API/Core/GeometryBuilder.cpp
    Graphics/API/Core/GeometryBuilder.h
    Graphics/API/Core/GraphicsContext.cpp
    Graphics/API/Core/GraphicsContext.h
    Graphics/API/Core/ResourceCache.cpp
    Graphics/API/Core/ResourceCache.h
    Graphics/API/Core/TransformManager.cpp
    Graphics/API/Core/TransformManager.h
)
source_group("Graphics\\API\\Renderers" FILES
    Graphics/API/Renderers/EffectsRenderer.cpp
    Graphics/API/Renderers/EffectsRenderer.h
    Graphics/API/Renderers/GradientRenderer.cpp
    Graphics/API/Renderers/GradientRenderer.h
    Graphics/API/Renderers/PrimitiveRenderer.cpp
    Graphics/API/Renderers/PrimitiveRenderer.h
    Graphics/API/Renderers/SpectrumRenderer.cpp
    Graphics/API/Renderers/SpectrumRenderer.h
    Graphics/API/Renderers/TextRenderer.cpp
    Graphics/API/Renderers/TextRenderer.h
)
source_group("Graphics\\Base" FILES
    Graphics/Base/BaseRenderer.cpp
    Graphics/Base/BaseRenderer.h
    Graphics/Base/RenderUtils.cpp
    Graphics/Base/RenderUtils.h
)
source_group("Graphics\\Visualizers" FILES
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
    Graphics/Visualizers/WaveRenderer.cpp
    Graphics/Visualizers/WaveRenderer.h
)
source_group("Platform" FILES
    Platform/InputManager.cpp
    Platform/InputManager.h
    Platform/MainWindow.cpp
    Platform/MainWindow.h
    Platform/WindowHelper.cpp
    Platform/WindowHelper.h
    Platform/WindowManager.cpp
    Platform/WindowManager.h
)
source_group("Project Files" FILES
    "Project Files/CMakeLists.txt"
)
source_group("Resources" FILES
    Resources/resource.rc.in
    Resources/Resource.h
)
source_group("UI\\Common" FILES
    UI/Common/UILayout.h
)
source_group("UI\\Components" FILES
    UI/Components/UIButton.cpp
    UI/Components/UIButton.h
    UI/Components/UISlider.cpp
    UI/Components/UISlider.h
)
source_group("UI\\Core" FILES
    UI/Core/UIManager.cpp
    UI/Core/UIManager.h
)
source_group("UI\\Panels" FILES
    UI/Panels/PanelAnimator.h
    UI/Panels/PanelDrawHelper.h
)
source_group("UI\\Panels\\AudioSettingsPanel" FILES
    UI/Panels/AudioSettingsPanel/AudioSettingsPanel.cpp
    UI/Panels/AudioSettingsPanel/AudioSettingsPanel.h
)
source_group("UI\\Panels\\ColorPicker" FILES
    UI/Panels/ColorPicker/ColorPicker.cpp
    UI/Panels/ColorPicker/ColorPicker.h
    UI/Panels/ColorPicker/ColorWheelGenerator.h
    UI/Panels/ColorPicker/ColorWheelRenderer.h
)
source_group("UI\\Panels\\ControlPanel" FILES
    UI/Panels/ControlPanel/ControlPanel.cpp
    UI/Panels/ControlPanel/ControlPanel.h
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
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/App"
    "${CMAKE_SOURCE_DIR}/Audio"
    "${CMAKE_SOURCE_DIR}/Audio/Capture"
    "${CMAKE_SOURCE_DIR}/Audio/Processing"
    "${CMAKE_SOURCE_DIR}/Audio/Sources"
    "${CMAKE_SOURCE_DIR}/Common"
    "${CMAKE_SOURCE_DIR}/Graphics"
    "${CMAKE_SOURCE_DIR}/Graphics/API"
    "${CMAKE_SOURCE_DIR}/Graphics/API/Core"
    "${CMAKE_SOURCE_DIR}/Graphics/API/Renderers"
    "${CMAKE_SOURCE_DIR}/Graphics/Base"
    "${CMAKE_SOURCE_DIR}/Graphics/Visualizers"
    "${CMAKE_SOURCE_DIR}/Platform"
    "${CMAKE_SOURCE_DIR}/Resources"
    "${CMAKE_SOURCE_DIR}/UI"
    "${CMAKE_SOURCE_DIR}/UI/Common"
    "${CMAKE_SOURCE_DIR}/UI/Components"
    "${CMAKE_SOURCE_DIR}/UI/Core"
    "${CMAKE_SOURCE_DIR}/UI/Panels"
    "${CMAKE_SOURCE_DIR}/UI/Panels/AudioSettingsPanel"
    "${CMAKE_SOURCE_DIR}/UI/Panels/ColorPicker"
    "${CMAKE_SOURCE_DIR}/UI/Panels/ControlPanel"
)