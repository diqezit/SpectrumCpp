# This file is for CMake users. If you are using Visual Studio directly,
# ensure all .cpp files are included in your project.

cmake_minimum_required(VERSION 3.20)
project(SpectrumCpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
    Controller.cpp
    Input.cpp
    Window.cpp
    GraphicsContext.cpp
    AudioCapture.cpp
    WASAPIHelper.cpp
    FFTProcessor.cpp
    SpectrumAnalyzer.cpp
    Utils.cpp
    BaseRenderer.cpp
    BarsRenderer.cpp
    WaveRenderer.cpp
    CircularWaveRenderer.cpp
    CubesRenderer.cpp
    FireRenderer.cpp
    LedPanelRenderer.cpp
    ColorPicker.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /EHsc)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link Windows libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d2d1
    dwrite
    ole32
    uuid
    dwmapi
)