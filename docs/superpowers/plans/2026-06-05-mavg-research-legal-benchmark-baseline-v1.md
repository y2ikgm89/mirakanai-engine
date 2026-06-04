# 2026-06-05 MAVG Research Legal Benchmark Baseline v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline execution only after this plan is selected for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-research-legal-benchmark-baseline-v1`

**Status:** Completed.

**Goal:** Select the first MAVG child candidate and lock clean-room architecture, official-source gates, benchmark methodology, stale-doc classification, and validation evidence before any MAVG code is implemented.

**Architecture:** Treat this as a docs/specification and agent-contract slice. It updates the master-roadmap audit, creates Phase 0 architecture and benchmark specs, selects the child plan in the composed manifest, reconciles current-truth docs, and keeps all runtime, renderer, package, streaming, deformation, ray tracing, and benchmark-exceeds claims unsupported until future focused implementation plans prove them.

**Tech Stack:** C++23 project policy, PowerShell repository validation, `engine/agent/manifest.fragments` plus compose output, Context7 current CMake/Vulkan/vcpkg documentation checks, official Microsoft/Khronos/Apple/Epic/NVIDIA/AMD documentation, and first-party docs/specs/plans.

---

## Context

The MAVG master roadmap exists at `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`. A 2026-06-05 full-project audit found that first-party Win32/WASAPI platform lanes, first-party UI/editor, `MK_environment`, and performance foundations changed enough that MAVG must not start from stale SDL3/Dear ImGui assumptions.

This plan is the first executable candidate. It does not implement MAVG runtime behavior.

## Constraints

- Keep `unsupportedProductionGaps = []`.
- Do not reopen Engine 1.0 blockers.
- Do not add SDL3, Dear ImGui, Qt, Slint, RmlUi, or UI middleware.
- Do not expose native Win32, D3D12, Vulkan, Metal, UIA, TSF, DirectWrite, WASAPI, or RHI handles through public APIs.
- Do not read, copy, port, transcribe, or pattern-match Unreal Engine source, Nanite source, UE shaders, private presentations, or cooked data.
- Do not add third-party code, datasets, or dependencies in this phase.
- Do not claim MAVG is implemented, Nanite-compatible, Nanite-equivalent, or Nanite-exceeding.
- Use official documentation and Context7 checks for API/toolchain constraints before future implementation.

## Files

- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Create: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Create: `docs/specs/2026-06-05-mavg-benchmark-methodology-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture-directory-verification.md`
- Modify: `docs/cpp-standard.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

## Official Source Checks Completed

- Context7 `/kitware/cmake`: CMake presets, install/export, package config, and CXX module `FILE_SET` guidance.
- Context7 `/khronosgroup/vulkan-docs`: Vulkan mesh/synchronization guidance, including compute-write to shader/host-read synchronization patterns.
- Context7 `/microsoft/vcpkg`: manifest mode, feature dependencies, triplet handling, and `VCPKG_MANIFEST_INSTALL` behavior.
- Epic Nanite public docs: feature taxonomy only.
- Microsoft DirectX Mesh Shader and Work Graphs specs.
- Microsoft D3D12 resource barrier and enhanced barrier docs.
- Microsoft Raw Input, WASAPI/Core Audio, DirectWrite, TSF, and UIA docs.
- Khronos Vulkan `VK_EXT_mesh_shader` and synchronization docs.
- Apple Metal capability and mesh/object shader docs.
- NVIDIA RTX Mega Geometry and AMD GPUOpen meshlet guidance as background only.

## Tasks

### Task 1: Select Phase 0

**Files:**

- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`

- [x] Set `currentActivePlan` to this plan.
- [x] Set `recommendedNextPlan.id` to `mavg-research-legal-benchmark-baseline-v1`.
- [x] Set `recommendedNextPlan.path` to this plan.
- [x] Keep `unsupportedProductionGaps = []`.
- [x] Record that this is post-1.0 MAVG research/specification work, not a ready runtime feature.

### Task 2: Lock MAVG Architecture Baseline

**Files:**

- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Create: `docs/specs/2026-06-05-mavg-architecture-v1.md`

- [x] Add the 2026-06-05 full-project audit addendum to the master plan.
- [x] Record current first-party Win32/WASAPI and UI/editor baselines.
- [x] Record `MK_environment` and performance-foundation prerequisites.
- [x] Record clean-room legal guardrails and forbidden inputs.
- [x] Record official-source matrix and Context7 usage.
- [x] Define subsystem boundaries for asset cook, selection, conventional rendering, GPU culling, mesh shaders, streaming, deformation, ray tracing, and editor/tool UI.

### Task 3: Lock Benchmark Methodology

**Files:**

- Create: `docs/specs/2026-06-05-mavg-benchmark-methodology-v1.md`

- [x] Define the benchmark claim ladder.
- [x] Define allowed and forbidden benchmark assets.
- [x] Define static, environment-aware, streaming-pressure, deformation, and raster/RT consistency scene families.
- [x] Define required counters, host evidence, pass conditions, artifact policy, and initial non-claims.

### Task 4: Classify Stale Docs

**Files:**

- Modify: `docs/architecture-directory-verification.md`
- Modify: `docs/cpp-standard.md`

- [x] Reclassify old optional SDL3 / Dear ImGui shell wording as historical.
- [x] Point active editor/platform truth at first-party Win32/WASAPI and first-party retained UI/editor lanes.
- [x] Keep historical validation evidence where it is clearly marked as historical.

### Task 5: Static Checks And Validation

**Files:**

- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`
- Compose: `engine/agent/manifest.json`

- [x] Add `mavg-research-legal-benchmark-baseline-v1` to the recommended-next-plan static contract allowlist.
- [x] Add needles for clean-room/legal, official-source refresh, benchmark methodology, stale-doc cleanup, no SDL3/Dear ImGui, no public native handles, no Nanite/UE compatibility, and `unsupportedProductionGaps = []`.
- [x] Run `tools/compose-agent-manifest.ps1 -Write`.
- [x] Run `tools/check-text-format.ps1`.
- [x] Run `tools/check-agents.ps1`.
- [x] Run `tools/check-ai-integration.ps1`.
- [x] Run `tools/check-json-contracts.ps1`.
- [x] Run `git diff --check`.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: passed.
- `git diff --check`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed.

## Done When

- The Phase 0 child plan exists and is selected in the composed manifest.
- The MAVG master plan links to the 2026-06-05 audit baseline.
- The architecture and benchmark methodology specs exist.
- Current-truth docs identify MAVG Phase 0 as active without making runtime ready claims.
- Stale SDL3/Dear ImGui active guidance is corrected or clearly historical in the docs used by Phase 0.
- Static checks know the new `recommendedNextPlan.id`.
- Focused validation commands pass.

## Non-Claims

This plan does not implement:

- `MAVGClusterGraph`
- cluster cook tooling
- CPU selector
- conventional renderer adoption
- GPU culling
- indirect draws
- mesh shaders
- streaming/residency
- deformation
- ray tracing
- benchmark runner
- package recipe
- editor MAVG panel

It also does not claim Nanite compatibility, Nanite equivalence, Nanite superiority, general renderer quality, Vulkan readiness by D3D12 inference, Metal readiness by Windows inference, or broad environment/performance readiness.

## Next Plan

After this plan is validated and published, the selected implementation child is:

- `docs/superpowers/plans/2026-06-05-mavg-asset-graph-v1.md`

That plan should implement deterministic asset graph and cook validation only.
