# MWA — Claude Code Project Configuration

## Project

Microfluidics Workstation Automation (MWA) — a cross-platform C++20 / Qt 6 desktop application for controlling microfluidic experiment workstations.

## Roles

This project uses four agents + the human Owner with strict role separation:

- **Owner** (HUMAN) — Final authority. Reviews all PRs on GitHub directly. Confirms or rejects. No code merges without Owner confirmation.
- **Lead** (`.claude/agents/lead.md`) — **Opus** — Coordinates tasks, assigns work, owns architecture decisions. Never writes code or reviews PRs.
- **SWE** (`.claude/agents/swe.md`) — **Sonnet** — Writes production code with mandatory Doxygen documentation. Never tests or designs.
- **TE** (`.claude/agents/te.md`) — **Sonnet** — Writes tests, finds bugs, verifies Doxygen completeness. Creates PRs (code + tests) for Owner review after all tests pass. Never writes production code.
- **UX** (`.claude/agents/ux.md`) — **Sonnet** — Designs UI. Never writes code.

## Workflow

All development follows `.claude/workflows/development_workflow.md`. Key rules:

1. **Lead assigns, describes, and immediately starts tasks** — the full chain Lead → SWE → TE → PR runs without pausing for "go on" at each step.
2. **Owner (human) is the only review gate** — reviews the final PR and merges if everything is okay. No intermediate approvals except UX design sign-off for GUI features.
3. No cross-role work (SWE doesn't test, TE doesn't code, UX doesn't code).
4. GUI features require UX design approval BEFORE SWE implementation (only intermediate pause — before coding starts).
5. **TE writes unit tests, runs them, and opens the PR** — testing and PR creation are one continuous step after SWE finishes.
6. Zero compiler warnings on both macOS and Windows.
7. **Doxygen documentation is mandatory** — all public APIs must have complete Doxygen comments (C++ standard). Code without Doxygen is rejected.
8. **Every PR includes a Handoff Summary** telling the Owner exactly what to test and what to focus on.

## Build

- **Language:** C++20
- **Build System:** CMake ≥ 3.21
- **Framework:** Qt 6.5+ (LGPL-v3)
- **Platforms:** macOS (ARM64/x86_64), Windows (x86_64)
- **IDE:** Qt Creator (opens CMakeLists.txt natively)

## Code Conventions

**Base Standard: [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)** with Qt-specific adaptations.

- **Naming (Google + Qt adaptation)**:
  - Types/Classes: `CamelCase` — `LedPanel`, `DeviceInterface`
  - Methods: `camelCase` — `setFlowRate()` (Qt adaptation, not Google's CamelCase)
  - Local variables: `snake_case` — `exposure_time`, `frame_count`
  - Members: `trailing_underscore_` — `serial_port_`, `is_connected_`
  - Constants: `kCamelCase` — `kMaxFlowRate`, `kDefaultTimeout`
  - Enum values: `kCamelCase` — `DeviceState::kConnected`
  - Files: `snake_case` — `led_panel.h`, `device_interface.cpp`
  - Namespaces: `snake_case` — `mwa::gui`, `mwa::hardware`
- **Formatting**: 2-space indent, 80-char line limit, opening brace on same line
- Qt new-style signal/slot connections only
- No raw new/delete — smart pointers or Qt parent ownership
- `#pragma once` for header guards
- `explicit` on single-argument constructors, `override` on all virtual overrides
- C++-style casts only — no C-style casts
- **Doxygen comments mandatory** — `@file` on every file, `@class` on every class, `@brief`/`@param`/`@return` on every public method

## Documentation

- `docs/PROJECT_DESCRIPTION.md` — Full project description
- `docs/requirements.json` — All requirements with IDs
- `.claude/workflows/development_workflow.md` — Full workflow with PR Handoff Summary format
