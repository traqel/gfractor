@echo off
REM Build script for gFractor audio plugin (Windows)

setlocal enabledelayedexpansion

REM Configuration
set BUILD_TYPE=Release
set BUILD_DIR=build
set PARALLEL_JOBS=4

echo ========================================
echo gFractor Plugin Build Script
echo ========================================

REM Check if JUCE submodule exists
if not exist "JUCE\CMakeLists.txt" (
    echo JUCE submodule not found. Initializing submodules...
    git submodule update --init --recursive
)

REM Parse command line arguments
set CLEAN=false
set RUN_TESTS=false
set INSTALL=false

:parse_args
if "%~1"=="" goto end_parse_args
if /i "%~1"=="--clean" (
    set CLEAN=true
    shift
    goto parse_args
)
if /i "%~1"=="--test" (
    set RUN_TESTS=true
    shift
    goto parse_args
)
if /i "%~1"=="--install" (
    set INSTALL=true
    shift
    goto parse_args
)
if /i "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if /i "%~1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if /i "%~1"=="--help" (
    echo Usage: build.bat [options]
    echo.
    echo Options:
    echo   --clean      Clean build directory before building
    echo   --test       Run tests after building
    echo   --install    Install plugin to system directories
    echo   --debug      Build in Debug mode (default: Release)
    echo   --release    Build in Release mode
    echo   --help       Show this help message
    exit /b 0
)
echo Unknown option: %~1
exit /b 1

:end_parse_args

REM Clean if requested
if "%CLEAN%"=="true" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Configure
echo Configuring CMake (%BUILD_TYPE%)...
cmake -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo Building with %PARALLEL_JOBS% parallel jobs...
cmake --build "%BUILD_DIR%" ^
    --config %BUILD_TYPE% ^
    --parallel %PARALLEL_JOBS%

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

REM Run tests if requested
if "%RUN_TESTS%"=="true" (
    echo Running tests...
    ctest --test-dir "%BUILD_DIR%" ^
        --build-config %BUILD_TYPE% ^
        --output-on-failure

    if errorlevel 1 (
        echo Tests failed!
        exit /b 1
    )
)

REM Install if requested
if "%INSTALL%"=="true" (
    echo Installing plugin...
    cmake --install "%BUILD_DIR%" --config %BUILD_TYPE%

    if errorlevel 1 (
        echo Installation failed!
        exit /b 1
    )
)

REM Summary
echo ========================================
echo Build completed successfully!
echo ========================================
echo Build type: %BUILD_TYPE%
echo Build directory: %BUILD_DIR%
echo.

REM Show plugin locations
echo Plugin artifacts:
if exist "%BUILD_DIR%" (
    dir /s /b "%BUILD_DIR%\*.vst3" 2>nul
)

echo.
echo Tip: Use --test to run tests, --clean to rebuild from scratch

endlocal
