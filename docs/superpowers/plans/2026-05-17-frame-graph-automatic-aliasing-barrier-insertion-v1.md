# Frame Graph Automatic Aliasing Barrier Insertion v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

- **Plan ID:** `frame-graph-automatic-aliasing-barrier-insertion-v1`
- **Status:** Implementation complete; final validation and publication pending.
- **Goal:** Insert backend-neutral texture aliasing barriers automatically inside `execute_frame_graph_rhi_texture_schedule` when a transient texture alias group switches from one planned resource lifetime to the next.
- **Architecture:** Reuse the existing `FrameGraphTransientTextureAliasPlan` lifetime rows as the executor-owned aliasing schedule input. The executor prevalidates lifetime rows against `FrameGraphTextureBinding` resources and the linear frame-graph schedule, records one `IRhiCommandList::texture_aliasing_barrier` before the first pass that uses each later resource in an alias group, then records existing pass target-state transitions and pass callbacks.
- **Tech Stack:** C++23, `MK_renderer`, backend-neutral `MK_rhi`, NullRHI/D3D12/Vulkan existing aliasing-barrier implementations, PowerShell 7 validation entrypoints.

---

## Context

- The active master plan is [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md).
- The selected gap remains `frame-graph-v1`.
- The immediately preceding slice, [2026-05-17-frame-graph-backend-neutral-distinct-alias-group-lease-binding-v1.md](2026-05-17-frame-graph-backend-neutral-distinct-alias-group-lease-binding-v1.md), made alias-group lease binding return one distinct first-party `TextureHandle` per resource.
- This slice is narrower than production graph ownership, package streaming, multi-queue scheduling, Vulkan/Metal alias-memory allocation, data inheritance/content preservation, or public wildcard/null aliasing barriers.

## Official Practice Sources

- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` and Microsoft Learn `D3D12_RESOURCE_ALIASING_BARRIER`: aliasing barriers describe transitions between resources that map to the same heap and can name before/after resources.
- Microsoft Learn Direct3D 12 resource-barrier synchronization: aliasing barriers are used when resources with overlapping mappings transition between usages.
- Context7 `/khronosgroup/vulkan-docs`, Khronos Vulkan memory aliasing, and `vkCmdPipelineBarrier2`: Vulkan memory aliasing is explicit memory-range aliasing, and synchronization2 pipeline barriers define memory dependencies between earlier and later commands on the same queue.
- Source URLs: `https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_aliasing_barrier`, `https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12`, `https://docs.vulkan.org/spec/latest/chapters/resources.html#resources-memory-aliasing`, and `https://docs.vulkan.org/spec/latest/chapters/synchronization.html#vkCmdPipelineBarrier2`.

## Constraints

- Keep native handles private; use only `IRhiCommandList::texture_aliasing_barrier`.
- Do not add public wildcard/null alias barriers.
- Do not claim Vulkan/Metal memory alias allocation, data inheritance/content preservation, package streaming, multi-queue scheduling, or broad renderer readiness.
- The executor must validate automatic alias barrier rows before recording any command or invoking any pass callback.
- Existing explicit `record_frame_graph_texture_aliasing_barriers` remains supported for standalone resource-name barrier recording.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify after green: `docs/rhi.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify after green: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, composed `engine/agent/manifest.json`, relevant static guard scripts, and rendering skill/subagent guidance when stale.

## Task Checklist

- [x] Add the failing renderer unit test for automatic aliasing-barrier insertion before a later aliased resource pass.
- [x] Run the focused build/test command and record the expected RED failure.
- [x] Add `FrameGraphRhiTextureExecutionDesc::transient_texture_lifetimes` and `FrameGraphRhiTextureExecutionResult::aliasing_barriers_recorded`.
- [x] Implement minimal executor planning to group lifetimes by alias group, validate resource bindings and pass indices, and record adjacent alias barriers before the later resource's first pass.
- [x] Run the focused renderer test and record GREEN evidence.
- [x] Add a failing validation test for malformed automatic aliasing lifetime rows that must stop before callbacks or command recording.
- [x] Implement validation for malformed lifetime rows and rerun the focused renderer test.
- [x] Update docs, manifest fragments, composed manifest, skills/subagents, and static guards for the new ready surface and remaining unsupported claims.
- [ ] Run focused validation: renderer tests, static checks touching renderer/agent surfaces, `tools/validate.ps1`, and `tools/build.ps1` if the slice is publication-ready.
- [ ] Commit, push, create PR, and inspect PR checks.

## Done When

- `execute_frame_graph_rhi_texture_schedule` automatically records one aliasing barrier between adjacent non-overlapping lifetime rows in each supplied transient alias group before the later resource's first pass callback.
- Automatic insertion counts are visible through `FrameGraphRhiTextureExecutionResult::aliasing_barriers_recorded`.
- Invalid automatic aliasing inputs fail deterministically before command recording or callback invocation.
- Existing explicit aliasing-barrier recording, texture state transitions, pass-target states, final states, and shared-handle state handoff remain covered.
- Docs, manifest, skills/subagents, and static guards no longer claim automatic aliasing-barrier insertion is unsupported, while keeping package streaming, multi-queue scheduling, Vulkan/Metal alias memory, data inheritance/content preservation, and broad renderer readiness unsupported.
- Focused tests and full slice validation pass, or exact host blockers are recorded.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| Context7 Direct3D 12 docs lookup | Passed | Official aliasing barrier/resource barrier source anchors recorded above. |
| Context7 Vulkan docs lookup | Passed | Official memory aliasing / synchronization2 source anchors recorded above. |
| `cmake --build --preset dev --target MK_renderer_tests` | RED as expected | Positive test failed to compile before implementation because `FrameGraphRhiTextureExecutionDesc::transient_texture_lifetimes` did not exist. |
| `cmake --build --preset dev --target MK_renderer_tests`; `ctest --preset dev --output-on-failure -R MK_renderer_tests` | Passed | Positive automatic insertion test green after executor field/counter/planning implementation. |
| `cmake --build --preset dev --target MK_renderer_tests; if ($LASTEXITCODE -eq 0) { ctest --preset dev --output-on-failure -R MK_renderer_tests }` | RED as expected | Malformed lifetime test failed before validation because the executor still invoked callbacks. |
| `cmake --build --preset dev --target MK_renderer_tests; if ($LASTEXITCODE -eq 0) { ctest --preset dev --output-on-failure -R MK_renderer_tests }` | Passed | Malformed lifetime rows now fail before command recording or callback invocation. |
| Read-only rendering-auditor subagent review | Finding fixed | Review caught same-handle automatic aliases failing at recording time rather than during executor prevalidation, plus stale `docs/architecture.md` unsupported text. |
| `cmake --build --preset dev --target MK_renderer_tests; if ($LASTEXITCODE -eq 0) { ctest --preset dev --output-on-failure -R MK_renderer_tests }` | Blocked by host build concurrency | MSVC reported `C1041` PDB contention on shared `mirakana_renderer.pdb`; reran the same focused target with `/m:1`. |
| `cmake --build --preset dev --target MK_renderer_tests -- /m:1; if ($LASTEXITCODE -eq 0) { ctest --preset dev --output-on-failure -R MK_renderer_tests }` | Passed | Added same-handle automatic alias prevalidation test; `MK_renderer_tests` passed. |
| `cmake --preset dev`; `cmake --build --preset dev --target MK_renderer_tests`; `ctest --preset dev --output-on-failure -R MK_renderer_tests` | Passed | Focused renderer validation before rebasing onto latest `origin/main`; current branch still showed duplicated parent `PATH`, so final validation must use the updated CMake normalization from main. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed | Composed manifest and repository formatting were refreshed after docs/source-link updates. |
| `cmake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_d3d12_rhi_tests MK_runtime_host_tests MK_backend_scaffold_tests` | Passed | Focused public-header/backend-adjacent build passed; MSBuild emitted existing shared-intermediate `MSB8028` warnings. |
| `ctest --preset dev --output-on-failure -R "MK_rhi_tests|MK_renderer_tests|MK_d3d12_rhi_tests|MK_runtime_host_tests|MK_backend_scaffold_tests"` | Passed | 5/5 focused tests passed. |
| `cmake --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_d3d12_rhi_tests MK_runtime_host_tests MK_backend_scaffold_tests -- /m:1; if ($LASTEXITCODE -eq 0) { ctest --preset dev --output-on-failure -R "MK_rhi_tests|MK_renderer_tests|MK_d3d12_rhi_tests|MK_runtime_host_tests|MK_backend_scaffold_tests" }` | Passed | Focused public-header/backend-adjacent build/test passed after the same-handle prevalidation fix; 5/5 focused tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | `text-format-check: ok`; `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | `agent-config-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent integration static guards passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full slice gate passed after the review fixes; CTest reported 65/65 tests passed. Host-gated Apple/Metal diagnostics remained diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit preflight build passed after the review fixes; MSBuild emitted existing shared-intermediate `MSB8028` warnings. |
