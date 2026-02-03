#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="$ROOT_DIR/.venv"
REQ_FILE="$ROOT_DIR/tools/requirements.txt"
CONAN_BIN="$VENV_DIR/bin/conan"

echo "=== Dev bootstrap (Linux/macOS) ==="

# Find Python
if command -v python3 >/dev/null 2>&1; then PY=python3
elif command -v python >/dev/null 2>&1; then PY=python
else
  echo "ERROR: Python 3 not found. Install Python 3.10+ and retry."
  exit 1
fi

# Create venv
if [ ! -d "$VENV_DIR" ]; then
  echo "[1/6] Creating venv: $VENV_DIR"
  "$PY" -m venv "$VENV_DIR"
fi

# Ensure Conan version (always)
echo "[2/6] Ensuring Conan version..."
"$VENV_DIR/bin/pip" install --upgrade pip
"$VENV_DIR/bin/pip" install -r "$REQ_FILE"

# Ensure Conan default profile exists
if [ ! -f "$HOME/.conan2/profiles/default" ]; then
  echo "[3/6] Detecting Conan default profile..."
  "$CONAN_BIN" profile detect --force
fi

# Optional: restore offline cache bundle
if [ -n "${CONAN_CACHE_FILE:-}" ] && [ -f "$CONAN_CACHE_FILE" ]; then
  echo "[4/6] Restoring Conan cache from: $CONAN_CACHE_FILE"
  "$CONAN_BIN" cache restore "$CONAN_CACHE_FILE"
else
  echo "[4/6] No CONAN_CACHE_FILE provided. Offline install will succeed only if cache already has all packages."
fi

# Offline, lockfile-enforced install (guarantee mode)
echo "[5/6] Installing dependencies (offline, lockfile enforced)..."
"$CONAN_BIN" install "$ROOT_DIR" \
  --lockfile="$ROOT_DIR/conan.lock" \
  --output-folder="$ROOT_DIR/build" \
  --build=missing \
  --no-remote

# Configure CMake (optional, keep your preset)
echo "[6/6] Configuring CMake..."
cmake --preset release

echo "=== Done ==="

