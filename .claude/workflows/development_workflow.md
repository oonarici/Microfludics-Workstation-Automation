# MWA Development Workflow — Strict Rules

## Roles

| Role | Agent/Human | Creates | Reviews | Never Does |
|------|-------------|---------|---------|------------|
| **Owner** | **HUMAN (You)** | Final PR approval/rejection | PRs on GitHub (code + tests) | — |
| **Lead** | Agent | Task specs, assignments, coordination | Nothing | Write production code, review code |
| **SWE** | Agent | Production code | Nothing | Write tests, design UI |
| **TE** | Agent | Test code, defect reports, PRs (code + tests) | Nothing | Write production code |
| **UX** | Agent | Design specs, layouts | Nothing | Write code, write tests |

### Owner (Human) Authority
- **The Owner is the final authority.** The Owner reviews every PR directly — there is no intermediate review gate.
- Testing happens BEFORE the PR is created. TE creates the PR after all tests pass, including both code and tests.
- The Owner reviews and either **confirms** or **rejects**.
- If the Owner rejects, the agent must fix and resubmit.
- No code merges to `development` without Owner confirmation.

---

## Workflow Sequences

### Sequence A: Standard Feature (non-GUI)

```
Step 1  │ Lead    │ Creates task with requirement IDs, acceptance criteria — then IMMEDIATELY starts Step 2 (no pause)
Step 2  │ SWE     │ Implements the feature, verifies build on both platforms — then IMMEDIATELY starts Step 3 (no pause)
Step 3  │ TE      │ Writes unit tests, runs full test suite
Step 4a │ TE      │ DEFECTS FOUND → SWE fixes (Step 2), TE retests (Step 3)
Step 4b │ TE      │ ALL CLEAR → Pushes branch, waits for CI, creates PR with Handoff Summary
Step 5  │ Owner   │ Reviews PR (code + tests) → CONFIRM (merge) or REJECT (fix and resubmit)
```

> **The only place the workflow pauses and waits for Owner input is Step 5 (PR review).**

### Sequence B: GUI Feature

```
Step 1  │ Lead    │ Creates task with requirement IDs — immediately starts UX design (no pause)
Step 2  │ UX      │ Creates design spec (layout, widgets, states, interactions)
Step 3  │ Owner   │ *** PAUSE — reviews design → APPROVE (continue to Step 4) or REJECT (UX revises, back to Step 2)
Step 4  │ SWE     │ Implements EXACTLY the approved design — immediately hands off to TE (no pause)
Step 5  │ TE      │ Writes tests for all states, interactions, edge cases from design spec
Step 6a │ TE      │ DEFECTS FOUND → SWE fixes (Step 4), TE retests (Step 5)
Step 6b │ TE      │ ALL CLEAR → Pushes branch, waits for CI, creates PR with Handoff Summary
Step 7  │ Owner   │ *** PAUSE — reviews PR (code + tests) → CONFIRM (merge) or REJECT (fix and resubmit)
```

> **GUI workflow pauses exactly twice: design review (Step 3) and PR review (Step 7).**

### Sequence C: Bug Fix

```
Step 1  │ Lead    │ Creates bug fix task from TE defect report, assigns to SWE
Step 2  │ SWE     │ Reproduces the bug, identifies root cause, implements fix
Step 3  │ TE      │ Verifies fix, runs regression tests, adds new test for the bug
Step 4a │ TE      │ DEFECTS FOUND → SWE fixes (Step 2), TE retests (Step 3)
Step 4b │ TE      │ ALL CLEAR → Pushes branch, waits for CI, creates PR with Handoff Summary
Step 5  │ Owner   │ Reviews PR (code + tests) → CONFIRM (merge) or REJECT (fix and resubmit)
```

---

## PR Handoff Summary (MANDATORY — TE → Owner)

Every Pull Request MUST include this section so the Owner knows exactly what to review, test, and focus on. TE creates the PR after all tests pass, including both production code and test code. This is non-negotiable.

### Format
```
## PR Handoff Summary for Owner

### What Changed
- <bullet list of what was added/modified/removed>

### Requirement IDs Addressed
- <list of requirement IDs, e.g., GUI-REQ-001, HW-REQ-003>

### What to Test (Owner Manual Testing)
- <specific actions the Owner should perform to verify the feature>
- <step-by-step instructions, not vague "test the feature">
- Example: "Open the LED panel, set intensity to 50%, click Connect, verify green indicator appears"

### What to Focus On
- <areas of highest risk or complexity>
- <any known limitations or platform-specific behavior>
- <architectural decisions that need Owner's attention>
- Example: "Thread safety of camera frame grabbing — verify no UI freezes during batch capture"

### Doxygen Documentation Check
- <confirm all new/modified public APIs have Doxygen comments>
- <list any classes/methods that were documented>

### Test Results Summary
- Total tests: <N>
- Passed: <N>
- Failed: <N>
- Coverage: <N%>
- Platforms tested: macOS / Windows / Both

### Known Issues / Limitations
- <any open LOW/MEDIUM issues>
- <any platform-specific limitations>
```

> **If this section is missing or incomplete, the Owner should REJECT the PR immediately.**

---

## Coding Standard: Google C++ Style Guide

This project follows the **Google C++ Style Guide** (https://google.github.io/styleguide/cppguide.html) as the base coding standard. Qt-specific adaptations (camelCase methods, parent-child ownership) are documented in the SWE agent definition. All agents must enforce Google Style compliance. See `.claude/agents/swe.md` for the complete naming table, formatting rules, and Qt adaptations.

---

## Doxygen Documentation Standard (MANDATORY)

All code MUST use Doxygen-style comments. This is a hard requirement — code without proper Doxygen documentation is REJECTED. Inline comments (non-Doxygen) follow Google Style: use `//` with a space, placed above or at the end of the line.

### Rules
1. Every file must have a `@file` header block.
2. Every class must have a `@class` block with `@brief` description.
3. Every public and protected method must have a Doxygen comment block.
4. Every parameter documented with `@param`.
5. Every return value documented with `@return`.
6. Exceptions documented with `@throws`.
7. Non-obvious side effects documented with `@note`.
8. Deprecated items marked with `@deprecated`.

### File Header Template
```cpp
/**
 * @file led_panel.h
 * @brief LED light source control panel widget.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides the GUI panel for controlling the LED light source,
 * including intensity adjustment and on/off toggling.
 *
 * @copyright LGPL-3.0-or-later
 */
```

### Class Documentation Template
```cpp
/**
 * @class LedPanel
 * @brief Control panel widget for the LED light source.
 *
 * Provides controls for connecting to an LED driver,
 * adjusting intensity, and toggling the light on/off.
 * Emits signals when the user changes settings.
 *
 * @note This widget does not communicate with hardware directly.
 *       It emits signals that the hardware layer connects to.
 *
 * @see DeviceInterface
 * @see LedDriver
 */
```

### Method Documentation Template
```cpp
/**
 * @brief Set the LED intensity level.
 *
 * Adjusts the LED brightness. The value is clamped to the
 * valid range [0.0, 100.0]. Emits intensityChanged() signal.
 *
 * @param intensity Intensity percentage (0.0 = off, 100.0 = full brightness).
 * @return true if the intensity was set successfully, false if the device is disconnected.
 * @throws std::out_of_range if intensity is negative.
 *
 * @note Does not take effect until the device is connected.
 * @see intensityChanged()
 */
bool setIntensity(double intensity);
```

### Enum Documentation Template
```cpp
/**
 * @enum DeviceState
 * @brief Represents the connection state of a hardware device.
 */
enum class DeviceState {
  kDisconnected,  ///< Device is not connected.
  kConnecting,    ///< Connection attempt in progress.
  kConnected,     ///< Device is connected and ready.
  kError          ///< Device encountered an error.
};
```

### What Does NOT Need Doxygen
- Private helper methods with obvious purpose (e.g., `updateUi()`)
- Overridden Qt methods (e.g., `paintEvent`, `resizeEvent`) — unless behavior is non-standard
- Test code (TE's test files)
- Getters/setters that are trivial — but still require `@brief` one-liner

---

## Strict Rules — Violations Are Grounds for Rejection

### Rule 1: No Skipping Steps
Every task follows its sequence completely. No step may be skipped, shortened, or combined.

### Rule 2: No Self-Approval
- SWE cannot approve their own code.
- TE cannot approve their own test results.
- UX cannot approve their own designs.
- Only the Owner approves via PR review.

### Rule 3: No Cross-Role Work
- SWE does not write tests. TE does not write production code. UX does not write any code.
- If an agent produces an artifact outside their role, the Owner REJECTS it immediately.

### Rule 4: No Scope Creep
- SWE implements only what is in the task specification. No "while I'm here" improvements.
- TE tests only what is in the test plan (plus edge cases). No production code changes.
- UX designs only for the assigned requirement IDs. No speculative features.

### Rule 5: No Unreviewed Code in Codebase
- Every line of code (production and test) must pass Owner review before being considered part of the project.
- Work-in-progress code is clearly marked and never mixed with approved code.

### Rule 6: Build Must Pass
- SWE must verify the build succeeds with zero warnings before submitting.
- TE must verify all tests compile and run before submitting.
- If the build is broken, that is the HIGHEST priority — everything stops until it's fixed.

### Rule 7: Tests Must Be Adversarial
- TE writes tests designed to FAIL, not to PASS.
- Every test must be able to detect a real bug — no tautological assertions (`QVERIFY(true)`).
- If TE's test suite has 100% pass rate on first run, Owner should question whether the tests are tough enough.

### Rule 8: Design Before Code (GUI)
- No GUI code is written without an approved UX design.
- SWE implements the approved design pixel-for-pixel (within layout manager constraints).
- If SWE discovers a design issue during implementation, they stop and report — they do not "fix" the design themselves.

### Rule 9: Defects Block Approval
- A task with ANY open HIGH or CRITICAL defect cannot be approved.
- MEDIUM defects must have a tracked fix plan.
- LOW defects are documented but do not block.

### Rule 10: Cross-Platform Is Not Optional
- Every feature must work on both macOS and Windows.
- "Works on my machine" is not acceptance criteria.
- Platform-specific behavior must be documented and tested on both platforms.

### Rule 11: Doxygen Comments Are Mandatory
- Every public/protected class, method, enum, and file MUST have Doxygen documentation.
- Code without Doxygen comments is REJECTED — no exceptions.
- Owner's review includes a Doxygen completeness check.
- See "Doxygen Documentation Standard" section above for exact format.

### Rule 12: Owner PR Review Is the Only Gate
- There is no intermediate Lead review — the Owner (human) reviews all PRs directly.
- SWE and TE create PRs with the mandatory "PR Handoff Summary."
- Only the Owner can confirm (merge) or reject the PR.
- If Owner rejects, the agent fixes and resubmits.
- No exceptions. No "auto-merge." No merging without Owner confirmation.

---

## Task States

```
PLANNED        → Task is defined but not yet assigned
ASSIGNED       → Task is assigned to an agent
IN_PROGRESS    → Agent is actively working on the task
PR_REVIEW      → PR created, Owner is reviewing
REJECTED       → Owner rejected — agent must fix and resubmit
TESTING        → TE is testing the approved implementation
DEFECT_FIX     → SWE is fixing defects found by TE
MERGED         → Owner confirmed — PR merged, task is complete ✓
```

### State Transition Rules
- Only Lead can transition a task to `ASSIGNED`.
- Only the assigned agent can transition to `IN_PROGRESS`.
- Creating a PR transitions to `PR_REVIEW`.
- `REJECTED` always transitions back to `IN_PROGRESS` (same agent fixes).
- Only the Owner (human) can transition to `MERGED` or `REJECTED`.
- `MERGED` is final — no further changes to that task's scope.

---

## Communication Protocol

### Task Assignment (Lead → Agent)
```
TASK: <task ID>
REQUIREMENT: <requirement IDs>
ASSIGNED TO: <agent>
DESCRIPTION: <what to do>
ACCEPTANCE CRITERIA:
  - <criterion 1>
  - <criterion 2>
DEPENDENCIES: <other task IDs or "none">
PRIORITY: HIGH / MEDIUM / LOW
```

### Submission (Agent → Owner via PR)
```
SUBMISSION: <task ID>
AGENT: <agent name>
FILES CHANGED: <list>
REQUIREMENT: <requirement IDs addressed>
STATUS: Ready for review
NOTES: <any relevant context>
```

### Defect Report (TE → Owner via PR)
```
(See TE agent definition for exact format)
```

---

## Git Branch Strategy

| Branch | Purpose | Who merges |
|--------|---------|------------|
| `development` | Integration branch | Owner only |
| `MWA-XX-<description>` | Feature/task branch | SWE creates, Owner merges after approval |
| `test/MWA-XX-<description>` | Test branch (if separate) | TE creates, Owner merges after approval |

- Every task gets its own branch from `development`.
- Branch is merged to `development` only after Owner approval (code + tests).
- No direct commits to `development` — all work goes through branches and review.
