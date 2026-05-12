# Physics Raycast Foundation Implementation Plan (2026-04-27)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add deterministic collision-bounds raycasts to the first-party 2D and 3D physics worlds.

**Architecture:** Extend `mirakana_physics` public contracts with raycast descriptor and hit value types for 2D and 3D. The first slice casts against collision bounds for all bodies, respects collision enablement and ray collision masks, and returns the nearest deterministic hit.

**Tech Stack:** C++23, `mirakana_math`, CTest, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

### Task 1: Red Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] Add 2D tests proving nearest bounds hit distance, point, normal, mask filtering, and disabled-body filtering.
- [x] Add 3D tests proving nearest bounds hit distance, point, and normal.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Expected: build fails because `PhysicsRaycast2DDesc`, `PhysicsRaycast3DDesc`, and `raycast` are not implemented.

### Task 2: Public API And Implementation

**Files:**
- Modify: `engine/physics/include/mirakana/physics/physics2d.hpp`
- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`
- Modify: `engine/physics/src/physics2d.cpp`
- Modify: `engine/physics/src/physics3d.cpp`

- [x] Add raycast descriptor and hit value types.
- [x] Add `PhysicsWorld2D::raycast` and `PhysicsWorld3D::raycast`.
- [x] Implement normalized finite rays, bounds slab intersection, nearest-hit selection, disabled-body filtering, and ray collision mask filtering.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.

### Task 3: Guidance And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`

- [x] Record collision-bounds raycasts as implemented.
- [x] Keep sweeps, triggers, joints, character controllers, and authored collision assets listed as missing.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
