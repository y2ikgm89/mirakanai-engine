# Generated 3D Committed Package Sample v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a committed `DesktopRuntime3DPackage` generated sample and validation evidence so the 3D playable gap advances from scaffold-only telemetry toward a concrete AI-created visible 3D package proof.

**Architecture:** Reuse the existing `tools/new-game.ps1 -Template DesktopRuntime3DPackage` output without creating a second hand-written 3D sample path. The committed sample stays on public `mirakana::` gameplay APIs, ships only cooked runtime package files, uses manifest-driven CMake `PACKAGE_FILES_FROM_MANIFEST`, and validates through source-tree smoke plus selected installed desktop-runtime package smoke.

**Tech Stack:** PowerShell scaffold tooling, C++23 generated game code, `mirakana_runtime`, `mirakana_runtime_scene`, `mirakana_scene_renderer`, `mirakana_animation`, SDL3 desktop runtime packaging, manifest/static checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Context

- `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps` still lists `3d-playable-vertical-slice` as `planned-plus-scene-mesh-package-telemetry` with `AI-created visible 3D game` as the remaining before-ready claim.
- The generator already emits a rich `DesktopRuntime3DPackage` with cooked texture/mesh/skinned mesh/morph/material/animation/scene payloads, camera/controller smoke, transform/quaternion animation smoke, D3D12 compute morph package smoke metadata, Vulkan POSITION compute morph metadata, source Scene/Prefab v2 descriptors, and validation recipes.
- The repository currently validates the generator through temporary static scaffold checks, but it does not keep a committed generated 3D package sample in `games/` for normal CMake, package, and operator workflows.

## Constraints

- Do not add third-party assets or dependencies.
- Do not hand-edit generated gameplay behavior beyond necessary repository integration.
- Do not ship source authoring files in `runtimePackageFiles`.
- Do not claim broad generated 3D production readiness, broad dependency cooking, runtime source parsing, package streaming, material/shader graph production, live shader generation, skeletal animation production, async compute performance, Vulkan NORMAL/TANGENT or skin+compute package parity, Metal readiness, editor productization, public native/RHI handles, or general renderer quality.
- Keep `future-3d-playable-vertical-slice` planned until a separate closeout accepts the broader 3D production surface.

## Done When

- [x] RED static/schema checks fail first because the committed generated 3D sample is missing and the 3D gap status still reflects only scaffold telemetry.
- [x] `games/sample_generated_desktop_runtime_3d_package/` exists as output from `DesktopRuntime3DPackage` and is registered through `mirakana_add_desktop_runtime_game`.
- [x] The committed sample manifest selects `3d-playable-desktop-package`, lists cooked-only runtime package files, exposes runtime scene validation, package streaming residency, prefab scene authoring, registered source cook, and validation recipe descriptors.
- [x] Source-tree desktop runtime smoke covers `--require-primary-camera-controller`, `--require-transform-animation`, `--require-morph-package`, and `--require-quaternion-animation`.
- [x] Installed package validation covers the selected D3D12 package smoke including compute morph, skin+compute, async telemetry, scene GPU bindings, and postprocess when the Windows/DXC host is ready.
- [x] `unsupportedProductionGaps.3d-playable-vertical-slice` is updated to a non-ready but more accurate committed generated-package proof status with remaining broad-production requirements.
- [x] Docs, manifest, plan registry, static checks, and production readiness audit vocabulary agree.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes, or a concrete local host/tool blocker is recorded here.

## Implementation Tasks

### Task 1: RED Checks

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/check-production-readiness-audit.ps1`

- [x] Add static checks for `games/sample_generated_desktop_runtime_3d_package/game.agent.json`, `main.cpp`, runtime payloads, shader sources, and `games/CMakeLists.txt` registration.
- [x] Add schema/static status checks expecting `3d-playable-vertical-slice` to advance to `implemented-generated-desktop-3d-package-proof`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` as appropriate and record the expected RED failures.

### Task 2: Commit The Generated Sample

**Files:**
- Create: `games/sample_generated_desktop_runtime_3d_package/**`
- Modify: `games/CMakeLists.txt`

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name sample_generated_desktop_runtime_3d_package -Template DesktopRuntime3DPackage`.
- [x] Inspect generated files for cooked/source separation, package paths, and CMake metadata.
- [x] Keep generated source on public engine headers and manifest-selected package files.

### Task 3: Manifest And Docs Sync

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/workflows.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Record the committed sample proof and update `unsupportedProductionGaps.3d-playable-vertical-slice` without making it `ready`.
- [x] Update docs to explain the committed sample as generated-package evidence, not a broad 3D production closeout.
- [x] Keep remaining broad/host-gated 3D claims explicit.

### Task 4: Validation

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

- [x] Record focused command results.
- [x] If the package smoke hits a legitimate host gate, record the blocker and leave ready claims narrow.

## Validation Evidence

Record command results here while implementing this plan.

- RED evidence:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected before the sample existed, reporting the missing `games/sample_generated_desktop_runtime_3d_package` path.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected before manifest sync, reporting that `3d-playable-vertical-slice` still needed `implemented-generated-desktop-3d-package-proof`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed before the manifest status change because the old planned-plus status was still allowlisted.
- Generation evidence:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name sample_generated_desktop_runtime_3d_package -Template DesktopRuntime3DPackage` generated the committed sample directory and appended `mirakana_add_desktop_runtime_game(sample_generated_desktop_runtime_3d_package ...)` with `PACKAGE_FILES_FROM_MANIFEST`.
- Focused GREEN evidence:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after the committed sample, README marker, manifest descriptors, CMake registration, shader markers, and docs markers were synchronized.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after the 3D gap moved to `implemented-generated-desktop-3d-package-proof` and the Source Asset Registry row-order check was made kind/source-format based instead of assuming material at `asset.2`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=11` and one `implemented-generated-desktop-3d-package-proof` gap.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed 18/18 tests, including `sample_generated_desktop_runtime_3d_package_smoke` and `sample_generated_desktop_runtime_3d_package_shader_artifacts_smoke`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed, including installed D3D12 scene GPU package validation with `scene_gpu_status=ready`, `postprocess_status=ready`, skinned compute morph bindings/draws, async telemetry counters, and `morph_package=1`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after targeted clang-format on the generated sample and D3D12 SDL desktop presentation source.
  - A temporary `DesktopRuntime3DPackage` generation dry run produced a `main.cpp` that passed clang-format with the repository `.clang-format` and matched the committed sample `main.cpp`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` completed with exit code 0 after the interrupted process was allowed to finish.
  - The focused installed package smoke did not hit a host gate on this Windows/DXC host; ready claims remain narrowed to the committed generated-package proof.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`, including static checks, dependency/toolchain checks, diagnostic shader/mobile/Apple host gates, public API boundary check, CMake configure/build, and 29/29 `ctest --preset dev` tests.
- Fix evidence:
  - The initial source-tree smoke exposed an incorrect generated morph package pass condition that compared cumulative morphed vertex rows to frame count. The template now records vertices per sample and validates cumulative rows as `frames * vertices_per_sample`.
  - The initial installed package smoke exposed a stale generated skinned vertex stride and a D3D12 scene renderer path that treated skinned compute morph as if the static mesh compute morph vertex shader were required. The template now uses `runtime_skinned_mesh_vertex_stride_bytes`, links `mirakana_runtime_rhi`, emits skinned upload/resolution fields, and the D3D12 scene renderer separates static compute morph pipeline setup from skinned compute morph dispatch.
  - The generated `main.cpp` template now formats C++ output through the resolved repository clang-format when available, with explicit `.clang-format` selection and a final newline, so future `DesktopRuntime3DPackage` scaffolds stay aligned with repository formatting.
