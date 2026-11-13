@echo off
REM Puts the MSVC x64 toolchain on the calling script's environment: the newest
REM Visual Studio (any edition, Build Tools included) with the C++ tools, found
REM by vswhere. Does nothing when a toolchain is already set up.

if defined VSCMD_VER exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [build] ERROR: vswhere.exe not found; install Visual Studio with the C++ tools
    exit /b 1
)
set VS=
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"
if not defined VS (
    echo [build] ERROR: no Visual Studio with the C++ x64 tools
    exit /b 1
)
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if not defined VSCMD_VER (
    echo [build] ERROR: could not initialise MSVC x64 environment from %VS%
    exit /b 1
)
exit /b 0
