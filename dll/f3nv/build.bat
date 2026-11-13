@echo off
REM Build F3NV.dll (F4SE plugin, Fallout4.exe 1.11.240) into %1 (default: the
REM repo's dist folder).
REM
REM Needs F4SE 0.7.9's source and ianpatt's common, built as static libraries
REM (docs/spin_offs.md#runtime). TOOLS_EXT defaults to a tools_ext folder beside
REM the repo; F4SE_ROOT and COMMON_ROOT default to folders inside it.

setlocal
call "%~dp0..\msvc.bat"
if errorlevel 1 exit /b 1
if not defined TOOLS_EXT set "TOOLS_EXT=%~dp0..\..\..\tools_ext"
if not defined F4SE_ROOT set "F4SE_ROOT=%TOOLS_EXT%\f4se_src\f4se-0.7.9"
if not defined COMMON_ROOT set "COMMON_ROOT=%TOOLS_EXT%\extern"
set "OUT=%~1"
if not defined OUT set "OUT=%~dp0..\..\dist"
if not exist "%F4SE_ROOT%\f4se\PluginAPI.h" (
    echo [build] ERROR: F4SE source not found at %F4SE_ROOT%
    exit /b 1
)

cd /d "%~dp0"
if not exist obj mkdir obj
if not exist "%OUT%" mkdir "%OUT%"
del /q obj\*.obj 2>nul

echo [build] compiling F3NV...
cl /nologo /c /EHsc /std:c++17 /O2 /MT /W3 /DNDEBUG /DRUNTIME /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS ^
   /I"%F4SE_ROOT%" /I"%COMMON_ROOT%\include" /FI"common/IPrefix.h" plugin.cpp pipboy.cpp repair.cpp transfer.cpp vendor.cpp pages.cpp ammo.cpp card.cpp gfx.cpp sex.cpp damage.cpp hardcore.cpp dialogue.cpp /Fo:obj\
if errorlevel 1 (
    echo [build] ERROR: F3NV compilation failed
    exit /b 1
)
link /nologo /DLL /OUT:"%OUT%\F3NV.dll" /IMPLIB:obj\F3NV.lib obj\*.obj ^
     "%F4SE_ROOT%\build\f4se\Release\f4se_1_11_240.lib" ^
     "%F4SE_ROOT%\build\f4se\f4se_common\Release\f4se_common.lib" ^
     "%COMMON_ROOT%\lib\common.lib" kernel32.lib user32.lib shell32.lib ole32.lib advapi32.lib comdlg32.lib /LTCG
if errorlevel 1 (
    echo [build] ERROR: F3NV link failed
    exit /b 1
)
echo [build] OK -^> %OUT%\F3NV.dll
endlocal
