# Animation CPU Skinning Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend `mirakana_animation` with a deterministic dependency-free CPU skinned-position deformation path that consumes the existing skin payload and skinning palette contracts.

**Architecture:** Keep the slice inside `mirakana_animation` and `mirakana_math` only. The new API validates bind-position streams, skin influence rows, and palette matrices, then produces skinned position output by applying palette matrices and normalized influence weights in stable vertex order. This is a CPU skinned-position foundation only; normals, tangents, morph targets, GPU palette upload, renderer/RHI integration, glTF skin import, IK, root motion, full 3D joint orientation, and animation graph authoring remain follow-up work.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_math`, existing unit tests, no new third-party dependencies.

---

## Context

- `Animation Skeleton Pose Foundation v0` already provides validated skeletons, rest poses, and local pose sampling.
- `Animation Skin Payload Foundation v0` already provides skin joint entries, per-vertex influence rows, payload diagnostics, influence normalization, model-space matrices, and skinning palette construction.
- The next production gap is connecting those palette matrices to deterministic vertex deformation before renderer/RHI skinning or glTF skin import can be scoped honestly.

## Constraints

- Do not add renderer, RHI, scene, runtime, platform, editor, SDL3, Dear ImGui, glTF importer, or third-party dependencies.
- Do not implement normal/tangent deformation, GPU skinning/upload, renderer integration, cooked mesh schema changes, glTF animation/skin import, IK, root motion, or animation graph authoring in this slice.
- Keep public API in namespace `mirakana::`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_animation` exposes CPU skinning value types, diagnostics, validation, and a skinned-position deformation helper.
- [x] Validation reports deterministic diagnostics for empty bind streams, vertex count mismatches, palette count mismatches, invalid bind positions, invalid palette matrices, invalid influence counts, invalid joint indices, invalid weights, and zero-weight vertices.
- [x] CPU skinning applies palette matrices to bind positions using normalized influence weights, preserves deterministic vertex order, and rejects invalid programmer input.
- [x] Tests cover rest/identity behavior, single-joint transform, multi-joint weighted blending, rotation/scale matrix application, deterministic replay, diagnostics, and throwing invalid paths.
- [x] Docs, roadmap, gap analysis, manifest, and Codex/Claude guidance describe this as CPU skinned-position deformation, not full CPU mesh deformation, normal/tangent skinning, GPU skinning, glTF import, IK, root motion, full 3D joint orientation, or graph authoring.
- [x] Focused animation tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED CPU Skinning Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Inspect: `engine/animation/include/mirakana/animation/skin.hpp`
- Inspect: `engine/animation/src/skin.cpp`

- [x] Add tests for deterministic skinned-position deformation from a skeleton-derived palette.
- [x] Add tests for rotation/scale matrix application through a direct palette.
- [x] Add tests for diagnostics: empty bind stream, vertex count mismatch, palette count mismatch, invalid bind position, invalid palette matrix, invalid influence count, invalid joint index, invalid/non-finite/overflowed weights, and zero-weight vertices.
- [x] Add tests that invalid CPU skinning input throws before deformation.
- [x] Run `cmake --build --preset dev --target mirakana_core_tests` and verify the tests fail because the CPU skinning API is missing.

### Task 2: CPU Skinning Contracts

**Files:**
- Modify: `engine/animation/include/mirakana/animation/skin.hpp`
- Modify: `engine/animation/src/skin.cpp`

- [x] Add `AnimationCpuSkinningDesc`, `AnimationCpuSkinnedVertexStream`, `AnimationCpuSkinningDiagnosticCode`, and `AnimationCpuSkinningDiagnostic`.
- [x] Add declarations and definitions for `validate_animation_cpu_skinning_desc`, `is_valid_animation_cpu_skinning_desc`, and `skin_animation_vertices_cpu`.
- [x] Implement finite-input validation, stable diagnostic ordering, palette count checks, influence count checks, joint-index checks, weight-sum validation, and invalid-input throwing.
- [x] Implement skinned-position output by applying each influence palette matrix to the bind position and accumulating by normalized influence weights.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/architecture.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Mark CPU skinned-position deformation honestly as implemented only after tests and validation pass.
- [x] Keep normal/tangent skinning, GPU skinning/upload, renderer/RHI integration, glTF animation/skin import, IK, root motion, full 3D joint orientation, and graph authoring as follow-up work.

### Task 4: Verification

- [x] Run focused animation tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED build: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` failed before implementation because `AnimationCpuSkinningDesc`, `validate_animation_cpu_skinning_desc`, `is_valid_animation_cpu_skinning_desc`, `skin_animation_vertices_cpu`, and `AnimationCpuSkinningDiagnosticCode` were not defined.
- Focused animation validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_core_tests"` passed after implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: initially failed after adding stricter animation guidance checks because `docs/roadmap.md` did not contain the exact API name; the static check was corrected to distinguish capability docs from API guidance, and retry PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS, diagnostic-only blocker for strict analysis because `out/build/dev/compile_commands.json` is missing for the Visual Studio generator.
- `cpp-reviewer`: no C++ correctness, ownership/lifetime, or API-boundary blockers. Low finding about incomplete plan validation evidence was addressed here.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Diagnostic-only host gates remain Metal `metal` / `metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database missing.
