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

**Status:** Completed.

### Goal

Expose package-visible counters or rows in the material/shader package sample so AI agents can verify the new material diagnostics contract from a generated game package lane.

### Done When

- Sample/package smoke reports deterministic modern-material readiness and diagnostics counters.
- Installed desktop runtime validation checks the counters for the selected material/shader package target.
- Docs, manifest fragments, game guidance, and static checks describe the exact supported material claims and keep Vulkan/Metal host gates explicit.

## Phase 3: Backend-Gated Promotion Review

**Status:** Completed.

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
- Phase 2 RED evidence: after `tools/validate-installed-desktop-runtime.ps1` began requiring modern material counters for `sample_generated_desktop_runtime_material_shader_package`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` failed because the installed smoke status line did not include `modern_material_variants`.
- Phase 2 implementation exposes package-visible `modern_material_variants`, `modern_material_ready`, `modern_material_host_gated`, `modern_material_unsupported`, `modern_material_invalid`, `modern_material_diagnostics`, `modern_material_texture_dependencies`, and `modern_material_shader_evidence_ready` counters from the committed material/shader sample and generated cooked-scene/material-shader template by planning loaded package material rows through `plan_modern_material_variants` with descriptor-selected texture slots and host-owned shader evidence. The proof stays value-only and does not execute shader graphs, compile shaders, mutate packages, create renderer/RHI residency, stream packages, or expose native handles.
- Phase 2 focused package evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` passed; installed smoke reported `modern_material_variants=1`, `modern_material_ready=1`, `modern_material_diagnostics=0`, `modern_material_texture_dependencies=1`, and `modern_material_shader_evidence_ready=1`.
- Phase 2 focused source-tree/static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_material_shader_package`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sample_generated_desktop_runtime_material_shader_package"`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files games/sample_generated_desktop_runtime_material_shader_package/main.cpp` passed.
- Phase 2 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 67/67 CTest tests and `production-readiness-audit: unsupported_gaps=0`; Apple/Metal host diagnostics remain host-gated/diagnostic-only as before.
- Phase 2 PR hardening evidence: hosted PR #147 initially passed Windows MSVC validation but failed while saving the optional Windows dev build cache. The CI contract now keeps `actions/cache/restore` as acceleration and makes post-success `actions/cache/save` non-blocking, with AGENTS/docs/skills/subagents/manifest/static-guard updates so cache archive or transport failures cannot turn successful validation red.
- Phase 3 official-doc review: Microsoft Direct3D 12 documentation models graphics pipeline state as carrying shader bytecode for the selected graphics shaders, including vertex and pixel shader bytecode in `D3D12_GRAPHICS_PIPELINE_STATE_DESC::VS` and `::PS`. The D3D12 package proof therefore stays tied to selected D3D12 scene/postprocess DXIL artifacts, not to Vulkan/Metal readiness.
- Phase 3 RED evidence: after `tools/validate-installed-desktop-runtime.ps1` began requiring backend-specific material shader evidence counters for `sample_generated_desktop_runtime_material_shader_package`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` failed because the installed smoke status line did not include `modern_material_d3d12_shader_evidence_ready`.
- Phase 3 implementation makes the committed and generated material/shader package smokes report `modern_material_d3d12_shader_evidence_ready`, `modern_material_vulkan_shader_evidence_ready`, and `modern_material_selected_shader_evidence_ready` in addition to the existing aggregate `modern_material_shader_evidence_ready`. The aggregate and selected-backend counters now use the selected presentation backend instead of `D3D12 || Vulkan`, so D3D12 package evidence does not accidentally promote Vulkan or Metal.
- Phase 3 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` passed and the installed smoke reported `modern_material_d3d12_shader_evidence_ready=1`, `modern_material_vulkan_shader_evidence_ready=0`, and `modern_material_selected_shader_evidence_ready=1`; `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, focused dev build/CTest for `sample_generated_desktop_runtime_material_shader_package`, and `tools/check-tidy.ps1 -Files games/sample_generated_desktop_runtime_material_shader_package/main.cpp` passed.
- Phase 3 Vulkan host-gate check: strict `-RequireVulkanShaders` package validation built and installed the SPIR-V artifacts on this host, then stopped before material counter validation with `required_vulkan_renderer_unavailable` because the Vulkan runtime path is still host-gated here. The installed validation script now derives the expected Vulkan material shader evidence counter from `-RequireVulkanShaders` so ready Vulkan hosts must prove `modern_material_vulkan_shader_evidence_ready=1` instead of inheriting the default D3D12 `0` expectation.
- Phase 3 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 67/67 CTest tests and `production-readiness-audit: unsupported_gaps=0`; Apple/Metal host diagnostics remain host-gated/diagnostic-only as before.
