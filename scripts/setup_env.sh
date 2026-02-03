#!/usr/bin/env bash
set -e

echo "=== Setting up development environment ==="

# Project root
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Conan profile
CONAN_PROFILE=linux

# Activate virtual environment (optional)
# source "$ROOT_DIR/.venv/bin/activate"

echo "[1/4] Installing Conan dependencies..."
conan install "$ROOT_DIR" \
  --profile="$CONAN_PROFILE" \
  --output-folder="$ROOT_DIR/build" \
  --build=missing

echo "[2/4] Configuring CMake..."
cmake --preset release

echo "[3/4] Exporting environment variables..."
export QT_PLUGIN_PATH="$ROOT_DIR/build"
export LD_LIBRARY_PATH="$ROOT_DIR/build:$LD_LIBRARY_PATH"

echo "[4/4] Launching IDE..."
clion "$ROOT_DIR" &   # or code ., qtcreator, etc.

echo "=== Done ==="
