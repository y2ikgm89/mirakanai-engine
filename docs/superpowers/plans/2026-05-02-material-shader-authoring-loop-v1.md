# Material Shader Authoring Loop v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move material/shader authoring from narrow first-party material instance and generated shader-artifact scaffolds toward an AI-operable authored material/shader review loop without claiming shader graphs, material graphs, live shader generation, or broad renderer quality.

**Architecture:** Reuse `MaterialAuthoringDocument`, first-party `.material` package update surfaces, shader source metadata, shader compile request planning, shader artifact manifests, package metadata, and host-gated D3D12/Vulkan shader artifact validation. Keep gameplay on cooked packages and public material/scene submission contracts. Do not expose native handles, execute arbitrary shell, add third-party shader graph dependencies, or claim Metal/general renderer readiness.

**Tech Stack:** C++23, `mirakana_assets`, `mirakana_tools`, `mirakana_editor_core`, shader metadata, material authoring documents, package update tooling, static checks, docs, and focused validation.

---

## Goal

Make AI/editor-authored material and shader updates reviewable and package-safe:

- select first-party material inputs and fixed shader artifact requests through reviewed descriptors
- keep source material/HLSL authoring inputs out of `runtimePackageFiles`
- validate package rows, material texture dependencies, and shader artifact metadata before desktop package smoke
- preserve host-gated D3D12/Vulkan shader validation as separate from material authoring

## Constraints

- Do not implement shader graphs or material graphs.
- Do not run live shader generation from gameplay.
- Do not expose native renderer/RHI handles.
- Do not claim package streaming, Metal readiness, or general renderer quality.
- Do not add dependencies without license/dependency docs.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks or tests distinguish material authoring, package update, shader artifact validation, and host-gated desktop smoke.
- `engine/agent/manifest.json`, docs, static checks, and validation recipes remain honest about unsupported shader/material graph and live-generation claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Material And Shader Authoring Boundaries

- [x] Read `mirakana_assets` material/shader metadata, `mirakana_tools` material package update helpers, editor material authoring, shader compile planning, and generated material/shader package scaffolds.
- [x] Identify the smallest review loop that can be made AI-operable without shader/material graphs.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- `mirakana_assets` already owns `MaterialDefinition`, `MaterialInstanceDefinition`, backend-neutral material binding metadata, `ShaderSourceMetadata`, `ShaderGeneratedArtifact`, and deterministic shader compile command planning.
- `mirakana_tools` already owns the reviewed `.material` plus `.geindex` package update surface through `plan_material_instance_package_update` / `apply_material_instance_package_update`, including rejection of material graph, shader graph, and live shader generation sentinel claims.
- `mirakana_editor_core` already owns `MaterialAuthoringDocument`, registry-backed texture validation, staged dirty state, undoable material edits, fixed material preview shader compile request planning, and shader artifact readiness state.
- The generated material/shader package scaffold already keeps `source/materials/lit.material` and `shaders/*.hlsl` outside `runtimePackageFiles`, while CMake owns host-built D3D12 DXIL and toolchain-gated Vulkan SPIR-V artifacts.
- The smallest safe loop is descriptor-first: `materialShaderAuthoringTargets` rows select source material, cooked runtime material, package index, fixed HLSL source inputs, and selected shader artifact paths without adding command strings or runtime source parsing.
- Non-goals for this slice: shader graphs, material graphs, live shader generation, runtime shader compilation from gameplay, public native/RHI handles, Metal readiness, package streaming, broad dependency cooking, arbitrary shell, or general renderer quality.

### Task 2: RED Checks

- [x] Add failing checks for material package/shader artifact review order.
- [x] Add failing checks rejecting shader graph, material graph, live shader generation, native handles, Metal, package streaming, and broad renderer quality claims.
- [x] Record RED evidence.

### Task 3: Authoring Loop Implementation

- [x] Implement or tighten the selected material/shader review surface.
- [x] Keep package mutation on reviewed `.material`/`.geindex` helpers.
- [x] Keep shader artifact validation host-gated and separate from material authoring.

Implementation notes:

- Added `game.agent.json.materialShaderAuthoringTargets` schema and static checks for safe game-relative source material paths, runtime material paths, package `.geindex` paths, fixed HLSL source paths, selected D3D12 `.dxil` paths, and selected Vulkan `.spv` paths.
- Added `aiOperableProductionLoop.materialShaderAuthoringLoops` with an ordered review sequence over source material review, material texture/package validation, fixed shader artifact request review, host-gated shader artifact validation, and selected material/shader package smoke.
- Updated the committed generated material/shader sample manifest and `tools/new-game.ps1` material/shader and 3D templates to emit descriptor rows while keeping source material/HLSL inputs out of `runtimePackageFiles`.
- Added the reviewed `desktop-runtime-generated-material-shader-scaffold-package` recipe to `tools/run-validation-recipe.ps1` with an allowlisted target and D3D12 host-gate acknowledgement.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding static checks because `schemas/game-agent.schema.json` did not define `materialShaderAuthoringTargets`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `schemas/game-agent.schema.json` did not contain `"materialShaderAuthoringTargets"` and `engine/agent/manifest.json` did not yet expose the material/shader authoring loop.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding `materialShaderAuthoringTargets`, `materialShaderAuthoringLoops`, sample/generated descriptor rows, docs, and static checks (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after synchronizing generated scaffold checks and docs (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe desktop-runtime-generated-material-shader-scaffold-package -HostGateAcknowledgements d3d12-windows-primary` returned the reviewed package command plan for `sample_generated_desktop_runtime_material_shader_package`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` passed after adding the material/shader package recipe to the reviewed allowlist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reports `currentActivePlan` as this material/shader plan with package streaming/residency budget contract as the recommended next plan.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe desktop-runtime-generated-material-shader-scaffold-package -HostGateAcknowledgements d3d12-windows-primary` passed (`status: passed`, `durationSeconds: 50.979`) for the selected generated material/shader package lane.
- GREEN: After advancing the registry, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reports `currentActivePlan` as `docs/superpowers/plans/2026-05-02-package-streaming-residency-budget-contract-v1.md` with `docs/superpowers/plans/2026-05-02-safe-point-package-unload-replacement-execution-v1.md` as the recommended next plan.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after the registry/docs/manifest advancement (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after the registry/docs/manifest advancement (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`status: passed`, `durationSeconds: 3.504`) and ran `agent-check` plus `schema-check`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the registry/docs/manifest advancement, including CMake configure/build and CTest `28/28`; Metal tools, Apple packaging/signing, Android release/device smoke, and strict tidy compile database availability remain diagnostic-only host gates.
