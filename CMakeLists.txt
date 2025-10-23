cmake_minimum_required(VERSION 3.20)
project(SpectrumCpp)

# ==============================================================================
# Project-wide Settings
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ==============================================================================
# Executable Target
# ==============================================================================
configure_file(
    "${CMAKE_SOURCE_DIR}/Resources/resource.rc.in"
    "${CMAKE_BINARY_DIR}/resource.rc"
)

add_executable(${PROJECT_NAME} WIN32)

target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_BINARY_DIR}/resource.rc"
)

# ==============================================================================
# Add ALL Source and Header Files to Target
# ==============================================================================
target_sources(${PROJECT_NAME} PRIVATE
    # Headers
    App/ControllerCore.h
    Audio/AudioManager.h
    Audio/Capture/AudioCapture.h
    Audio/Capture/AudioCaptureEngine.h
    Audio/Capture/WASAPIHelper.h
    Audio/Processing/AudioBuffer.h
    Audio/Processing/FFTProcessor.h
    Audio/Processing/FrequencyMapper.h
    Audio/Processing/GainNormalizer.h
    Audio/Processing/SpectrumAnalyzer.h
    Audio/Processing/SpectrumPostProcessor.h
    Audio/Sources/AnimatedAudioSource.h
    Audio/Sources/IAudioSource.h
    Audio/Sources/RealtimeAudioSource.h
    
    # ⭐ COMMON - Minimized Core Foundation
    Common/Common.h
    Common/EventBus.h
    Common/Logger.h
    Common/SpectrumTypes.h
    Common/Types.h
    
    Graphics/IRenderer.h
    Graphics/RendererManager.h
    Graphics/RenderersController.h
    Graphics/API/Canvas.h
    Graphics/API/D2DHelpers.h
    Graphics/API/Brushes/GradientStop.h
    Graphics/API/Brushes/IBrush.h
    Graphics/API/Brushes/LinearGradientBrush.h
    Graphics/API/Brushes/RadialGradientBrush.h
    Graphics/API/Brushes/SolidColorBrush.h
    
    # ⭐ UPDATED - Core with new refactored files
    Graphics/API/Core/DeviceResourceManager.h      # NEW
    Graphics/API/Core/DrawStateController.h         # NEW
    Graphics/API/Core/GeometryBuilder.h
    Graphics/API/Core/IRenderComponent.h
    Graphics/API/Core/RenderEngine.h
    Graphics/API/Core/ResourceCache.h
    Graphics/API/Core/TransformManager.h
    
    Graphics/API/Enums/PaintEnums.h
    Graphics/API/Enums/RenderEnums.h
    Graphics/API/Enums/TextEnums.h
    
    # ⭐ HELPERS - Hierarchical Structure (v2.0)
    Graphics/API/Helpers/Core/HResultHelpers.h
    Graphics/API/Helpers/Core/Sanitization.h
    Graphics/API/Helpers/Core/Validation.h
    Graphics/API/Helpers/Math/MathHelpers.h
    Graphics/API/Helpers/Conversion/EnumConversion.h
    Graphics/API/Helpers/Conversion/TypeConversion.h
    Graphics/API/Helpers/Geometry/ColorHelpers.h
    Graphics/API/Helpers/Geometry/GeometryHelpers.h
    Graphics/API/Helpers/Platform/GdiHelpers.h
    Graphics/API/Helpers/Platform/WindowHelpers.h
    Graphics/API/Helpers/D2D/D2DScopes.h
    Graphics/API/Helpers/D2D/RenderTargetHelpers.h
    Graphics/API/Helpers/Rendering/RenderHelpers.h
    Graphics/API/Helpers/Utils/Random.h
    Graphics/API/Helpers/Utils/StringUtils.h
    Graphics/API/Helpers/Utils/TemplateUtils.h
    Graphics/API/Helpers/Utils/Timer.h
    
    Graphics/API/Renderers/EffectsRenderer.h
    Graphics/API/Renderers/GradientRenderer.h
    Graphics/API/Renderers/PrimitiveRenderer.h
    Graphics/API/Renderers/SpectrumRenderer.h
    Graphics/API/Renderers/TextRenderer.h
    Graphics/API/Structs/Paint.h
    Graphics/API/Structs/StrokeOptions.h
    Graphics/API/Structs/TextStyle.h
    Graphics/Base/BaseRenderer.h
    Graphics/Base/RenderUtils.h
    
    # ⭐ VISUALIZERS - Quality Settings (NEW)
    Graphics/Visualizers/Settings/QualityTraits.h
    Graphics/Visualizers/Settings/QualityPresets.h
    
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
    
    Platform/Input/IKeyboard.h
    Platform/Input/InputManager.h
    Platform/Input/Win32Keyboard.h
    Platform/MessageHandler.h
    Platform/MainWindow.h
    Platform/WindowManager.h
    Platform/Win32Utils.h
    Resources/Resource.h
    UI/Common/UILayout.h
    UI/Components/UIButton.h
    UI/Components/UIButtonColors.h
    UI/Components/UISlider.h
    UI/Core/UIManager.h
    UI/Panels/PanelAnimator.h
    UI/Panels/PanelDrawHelper.h
    UI/Panels/AudioSettingsPanel/AudioSettingsPanel.h
    UI/Panels/ColorPicker/ColorPicker.h
    UI/Panels/ColorPicker/ColorWheelGenerator.h
    UI/Panels/ColorPicker/ColorWheelRenderer.h
    UI/Panels/ControlPanel/ControlPanel.h

    # Sources
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
    
    # ⭐ HELPERS - Implementation Files
    Graphics/API/Helpers/Geometry/ColorHelpers.cpp
    Graphics/API/Helpers/Rendering/RenderHelpers.cpp
    Graphics/API/Helpers/Utils/Random.cpp
    Graphics/API/Helpers/Utils/StringUtils.cpp
    Graphics/API/Helpers/Utils/Timer.cpp
    
    Graphics/RendererManager.cpp
    Graphics/RenderersController.cpp
    Graphics/API/Canvas.cpp
    Graphics/API/Structs/Paint.cpp
    
    # ⭐ UPDATED - Core implementations with new files
    Graphics/API/Core/DeviceResourceManager.cpp     # NEW
    Graphics/API/Core/DrawStateController.cpp       # NEW
    Graphics/API/Core/GeometryBuilder.cpp
    Graphics/API/Core/RenderEngine.cpp
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
    Graphics/Visualizers/MatrixLedRenderer.cpp
    Graphics/Visualizers/ParticlesRenderer.cpp
    Graphics/Visualizers/PolylineWaveRenderer.cpp
    Graphics/Visualizers/SphereRenderer.cpp
    Graphics/Visualizers/WaveRenderer.cpp
    Platform/Input/InputManager.cpp
    Platform/MessageHandler.cpp
    Platform/MainWindow.cpp
    Platform/WindowManager.cpp
    UI/Core/UIManager.cpp
    UI/Components/UIButton.cpp
    UI/Components/UISlider.cpp
    UI/Panels/AudioSettingsPanel/AudioSettingsPanel.cpp
    UI/Panels/ColorPicker/ColorPicker.cpp
    UI/Panels/ColorPicker/ColorWheelRenderer.cpp
    UI/Panels/ControlPanel/ControlPanel.cpp
)

# ==============================================================================
# Group sources into filters for Visual Studio
# ==============================================================================
source_group("App" FILES 
    App/Application.cpp 
    App/ControllerCore.cpp 
    App/ControllerCore.h)

source_group("Audio" FILES 
    Audio/AudioManager.cpp 
    Audio/AudioManager.h)

source_group("Audio\\Capture" FILES
    Audio/Capture/AudioCapture.cpp 
    Audio/Capture/AudioCapture.h
    Audio/Capture/AudioCaptureEngine.cpp 
    Audio/Capture/AudioCaptureEngine.h
    Audio/Capture/WASAPIHelper.cpp 
    Audio/Capture/WASAPIHelper.h)

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
    Audio/Processing/SpectrumPostProcessor.h)

source_group("Audio\\Sources" FILES
    Audio/Sources/AnimatedAudioSource.cpp 
    Audio/Sources/AnimatedAudioSource.h
    Audio/Sources/IAudioSource.h
    Audio/Sources/RealtimeAudioSource.cpp 
    Audio/Sources/RealtimeAudioSource.h)

source_group("Common" FILES
    Common/Common.h 
    Common/EventBus.h
    Common/Logger.h 
    Common/SpectrumTypes.h 
    Common/Types.h)

source_group("Graphics" FILES
    Graphics/IRenderer.h 
    Graphics/RendererManager.cpp 
    Graphics/RendererManager.h
    Graphics/RenderersController.cpp
    Graphics/RenderersController.h)

source_group("Graphics\\API" FILES
    Graphics/API/Canvas.cpp 
    Graphics/API/Canvas.h
    Graphics/API/D2DHelpers.h)

source_group("Graphics\\API\\Brushes" FILES
    Graphics/API/Brushes/GradientStop.h
    Graphics/API/Brushes/IBrush.h
    Graphics/API/Brushes/LinearGradientBrush.h
    Graphics/API/Brushes/RadialGradientBrush.h
    Graphics/API/Brushes/SolidColorBrush.h)

# ⭐ UPDATED - Core group with new files
source_group("Graphics\\API\\Core" FILES
    Graphics/API/Core/DeviceResourceManager.cpp     # NEW
    Graphics/API/Core/DeviceResourceManager.h       # NEW
    Graphics/API/Core/DrawStateController.cpp       # NEW
    Graphics/API/Core/DrawStateController.h         # NEW
    Graphics/API/Core/GeometryBuilder.cpp 
    Graphics/API/Core/GeometryBuilder.h
    Graphics/API/Core/IRenderComponent.h
    Graphics/API/Core/RenderEngine.cpp 
    Graphics/API/Core/RenderEngine.h
    Graphics/API/Core/ResourceCache.cpp 
    Graphics/API/Core/ResourceCache.h
    Graphics/API/Core/TransformManager.cpp 
    Graphics/API/Core/TransformManager.h)

source_group("Graphics\\API\\Enums" FILES
    Graphics/API/Enums/PaintEnums.h
    Graphics/API/Enums/RenderEnums.h
    Graphics/API/Enums/TextEnums.h)

source_group("Graphics\\API\\Helpers\\Core" FILES
    Graphics/API/Helpers/Core/HResultHelpers.h
    Graphics/API/Helpers/Core/Sanitization.h
    Graphics/API/Helpers/Core/Validation.h)

source_group("Graphics\\API\\Helpers\\Math" FILES
    Graphics/API/Helpers/Math/MathHelpers.h)

source_group("Graphics\\API\\Helpers\\Conversion" FILES
    Graphics/API/Helpers/Conversion/EnumConversion.h
    Graphics/API/Helpers/Conversion/TypeConversion.h)

source_group("Graphics\\API\\Helpers\\Geometry" FILES
    Graphics/API/Helpers/Geometry/ColorHelpers.cpp
    Graphics/API/Helpers/Geometry/ColorHelpers.h
    Graphics/API/Helpers/Geometry/GeometryHelpers.h)

source_group("Graphics\\API\\Helpers\\Platform" FILES
    Graphics/API/Helpers/Platform/GdiHelpers.h
    Graphics/API/Helpers/Platform/WindowHelpers.h)

source_group("Graphics\\API\\Helpers\\D2D" FILES
    Graphics/API/Helpers/D2D/D2DScopes.h
    Graphics/API/Helpers/D2D/RenderTargetHelpers.h)

source_group("Graphics\\API\\Helpers\\Rendering" FILES
    Graphics/API/Helpers/Rendering/RenderHelpers.cpp
    Graphics/API/Helpers/Rendering/RenderHelpers.h)

source_group("Graphics\\API\\Helpers\\Utils" FILES
    Graphics/API/Helpers/Utils/Random.cpp
    Graphics/API/Helpers/Utils/Random.h
    Graphics/API/Helpers/Utils/StringUtils.cpp
    Graphics/API/Helpers/Utils/StringUtils.h
    Graphics/API/Helpers/Utils/TemplateUtils.h
    Graphics/API/Helpers/Utils/Timer.cpp
    Graphics/API/Helpers/Utils/Timer.h)

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
    Graphics/API/Renderers/TextRenderer.h)

source_group("Graphics\\API\\Structs" FILES
    Graphics/API/Structs/Paint.cpp
    Graphics/API/Structs/Paint.h
    Graphics/API/Structs/StrokeOptions.h
    Graphics/API/Structs/TextStyle.h)

source_group("Graphics\\Base" FILES
    Graphics/Base/BaseRenderer.cpp 
    Graphics/Base/BaseRenderer.h
    Graphics/Base/RenderUtils.cpp 
    Graphics/Base/RenderUtils.h)

# ⭐ NEW - Visualizers Settings Group
source_group("Graphics\\Visualizers\\Settings" FILES
    Graphics/Visualizers/Settings/QualityTraits.h
    Graphics/Visualizers/Settings/QualityPresets.h)

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
    Graphics/Visualizers/MatrixLedRenderer.cpp 
    Graphics/Visualizers/MatrixLedRenderer.h
    Graphics/Visualizers/ParticlesRenderer.cpp 
    Graphics/Visualizers/ParticlesRenderer.h
    Graphics/Visualizers/PolylineWaveRenderer.cpp 
    Graphics/Visualizers/PolylineWaveRenderer.h
    Graphics/Visualizers/SphereRenderer.cpp 
    Graphics/Visualizers/SphereRenderer.h
    Graphics/Visualizers/WaveRenderer.cpp 
    Graphics/Visualizers/WaveRenderer.h)

source_group("Platform" FILES
    Platform/MainWindow.cpp 
    Platform/MainWindow.h
    Platform/WindowManager.cpp 
    Platform/WindowManager.h
    Platform/MessageHandler.cpp 
    Platform/MessageHandler.h
    Platform/Win32Utils.h)

source_group("Platform\\Input" FILES
    Platform/Input/InputManager.cpp 
    Platform/Input/InputManager.h
    Platform/Input/IKeyboard.h 
    Platform/Input/Win32Keyboard.h)

source_group("Project Files" FILES 
    CMakeLists.txt)

source_group("Resources" FILES
    Resources/resource.rc.in
    Resources/Resource.h)

source_group("UI\\Common" FILES 
    UI/Common/UILayout.h)

source_group("UI\\Components" FILES
    UI/Components/UIButton.cpp 
    UI/Components/UIButton.h
    UI/Components/UIButtonColors.h
    UI/Components/UISlider.cpp 
    UI/Components/UISlider.h)

source_group("UI\\Core" FILES 
    UI/Core/UIManager.cpp 
    UI/Core/UIManager.h)

source_group("UI\\Panels" FILES 
    UI/Panels/PanelAnimator.h 
    UI/Panels/PanelDrawHelper.h)

source_group("UI\\Panels\\AudioSettingsPanel" FILES
    UI/Panels/AudioSettingsPanel/AudioSettingsPanel.cpp
    UI/Panels/AudioSettingsPanel/AudioSettingsPanel.h)

source_group("UI\\Panels\\ColorPicker" FILES
    UI/Panels/ColorPicker/ColorPicker.cpp 
    UI/Panels/ColorPicker/ColorPicker.h
    UI/Panels/ColorPicker/ColorWheelGenerator.h 
    UI/Panels/ColorPicker/ColorWheelRenderer.cpp
    UI/Panels/ColorPicker/ColorWheelRenderer.h)

source_group("UI\\Panels\\ControlPanel" FILES
    UI/Panels/ControlPanel/ControlPanel.cpp
    UI/Panels/ControlPanel/ControlPanel.h)

# ==============================================================================
# Compile Definitions and Options
# ==============================================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:RelWithDebInfo>:SHOW_CONSOLE=1>
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /EHsc)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
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
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/Resources"
)