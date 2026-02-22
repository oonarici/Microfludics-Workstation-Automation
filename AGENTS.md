# AGENTS.md

## Goal
You are a coding agent working in this repository. Prefer small, verifiable changes.

## Setup
- Build: `cmake --preset <...>` then `cmake --build --preset <...>`
- Test: `ctest --preset <...> --output-on-failure`

## Conventions
- Test Driven Development
- Test First!
- Language: C++20
- Formatting: clang-format
- Keep diffs minimal; no unrelated refactors.

## Definition of Done
- After every changes builds cleanly
- After every build check tests pass
- If you change behavior, add/adjust a test
