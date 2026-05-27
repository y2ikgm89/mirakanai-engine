# Renderer Production Quality And Backend Parity v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote renderer production quality from selected evidence rows into backend-local D3D12, strict Vulkan, and Apple-host-gated Metal production gates with broad profiling, residency, frame-graph, shader, package, and non-claim evidence.

**Architecture:** Keep renderer, RHI, runtime upload, scene rendering, and package smoke contracts backend-neutral at public boundaries while each backend proves synchronization, shader validation, residency, timing, and package evidence independently. Use clean breaking changes when existing value rows or package counters are too narrow; update all callers, tests, manifests, docs, and static checks in the same slice.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_runtime_rhi`, `MK_runtime_scene_rhi`, `MK_scene_renderer`, generated desktop runtime package samples, CMake/CTest, PowerShell validation wrappers, D3D12 resource barriers/fences/residency, Vulkan synchronization/validation/SPIR-V tools, Apple Metal resource synchronization/capability host gates, and first-party diagnostics/profile capture.

---

**Plan ID:** `renderer-production-quality-backend-parity-v1`

**Status:** Completed.

Selected child plan of `clean-break-broad-production-readiness-master-plan-v1`.

**Date:** 2026-05-27

## Context

Existing renderer evidence is useful but still narrow:

- `renderer-general-quality-matrix-v1` records backend-local selected D3D12/Vulkan rows and Metal host-gated rows.
- `production-rendering-vfx-profiling-v1`, `renderer-gpu-memory-v1`, `renderer-debug-profiling-v1`, and `renderer-backend-parity-v1` have selected counters and package evidence.
- Current docs still correctly reject broad production renderer quality, full backend parity, and broad profiling claims.

This child plan is the first clean-break implementation candidate for the broad production readiness master plan. It does not reopen Engine 1.0; it strengthens post-1.0 renderer breadth.

## Official Practice Check

Before code changes in each task, re-check and record the exact docs touched by that task:

- D3D12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- D3D12 multi-engine synchronization: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>
- D3D12 residency: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency>
- Vulkan synchronization guide: <https://docs.vulkan.org/guide/latest/synchronization.html>
- Vulkan validation overview: <https://docs.vulkan.org/guide/latest/validation_overview.html>
- Vulkan memory allocation guide: <https://docs.vulkan.org/guide/latest/memory_allocation.html>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
- Apple Metal capabilities: <https://developer.apple.com/metal/capabilities/>

## Constraints

- No backend-native handles in public gameplay, scene, material, UI, or package APIs.
- D3D12, Vulkan, and Metal evidence is independent. No backend inherits another backend's readiness.
- Metal remains host-gated until macOS/Xcode/Metal validation lands.
- Performance claims require deterministic timing/profile rows, budget diagnostics, and package-visible counters. Do not claim measured frame-rate parity from synthetic or value-only rows.
- Renderer package counters must distinguish ready, host-gated, dependency-gated, and unsupported rows.
- Any public aggregate changes must update designated initializers in tests in declaration order.

## Candidate Evidence

| Candidate | Scope | Current evidence | Remaining gap |
| --- | --- | --- | --- |
| `renderer-quality-status-taxonomy-v1` | `Renderer General Quality Matrix v1` / `plan_renderer_quality_matrix` clean-break `RendererQualityMatrixRowStatus` taxonomy plus package-visible dependency/unsupported counters. | Focused RED build failed before implementation because `RendererQualityMatrixRowStatus`, `dependency_gate_required`, `dependency_gated_row_count`, and `unsupported_row_count` were absent. Focused GREEN passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_quality_matrix_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R renderer_quality_matrix`. Package-visible fields remain `renderer_quality_matrix_status=host_evidence_required`, `renderer_quality_matrix_dependency_gated_rows=0`, `renderer_quality_matrix_unsupported_rows=0`, and `renderer_quality_matrix_general_renderer_quality_ready=0`. | Backend parity policy, broad profiling/residency rows, package promotion, and full slice validation remain open tasks in this plan. |
| `renderer-backend-local-quality-contracts-v1` | `Backend Renderer Parity v1` clean-break `BackendRendererParityPolicyRequest` / `BackendRendererParityProofRow` / `BackendRendererParityPolicyPlan` / `plan_backend_renderer_parity_policy` proof review for synchronization, shader-validation, memory-residency, profiling, and package-evidence rows. | Official docs checked: D3D12 resource barriers and residency from Microsoft Learn, Vulkan synchronization/validation from Khronos docs through Context7, and Metal capability/resource synchronization gates from Apple docs. Focused RED build failed before implementation because `BackendRendererParityFeatureKind`, `BackendRendererParityProofRow`, `BackendRendererParityPolicyRequest`, `BackendRendererParityPolicyPlan`, `BackendRendererParityPolicyStatus`, and `plan_backend_renderer_parity_policy` were absent. Focused GREEN passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_quality_matrix_tests|MK_renderer_tests"`. | Package-visible promotion and broad profiling/residency rows remain open tasks in this plan; Metal remains Apple-host-gated and broad renderer quality is still unclaimed. |
| `renderer-profiling-residency-evidence-v1` | Clean-break `DebugProfilingPolicyDesc` / `DebugProfilingRequestDesc` and `GpuMemoryPolicyDesc` / `GpuMemoryRequestDesc` evidence expansion plus sample package counters. | Official docs checked: Microsoft D3D12 residency and Khronos Vulkan memory allocation/synchronization guidance. Focused RED tests first failed because CPU zone / trace handoff / package counter diagnostics and memory budget / residency pressure / package counter diagnostics were absent. Focused GREEN passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests MK_runtime_host_sdl3_tests sample_desktop_runtime_game sample_generated_desktop_runtime_3d_package` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_tests|MK_runtime_host_sdl3_tests"`. Package-visible fields now include `debug_profiling_policy_cpu_profile_zone_evidence_ready=1`, `debug_profiling_policy_trace_capture_handoff_evidence_ready=1`, `debug_profiling_policy_package_counter_evidence_ready=1`, `gpu_memory_policy_memory_budget_evidence_ready=1`, `gpu_memory_policy_residency_pressure_evidence_ready=1`, `gpu_memory_policy_package_counter_evidence_ready=1`, and generated `rendering_vfx_profiling_debug_*` / `rendering_vfx_profiling_memory_*` counters. Package GREEN passed with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package`. Static/full validation GREEN passed with `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, `tools/validate.ps1`, and `git diff --check`. PR #263 merged as merge commit `97b4b0d8e680b7da723a294ed77555ba9c7c5a8d`; hosted PR checks passed including `PR Gate`, `Windows MSVC`, Linux, CodeQL, static analysis, iOS smoke, and macOS Metal CMake. | Renderer child plan closed with Metal still Apple-host-gated and broad renderer quality still unclaimed. |

## Closeout Evidence

- PR #261 completed `renderer-quality-status-taxonomy-v1`.
- PR #262 completed `renderer-backend-local-quality-contracts-v1`.
- PR #263 completed `renderer-profiling-residency-evidence-v1` and merged as `97b4b0d8e680b7da723a294ed77555ba9c7c5a8d`.
- Local validation for the final renderer checkpoint passed with both selected desktop runtime package commands, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, `tools/validate.ps1`, and `git diff --check`.
- Hosted validation for PR #263 passed through `PR Gate`, `Windows MSVC`, Linux, CodeQL, static analysis, iOS smoke, and macOS Metal CMake.
- Broad renderer quality, Metal visible parity, and subjective/general performance parity remain unclaimed where host/backend evidence is absent.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp`
- Modify: `engine/renderer/src/renderer_quality_matrix.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp`
- Modify: `engine/renderer/src/backend_renderer_parity_policy.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/debug_profiling_policy.hpp`
- Modify: `engine/renderer/src/debug_profiling_policy.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp`
- Modify: `engine/renderer/src/gpu_memory_policy.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp`
- Modify: `engine/renderer/src/production_vfx_profiling.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify as discovered: backend implementation files under `engine/rhi/`, `platform/`, or backend-specific renderer folders.
- Modify: `tests/unit/renderer_quality_matrix_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/renderer_production_vfx_profiling_tests.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify as needed: `tools/check-ai-integration*.ps1`, `tools/check-json-contracts*.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/*.json`
- Generate: `engine/agent/manifest.json`

## Task 1 - Baseline Renderer Evidence Audit

- [x] Read the current renderer/RHI/package tests and identify which broad production claims are still unsupported.
- [x] Add a short evidence table to this plan with current supported rows, missing rows, and host-gated rows.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_quality_matrix_tests MK_renderer_production_vfx_profiling_tests MK_renderer_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "renderer_quality_matrix|renderer_production_vfx_profiling|renderer_rhi"
```

Expected: current baseline passes or records exact pre-existing tool/host blocker before implementation.

## Task 2 - RED Tests For Production Quality Gate Expansion

- [x] Add tests that fail until renderer quality rows distinguish production-ready, host-gated, dependency-gated, and unsupported status per backend and feature.
- [x] Add tests that reject inferred backend parity when a Vulkan or Metal row is missing backend-local synchronization/shader/validation evidence.
- [x] Add tests that reject broad performance profiling readiness without CPU profile rows, GPU timestamp availability rows, budget rows, package counters, and trace/capture handoff rows.
- [x] Add tests that reject public native-handle leakage and backend-native token strings in gameplay-facing row notes.
- [x] Run the focused tests and record the expected RED failures in this plan.

## Task 3 - Backend-Local Renderer Quality Contracts

- [x] Extend renderer quality value rows with explicit evidence categories: synchronization, shader/tool validation, memory/residency, render-pass/frame-graph behavior, profiling, package evidence, host gate, dependency gate, unsupported claim.
- [x] Implement fail-closed diagnostics for missing categories, duplicate feature/backend rows, unsupported broad claims, backend inference, and native handle leakage.
- [x] Keep any aggregate changes clean-break and update all designated initializers in tests.
- [x] Run focused renderer quality tests and record GREEN evidence.

## Task 4 - Profiling And Residency Evidence Expansion

- [x] Extend debug profiling and GPU memory policy rows so broad profiling requires CPU zones, GPU timestamp capability, memory budget, residency pressure, package counter, and trace/capture handoff evidence.
- [x] Keep actual PIX/RenderDoc/Xcode capture execution host/operator-gated; the engine may publish request/review rows only.
- [x] Add package-visible counters to selected desktop runtime samples without a single broad `renderer_ready` flag.
- [x] Run focused profiling, memory, and package tests.

## Task 5 - Package And Validation Recipe Promotion

- [x] Update generated 3D package smoke and installed validation to require exact renderer production quality fields.
- [x] Update validation recipes and manifest rows only for evidence implemented in Tasks 2-4.
- [x] Keep Metal Apple-host-gated and keep strict Vulkan validation/toolchain-gated where local host evidence is absent.
- [x] Run package and installed validation.

## Task 6 - Docs, Manifest, Static Checks, And Closeout

- [x] Update current capabilities, roadmap, plan registry, backlog, projection chapter, manifest fragments, schema/static checks if literals changed, and generated-game guidance.
- [x] Compose the manifest.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

Expected: all checks pass, or host-gated blockers are recorded with exact command output. At closeout, return `currentActivePlan` to the master plan or select the next child plan.

## Done When

- The renderer has backend-local production quality evidence gates across D3D12, strict Vulkan, and Apple-host-gated Metal.
- Broad renderer readiness remains unclaimed for any missing host/backend lane.
- Profiling and residency rows are package-visible and fail closed on missing evidence.
- Docs, manifest, schemas/static checks, generated-game guidance, and validation recipes match the implemented scope.
- A validated commit and reviewable PR exist for this candidate before moving to the next child plan.
