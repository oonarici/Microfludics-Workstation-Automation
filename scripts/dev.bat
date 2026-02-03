@echo off
setlocal

set ROOT_DIR=%~dp0..
set VENV_DIR=%ROOT_DIR%\.venv
set REQ_FILE=%ROOT_DIR%\tools\requirements.txt
set CONAN_BIN=%VENV_DIR%\Scripts\conan.exe

echo === Dev bootstrap (Windows) ===

REM Find Python (python or py -3)
where python >nul 2>nul
if %ERRORLEVEL%==0 (
  set PY=python
) else (
  where py >nul 2>nul
  if %ERRORLEVEL%==0 (
    set PY=py -3
  ) else (
    echo ERROR: Python 3 not found. Install Python 3.10+ and retry.
    exit /b 1
  )
)

REM Create venv if missing
if not exist "%VENV_DIR%" (
  echo [1/4] Creating venv: %VENV_DIR%
  %PY% -m venv "%VENV_DIR%"
)

if not exist "%USERPROFILE%\.conan2\profiles\default" (
  echo Detecting Conan default profile...
  "%CONAN_BIN%" profile detect --force
)

REM Install Conan if missing
if not exist "%CONAN_BIN%" (
  echo [2/4] Installing Conan...
  "%VENV_DIR%\Scripts\python.exe" -m pip install --upgrade pip
  "%VENV_DIR%\Scripts\python.exe" -m pip install -r "%REQ_FILE%"
)

REM Conan install
echo [3/4] Conan install...
"%CONAN_BIN%" --version
"%CONAN_BIN%" install "%ROOT_DIR%" --output-folder "%ROOT_DIR%\build" --build=missing

REM Configure + open IDE
echo [4/4] Configure + launch IDE...
cmake --preset windows-release

where code >nul 2>nul
if %ERRORLEVEL%==0 (
  start "" code "%ROOT_DIR%"
) else (
  echo IDE not found on PATH. Open manually: %ROOT_DIR%
)

echo === Done ===
