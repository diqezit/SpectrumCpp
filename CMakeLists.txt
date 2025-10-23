cmake_minimum_required(VERSION 3.20)
project(SpectrumCpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# raylib library configuration
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
set(RAYLIB_ROOT "${CMAKE_SOURCE_DIR}/raylib")
set(RAYLIB_INCLUDE_DIR "${RAYLIB_ROOT}/include")
set(RAYLIB_LIB_DIR "${RAYLIB_ROOT}/lib")

# Validate raylib installation
if(NOT EXISTS "${RAYLIB_INCLUDE_DIR}/raylib.h")
    message(FATAL_ERROR "raylib not found at ${RAYLIB_ROOT}. Please place raylib library in ${RAYLIB_ROOT}")
endif()

if(NOT EXISTS "${RAYLIB_LIB_DIR}/raylib.dll")
    message(FATAL_ERROR "raylib.dll not found at ${RAYLIB_LIB_DIR}")
endif()

if(NOT EXISTS "${RAYLIB_LIB_DIR}/raylibdll.lib")
    message(FATAL_ERROR "raylibdll.lib not found at ${RAYLIB_LIB_DIR}")
endif()

message(STATUS "raylib found: ${RAYLIB_ROOT}")
message(STATUS "raylib include: ${RAYLIB_INCLUDE_DIR}")
message(STATUS "raylib lib: ${RAYLIB_LIB_DIR}")

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# ImGui DLL library (как raylib)
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
set(IMGUI_ROOT "${CMAKE_SOURCE_DIR}/imgui_library")
set(IMGUI_INCLUDE_DIR "${IMGUI_ROOT}/include")
set(IMGUI_LIB_DIR "${IMGUI_ROOT}/lib")

# Validate ImGui installation
if(NOT EXISTS "${IMGUI_INCLUDE_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui headers not found at ${IMGUI_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${IMGUI_LIB_DIR}/imgui.lib")
    message(FATAL_ERROR "imgui.lib not found at ${IMGUI_LIB_DIR}")
endif()

if(NOT EXISTS "${IMGUI_LIB_DIR}/imgui.dll")
    message(FATAL_ERROR "imgui.dll not found at ${IMGUI_LIB_DIR}")
endif()

message(STATUS "ImGui found: ${IMGUI_ROOT}")
message(STATUS "ImGui include: ${IMGUI_INCLUDE_DIR}")
message(STATUS "ImGui lib: ${IMGUI_LIB_DIR}")

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Resource file configuration
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
configure_file(
    "${CMAKE_SOURCE_DIR}/Resources/resource.rc.in"
    "${CMAKE_BINARY_DIR}/resource.rc"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Main executable
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
add_executable(${PROJECT_NAME} WIN32)

target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_BINARY_DIR}/resource.rc"
)

target_sources(${PROJECT_NAME} PRIVATE
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
    Common/Logger.h
    Common/SpectrumTypes.h
    Common/Types.h
    
    Graphics/IRenderer.h
    Graphics/RendererManager.cpp
    Graphics/RendererManager.h
    Graphics/RenderersController.cpp
    Graphics/RenderersController.h
    
    Graphics/API/GraphicsAPI.h
    Graphics/API/GraphicsAPI.cpp
    Graphics/API/GraphicsHelpers.h
    
    Graphics/Base/BaseRenderer.cpp
    Graphics/Base/BaseRenderer.h
    Graphics/Base/PeakTracker.cpp
    Graphics/Base/PeakTracker.h
    Graphics/Base/RenderUtils.cpp
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
    
    Platform/MainWindow.cpp
    Platform/MainWindow.h
    Platform/MessageHandler.cpp
    Platform/MessageHandler.h
    Platform/UIWindow.cpp
    Platform/UIWindow.h
    Platform/UIMessageHandler.cpp
    Platform/UIMessageHandler.h
    Platform/Win32Utils.h
    Platform/WindowManager.cpp
    Platform/WindowManager.h
    
    Platform/Input/IKeyboard.h
    Platform/Input/InputManager.cpp
    Platform/Input/InputManager.h
    Platform/Input/Win32Keyboard.h
    
    Resources/Resource.h
    
    UI/Common/ImGuiHelpers.h
    UI/Common/UIFormatters.h
    UI/Common/UILayout.h
    
    UI/Core/ImGuiContext.cpp
    UI/Core/ImGuiContext.h
    UI/Core/UIManager.cpp
    UI/Core/UIManager.h
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Visual Studio folder organization
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
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
    Common/Common.h
    Common/EventBus.h
    Common/Logger.h
    Common/SpectrumTypes.h
    Common/Types.h
)

source_group("Graphics" FILES
    Graphics/IRenderer.h
    Graphics/RendererManager.cpp
    Graphics/RendererManager.h
    Graphics/RenderersController.cpp
    Graphics/RenderersController.h
)

source_group("Graphics\\API" FILES
    Graphics/API/GraphicsAPI.h
    Graphics/API/GraphicsAPI.cpp
    Graphics/API/GraphicsHelpers.h
)

source_group("Graphics\\Base" FILES
    Graphics/Base/BaseRenderer.cpp
    Graphics/Base/BaseRenderer.h
    Graphics/Base/PeakTracker.cpp
    Graphics/Base/PeakTracker.h
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
)

source_group("Graphics\\Visualizers\\Settings" FILES
    Graphics/Visualizers/Settings/QualityPresets.h
    Graphics/Visualizers/Settings/QualityTraits.h
)

source_group("Platform" FILES
    Platform/MainWindow.cpp
    Platform/MainWindow.h
    Platform/MessageHandler.cpp
    Platform/MessageHandler.h
    Platform/UIWindow.cpp
    Platform/UIWindow.h
    Platform/UIMessageHandler.cpp
    Platform/UIMessageHandler.h
    Platform/Win32Utils.h
    Platform/WindowManager.cpp
    Platform/WindowManager.h
)

source_group("Platform\\Input" FILES
    Platform/Input/IKeyboard.h
    Platform/Input/InputManager.cpp
    Platform/Input/InputManager.h
    Platform/Input/Win32Keyboard.h
)

source_group("Resources" FILES
    "${CMAKE_BINARY_DIR}/resource.rc"
    Resources/resource.rc.in
    Resources/Resource.h
)

source_group("UI\\Common" FILES
    UI/Common/ImGuiHelpers.h
    UI/Common/UIFormatters.h
    UI/Common/UILayout.h
)

source_group("UI\\Core" FILES
    UI/Core/ImGuiContext.cpp
    UI/Core/ImGuiContext.h
    UI/Core/UIManager.cpp
    UI/Core/UIManager.h
)

source_group("Project Files" FILES
    CMakeLists.txt
    README.md
    LICENSE.txt
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Preprocessor definitions
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    IMGUI_USE_DLL
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:SHOW_CONSOLE=1>
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Compiler flags
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4
        /EHsc
        /permissive-
        /wd4828
    )
    
    # Configuration-specific compiler flags
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/MD /O1 /Ob1 /DNDEBUG")
    
    # Linker flags for debugging
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /OPT:REF /OPT:ICF")
    
else()
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
    )
endif()

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Include paths
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/Resources"
    "${RAYLIB_INCLUDE_DIR}"
    "${IMGUI_INCLUDE_DIR}"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Library search paths
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
target_link_directories(${PROJECT_NAME} PRIVATE
    "${RAYLIB_LIB_DIR}"
    "${IMGUI_LIB_DIR}"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Link dependencies
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui.lib
    raylibdll
    d2d1
    d3d11
    dwrite
    dxgi
    ole32
    uuid
    dwmapi
    windowscodecs
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Output directory structure
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/MinSizeRel"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Copy DLLs after build
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "======================================"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying DLLs to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RAYLIB_LIB_DIR}/raylib.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/raylib.dll"
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${IMGUI_LIB_DIR}/imgui.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/imgui.dll"
    
    COMMAND ${CMAKE_COMMAND} -E echo "raylib.dll & imgui.dll copied"
    COMMAND ${CMAKE_COMMAND} -E echo "======================================"
    COMMENT "Copying required DLLs"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# IDE folder structure
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set working directory for debugging in Visual Studio
set_property(TARGET ${PROJECT_NAME} PROPERTY 
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Installation rules
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT application
)

install(FILES "${RAYLIB_LIB_DIR}/raylib.dll"
    DESTINATION bin
    COMPONENT application
)

install(FILES "${IMGUI_LIB_DIR}/imgui.dll"
    DESTINATION bin
    COMPONENT application
)

install(DIRECTORY Resources/
    DESTINATION bin/Resources
    COMPONENT resources
    FILES_MATCHING 
    PATTERN "*.png" 
    PATTERN "*.jpg" 
    PATTERN "*.ico"
    PATTERN "*.ttf"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Build notification
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "======================================"
    COMMAND ${CMAKE_COMMAND} -E echo "Build completed successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "Executable: $<TARGET_FILE:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E echo "======================================"
    COMMENT "Post-build notification"
)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Package configuration
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP")
include(CPack)

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Set SpectrumCpp as default startup project in Visual Studio
# This prevents "ALL_BUILD" from being selected by default
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# Print configuration summary
# =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
message(STATUS "====================================")
message(STATUS "Configuration Summary:")
message(STATUS "====================================")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "raylib: ${RAYLIB_ROOT}")
message(STATUS "ImGui: ${IMGUI_ROOT}")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Startup project: ${PROJECT_NAME}")
message(STATUS "====================================")