# Renderer Scene Scale v1 (2026-05-21)

**Plan ID:** `renderer-scene-scale-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is renderer 1.x developer-owned capability work, not a reopened Engine 1.0 production gap.

## Goal

Add deterministic renderer scene-scale primitives so generated 2D/3D games can request instancing, CPU/GPU culling boundaries, LOD selection, batching budgets, and package-visible draw-submission diagnostics without native backend handles or unmeasured performance claims.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `renderer-postprocess-v1` completed through PR #155 for backend-neutral postprocess policy diagnostics, package-visible postprocess counters, selected D3D12 postprocess execution evidence, hosted checks, and full validation evidence.
- Existing renderer foundations include package-visible framegraph/render-pass/multi-queue counters, upload staging, material/shadow/postprocess package evidence, sprite batching counters, and strict host gates for Vulkan/Metal parity.
- The renderer advanced production track lists `renderer-scene-scale-v1` after modern materials, lighting/shadows, and postprocess.
- Official documentation consulted for this milestone:
  - Microsoft Direct3D 12 guidance through Context7 (`/websites/learn_microsoft_en-us_windows_win32_direct3d12`) for command-list draw recording, `DrawInstanced` / `DrawIndexedInstanced` execution evidence, and explicit resource-state barriers.
  - Khronos Vulkan documentation through Context7 (`/khronosgroup/vulkan-docs`) for `vkCmdDrawIndexed` / `VkDrawIndexedIndirectCommand` `instanceCount` evidence and explicit synchronization/layout ownership.
  - Apple Metal documentation through Context7 (`/websites/developer_apple`) for `drawPrimitives` / `drawIndexedPrimitives` `instanceCount` evidence and Apple-host-gated Metal claims.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep public renderer/game APIs backend-neutral. Native D3D12, Vulkan, Metal, descriptor, PSO, queue, command-buffer, fence, heap, capture, and profiler handles stay backend-private or behind explicit opaque interop plans.
- Do not claim Vulkan/Metal readiness from D3D12 evidence.
- Performance claims require measurement. Structural instancing, culling, LOD, or batching rows may prove deterministic intent and draw counters, but not frame-time wins without timestamp/profile evidence and budgets.
- Keep scene-scale work reusable across gameplay families; game-specific spawn rules, art direction, and placement heuristics stay game-owned.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Close the completed postprocess active pointer and select `renderer-scene-scale-v1` as the next active developer-owned renderer 1.x capability.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `renderer-postprocess-v1` as completed.
- `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, and `docs/roadmap.md` identify this plan as active without reopening production gaps.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Scene Scale Policy Diagnostics

**Status:** Completed.

### Goal

Add the smallest backend-neutral value contract that classifies scene-scale requests, supported instancing/culling/LOD/batching budgets, expected draw/instance counters, and fail-closed diagnostics before backend execution.

### Done When

- RED tests fail first for missing scene-scale diagnostics.
- Public value rows distinguish valid scene-scale requests, unsupported batching/culling/LOD claims, invalid budgets, missing scene resources, and host-gated backend evidence.
- Focused tests prove deterministic classification without native handles, package mutation, live shader generation, or broad performance claims.

## Phase 2: Package-Visible Scene Scale Evidence

**Status:** Completed.

### Goal

Expose deterministic scene-scale policy and draw/instance counters in selected desktop runtime package lanes so AI agents can verify the supported contract from package output.

### Done When

- Sample/package smoke reports deterministic scene-scale policy readiness, diagnostic counters, draw-call counts, instance counts, culled/skipped counts, LOD/batch rows where implemented, and host-gated backend evidence rows.
- Installed desktop runtime validation checks those counters for the selected package target.
- Docs, manifest fragments, generated-game guidance, and static checks describe the exact supported scene-scale claims and host gates.

## Phase 3: Backend-Gated Instanced Draw Proof

**Status:** Completed.

### Goal

Promote only backend-specific instanced draw execution evidence with fresh official-doc-aligned validation, starting with D3D12 and keeping Vulkan/Metal separately host-gated unless their own strict evidence lands.

### Done When

- D3D12 package evidence is backed by focused shader/toolchain/package validation and explicit command-list draw, instance-count, resource-state, and render-pass reasoning.
- Vulkan and Metal rows remain host-gated unless their own strict validation evidence is recorded.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 started after `renderer-postprocess-v1` completed through PR #155, merge commit `03f8326257e92d80f845a9e5a71c1a0231850589`, hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks, plus local full `tools/validate.ps1` evidence while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, `docs/roadmap.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-renderer-scene-scale-v1.md`, `recommendedNextPlan.id=renderer-scene-scale-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `git diff --check` passed with `unsupported_gaps=0`.
- Phase 1 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed first because `mirakana/renderer/scene_scale_policy.hpp` was missing.
- Phase 1 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/scene_scale_policy.cpp,tests/unit/renderer_rhi_tests.cpp` passed after adding `SceneScalePolicyDesc`, `SceneScalePolicyPlan`, `SceneScaleDrawGroupDesc`, `SceneScaleBatchingMode`, `SceneScaleDrawGroupRow`, `SceneScaleDiagnostic`, and `plan_scene_scale_policy`.
- Phase 1 agent-surface evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` passed; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 1 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 67/67 CTest tests. Apple/Metal checks remained diagnostic host-gated on this Windows host.
- Phase 2 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests` failed first because `SdlDesktopPresentationSceneScalePolicyDesc`, `SdlDesktopPresentationSceneScalePolicyStatus`, and `evaluate_sdl_desktop_presentation_scene_scale_policy` were missing.
- Phase 2 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests MK_runtime_host_sdl3_public_api_compile sample_desktop_runtime_game`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_host_sdl3_(tests|public_api_compile)"`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp,games/sample_desktop_runtime_game/main.cpp,tests/unit/runtime_host_sdl3_tests.cpp,tests/unit/runtime_host_sdl3_public_api_compile.cpp` passed. The package smoke reported `scene_scale_policy_status=ready`, `scene_scale_policy_ready=1`, `scene_scale_policy_diagnostics=0`, `scene_scale_policy_draw_groups=2`, `scene_scale_policy_requested_instances=6`, `scene_scale_policy_visible_instances=6`, `scene_scale_policy_draw_calls=6`, `scene_scale_policy_instanced_draw_calls=0`, `scene_scale_policy_lod_groups=0`, and `scene_scale_policy_backend_instancing_evidence_ready=0`.
- Phase 2 agent-surface evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `git diff --check` passed; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 2 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 67/67 CTest tests. Apple/Metal checks remained diagnostic host-gated on this Windows host.
- Phase 3 RED evidence: focused builds for `MK_rhi_tests`, `MK_d3d12_rhi_tests`, and `MK_runtime_host_sdl3_tests` failed first because `mirakana::rhi::RhiStats` and D3D12 device stats did not expose instanced draw/instance counters.
- Phase 3 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests MK_d3d12_rhi_tests MK_runtime_host_sdl3_tests sample_desktop_runtime_game`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_tests|MK_d3d12_rhi_tests|MK_runtime_host_sdl3_tests"`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_public_api_compile MK_scene_renderer_tests MK_renderer_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_host_sdl3_public_api_compile|MK_scene_renderer_tests|MK_renderer_tests"`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed. The selected D3D12 package smoke reported `scene_scale_policy_backend_instancing_evidence_required=1`, `scene_scale_policy_backend_instancing_evidence_ready=1`, `d3d12_instanced_draw_execution_status=ready`, `d3d12_instanced_draw_execution_expected_instances=6`, `d3d12_instanced_draw_calls=10`, `d3d12_instanced_indexed_draw_calls=10`, `d3d12_instanced_instances_submitted=30`, `d3d12_instanced_draws_ok=1`, `d3d12_instanced_instances_ok=1`, `rhi_instanced_draw_calls=10`, `rhi_instanced_indexed_draw_calls=10`, and `rhi_instanced_instances_submitted=30`.
- Phase 3 agent-surface evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/src/null_rhi.cpp,engine/rhi/d3d12/src/d3d12_backend.cpp,engine/rhi/vulkan/src/vulkan_backend.cpp,engine/renderer/src/rhi_frame_renderer.cpp,engine/renderer/src/rhi_postprocess_frame_renderer.cpp,engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp,engine/scene_renderer/src/scene_renderer.cpp,engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp,games/sample_desktop_runtime_game/main.cpp,tests/unit/rhi_tests.cpp,tests/unit/d3d12_rhi_tests.cpp,tests/unit/runtime_host_sdl3_tests.cpp`, and `git diff --check` passed; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 3 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 67/67 CTest tests. Apple/Metal checks remained diagnostic/host-gated on this Windows host; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 3 published through PR #159, merge commit `e459ef612b7ce34146ed9dad3369428647791038`; hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks passed while `unsupportedProductionGaps = []` stayed empty.
