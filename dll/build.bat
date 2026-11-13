@echo off
REM Build Cleanroom UI's F4SE plugins (Fallout4.exe 1.11.240) into %1 (default: the repo's dist folder):
REM CleanroomUI.dll (framework\) and CleanroomDialogue.dll (dialogue\), each with shared\ compiled in.
REM
REM Needs F4SE 0.7.9's source and ianpatt's common, built as static libraries
REM (docs/spin_offs.md#runtime). TOOLS_EXT defaults to a tools_ext folder beside
REM the repo; F4SE_ROOT and COMMON_ROOT default to folders inside it.

setlocal
call "%~dp0msvc.bat"
if errorlevel 1 exit /b 1
if not defined TOOLS_EXT set "TOOLS_EXT=%~dp0..\..\tools_ext"
if not defined F4SE_ROOT set "F4SE_ROOT=%TOOLS_EXT%\f4se_src\f4se-0.7.9"
if not defined COMMON_ROOT set "COMMON_ROOT=%TOOLS_EXT%\extern"
set "OUT=%~1"
if not defined OUT set "OUT=%~dp0..\dist"
if not exist "%F4SE_ROOT%\f4se\PluginAPI.h" (
    echo [build] ERROR: F4SE source not found at %F4SE_ROOT%
    exit /b 1
)
if not exist "%OUT%" mkdir "%OUT%"
cd /d "%~dp0"

call :plugin CleanroomUI framework plugin.cpp pipboy.cpp repair.cpp transfer.cpp vendor.cpp pages.cpp ammo.cpp card.cpp sex.cpp hud.cpp layout.cpp keys.cpp
if errorlevel 1 exit /b 1
call :plugin CleanroomDialogue dialogue plugin.cpp dialogue.cpp
if errorlevel 1 exit /b 1
endlocal
exit /b 0

REM :plugin <name> <folder> <sources in folder...>: <folder>'s sources and shared\gfx.cpp into %OUT%\<name>.dll.
:plugin
set "NAME=%~1"
set "DIR=%~2"
set "OBJ=obj\%NAME%"
set SOURCES=
shift
shift
:more
if "%~1"=="" goto compile
set SOURCES=%SOURCES% %DIR%\%~1
shift
goto more
:compile
if not exist "%OBJ%" mkdir "%OBJ%"
del /q "%OBJ%\*.obj" 2>nul
echo [build] compiling %NAME%...
cl /nologo /c /EHsc /std:c++17 /O2 /MT /W3 /DNDEBUG /DRUNTIME /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS ^
   /I"%F4SE_ROOT%" /I"%COMMON_ROOT%\include" /Ishared /FI"common/IPrefix.h" shared\gfx.cpp %SOURCES% /Fo:%OBJ%\
if errorlevel 1 (
    echo [build] ERROR: %NAME% compilation failed
    exit /b 1
)
link /nologo /DLL /OUT:"%OUT%\%NAME%.dll" /IMPLIB:%OBJ%\%NAME%.lib %OBJ%\*.obj ^
     "%F4SE_ROOT%\build\f4se\Release\f4se_1_11_240.lib" ^
     "%F4SE_ROOT%\build\f4se\f4se_common\Release\f4se_common.lib" ^
     "%COMMON_ROOT%\lib\common.lib" kernel32.lib user32.lib shell32.lib ole32.lib advapi32.lib comdlg32.lib /LTCG
if errorlevel 1 (
    echo [build] ERROR: %NAME% link failed
    exit /b 1
)
echo [build] OK -^> %OUT%\%NAME%.dll
exit /b 0
