# Animation Skeleton Pose Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first deterministic skeleton, joint pose, and sampled pose foundation in `mirakana_animation` so future skeletal animation, CPU/GPU skinning payloads, glTF animation import, IK, root motion, and animation graph authoring can build on a clean first-party contract.

**Architecture:** Keep skeleton and pose data in `mirakana_animation` as standard-library/C++23 value types. Do not add renderer, RHI, editor, platform, asset importer, or third-party dependencies. This slice defines validation, bind/rest pose data, local pose sampling helpers, and deterministic diagnostics only; skinning, IK, glTF import, GPU upload, and editor graph authoring remain follow-up slices.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_math`, existing core/unit tests, no new third-party dependencies.

---

## Context

- `mirakana_animation` already validates and samples scalar/Vec3 keyframes, state machines, blend trees, layered samples, timeline events, and retargeting policies.
- Production animation is still missing skeleton hierarchy, bind/rest poses, sampled joint transforms, skin payloads, root motion, IK, and authored animation asset workflows.
- A small skeleton/pose contract is host-feasible on the current Windows environment and has low dependency risk.

## Constraints

- Do not add new third-party dependencies.
- Do not add renderer, RHI, scene, editor, platform, or asset-tool dependencies to `mirakana_animation`.
- Do not implement skinning, IK, glTF import, animation compression, or graph authoring in this slice.
- Keep public API under `mirakana::`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_animation` exposes deterministic skeleton/joint/rest-pose data structures and validation diagnostics.
- [x] Pose sampling helpers can build a local joint pose from translation/scale `Vec3` tracks and rotation scalar tracks without scene, renderer, or RHI dependencies.
- [x] Tests cover valid skeleton hierarchy order, invalid parent indices, duplicate/empty joint names, mismatched pose counts, clamped sampling, and deterministic output order.
- [x] Docs, roadmap, gap analysis, manifest, and Codex/Claude guidance describe this as Animation Skeleton Pose Foundation v0, not skinning, IK, glTF animation import, or animation graph authoring.
- [x] Focused animation tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Animation Skeleton/Pose Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Inspect: `engine/animation/include/mirakana/animation/keyframe_animation.hpp`
- Inspect: `engine/animation/include/mirakana/animation/state_machine.hpp`

- [x] Add tests for valid and invalid skeleton descriptions.
- [x] Add tests for sampled local pose output from deterministic joint track bindings.
- [x] Add tests that reject mismatched pose counts and unsafe joint names.

### Task 2: Skeleton And Pose Contracts

**Files:**
- Add or modify under `engine/animation/include/mirakana/animation/`
- Add or modify under `engine/animation/src/`
- Modify: `engine/animation/CMakeLists.txt` only if a new translation unit is added.

- [x] Add value types for skeleton joints, skeleton descriptions, local joint transforms, joint track bindings, sampled poses, and validation diagnostics.
- [x] Keep transform data independent from `mirakana_scene` while reusing `mirakana_math` value types.
- [x] Provide non-throwing validation plus throwing sampling for invalid programmer input, matching existing `mirakana_animation` style.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: relevant Codex/Claude architecture guidance only if guidance would otherwise drift.

- [x] Mark skeleton/pose foundation honestly as implemented only after tests and validation pass.
- [x] Keep skinning, glTF animation import, IK, root motion, and animation graph authoring as follow-up work.

### Task 4: Verification

- [x] Run focused animation tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED build: `cmake --build --preset dev --target mirakana_core_tests` failed before implementation because `mirakana/animation/skeleton.hpp` did not exist.
- Focused animation validation: `cmake --build --preset dev --target mirakana_core_tests` passed and `ctest --preset dev --output-on-failure -R "mirakana_core_tests"` passed with 1/1 tests after implementation and edge-case fixes.
- Final focused validation after review fixes: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_core_tests"` passed with 1/1 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
- Host/toolchain blockers remain diagnostic-only and unrelated to this slice: Metal `metal` / `metallib` missing, Apple packaging requires macOS with Xcode (`xcodebuild` / `xcrun` missing), Android release signing is not configured, Android device smoke is not connected, and strict clang-tidy is diagnostic-only when the active Visual Studio generator does not provide `compile_commands.json`.
