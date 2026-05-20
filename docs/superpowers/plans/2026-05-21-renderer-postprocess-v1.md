# Renderer Postprocess v1 (2026-05-21)

**Plan ID:** `renderer-postprocess-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is renderer 1.x developer-owned capability work, not a reopened Engine 1.0 production gap.

## Goal

Add production postprocess building blocks so generated 3D games can request deterministic tone/exposure/bloom/color/fog/AA policy rows and package-visible framegraph/postprocess diagnostics without relying on native backend handles or subjective visual-quality claims.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `renderer-lighting-shadows-v1` completed through PR #151 for backend-gated D3D12 shadow cascade/atlas/light-space package evidence.
- Existing renderer foundations include Frame Graph v1 pass rows, render pass envelopes, package-visible postprocess/depth counters, D3D12/Vulkan host-gated package smokes, and strict host gates for Vulkan/Metal parity.
- The renderer advanced production track lists `renderer-postprocess-v1` after lighting/shadows.
- Official documentation consulted for this milestone: Microsoft Direct3D 12 resource barrier guidance through Context7 (`/websites/learn_microsoft_en-us_windows_win32_direct3d12`), Khronos Vulkan synchronization examples through Context7 (`/khronosgroup/vulkan-docs`), Apple Metal load/store and resource synchronization documentation, and Metal Shading Language fragment/texture guidance through Context7.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep public renderer/game APIs backend-neutral. Native D3D12, Vulkan, Metal, descriptor, PSO, queue, shader-module, and capture handles stay backend-private or behind explicit opaque interop plans.
- Do not claim Vulkan/Metal readiness from D3D12 evidence.
- Keep postprocess quality claims evidence-backed: deterministic rows and counters first; subjective image quality, temporal upscalers, vendor SDKs, and cinematic parity are out of scope until separately planned.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Close the completed lighting/shadows active pointer and select `renderer-postprocess-v1` as the next active developer-owned renderer 1.x capability.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `renderer-lighting-shadows-v1` as completed.
- `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` and `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md` identify this plan as active without reopening production gaps.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Postprocess Chain Policy Diagnostics

**Status:** Completed.

### Goal

Add the smallest backend-neutral value contract that classifies postprocess chain requests, supported tone/exposure/bloom/color/fog/AA options, framegraph pass expectations, and fail-closed diagnostics before backend shader execution.

### Done When

- RED tests fail first for missing postprocess chain diagnostics.
- Public value rows distinguish valid postprocess chain requests, unsupported effects, invalid budgets, missing framegraph resources, and host-gated backend evidence.
- Focused tests prove deterministic classification without native handles, package mutation, live shader generation, or broad visual-quality claims.

## Phase 2: Package-Visible Postprocess Evidence

**Status:** Completed.

### Goal

Expose deterministic postprocess policy and framegraph counters in the selected desktop runtime package lane so AI agents can verify the supported contract from package output.

### Done When

- Sample/package smoke reports deterministic postprocess policy readiness/diagnostic counters.
- Installed desktop runtime validation checks those counters for the selected package target.
- Docs, manifest fragments, generated-game guidance, and static checks describe the exact supported postprocess claims and host gates.

## Phase 3: Backend-Gated Shader and Pass Proof

**Status:** Pending.

### Goal

Promote only backend-specific postprocess execution evidence with fresh official-doc-aligned validation, starting with D3D12 and keeping Vulkan/Metal separately host-gated unless their own strict evidence lands.

### Done When

- D3D12 package evidence is backed by focused shader/toolchain/package validation and explicit resource-state/render-pass/load-store reasoning.
- Vulkan and Metal rows remain host-gated unless their own strict validation evidence is recorded.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 started after `renderer-lighting-shadows-v1` completed through PR #151, merge commit `72c7539b71c34d4ed5a1865190b3cc92153167dc`, hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks, plus local full `tools/validate.ps1` evidence while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-renderer-postprocess-v1.md`, `recommendedNextPlan.id=renderer-postprocess-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
- Phase 1 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed first with `fatal error C1083: Cannot open include file: 'mirakana/renderer/postprocess_policy.hpp'`.
- Phase 1 implemented backend-neutral `PostprocessChainPolicyDesc`, `PostprocessEffectDesc`, `PostprocessChainPolicyPlan`, diagnostics, and `plan_postprocess_chain_policy` for tone/exposure/bloom/color/fog/AA classification, frame extent/resource/budget checks, host-gated backend shader evidence, and deterministic framegraph pass/barrier expectations without native backend handles.
- Phase 1 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/postprocess_policy.cpp,tests/unit/renderer_rhi_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` passed; production readiness reported `unsupported_gaps=0`.
- Phase 1 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 67/67 CTest tests, `unsupported_gaps=0`, and expected Windows host-gated Apple/Metal diagnostic-only blockers.
- Phase 1 published through PR #152, merge commit `a1c7478092f6e4a841fc0e93117fa60c69ac1e2c`; hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks passed.
- Phase 2 RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests` failed first because `evaluate_sdl_desktop_presentation_postprocess_policy` and `SdlDesktopPresentationPostprocessPolicyStatus` did not exist.
- Phase 2 implemented `SdlDesktopPresentationPostprocessPolicyReport`, `evaluate_sdl_desktop_presentation_postprocess_policy`, and sample/installed package smoke fields for `postprocess_policy_status=ready`, `postprocess_policy_ready=1`, diagnostics, effect/pass/framegraph/barrier counts, scene color/depth requirements, selected color-grading effect, and backend shader-evidence readiness over the existing selected package postprocess path.
- Phase 2 focused evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests sample_desktop_runtime_game`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_host_sdl3_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp,games/sample_desktop_runtime_game/main.cpp,tests/unit/runtime_host_sdl3_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` passed; production readiness reported `unsupported_gaps=0`.
- Phase 2 full gate evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 67/67 CTest tests, `unsupported_gaps=0`, and expected Windows host-gated Apple/Metal diagnostic-only blockers.
