@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build\windows-msvc
set TOOLCHAIN_FILE=%BUILD_DIR%\conan_toolchain.cmake

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

where conan >nul 2>nul
if %ERRORLEVEL%==0 (
  echo Running Conan to install dependencies...
  conan install "%SCRIPT_DIR%" --output-folder="%BUILD_DIR%" --build=missing
) else (
  echo Conan not found. Skipping dependency install.
)

echo Configuring CMake (Visual Studio 17 2022)...
if exist "%TOOLCHAIN_FILE%" (
  cmake --preset windows-msvc -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_FILE%"
) else (
  cmake --preset windows-msvc
)

if exist "%BUILD_DIR%\MicrofluidicsAutomationTool.sln" (
  echo Opening solution...
  start "" "%BUILD_DIR%\MicrofluidicsAutomationTool.sln"
) else (
  echo Solution file not found at %BUILD_DIR%\MicrofluidicsAutomationTool.sln
  exit /b 1
)

endlocal
