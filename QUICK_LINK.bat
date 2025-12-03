@echo off
REM Quick symlink setup for UnseenFrontier project
REM Adjust the path below to match your actual project location

set PROJECT_PATH=C:\Users\skh\Desktop\unreal\project-unseen-frontier\UnseenFrontier
set REPO_PATH=%~dp0
set REPO_PATH=%REPO_PATH:~0,-1%

echo Creating symbolic link...
echo Project: %PROJECT_PATH%
echo Repository: %REPO_PATH%
echo.

REM Create Plugins folder if needed
if not exist "%PROJECT_PATH%\Plugins" mkdir "%PROJECT_PATH%\Plugins"

REM Remove existing if present
if exist "%PROJECT_PATH%\Plugins\UnrealGraph" (
    echo Removing existing UnrealGraph folder/link...
    rmdir "%PROJECT_PATH%\Plugins\UnrealGraph" 2>nul
)

REM Create the junction
echo Creating link...
mklink /J "%PROJECT_PATH%\Plugins\UnrealGraph" "%REPO_PATH%"

if %errorlevel% equ 0 (
    echo.
    echo ✅ SUCCESS! Link created!
    echo.
    echo Now your project uses the plugin from this repository.
    echo Changes here will automatically appear in your project!
) else (
    echo.
    echo ❌ Failed to create link.
    echo Try running as Administrator or use SETUP_SYMLINK.bat
)

pause

