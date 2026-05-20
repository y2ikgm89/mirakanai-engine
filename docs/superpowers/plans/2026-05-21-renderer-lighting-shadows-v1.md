# Renderer Lighting Shadows v1 (2026-05-21)

**Plan ID:** `renderer-lighting-shadows-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is renderer 1.x developer-owned capability work, not a reopened Engine 1.0 production gap.

## Goal

Improve scene lighting beyond the current directional shadow smoke so generated 3D games can use deterministic, backend-neutral light-list and shadow policy primitives with package-visible diagnostics before any broader renderer-quality claim.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `renderer-modern-materials-v1` is completed through PR #148 for backend-gated modern material package evidence.
- Existing renderer foundations include Frame Graph v1 execution, D3D12/Vulkan host-gated package smokes, directional shadow smoke/filtering evidence, renderer quality gates, package-visible framegraph counters, and strict host gates for Vulkan/Metal parity.
- The renderer advanced production track lists `renderer-lighting-shadows-v1` after modern materials.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep public renderer/game APIs backend-neutral. Native D3D12, Vulkan, Metal, descriptor, PSO, queue, and shader-module handles stay backend-private or behind explicit opaque interop plans.
- Do not claim Vulkan/Metal readiness from D3D12 evidence.
- Prefer official D3D12, Vulkan, Metal, CMake, and CTest documentation before backend-specific implementation decisions.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Close the completed modern materials active pointer and select `renderer-lighting-shadows-v1` as the next active developer-owned renderer 1.x capability.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `renderer-modern-materials-v1` as completed.
- `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` and `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md` identify this plan as active without reopening production gaps.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Light List and Shadow Policy Diagnostics

**Status:** Completed.

### Goal

Add the smallest backend-neutral value contract that can classify generated-scene light inputs, multi-light budgets, shadow-map policy rows, and fail-closed diagnostics before renderer backend execution.

### Done When

- RED tests fail first for missing lighting/shadow diagnostics.
- Public value rows distinguish valid lights, over-budget lights, invalid light parameters, unsupported shadow policy claims, and host-gated backend evidence.
- Focused tests prove deterministic classification without native handles, package mutation, live shader generation, or broad visual-quality claims.

## Phase 2: Package-Visible Lighting Evidence

**Status:** Completed.

### Goal

Expose package-visible lighting/shadow counters in the generated 3D or desktop runtime sample lane so AI agents can verify the new diagnostics contract from a generated package.

### Done When

- Sample/package smoke reports deterministic light-list and shadow-policy readiness/diagnostic counters.
- Installed desktop runtime validation checks those counters for the selected package target.
- Docs, manifest fragments, generated-game guidance, and static checks describe the exact supported lighting/shadow claims and host gates.

## Phase 3: Backend-Gated D3D12 Proof

**Status:** Planned.

### Goal

Promote only the backend-specific lighting/shadow execution evidence that has fresh official-doc-aligned validation, starting with D3D12 and keeping Vulkan/Metal separately host-gated.

### Done When

- D3D12 package evidence is backed by focused shader/toolchain/package validation.
- Vulkan and Metal rows remain host-gated unless their own strict validation evidence is recorded.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 started after `renderer-modern-materials-v1` completed through PR #148, merge commit `d50e2055c77ea9eca1d99f21e86687a91e8f2572`, hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks, plus local full `tools/validate.ps1` evidence while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-renderer-lighting-shadows-v1.md`, `recommendedNextPlan.id=renderer-lighting-shadows-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
- Phase 1 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_renderer_tests` failed before implementation because `plan_scene_lighting_shadow_policy`, `SceneLightingShadowPolicyDesc`, `LightingShadowPolicyPlan`, `LightingShadowPolicyLightType`, and `has_lighting_shadow_policy_diagnostic` did not exist.
- Phase 1 implementation adds backend-neutral `LightingShadowPolicyDesc`, `LightingShadowPolicyPlan`, `LightingShadowPolicyLightRow`, `LightingShadowPolicyDiagnostic`, and `plan_lighting_shadow_policy` in `MK_renderer`, plus `SceneLightingShadowPolicyDesc` and `plan_scene_lighting_shadow_policy` in `MK_scene_renderer` for converting `SceneRenderPacket` light rows into deterministic light-count, directional/point/spot, shadowed-light, atlas/cascade, and fail-closed diagnostic rows. The contract is value-only and does not render additional lights, mutate packages, expose native/RHI handles, or claim Vulkan/Metal parity.
- Phase 1 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests MK_scene_renderer_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_tests|MK_scene_renderer_tests"`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/shadow_map.cpp,engine/scene_renderer/src/scene_renderer.cpp,tests/unit/renderer_rhi_tests.cpp,tests/unit/scene_renderer_tests.cpp` passed.
- Phase 1 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 67/67 CTest tests and `production-readiness-audit: unsupported_gaps=0`; Apple/Metal host diagnostics remain host-gated/diagnostic-only as before.
- Phase 2 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed before implementation because `games/sample_desktop_runtime_game/main.cpp` was missing `--require-lighting-shadow-policy`.
- Phase 2 implementation adds `sample_desktop_runtime_game --require-lighting-shadow-policy`, adapts the loaded `SceneRenderPacket` through `plan_scene_lighting_shadow_policy`, and reports package-visible counters including `lighting_shadow_policy_status=ready`, `lighting_shadow_policy_ready=1`, `lighting_shadow_policy_diagnostics=0`, `lighting_shadow_policy_lights=1`, `lighting_shadow_policy_directional_lights=1`, `lighting_shadow_policy_shadowed_lights=1`, `lighting_shadow_policy_shadow_atlas_width=1024`, `lighting_shadow_policy_shadow_atlas_height=1024`, `lighting_shadow_policy_light_rows=1`, and `lighting_shadow_policy_ready_frames=2`.
- Phase 2 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_desktop_runtime_game`, `out/build/dev/games/Debug/sample_desktop_runtime_game/sample_desktop_runtime_game.exe --smoke --require-config runtime/sample_desktop_runtime_game.config --require-scene-package runtime/sample_desktop_runtime_game.geindex --require-lighting-shadow-policy`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files games/sample_desktop_runtime_game/main.cpp` passed.
- Phase 2 package/static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
- Phase 2 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 67/67 CTest tests and `production-readiness-audit: unsupported_gaps=0`; Apple/Metal diagnostics remained host-gated/diagnostic-only.
