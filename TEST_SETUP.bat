@echo off
REM Quick setup script for testing Unreal Graph plugin
REM This script helps set up the plugin for testing

echo ========================================
echo Unreal Graph Plugin - Test Setup
echo ========================================
echo.

set /p PROJECT_PATH="Enter your Unreal project path (or press Enter to use current directory): "
if "%PROJECT_PATH%"=="" set PROJECT_PATH=%CD%

echo.
echo Project path: %PROJECT_PATH%
echo.

REM Check if .uproject file exists
if not exist "%PROJECT_PATH%\*.uproject" (
    echo ERROR: No .uproject file found in %PROJECT_PATH%
    echo Please provide a valid Unreal Engine project path.
    pause
    exit /b 1
)

echo [1/4] Checking project structure...
if not exist "%PROJECT_PATH%\Plugins" (
    echo Creating Plugins folder...
    mkdir "%PROJECT_PATH%\Plugins"
)

echo [2/4] Checking if plugin already exists...
if exist "%PROJECT_PATH%\Plugins\UnrealGraph" (
    echo WARNING: UnrealGraph plugin already exists!
    set /p OVERWRITE="Overwrite existing plugin? (y/n): "
    if /i not "%OVERWRITE%"=="y" (
        echo Setup cancelled.
        pause
        exit /b 0
    )
    echo Removing existing plugin...
    rmdir /s /q "%PROJECT_PATH%\Plugins\UnrealGraph"
)

echo [3/4] Copying plugin files...
xcopy /E /I /Y "%CD%" "%PROJECT_PATH%\Plugins\UnrealGraph"

echo [4/4] Setup complete!
echo.
echo Next steps:
echo 1. Open your project: %PROJECT_PATH%
echo 2. Generate Visual Studio project files (right-click .uproject)
echo 3. Enable plugin in Editor: Edit -^> Plugins -^> Unreal Graph
echo 4. Compile in Visual Studio
echo.
echo See TESTING_GUIDE.md for detailed instructions.
echo.
pause

