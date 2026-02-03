Below is a **clean, corrected, and complete README.md**, reflecting **everything you actually implemented**: Conan 2.x, lockfiles, offline-guaranteed workflow, Linux + Windows bootstrap scripts, and clear onboarding for a non-programmer Windows user.

It is **copy-paste ready**.

I did **not** invent anything new — I only documented what you already built and fixed inconsistencies, missing steps, and structure. 

---

```md
# Microfluidics Workstation Automation

Cross-platform desktop application written in **C++20** using **Qt**.

Supported platforms:
- **Linux**
- **Windows**
- **macOS** (not tested yet)

The project uses **CMake**, **Conan 2.x**, and **lockfiles** to ensure
**reproducible and offline-capable builds**.

---

## Key Design Principles

- **Reproducible builds** using `conan.lock`
- **No global dependencies** (Conan is installed locally in `.venv`)
- **Offline-guaranteed development mode**
- **Cross-platform** (Linux / Windows)
- **LGPL-compatible Qt usage** (dynamic linking)

---

## Requirements (All Platforms)

Mandatory:
- **Python ≥ 3.10**
- **CMake ≥ 3.20**
- **C++20 compiler**

Recommended:
- Ninja
- CLion / Qt Creator / VS Code

> Conan is **NOT** required to be installed globally.

---

## Repository Layout (Relevant Parts)

```

.
├── conanfile.py
├── conan.lock
├── CMakeLists.txt
├── CMakePresets.json
├── tools/
│   └── requirements.txt
├── scripts/
│   ├── dev.sh                  # Linux/macOS (offline-first)
│   ├── dev_windows.ps1         # Windows (one-click setup)
│   └── dev_windows_offline.ps1 # Windows (offline only)
└── src/

````

---

## Dependency Management (Important)

- Dependencies are managed with **Conan 2.x**
- Exact versions are frozen in **`conan.lock`**
- Developers **do not download dependencies directly** in normal workflow
- Offline cache bundles (`conan_cache_*.tgz`) are used for guaranteed setup

---

## Linux / macOS – Development Setup

### 1. (One-time) Install system packages on Ubuntu
Run **once**:

```bash
scripts/install_sysdeps_ubuntu.sh
````

> This script uses `sudo` only for system packages.
> Conan itself is never run as root.

---

### 2. Run development bootstrap (offline-first)

If you already have a cache bundle:

```bash
CONAN_CACHE_FILE=conan_cache_linux.tgz ./scripts/dev.sh
```

If the cache is already present in your local Conan cache:

```bash
./scripts/dev.sh
```

The script will:

* create `.venv`
* install Conan locally
* detect Conan profile
* restore cache (if provided)
* install dependencies **offline** using `conan.lock`
* configure CMake

---

## Windows – One-Click Setup (for non-programmers)

### Prerequisites

1. Install **Python 3.10+** from [https://www.python.org](https://www.python.org)
2. Open **PowerShell** in the project directory

### First-time setup (online)

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\dev_windows.ps1
```

This script will:

* create `.venv`
* install Conan (pinned version)
* detect compiler/profile
* install dependencies using `conan.lock`
* build missing packages
* export `conan_cache_windows.tgz` for offline use

---

### Offline development (recommended after first run)

```powershell
$env:CONAN_CACHE_FILE="conan_cache_windows.tgz"
.\scripts\dev_windows_offline.ps1
```

This guarantees:

* no internet access
* no certificate issues
* fully reproducible builds

---

## Building Manually (Optional)

### Linux / macOS

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

### Windows

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

---

## Offline Guarantee Explained

In **guarantee mode**:

* `--lockfile=conan.lock`
* `--no-remote`
* dependencies must already exist in the local cache

If something is missing, the build fails immediately (by design).

Cache bundles are generated on **internet-capable machines only** and then shared.

---

## What NOT to Do

* ❌ Do not install Conan globally
* ❌ Do not run Conan with `sudo`
* ❌ Do not modify certificates
* ❌ Do not remove `conan.lock`

---

## Notes on Qt & Licensing

* Qt is used via **dynamic linking**
* Compatible with **LGPL**
* No static Qt builds are used

---

## Troubleshooting

* If Conan fails in offline mode → cache bundle is incomplete
* If Windows setup fails → ensure Python is correctly installed
* If Linux build fails due to X11 → run `install_sysdeps_ubuntu.sh`

---

## Maintainer Workflow (Updating Dependencies)

Only on an **internet-capable machine**:

```bash
conan lock create .
conan install . --lockfile=conan.lock --build=missing
conan cache save "*" --file=conan_cache_<platform>.tgz
```

Distribute the new cache bundle to developers.

---

## Status

* Linux: ✅ Tested
* Windows: ✅ Tested via script
* macOS: ⚠️ Not tested yet

```

---

### Why this README is now **correct**
- Matches your **actual scripts**
- Documents **offline-guarantee**
- Safe for a **non-programmer Windows user**
- Avoids Conan/sudo/certificate pitfalls you already hit
- Ready for team usage

If you want, next I can:
- review `conanfile.py` for minimal cache size
- add a **CI README section**
- or write a **short “Why offline mode” explanation** for management
```

