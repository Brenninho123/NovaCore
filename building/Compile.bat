@echo off
setlocal enabledelayedexpansion

set TARGET=%1
set CONFIG=%2

if "%TARGET%"=="" (
    goto usage
)

if "%CONFIG%"=="" (
    set CONFIG=Debug
)

if not "%CONFIG%"=="Debug" if not "%CONFIG%"=="Release" (
    echo Invalid config: %CONFIG% ^(expected Debug or Release^)
    exit /b 1
)

if "%TARGET%"=="tool" (
    goto build_tool
)

if "%TARGET%"=="windows" (
    goto build_windows
)

if "%TARGET%"=="android" (
    goto build_android
)

if "%TARGET%"=="clean" (
    goto clean
)

echo Unknown target: %TARGET%
goto usage

:build_tool
where cl >nul 2>nul
if %errorlevel% equ 0 (
    cl building\Build.c /Fe:build_tool.exe
) else (
    where gcc >nul 2>nul
    if !errorlevel! equ 0 (
        gcc building\Build.c -o build_tool.exe
    ) else (
        echo Neither cl nor gcc found in PATH
        exit /b 1
    )
)
if %errorlevel% neq 0 (
    echo Failed to compile build_tool.exe
    exit /b 1
)
echo build_tool.exe compiled successfully
exit /b 0

:build_windows
call :ensure_tool
if %errorlevel% neq 0 exit /b 1
build_tool.exe windows %CONFIG%
exit /b %errorlevel%

:build_android
call :ensure_tool
if %errorlevel% neq 0 exit /b 1
if "%VCPKG_ROOT%"=="" (
    echo VCPKG_ROOT environment variable is not set
    exit /b 1
)
if "%ANDROID_NDK_HOME%"=="" (
    echo ANDROID_NDK_HOME environment variable is not set
    exit /b 1
)
build_tool.exe android %CONFIG%
exit /b %errorlevel%

:clean
call :ensure_tool
if %errorlevel% neq 0 exit /b 1
build_tool.exe clean
exit /b %errorlevel%

:ensure_tool
if not exist build_tool.exe (
    call :build_tool
)
exit /b 0

:usage
echo Usage: Compile.bat ^<target^> [config]
echo.
echo Targets:
echo   tool          Compile building\Build.c into build_tool.exe
echo   windows       Configure and build the Windows desktop target
echo   android       Configure and build the Android arm64-v8a target
echo   clean         Remove the build directory
echo.
echo Config (optional, defaults to Debug):
echo   Debug
echo   Release
echo.
echo Examples:
echo   Compile.bat windows
echo   Compile.bat windows Release
echo   Compile.bat android
echo   Compile.bat clean
exit /b 1
