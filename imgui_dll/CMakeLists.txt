cmake_minimum_required(VERSION 3.20)
project(ImGuiDLL)

set(CMAKE_CXX_STANDARD 17)

add_library(imgui SHARED
    ../imgui/imgui.cpp
    ../imgui/imgui_demo.cpp
    ../imgui/imgui_draw.cpp
    ../imgui/imgui_tables.cpp
    ../imgui/imgui_widgets.cpp
    ../imgui/backends/imgui_impl_win32.cpp
    ../imgui/backends/imgui_impl_dx11.cpp
    ../imgui/misc/cpp/imgui_stdlib.cpp
)

target_compile_definitions(imgui PRIVATE IMGUI_BUILD_DLL)

target_include_directories(imgui PUBLIC
    ../imgui
    ../imgui/backends
)

target_link_libraries(imgui PRIVATE d3d11 dxgi)

if(MSVC)
    target_compile_options(imgui PRIVATE /W3 /wd4996)
endif()

set_target_properties(imgui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
