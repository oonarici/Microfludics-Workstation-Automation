#!/usr/bin/env bash
set -e

echo "==> Bootstrapping C++20 + Qt + CMake repository..."

# ---------- directories ----------
mkdir -p app src include tests docs .github/workflows

# ---------- README.md ----------
cat > README.md << 'EOF'
# Device Control Application

Cross-platform desktop application written in **C++20** using **Qt**.

## Platforms
- Linux
- Windows
- macOS

## Requirements
- C++20 compiler
- CMake >= 3.20
- Qt 6.x or Qt 5.15.x
- Ninja (recommended)

## Build (Linux / macOS)
```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
