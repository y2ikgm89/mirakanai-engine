# Frame Graph Transient Texture Alias Planning v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a backend-neutral Frame Graph v1 transient texture lifetime and alias planning contract that can prove conservative alias groups before native heap aliasing execution exists.

**Architecture:** Keep resource ownership backend-private. The planner lives in `MK_renderer` next to `frame_graph_rhi` because it needs `mirakana::rhi::TextureDesc` but must not allocate, bind heaps, expose native handles, or record D3D12/Vulkan/Metal barriers. It compiles the existing `FrameGraphV1Desc`, computes first/last pass lifetimes for transient texture resources, validates descriptor/access compatibility, and groups only non-overlapping exact descriptor matches into alias groups.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi` public value contracts, `MK_renderer_tests`, PowerShell validation wrappers.

---

## Status

Completed.

## Official Practice Check

- Microsoft Direct3D 12 `ID3D12GraphicsCommandList::ResourceBarrier` documents `D3D12_RESOURCE_ALIASING_BARRIER` for transitions between different resources that map into the same heap, and barrier arrays execute in order. This slice plans alias groups only; it does not emit native aliasing barriers.
- Vulkan memory guidance treats sub-allocation as first-class because OS/driver allocations are expensive and active allocation count is limited. Vulkan spec text also keeps lazily allocated transient attachment memory constrained to images with transient attachment usage. This slice records conservative texture lifetime/usage compatibility only; Vulkan allocation, lazy memory selection, image layout barriers, queue-family ownership, and memory aliasing execution remain backend work.

Sources:

- https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier
- https://docs.vulkan.org/guide/latest/memory_allocation.html
- https://docs.vulkan.org/spec/latest/chapters/memory.html
- https://docs.vulkan.org/spec/latest/chapters/synchronization.html

## Context

`frame-graph-v1` already has deterministic v1 compile/schedule, RHI texture barrier execution, pass callbacks, final-state restoration, and package-visible executor evidence for the completed postprocess-depth and directional-shadow paths. The remaining gap row still requires production graph ownership, broader renderer migration, native transient heap allocation and alias execution, and general multi-pass scheduling.

This slice attacks only the transient texture lifetime/aliasing policy. It produces data a future backend allocator can consume, while keeping existing renderer-owned texture allocation untouched.

## Constraints

- Do not expose native handles, heaps, memory objects, queue objects, or backend barrier structs.
- Do not allocate resources, bind memory, emit native aliasing barriers, or change renderer frame execution.
- Alias only exact `TextureDesc` matches whose frame-graph lifetimes do not overlap.
- Reject transient texture descriptors that do not satisfy the graph access usage requirements.
- Keep `frame-graph-v1` `status` as `implemented-foundation-only`.
- No compatibility shims or duplicate public APIs.

## Done When

- `frame_graph_rhi.hpp` exposes a small value contract for transient texture alias planning.
- `plan_frame_graph_transient_texture_aliases` validates descriptors and produces deterministic lifetimes, alias groups, unaliased byte estimate, and aliased peak byte estimate.
- `MK_renderer_tests` covers non-overlapping alias reuse, overlapping resources staying separate, descriptor/access rejection, and imported/unknown descriptor rejection.
- `docs/rhi.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, manifest fragment/composed manifest, and static guards describe the new narrow evidence and remaining non-goals.
- Focused tests, agent/static checks, full `tools/validate.ps1`, and `tools/build.ps1` pass or record a concrete environment blocker.

## Tasks

### Task 1: RED tests for transient texture alias planning

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Add tests that call the intended `plan_frame_graph_transient_texture_aliases` API:
  - non-overlapping same-desc transient textures share one alias group and reduce estimated aliased bytes
  - overlapping same-desc transient textures use separate alias groups
  - missing usage bits for declared accesses produce `FrameGraphDiagnosticCode::invalid_resource`
  - descriptors for imported or undeclared resources are rejected

- [x] Run `cmake --build --preset dev --target MK_renderer_tests`.

Expected: compile fails because the new types/functions do not exist yet.

### Task 2: Implement the planner value contract

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`

- [x] Add value types:
  - `FrameGraphTransientTextureDesc`
  - `FrameGraphTransientTextureLifetime`
  - `FrameGraphTransientTextureAliasGroup`
  - `FrameGraphTransientTextureAliasPlan`
- [x] Add `plan_frame_graph_transient_texture_aliases(const FrameGraphV1Desc&, std::span<const FrameGraphTransientTextureDesc>)`.
- [x] Validate duplicate descriptors, unknown resources, imported resources, missing descriptors for used transient resources, invalid texture descs, and access-to-usage compatibility.
- [x] Compute transient resource lifetimes from compiled `ordered_passes`.
- [x] Greedily assign deterministic alias groups only when lifetimes do not overlap and texture descriptors are exact matches.
- [x] Estimate bytes with `bytes_per_texel(format) * width * height * depth`; document this as a diagnostic estimate, not an allocator contract.

### Task 3: GREEN focused renderer tests

**Files:**

- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Run `cmake --build --preset dev --target MK_renderer_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_renderer_tests`.
- [x] Refactor only if tests are green.

### Task 4: Agent surface and documentation sync

**Files:**

- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/rhi.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json` via compose only
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1` if rendering needles need the new API text

- [x] Keep `frame-graph-v1` open and foundation-only.
- [x] State that this is conservative alias planning, not native heap aliasing, transient heap allocation, renderer-wide migration, multi-queue scheduling, package streaming, or Metal readiness.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 5: Slice validation and publication

**Files:**

- All touched files.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit, push, create PR, inspect PR state/checks, and register auto-merge only after required preflight is clean.

## Validation Evidence

- RED: `pwsh -NoProfile -Command '. (Join-Path (Get-Location) ''tools/common.ps1''); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests'` failed before implementation because `FrameGraphTransientTextureDesc` / `plan_frame_graph_transient_texture_aliases` did not exist.
- GREEN focused: `pwsh -NoProfile -Command '. (Join-Path (Get-Location) ''tools/common.ps1''); $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests'` passed.
- GREEN focused: `ctest --preset dev -R "^MK_renderer_tests$" --output-on-failure` passed with 1/1 test.
- Review hardening: read-only rendering/C++ subagents found missing rejection for backend-incompatible depth texture descriptors and unchecked byte-estimate overflow. Added RED tests for both cases, then fixed descriptor validation and checked arithmetic.
- Static/agent: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-production-readiness-audit.ps1` passed.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 65/65 CTest tests. Metal/Apple host checks remained diagnostic-only or host-gated on Windows as expected.
- Build gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed.
