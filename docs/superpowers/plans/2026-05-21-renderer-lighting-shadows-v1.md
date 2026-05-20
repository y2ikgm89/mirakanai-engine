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

**Status:** Planned.

### Goal

Add the smallest backend-neutral value contract that can classify generated-scene light inputs, multi-light budgets, shadow-map policy rows, and fail-closed diagnostics before renderer backend execution.

### Done When

- RED tests fail first for missing lighting/shadow diagnostics.
- Public value rows distinguish valid lights, over-budget lights, invalid light parameters, unsupported shadow policy claims, and host-gated backend evidence.
- Focused tests prove deterministic classification without native handles, package mutation, live shader generation, or broad visual-quality claims.

## Phase 2: Package-Visible Lighting Evidence

**Status:** Planned.

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
