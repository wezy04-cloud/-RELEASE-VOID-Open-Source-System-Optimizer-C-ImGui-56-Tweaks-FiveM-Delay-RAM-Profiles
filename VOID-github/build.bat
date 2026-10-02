@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

rem --- locate Visual Studio (C++ toolset) ---------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [!] vswhere.exe not found. Install Visual Studio with "Desktop development with C++".
    exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo [!] No Visual Studio installation with the C++ toolset was found.
    exit /b 1
)
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

rem --- fetch Dear ImGui if missing -----------------------------------------
if not exist "third_party\imgui\imgui.h" (
    echo [*] Cloning Dear ImGui...
    git clone --depth 1 https://github.com/ocornut/imgui.git third_party\imgui || exit /b 1
)

if not exist build mkdir build
if not exist build\obj mkdir build\obj

set IMGUI=third_party\imgui
set SOURCES=src\main.cpp src\app.cpp src\gui\theme.cpp src\gui\fx.cpp src\gui\icons.cpp src\gui\widgets.cpp ^
 src\core\license.cpp src\core\sysinfo.cpp src\core\sysinfo_detail.cpp src\core\cleaner.cpp src\core\tweaks.cpp src\core\network.cpp src\core\lang.cpp src\core\ram.cpp src\core\regpack.cpp ^
 %IMGUI%\imgui.cpp %IMGUI%\imgui_draw.cpp %IMGUI%\imgui_tables.cpp %IMGUI%\imgui_widgets.cpp ^
 %IMGUI%\backends\imgui_impl_win32.cpp %IMGUI%\backends\imgui_impl_dx11.cpp

echo [*] Building VOID.exe ...
cl /nologo /std:c++17 /O2 /MT /EHsc /utf-8 /W3 /MP ^
   /DNDEBUG /DUNICODE /D_UNICODE /DIMGUI_DEFINE_MATH_OPERATORS ^
   /I src /I %IMGUI% /I %IMGUI%\backends ^
   %SOURCES% ^
   /Fobuild\obj\ /Febuild\VOID.exe ^
   /link /SUBSYSTEM:WINDOWS d3d11.lib dxgi.lib d3dcompiler.lib dwmapi.lib user32.lib gdi32.lib advapi32.lib shell32.lib iphlpapi.lib
if errorlevel 1 (
    echo [!] Build failed.
    exit /b 1
)
echo [+] Done: build\VOID.exe
