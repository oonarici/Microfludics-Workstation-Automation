#requires -version 5.1
$ErrorActionPreference = "Stop"

$RootDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$VenvDir = Join-Path $RootDir ".venv"
$ReqFile = Join-Path $RootDir "tools\requirements.txt"
$BuildDir = Join-Path $RootDir "build"
$LockFile = Join-Path $RootDir "conan.lock"
$CacheFile = $env:CONAN_CACHE_FILE

if (-not (Test-Path $VenvDir)) { throw ".venv not found. Run scripts/dev_windows.ps1 first." }

$PyExe   = Join-Path $VenvDir "Scripts\python.exe"
$ConanExe = Join-Path $VenvDir "Scripts\conan.exe"

& $PyExe -m pip install -r $ReqFile | Out-Host

if ([string]::IsNullOrWhiteSpace($CacheFile) -or -not (Test-Path $CacheFile)) {
    throw "CONAN_CACHE_FILE not set or file not found. Set it to conan_cache_windows.tgz path."
}

& $ConanExe cache restore $CacheFile

& $ConanExe install $RootDir `
    --lockfile=$LockFile `
    --output-folder=$BuildDir `
    --build=missing `
    --no-remote

Write-Host "Offline install OK. Build folder: $BuildDir"

