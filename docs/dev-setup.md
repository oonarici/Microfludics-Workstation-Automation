# Development setup

## Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build
```

Install Qt via the official installer or distribution packages (Qt 6.x preferred).
If Qt is not on your `PATH`, set `CMAKE_PREFIX_PATH` to the Qt install prefix.

Example (Qt installer default):
```bash
export CMAKE_PREFIX_PATH="$HOME/Qt/6.6.0/gcc_64"
```

## macOS
```bash
brew install cmake ninja
```

Install Qt via the official installer (Qt 6.x preferred). If Qt is not on your
`PATH`, set `CMAKE_PREFIX_PATH` to the Qt install prefix.

Example:
```bash
export CMAKE_PREFIX_PATH="$HOME/Qt/6.6.0/macos"
```

## Windows
- Install Visual Studio 2022 with the "Desktop development with C++" workload.
- Install Qt (MSVC build) via the official installer.
- Ensure Qt is discoverable via `PATH` or set `CMAKE_PREFIX_PATH`.

Example (PowerShell):
```powershell
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.6.0\msvc2019_64"
```

## Build & test
```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

If you need a debug build, use the `debug` preset:
```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```
