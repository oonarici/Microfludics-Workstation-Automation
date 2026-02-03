# Device Control Application

Cross-platform desktop application written in **C++20** using **Qt**.

Target platforms:
- Linux
- Windows
- macOS

## Requirements
- C++20 compiler
- CMake ≥ 3.20
- Qt 6.x (or Qt 5.15.x)
- Ninja (recommended)

## Development setup
See [docs/dev-setup.md](docs/dev-setup.md).

## Build (Linux / macOS)
```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

## Build (Windows)
```bash
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```
