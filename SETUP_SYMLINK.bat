@echo off
REM Create a symbolic link to the UnrealGraph plugin in your project
REM This allows you to edit the plugin in the repository and have changes reflect in the project

echo ========================================
echo Unreal Graph Plugin - Symlink Setup
echo ========================================
echo.
echo This will create a symbolic link (junction) so your project
echo points to this repository. Changes here will appear in the project!
echo.

REM Get the current repository path
set REPO_PATH=%~dp0
REM Remove trailing backslash
set REPO_PATH=%REPO_PATH:~0,-1%

echo Repository path: %REPO_PATH%
echo.

REM Get project path
set /p PROJECT_PATH="Enter your Unreal project path (folder containing .uproject file): "
if "%PROJECT_PATH%"=="" (
    echo ERROR: Project path is required.
    pause
    exit /b 1
)

REM Normalize path (remove quotes if present)
set PROJECT_PATH=%PROJECT_PATH:"=%

REM Check if .uproject file exists
if not exist "%PROJECT_PATH%\*.uproject" (
    echo ERROR: No .uproject file found in %PROJECT_PATH%
    echo Please provide a valid Unreal Engine project path.
    pause
    exit /b 1
)

echo.
echo Project path: %PROJECT_PATH%
echo.

REM Create Plugins folder if it doesn't exist
if not exist "%PROJECT_PATH%\Plugins" (
    echo Creating Plugins folder...
    mkdir "%PROJECT_PATH%\Plugins"
)

REM Check if link already exists
if exist "%PROJECT_PATH%\Plugins\UnrealGraph" (
    echo WARNING: UnrealGraph already exists in Plugins folder!
    echo.
    echo Checking if it's a link or a folder...
    dir "%PROJECT_PATH%\Plugins\UnrealGraph" | find "<SYMLINK" >nul
    if %errorlevel% equ 0 (
        echo It appears to be a symbolic link. Removing old link...
        rmdir "%PROJECT_PATH%\Plugins\UnrealGraph"
    ) else (
        echo It appears to be a regular folder.
        set /p REMOVE="Remove existing folder and create link? (y/n): "
        if /i not "%REMOVE%"=="y" (
            echo Setup cancelled.
            pause
            exit /b 0
        )
        echo Removing existing folder...
        rmdir /s /q "%PROJECT_PATH%\Plugins\UnrealGraph"
    )
)

echo.
echo Creating symbolic link (junction)...
echo Source: %REPO_PATH%
echo Target: %PROJECT_PATH%\Plugins\UnrealGraph
echo.

REM Create junction (works without admin rights)
mklink /J "%PROJECT_PATH%\Plugins\UnrealGraph" "%REPO_PATH%"

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo SUCCESS! Symbolic link created!
    echo ========================================
    echo.
    echo Now your project will use the plugin from this repository.
    echo Any changes you make here will automatically appear in the project!
    echo.
    echo Next steps:
    echo 1. Generate Visual Studio project files (right-click .uproject)
    echo 2. Enable plugin in Editor: Edit -^> Plugins -^> Unreal Graph
    echo 3. Compile in Visual Studio
    echo.
) else (
    echo.
    echo ERROR: Failed to create symbolic link.
    echo.
    echo If you got "Access is denied", try running this script as Administrator.
    echo Alternatively, you can create the link manually:
    echo   mklink /J "%PROJECT_PATH%\Plugins\UnrealGraph" "%REPO_PATH%"
    echo.
)

pause

