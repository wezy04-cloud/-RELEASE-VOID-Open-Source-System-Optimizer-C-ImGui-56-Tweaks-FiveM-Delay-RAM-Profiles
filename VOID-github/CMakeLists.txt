cmake_minimum_required(VERSION 3.16)
project(VOID LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
if(NOT EXISTS ${IMGUI_DIR}/imgui.h)
    message(FATAL_ERROR "Dear ImGui not found. Run: git clone https://github.com/ocornut/imgui.git third_party/imgui")
endif()

add_executable(VOID WIN32
    src/main.cpp
    src/app.cpp
    src/gui/theme.cpp
    src/gui/fx.cpp
    src/gui/icons.cpp
    src/gui/widgets.cpp
    src/core/license.cpp
    src/core/sysinfo.cpp
    src/core/cleaner.cpp
    src/core/tweaks.cpp
    src/core/network.cpp
    src/core/lang.cpp
    src/core/sysinfo_detail.cpp
    src/core/ram.cpp
    src/core/regpack.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(VOID PRIVATE src ${IMGUI_DIR} ${IMGUI_DIR}/backends)
target_compile_definitions(VOID PRIVATE UNICODE _UNICODE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_options(VOID PRIVATE /utf-8 /W3)
target_link_libraries(VOID PRIVATE d3d11 dxgi d3dcompiler dwmapi user32 gdi32 advapi32 shell32 iphlpapi)
