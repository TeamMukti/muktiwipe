@echo off
rem ---------------------------------------------------------------
rem  muktiwipe build script - compiles muktiwipe.exe (Release x64) using
rem  the Visual Studio C++ toolchain (MSVC).
rem
rem  Usage:  build.bat            - build into .\muktiwipe.exe
rem          build.bat clean      - remove intermediate files
rem ---------------------------------------------------------------
setlocal enabledelayedexpansion

cd /d "%~dp0"

if /i "%~1" == "clean" (
    echo Cleaning...
    if exist muktiwipe.exe del /q muktiwipe.exe
    if exist muktiwipe.pdb del /q muktiwipe.pdb
    if exist muktiwipe.exp del /q muktiwipe.exp
    if exist muktiwipe.exe.manifest del /q muktiwipe.exe.manifest
    if exist obj rmdir /s /q obj
    echo Done.
    exit /b 0
)

rem --- locate Visual Studio via vswhere --------------------------
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
set VSPATH=

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set VSPATH=%%i
)

if not exist "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" (
    echo ERROR - Visual Studio with C++ tools not found.
    echo         Install "Desktop development with C++" workload,
    echo         or adjust VSPATH in this script.
    exit /b 1
)

echo Using Visual Studio at %VSPATH%
echo.

if not exist obj mkdir obj

rem --- load the VS toolchain environment --------------------------
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

rem --- resources (version info for the Details tab) ---------------
rc /nologo /fo obj\muktiwipe.res muktiwipe.rc
if errorlevel 1 (
    echo.
    echo RESOURCE COMPILATION FAILED
    exit /b 1
)

rem --- compile ---------------------------------------------------
cl /nologo /W3 /EHsc /O2 /GL /Gy /Zi /FS /MT /utf-8 ^
   /Foobj\ /Fdobj\muktiwipe.pdb ^
   /Femuktiwipe.exe ^
   wmain.cpp muktiwipe.cpp ultra_machine.cpp folder.cpp delete_file.cpp utils.cpp ^
   libp\string_utils.cpp libp\api_error.cpp libp\_system_api.cpp libp\_elpify.cpp ^
   libp\_console.cpp libp\_filesys.cpp libp\_cpu_info.cpp libp\_scan_folder_nt.cpp ^
   libp\_simple_work_queue.cpp libp\_security.cpp libp\_unlock.cpp libp\_lang.cpp ^
   obj\muktiwipe.res ^
   /link /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:CONSOLE

if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)

del /q muktiwipe.exp >nul 2>&1
del /q muktiwipe.exe.manifest >nul 2>&1

echo.
echo Build OK - muktiwipe.exe
muktiwipe.exe | find "muktiwipe 1.0" >nul
if errorlevel 1 echo WARNING - muktiwipe.exe did not print its banner

exit /b 0
