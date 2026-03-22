# MWA — Claude Code Project Configuration

## Project

Microfluidics Workstation Automation (MWA) — a cross-platform C++20 / Qt 6 desktop application for controlling microfluidic experiment workstations.

## Roles

This project uses four agents + the human Owner with strict role separation:

- **Owner** (HUMAN) — Final authority. Reviews PRs on GitHub. Confirms or rejects. No code merges without Owner confirmation.
- **Lead** (`.claude/agents/lead.md`) — **Opus** — Reviews, approves internally, assigns. Creates PRs with Handoff Summary for Owner. Never writes production code.
- **SWE** (`.claude/agents/swe.md`) — **Sonnet** — Writes production code with mandatory Doxygen documentation. Never tests or designs.
- **TE** (`.claude/agents/te.md`) — **Sonnet** — Writes tests, finds bugs, verifies Doxygen completeness. Never writes production code.
- **UX** (`.claude/agents/ux.md`) — **Sonnet** — Designs UI. Never writes code.

## Workflow

All development follows `.claude/workflows/development_workflow.md`. Key rules:

1. Every task follows the full workflow sequence — no skipping steps.
2. Lead approves internally, but **Owner (human) is the final gate** via PR review.
3. No cross-role work (SWE doesn't test, TE doesn't code, UX doesn't code).
4. GUI features require UX design approval BEFORE SWE implementation.
5. All code reviewed by Lead before accepted.
6. All code tested by TE after Lead code-review approval.
7. Zero compiler warnings on both macOS and Windows.
8. **Doxygen documentation is mandatory** — all public APIs must have complete Doxygen comments (C++ standard). Code without Doxygen is rejected.
9. **Every PR includes a Handoff Summary** telling the Owner exactly what to test and what to focus on.

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
