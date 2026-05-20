# Renderer Modern Materials v1 (2026-05-21)

**Plan ID:** `renderer-modern-materials-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is renderer 1.x developer-owned capability work, not a reopened Engine 1.0 production gap.

## Goal

Expand the material pipeline beyond the current smoke surface so generated 3D and material/shader package games can use reviewed, deterministic modern material variants, package diagnostics, and backend-gated shader evidence without exposing native handles or arbitrary shader execution.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `engine-construction-placement-v1` is completed for reviewed construction placement validation and runtime-scene placement intent rows.
- Existing material foundations include `GameEngine.Material.v1`, material instance package updates, material graph package lowering, shader pipeline cache planning, generated `DesktopRuntimeMaterialShaderPackage`, D3D12 primary shader/package evidence, and Vulkan/Metal host gates.
- The renderer advanced production track lists `renderer-modern-materials-v1` as the first renderer 1.x stream after gameplay-family and 2D/gameplay developer-owned foundations.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep public material/gameplay/package APIs backend-neutral. D3D12, Vulkan, Metal, descriptor heaps, PSOs, native shader modules, and native handles stay behind RHI/backend-private adapters or explicit opaque interop plans.
- Do not execute arbitrary shader code, run live shader generation, mutate cooked packages without reviewed command surfaces, or claim Vulkan/Metal readiness from D3D12 evidence.
- Prefer official backend/toolchain documentation for D3D12, Vulkan, Metal, DXC/SPIR-V, and CMake/CTest behavior before backend-specific implementation decisions.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Close the completed construction placement active pointer and select `renderer-modern-materials-v1` as the next active developer-owned renderer 1.x capability.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `engine-construction-placement-v1` as completed.
- `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` and `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md` identify this plan as active without reopening production gaps.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Material Variant Diagnostics Contract

**Status:** Completed.

### Goal

Add the smallest backend-neutral material variant diagnostics contract that can classify reviewed `GameEngine.Material.v1`, material instance, and material graph-lowered outputs for modern-material readiness without changing renderer backend execution.

### Done When

- RED tests fail first for missing modern-material diagnostics.
- Public value rows distinguish base material, material instance, and graph-lowered material inputs; report deterministic diagnostics for invalid factor ranges, missing texture dependencies, unsupported material graph claims, and backend-gated shader evidence.
- Focused tests prove valid PBR-ready package rows and fail-closed diagnostics without package writes, shader compiler execution, renderer/RHI residency, or native handles.

## Phase 2: Package-Visible Material Evidence

**Status:** Planned.

### Goal

Expose package-visible counters or rows in the material/shader package sample so AI agents can verify the new material diagnostics contract from a generated game package lane.

### Done When

- Sample/package smoke reports deterministic modern-material readiness and diagnostics counters.
- Installed desktop runtime validation checks the counters for the selected material/shader package target.
- Docs, manifest fragments, game guidance, and static checks describe the exact supported material claims and keep Vulkan/Metal host gates explicit.

## Phase 3: Backend-Gated Promotion Review

**Status:** Planned.

### Goal

Promote only the backend-specific material evidence that has fresh official-doc-aligned validation, starting with D3D12 and keeping Vulkan/Metal separately host-gated.

### Done When

- D3D12 package evidence is backed by focused shader/toolchain/package validation.
- Vulkan and Metal rows remain host-gated unless their own strict validation evidence is recorded.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 started after `engine-construction-placement-v1` completed through PR #145, merge commit `8ab437c694709c0f193246255c3d553a8188476c`, hosted checks, package validation, and full `tools/validate.ps1` while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-renderer-modern-materials-v1.md`, `recommendedNextPlan.id=renderer-modern-materials-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed. The first JSON/AI integration attempt failed because the registry row used `Active milestone` instead of the static-contract active-slice row label; the registry now keeps that literal and describes the milestone in the notes column.
- Phase 1 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed before implementation because `ModernMaterialVariantDesc`, `ModernMaterialVariantSourceKind`, `ModernMaterialVariantStatus`, `ModernMaterialVariantDiagnosticCode`, and `plan_modern_material_variants` did not exist.
- Phase 1 implementation adds `ModernMaterialVariantDesc`, `ModernMaterialVariantRow`, `ModernMaterialVariantDiagnostic`, `ModernMaterialVariantPlan`, and `plan_modern_material_variants` in `MK_assets` for backend-neutral value-only PBR material readiness diagnostics across base, instance, and graph-lowered material rows. It reports invalid factor/source/definition rows, missing texture dependencies, missing backend shader evidence, unsupported non-lit shading, and unsupported shader graph execution without mutating packages, compiling shaders, creating renderer/RHI residency, streaming packages, or exposing native handles.
- Phase 1 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests"`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/material.cpp,tests/unit/core_tests.cpp` passed.
- Phase 1 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `production-readiness-audit: unsupported_gaps=0`; Apple/Metal host diagnostics remain host-gated/diagnostic-only as before.
