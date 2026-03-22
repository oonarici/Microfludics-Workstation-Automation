# MWA-01: Development Environment Setup — Task List

**Created by:** Lead
**Date:** 2026-03-22
**Status:** PLANNED

---

## Dependency Graph

```
MWA-01-A  (CMake + minimal main.cpp)
   │
   ├──→ MWA-01-B  (CI/CD workflow)
   ├──→ MWA-01-C  (Directory structure)
   ├──→ MWA-01-D  (.clang-format — Google Style)
   │       └──→ MWA-01-E  (.clang-tidy)
   ├──→ MWA-01-F  (Qt Test skeleton + CTest)
   └──→ MWA-01-G  (Doxyfile configuration)
```

## Execution Order

- **Round 1:** MWA-01-A
- **Round 2 (parallel):** MWA-01-B, MWA-01-C, MWA-01-D, MWA-01-F, MWA-01-G
- **Round 3:** MWA-01-E

---

## MWA-01-A: CMake Project Foundation

```
TASK: MWA-01-A
REQUIREMENT: NFR-002, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create the root CMakeLists.txt and CMakePresets.json for the MWA project.

  The root CMakeLists.txt must:
  - Set cmake_minimum_required to 3.21
  - Set the project name to "MWA" with version 0.1.0 and LANGUAGES CXX
  - Enforce C++20 standard (CMAKE_CXX_STANDARD 20, CMAKE_CXX_STANDARD_REQUIRED ON)
  - Find Qt6 (REQUIRED COMPONENTS Core Gui Widgets Test)
  - Set CMAKE_AUTOMOC, CMAKE_AUTORCC, CMAKE_AUTOUIC to ON
  - Enable CTest via include(CTest) and enable_testing()
  - Add src/ and tests/ subdirectories
  - Set compiler warning flags: -Wall -Wextra -Wpedantic -Werror for Clang/GCC,
    /W4 /WX for MSVC (using generator expressions)

  CMakePresets.json must define:
  - A "macos-clang" configure preset (Ninja generator, build/macos-clang dir)
  - A "windows-msvc" configure preset (Ninja generator, build/windows-msvc dir)
  - Corresponding build presets and test presets for each
  - All presets should set CMAKE_EXPORT_COMPILE_COMMANDS=ON

  src/CMakeLists.txt must:
  - Define the MWA executable target with src/main.cpp as the only source
  - Link to Qt6::Core, Qt6::Gui, Qt6::Widgets

  tests/CMakeLists.txt must:
  - Be an empty but valid CMakeLists.txt (placeholder, no tests yet)

  src/main.cpp must:
  - Be a minimal Qt application: include QApplication, create QApplication instance,
    return app.exec()
  - Have the mandatory Doxygen @file header

ACCEPTANCE CRITERIA:
  - cmake --preset macos-clang configures without errors on macOS
  - cmake --build --preset macos-clang builds with zero warnings
  - The resulting binary launches and exits cleanly
  - CMakePresets.json contains both macos-clang and windows-msvc presets
  - C++20 is enforced (verified via CMAKE_CXX_STANDARD in CMakeLists.txt)
  - Doxygen @file header present on main.cpp
  - No Conan dependency — Qt is found via system/Qt installer path
DEPENDENCIES: none
PRIORITY: HIGH
```

---

## MWA-01-B: CI/CD Pipeline

```
TASK: MWA-01-B
REQUIREMENT: NFR-002
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Update the GitHub Actions CI workflow (.github/workflows/ci.yml) to build and
  test on BOTH macOS and Windows using the CMake presets from MWA-01-A.

  The workflow must:
  - Trigger on push to development and pull_request to development
  - Use a matrix strategy with macos-latest and windows-latest
  - Install Qt 6.8.x using jurplel/install-qt-action@v4 (with correct arch per OS)
  - On Windows: set up MSVC via ilammy/msvc-dev-cmd@v1
  - Configure using cmake --preset (macos-clang or windows-msvc based on runner)
  - Build using cmake --build --preset
  - Run tests using ctest --preset
  - Use preset names from MWA-01-A

ACCEPTANCE CRITERIA:
  - ci.yml has a matrix with both macos-latest and windows-latest
  - macos-latest job uses macos-clang preset
  - windows-latest job uses windows-msvc preset
  - Qt install step uses correct arch for each OS (win64_msvc2022_64 for Windows)
  - CTest step is present for both platforms
  - Workflow YAML is valid (no syntax errors)
DEPENDENCIES: MWA-01-A
PRIORITY: HIGH
```

---

## MWA-01-C: Project Directory Structure

```
TASK: MWA-01-C
REQUIREMENT: NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create the project directory structure as defined in PROJECT_DESCRIPTION.md.
  Create ONLY the directories and placeholder .gitkeep files — no production
  code beyond what MWA-01-A already provides.

  Directories to create (with empty .gitkeep where no files exist yet):
  - cmake/
  - src/app/
  - src/gui/panels/
  - src/gui/widgets/
  - src/hardware/
  - src/analysis/
  - src/core/
  - resources/
  - scripts/

  Remove the old include/microfluidics/ directory (legacy structure).

ACCEPTANCE CRITERIA:
  - All listed directories exist with .gitkeep files
  - include/microfluidics/ directory is removed
  - No production source files added (only .gitkeep placeholders)
  - The project still configures and builds cleanly after this change
DEPENDENCIES: MWA-01-A
PRIORITY: MEDIUM
```

---

## MWA-01-D: Clang-Format Configuration

```
TASK: MWA-01-D
REQUIREMENT: NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a .clang-format file at the project root that enforces Google C++ Style
  with the following project-specific overrides:

  - BasedOnStyle: Google
  - IndentWidth: 2
  - ColumnLimit: 80
  - Language: Cpp
  - Standard: c++20
  - SortIncludes: CaseSensitive
  - IncludeBlocks: Regroup (with IncludeCategories matching the project include
    order: related header, C system, C++ stdlib, Qt, third-party, project)
  - DerivePointerAlignment: false
  - PointerAlignment: Left

ACCEPTANCE CRITERIA:
  - .clang-format file exists at project root
  - BasedOnStyle is Google
  - Running "clang-format --dry-run -Werror src/main.cpp" produces zero warnings
  - ColumnLimit is 80
  - IndentWidth is 2
  - Include ordering categories are defined
DEPENDENCIES: MWA-01-A
PRIORITY: MEDIUM
```

---

## MWA-01-E: Clang-Tidy Configuration

```
TASK: MWA-01-E
REQUIREMENT: NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a .clang-tidy file at the project root with checks appropriate for
  C++20/Qt project following Google Style.

  Checks to enable:
  - bugprone-*, cppcoreguidelines-*, modernize-*, performance-*,
    readability-*, google-*, misc-*

  Disable Qt-incompatible checks:
  - cppcoreguidelines-owning-memory
  - modernize-use-trailing-return-type
  - readability-identifier-length
  - cppcoreguidelines-avoid-magic-numbers / readability-magic-numbers

  Configuration:
  - WarningsAsErrors: '' (warn only for now)
  - HeaderFilterRegex: 'src/.*'
  - FormatStyle: file

ACCEPTANCE CRITERIA:
  - .clang-tidy file exists at project root
  - Running "clang-tidy src/main.cpp" produces no errors
  - HeaderFilterRegex filters only project source
  - Qt-incompatible checks are explicitly disabled
DEPENDENCIES: MWA-01-A, MWA-01-D
PRIORITY: MEDIUM
```

---

## MWA-01-F: Test Framework Skeleton

```
TASK: MWA-01-F
REQUIREMENT: NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a testing skeleton using Qt Test framework integrated with CTest.

  Deliverables:
  - tests/CMakeLists.txt: defines test target "tst_smoke" from
    tests/tst_smoke.cpp, links to Qt6::Test and Qt6::Core, registers with add_test()
  - tests/tst_smoke.cpp: minimal Qt Test class with ONE test method that
    performs a real assertion (NOT QVERIFY(true) — use QCOMPARE on a computed value)
  - Doxygen @file header on tst_smoke.cpp

ACCEPTANCE CRITERIA:
  - ctest --preset macos-clang runs and reports 1 test, 1 passed
  - The test performs a real assertion (not QVERIFY(true))
  - tests/CMakeLists.txt uses add_test() and links Qt6::Test
  - Doxygen @file header present on tst_smoke.cpp
DEPENDENCIES: MWA-01-A
PRIORITY: HIGH
```

---

## MWA-01-G: Doxygen Configuration

```
TASK: MWA-01-G
REQUIREMENT: NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a Doxyfile at the project root.

  Configuration:
  - PROJECT_NAME = "Microfluidics Workstation Automation"
  - PROJECT_NUMBER = 0.1.0
  - OUTPUT_DIRECTORY = build/docs
  - INPUT = src/
  - RECURSIVE = YES
  - FILE_PATTERNS = *.h *.cpp
  - EXTRACT_ALL = NO
  - GENERATE_HTML = YES
  - GENERATE_LATEX = NO
  - WARN_IF_UNDOCUMENTED = YES
  - WARN_AS_ERROR = YES
  - USE_MDFILE_AS_MAINPAGE = docs/PROJECT_DESCRIPTION.md
  - JAVADOC_AUTOBRIEF = YES
  - QT_AUTOBRIEF = YES
  - EXCLUDE_PATTERNS = */build/* */tests/* */libs/*
  - SOURCE_BROWSER = YES
  - HAVE_DOT = NO

ACCEPTANCE CRITERIA:
  - Doxyfile exists at project root
  - Running "doxygen Doxyfile" produces HTML output in build/docs/html/
  - Running "doxygen Doxyfile" produces zero warnings
  - WARN_AS_ERROR is YES
  - build/docs/ is in .gitignore
DEPENDENCIES: MWA-01-A
PRIORITY: MEDIUM
```
