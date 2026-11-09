@echo off
rem Build EMTSim with the Visual Studio C++ compiler (MSVC) and run the tests.
rem Usage: build.bat
setlocal
cd /d "%~dp0"

rem Already in a Developer Command Prompt?
where cl >nul 2>nul && goto build

rem Otherwise find Visual Studio with the C++ tools and load its environment
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto nocl
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR goto nocl
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
where cl >nul 2>nul || goto nocl

:build
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build || exit /b 1
ctest --test-dir build --output-on-failure || exit /b 1
exit /b 0

:nocl
echo Visual Studio C++ build tools not found. Install "Desktop development with C++"
echo or run this from a Developer Command Prompt.
exit /b 1
