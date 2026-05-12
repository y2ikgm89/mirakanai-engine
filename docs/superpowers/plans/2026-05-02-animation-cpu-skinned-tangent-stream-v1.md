# Animation CPU Skinned Tangent Stream v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the existing `mirakana_animation` CPU skinning contract so it can deterministically skin an optional bind-tangent stream alongside bind positions and optional bind normals without adding renderer/RHI execution or broad skeletal-animation readiness.

**Architecture:** Keep the slice inside `mirakana_animation` and the existing `mirakana_math::transform_direction` helper. Reuse the `AnimationCpuSkinningDesc` validation and palette-weight accumulation path, add optional tangent-stream validation, and transform tangents with matrix direction terms only. Tangents are represented as `Vec3` directions in this slice; handedness, bitangent reconstruction, and material/render integration remain separate work.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_math`, existing `mirakana_core_tests`, docs/manifest synchronization, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Next-Production-Gap Selection

- Latest completed slice: `docs/superpowers/plans/2026-05-02-animation-cpu-skinned-normal-stream-v1.md`.
- That slice records focused RED -> GREEN evidence, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 28/28 CTest entries passed.
- Host gates remain explicit: Apple/iOS/Metal requires macOS/Xcode/metal/metallib, strict clang-tidy remains diagnostic-only without a compile database for the active Visual Studio generator, and broad Vulkan claims stay strict host/runtime/toolchain-gated.
- Manifest/registry still block `recommendedNextPlan` as `next-production-gap-selection`; broad gaps are `3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters`.
- Selected focused slice: from `3d-playable-vertical-slice`, implement only CPU skinned tangent stream output in the already dependency-free `mirakana_animation` CPU skinning path. This is Windows-verifiable through normal CTest and does not claim bitangent/handedness handling, morph deformation, skeletal animation production readiness, GPU skinning/upload, renderer execution, glTF skin import, or generated visible 3D game readiness.

## Goal

Make `skin_animation_vertices_cpu` preserve deterministic vertex order while optionally emitting skinned tangents for callers that supply bind tangents.

## Context

- `Animation CPU Skinning Foundation v0` emitted positions only.
- `Animation CPU Skinned Normal Stream v1` added optional normal streams and the direction-transform helper needed by tangent vectors.
- The remaining tangent follow-up can be cut smaller than morph deformation or renderer integration because it uses the same optional stream validation and direction-vector accumulation path.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset formats, editor, SDL3, Dear ImGui, and native handles.
- Do not add dependencies or touch renderer/RHI, runtime scene upload, platform, editor, SDL3, Dear ImGui, shader/material graph, importer, cooked mesh schema, generated-game scaffold behavior, or package scripts.
- Do not claim handedness/bitangent reconstruction, morph deformation, GPU skinning/upload, skeletal animation production readiness, glTF animation/skin import, renderer execution, package streaming, Metal readiness, public native/RHI handles, editor productization, production UI/importer/platform adapters, or broad generated 3D production readiness.
- Public API changes must stay in namespace `mirakana::`, follow `docs/cpp-style.md`, and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] A RED test is added before implementation and fails because `AnimationCpuSkinningDesc` has no bind-tangent stream and `AnimationCpuSkinnedVertexStream` has no tangent output.
- [x] `AnimationCpuSkinningDesc` accepts optional `bind_tangents`; empty means existing positions-only and positions-plus-normals behavior remains valid.
- [x] Validation rejects non-empty tangent streams whose count does not match `bind_positions`, and rejects non-finite bind tangents with deterministic diagnostics.
- [x] `skin_animation_vertices_cpu` emits normalized skinned tangents when bind tangents are supplied, preserves deterministic vertex order, and keeps existing position/normal output unchanged.
- [x] Focused tests cover weighted tangent blending, translation ignoring, replay determinism, tangent count mismatch, invalid tangents, invalid-input throwing, and coexistence with optional normals.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.json`, and relevant static guidance describe this as CPU skinned-tangent stream support only.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has PASS evidence or a concrete environment blocker recorded here.
- [x] Registry and manifest are re-read after completion to select the next focused slice or stop on host-gated/broad-only work.

## Implementation Tasks

- [x] Add RED assertions to `tests/unit/core_tests.cpp` near the existing animation CPU skinning tests:
  - construct a palette that translates one joint and rotates another,
  - set `bind_tangents` on `AnimationCpuSkinningDesc`,
  - assert skinned tangents are emitted, normalized, deterministic, and unaffected by translation,
  - assert tangents can be emitted alongside normals,
  - assert invalid tangent count and non-finite tangent diagnostics,
  - assert invalid tangent input throws before deformation.
- [x] Run a focused `mirakana_core_tests` build/test and record the expected RED failure.
- [x] Add `bind_tangents` to `AnimationCpuSkinningDesc`, `tangents` to `AnimationCpuSkinnedVertexStream`, and `tangent_count_mismatch` / `invalid_bind_tangent` diagnostics.
- [x] Update validation and no-alloc validity checks to accept empty tangent streams and reject malformed non-empty streams.
- [x] Update `skin_animation_vertices_cpu` to emit skinned tangents only when bind tangents are present.
- [x] Run focused tests, then update docs/registry/manifest/static guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-step decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| Focused `mirakana_core_tests` after RED test | RED | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` failed to compile as expected because `AnimationCpuSkinningDesc::bind_tangents`, `AnimationCpuSkinnedVertexStream::tangents`, `tangent_count_mismatch`, and `invalid_bind_tangent` did not exist yet. |
| Focused `mirakana_core_tests` after implementation | PASS | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` passed, then wrapper-normalized `ctest --preset dev --output-on-failure -R "mirakana_core_tests"` reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Initial run caught that `docs/ai-game-development.md` no longer contained the static-check phrase `Animation CPU Skinning`; after restoring the guidance phrase with the new tangent support, `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| Touched-file clang-format dry-run | PASS | `clang-format --dry-run --Werror` passed for `engine/animation/include/mirakana/animation/skin.hpp`, `engine/animation/src/skin.cpp`, `engine/math/include/mirakana/math/mat4.hpp`, and `tests/unit/core_tests.cpp`. Repository-wide `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` still reports pre-existing unrelated files outside this slice. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 28/28 CTest entries passed. Diagnostic-only gates remain explicit: Metal shader tools missing on this Windows host, Apple packaging blocked by no macOS/Xcode, and strict clang-tidy blocked by missing `compile_commands.json` for the active Visual Studio generator. |

## Next-Step Decision

After completion, `docs/superpowers/plans/README.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `engine/agent/manifest.json` were re-read. The manifest still routes through blocked `next-production-gap-selection`; no further Windows-host-verifiable focused production slice is selected now. Remaining work is either broad and needs a new design/plan before implementation (`bitangent/handedness reconstruction`, `morph deformation`, `GPU skinning/upload`, `glTF animation/skin import`, editor productization, production UI/importer/platform adapters, broader generated 3D production readiness) or host/toolchain-gated (`Apple/iOS/Metal` on macOS/Xcode/metal/metallib, strict clang-tidy enforcement with a generator that emits `compile_commands.json`).

## Non-Goals

- Bitangent reconstruction, tangent handedness/sign preservation, morph-target deformation, IK, root-motion, animation graph, glTF animation/skin import, or authored animation asset workflow support.
- GPU skinning/upload, renderer/RHI integration, native backend resource ownership changes, or public native/RHI handle exposure.
- Cooked mesh schema changes, runtime package mutation, broad package cooking, package streaming, or runtime source parsing.
- Generated 3D production readiness, renderer execution, renderer quality, material/shader graphs, live shader generation, Metal readiness, editor productization, or production UI/importer/platform adapters.
