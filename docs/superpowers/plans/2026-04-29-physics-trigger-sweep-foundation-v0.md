# Physics Trigger/Sweep Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend `mirakana_physics` with deterministic first-party trigger overlap reporting and shape sweep queries so gameplay code can ask safer movement/interaction questions without adding middleware, renderer, editor, platform, or native API dependencies.

**Architecture:** Keep trigger and sweep behavior in `mirakana_physics` as value-type C++23 APIs under `mirakana::`. Reuse existing 2D/3D body, shape, layer/mask, broadphase, contact, and raycast concepts. This slice adds deterministic query/event contracts only; joints, character controllers, CCD, authored collision assets, exact mesh collision, and optional Jolt integration remain follow-up work.

**Tech Stack:** C++23, `mirakana_physics`, `mirakana_math`, existing unit tests, no new third-party dependencies.

---

## Context

- `mirakana_physics` already has deterministic 2D/3D body integration, sweep-and-prune broadphase pairs, 2D AABB/circle contacts, 3D AABB/sphere/capsule contacts, layer/mask filtering, collision-bounds raycasts, and iterative contact solver coverage.
- Production gameplay still needs non-resolving trigger overlaps and shape sweeps before character-controller and authored-collision slices can be scoped cleanly.
- The current Windows host can validate this slice through normal CMake/CTest and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Constraints

- Do not add third-party dependencies or optional middleware.
- Do not add renderer, RHI, scene, editor, platform, SDL3, or native-handle dependencies to `mirakana_physics`.
- Do not implement joints, character controller, CCD, authored collision assets, mesh collision, or Jolt integration in this slice.
- Keep public API under `mirakana::`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_physics` exposes deterministic 2D and 3D trigger overlap result APIs that respect existing enabled/body/layer/mask semantics without applying contact resolution.
- [x] `mirakana_physics` exposes deterministic 2D and 3D shape sweep query APIs with nearest-hit selection, query masks, initial-overlap handling, stable ordering, and finite-input validation.
- [x] Tests cover trigger pair filtering, disabled/static/dynamic body behavior where applicable, sweep hit/miss/nearest ordering, initial overlap, invalid input, and deterministic replay.
- [x] Docs, roadmap, gap analysis, manifest, and Codex/Claude guidance describe this as Physics Trigger/Sweep Foundation v0, not joints, character controllers, CCD, authored collision assets, or middleware.
- [x] Focused physics tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Physics Trigger/Sweep Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp` or the existing physics test file if one exists.
- Inspect: `engine/physics/include/mirakana/physics/physics2d.hpp`
- Inspect: `engine/physics/include/mirakana/physics/physics3d.hpp`
- Inspect: `engine/physics/src/`

- [x] Add tests for deterministic trigger overlap results in 2D and 3D.
- [x] Add tests for 2D/3D shape sweep hit, miss, nearest-hit ordering, and initial-overlap behavior.
- [x] Add tests that layer/mask filtering and invalid query inputs are reported deterministically.
- [x] Verify the new tests fail for missing APIs before implementation.

### Task 2: Trigger Query Contracts

**Files:**
- Modify under `engine/physics/include/mirakana/physics/`
- Modify under `engine/physics/src/`

- [x] Add value-type trigger overlap descriptors/results that expose only body ids, shape/query metadata, and deterministic diagnostics.
- [x] Reuse existing broadphase/contact helpers where practical without applying solver impulses.
- [x] Preserve stable sorting by body ids/query order so replay output is deterministic.

### Task 3: Shape Sweep Query Contracts

**Files:**
- Modify under `engine/physics/include/mirakana/physics/`
- Modify under `engine/physics/src/`

- [x] Add value-type 2D/3D sweep descriptors/results for supported primitive shapes.
- [x] Implement finite-input validation, positive distance/extent checks, mask filtering, initial-overlap reporting, and nearest-hit selection.
- [x] Keep exact shape support conservative and document any bounds-only limitations honestly.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/architecture.md` if subsystem capability text changes.
- Modify: `docs/testing.md` if test coverage text changes.
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: relevant Codex/Claude subagents only if guidance would otherwise drift.

- [x] Mark triggers/sweeps honestly as implemented only after tests and validation pass.
- [x] Keep joints, character controllers, CCD, authored collision assets, and Jolt integration as follow-up work.

### Task 5: Verification

- [x] Run focused physics tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED build: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` failed before implementation because `PhysicsBody2DDesc` / `PhysicsBody3DDesc` had no trigger flag and `PhysicsWorld2D` / `PhysicsWorld3D` had no `trigger_overlaps` or `shape_sweep` API.
- Focused physics validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_core_tests"` passed with 1/1 tests after implementation.
- Focused physics validation after cpp-reviewer fixes: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_core_tests"` passed with 1/1 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: PASS for tidy config, diagnostic-only blocker for strict analysis because `compile_commands.json` is missing for the active Visual Studio generator.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: initially failed after review fixes; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` was applied; retry PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
- Host/toolchain blockers remain diagnostic-only and unrelated to this slice: Metal `metal` / `metallib` missing, Apple packaging requires macOS with Xcode (`xcodebuild` / `xcrun` missing), Android release signing is not configured, Android device smoke is not connected, and strict clang-tidy is diagnostic-only when the active Visual Studio generator does not provide `compile_commands.json`.
