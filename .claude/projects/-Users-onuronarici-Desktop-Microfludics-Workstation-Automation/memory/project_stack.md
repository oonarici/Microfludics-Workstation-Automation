---
name: Project Tech Stack
description: MWA uses C++20, CMake, Qt 6 (LGPL), Google C++ Style Guide, Doxygen, cross-platform macOS+Windows
type: project
---

Tech stack decisions (2026-03-22):
- C++20 with CMake and Qt 6 (LGPL-v3). Build system is CMake (not qmake).
- IDE is Qt Creator opening CMake projects natively.
- **Coding standard: Google C++ Style Guide** with Qt adaptations (camelCase methods to match Qt API).
- **Doxygen documentation mandatory** on all public APIs.
- Human Owner is final PR reviewer — agents cannot merge.

**Why:** User needs LGPL-compatible tooling, cross-platform (macOS + Windows), Qt Creator integration, and strict code quality with Google Style + Doxygen.

**How to apply:** All build configuration uses CMakeLists.txt. No .pro files. Qt 6 modules only. Google naming: CamelCase types, camelCase methods, snake_case variables, trailing_underscore_ members, kCamelCase constants. 2-space indent, 80-char lines. Every PR needs Handoff Summary for Owner review.
