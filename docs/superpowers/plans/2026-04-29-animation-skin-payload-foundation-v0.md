# Animation Skin Payload Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend `mirakana_animation` with deterministic first-party skin payload value types, diagnostics, normalization helpers, model-space pose matrices, and skinning palette construction so gameplay, tools, and future render/import adapters can describe skinned meshes without depending on renderer/RHI, glTF, middleware, or native handles.

**Architecture:** Keep this slice in `mirakana_animation` next to the existing skeleton/local-pose contracts but split skin-specific declarations into `skin.hpp`. The API should remain standard-library/C++23 value data under `mirakana::`, validate against `AnimationSkeletonDesc`, compose model-space matrices from the existing translation/positive-scale/Z-rotation local transforms, and build a palette as `model_joint * inverse_bind`; full CPU mesh deformation, GPU palette upload, glTF skin import, full 3D joint orientation, IK, root motion, and editor authoring remain follow-up work.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_math`, existing unit tests, no new third-party dependencies.

---

## Context

- `mirakana_animation` already owns keyframe sampling, state machines, timeline events, retargeting policy, and Skeleton Pose Foundation v0.
- Current skeleton/local-pose data has translation, positive scale, and Z-rotation-only radians, but no skinned mesh payload model.
- A skin payload contract is host-independent and can be validated on the current Windows host without renderer, RHI, importers, or platform SDKs.

## Constraints

- Do not add renderer, RHI, scene, runtime, platform, editor, SDL3, Dear ImGui, glTF importer, or third-party dependencies.
- Do not implement full CPU mesh deformation, GPU skinning, renderer/RHI upload, glTF skin import, IK, root motion, or animation graph authoring in this slice.
- Keep public API in namespace `mirakana::`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_animation` exposes skin payload value types for skeleton-bound joint palette entries and per-vertex joint influences.
- [x] Validation reports deterministic diagnostics for empty payloads, skeleton mismatch, out-of-range joints, duplicate palette joints, vertex count mismatch, invalid influence counts, invalid joint references, non-finite or negative weights, and zero-weight vertices.
- [x] A normalization helper produces deterministic per-vertex influence weights while preserving joint references and rejecting invalid payloads.
- [x] Model-pose and skinning-palette helpers compose parent-before-child local transforms and inverse-bind matrices deterministically.
- [x] Tests cover valid payloads, diagnostics, normalization, replay determinism, and throwing programmer-input paths.
- [x] Docs, roadmap, gap analysis, manifest, and Codex/Claude guidance describe this as Animation Skin Payload Foundation v0, not full CPU mesh deformation, GPU skinning, glTF import, IK, root motion, full 3D joint orientation, or graph authoring.
- [x] Focused animation tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Skin Payload Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Inspect: `engine/animation/include/mirakana/animation/skeleton.hpp`
- Inspect: `engine/animation/src/skeleton.cpp`

- [x] Add tests for a valid skeleton-bound skin payload with two vertices, deterministic palette entries, normalized weights, model-space matrices, skinning palette matrices, and replay-stable output.
- [x] Add tests for diagnostics: empty payload, skeleton joint count mismatch, duplicate palette joint, out-of-range palette joint, vertex count mismatch, empty vertex influences, too many vertex influences, out-of-range palette slot, non-finite/negative weights, and zero-weight vertices.
- [x] Add tests for throwing invalid normalization calls.
- [x] Verify the new tests fail for missing APIs before implementation.

### Task 2: Skin Payload Contracts

**Files:**
- Create: `engine/animation/include/mirakana/animation/skin.hpp`
- Create: `engine/animation/src/skin.cpp`
- Modify: `engine/animation/CMakeLists.txt`

- [x] Add `AnimationSkinJointDesc`, `AnimationSkinVertexInfluence`, `AnimationSkinVertexWeights`, and `AnimationSkinPayloadDesc`.
- [x] Add skin diagnostics through `AnimationSkinDiagnosticCode` and `AnimationSkinDiagnostic` for deterministic reporting.
- [x] Add declarations and definitions for `validate_animation_skin_payload`, `is_valid_animation_skin_payload`, `normalize_animation_skin_payload`, `build_animation_model_pose`, and `build_animation_skinning_palette`.
- [x] Keep validation deterministic and allocation-safe where existing `is_valid_*` helpers are `noexcept`.

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

- [x] Mark skin payload contracts honestly as implemented only after tests and validation pass.
- [x] Keep full CPU mesh deformation, GPU skinning, renderer/RHI upload, glTF skin import, full 3D joint orientation, IK, root motion, and graph authoring as follow-up work.

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

- RED test evidence: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` failed before implementation because `mirakana/animation/skin.hpp` did not exist.
- Focused animation validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_core_tests"` passed after implementation and reviewer fixes.
- cpp-reviewer findings addressed: inverse-bind matrices are validated against skeleton rest bind pose, model-space composition tests cover rotation and scale order, diagnostics include `influence_index`, and finite overflowed weight sums report invalid weight instead of zero-weight.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: initially failed after code edits; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format; retry PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS, strict analysis diagnostic-only because `out/build/dev/compile_commands.json` is missing for the Visual Studio generator.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Diagnostic-only host gates remain Metal `metal` / `metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database missing.
