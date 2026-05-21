# Renderer GPU Memory v1 (2026-05-21)

**Plan ID:** `renderer-gpu-memory-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is renderer 1.x developer-owned capability work, not a reopened Engine 1.0 production gap.

## Goal

Make GPU memory pressure explicit so generated 2D/3D games can request deterministic backend-neutral allocator-budget, residency-classification, transient-heap/aliasing-policy, and upload/staging-pressure diagnostics with package-visible counters and safe failure/remediation rows, before any broader renderer-quality claim and without exposing native backend handles.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `renderer-scene-scale-v1` completed through PR #159 for backend-neutral scene-scale policy diagnostics, package-visible scene-scale counters, selected D3D12 instanced draw execution evidence, hosted checks, and full validation evidence.
- Existing renderer/runtime foundations include package-visible framegraph/render-pass/multi-queue counters, upload staging staging-pool lease adoption, transient texture alias planning, D3D12 placed-resource alias evidence, and strict Vulkan/Metal host gates.
- The renderer advanced production track lists `renderer-gpu-memory-v1` after modern materials, lighting/shadows, postprocess, and scene-scale.
- Official documentation consulted for this milestone:
  - Microsoft Direct3D 12 memory management guidance (`IDXGIAdapter3::QueryVideoMemoryInfo`, `ID3D12Device::MakeResident` / `Evict` / `SetResidencyPriority`, `ID3D12Heap`, `CreatePlacedResource`, residency and budget recovery) for explicit GPU memory budgets, transient heap policy, and residency reasoning.
  - Khronos Vulkan memory documentation (`VkPhysicalDeviceMemoryProperties`, `VK_EXT_memory_budget` with `VkPhysicalDeviceMemoryBudgetPropertiesEXT`, host-visible vs device-local heap selection, dedicated allocations) for memory heap budgets, layout/heap selection, and validation-layer alignment.
  - Apple Metal memory documentation (`MTLDevice.recommendedMaxWorkingSetSize`, `MTLDevice.currentAllocatedSize`, `MTLHeap`, `MTLPurgeableState`) for Apple-host-gated GPU memory budget claims.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep public renderer/game APIs backend-neutral. Native D3D12, Vulkan, Metal, descriptor, PSO, queue, command-buffer, fence, heap, residency, capture, and profiler handles stay backend-private or behind explicit opaque interop plans.
- Do not claim Vulkan/Metal readiness from D3D12 evidence; do not claim automatic LRU eviction or background streaming readiness from structural budget reporting.
- Performance and headroom claims require measurement. Allocator-budget, residency-classification, transient-heap, and upload-pressure rows may report deterministic policy intent, budget headroom, and used-byte counters, but not asynchronous reclamation, frame-time wins, or stream-on-demand readiness without explicit further plans.
- Keep GPU memory work reusable across gameplay families; game-specific texture budgets, art-direction memory choices, and content streaming heuristics stay game-owned.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Close the completed scene-scale active pointer and select `renderer-gpu-memory-v1` as the next active developer-owned renderer 1.x capability.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `renderer-scene-scale-v1` as completed.
- `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, and `docs/roadmap.md` identify this plan as active without reopening production gaps.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: GPU Memory Policy Diagnostics

**Status:** Pending.

### Goal

Add the smallest backend-neutral value contract that classifies GPU memory requests, supported allocator budgets, residency requirements, transient-heap and upload-pressure expectations, and fail-closed diagnostics before backend execution.

### Done When

- RED tests fail first for missing GPU memory diagnostics.
- Public value rows distinguish valid GPU memory budget requests, unsupported residency/heap/streaming claims, invalid byte budgets, missing scene resources, and host-gated backend evidence.
- Focused tests prove deterministic classification without native handles, package mutation, automatic eviction, live shader generation, or broad performance claims.

## Phase 2: Package-Visible GPU Memory Evidence

**Status:** Pending.

### Goal

Expose deterministic GPU memory policy and budget/used-byte/transient/upload-pressure counters in selected desktop runtime package lanes so AI agents can verify the supported contract from package output.

### Done When

- Sample/package smoke reports deterministic GPU memory policy readiness, diagnostic counters, declared budget bytes, used bytes, transient/heap allocation counts, upload-staging pressure rows where implemented, and host-gated backend evidence rows.
- Installed desktop runtime validation checks those counters for the selected package target.
- Docs, manifest fragments, generated-game guidance, and static checks describe the exact supported GPU memory claims and host gates.

## Phase 3: Backend-Gated GPU Memory Evidence

**Status:** Pending.

### Goal

Promote only backend-specific GPU memory budget/residency evidence with fresh official-doc-aligned validation, starting with D3D12 and keeping Vulkan/Metal separately host-gated unless their own strict evidence lands.

### Done When

- D3D12 package evidence is backed by focused RHI/device-context/package validation and explicit `IDXGIAdapter3::QueryVideoMemoryInfo` budget, heap usage, transient-heap allocation, and residency/upload-pressure reasoning aligned with Microsoft Direct3D 12 memory documentation.
- Vulkan and Metal rows remain host-gated unless their own strict validation evidence is recorded.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 started after `renderer-scene-scale-v1` completed through PR #159, merge commit `e459ef612b7ce34146ed9dad3369428647791038`, hosted PR Gate, Windows MSVC, Full Repository Static Analysis shards, Linux, CodeQL, iOS, and macOS Metal CMake checks, plus local full `tools/validate.ps1` evidence while `unsupportedProductionGaps = []` stayed empty.
- Phase 0 pointer sync selected this plan in `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`, `docs/roadmap.md`, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; composed `engine/agent/manifest.json` reports `currentActivePlan=docs/superpowers/plans/2026-05-21-renderer-gpu-memory-v1.md`, `recommendedNextPlan.id=renderer-gpu-memory-v1`, and `unsupportedProductionGaps = []`.
- Phase 0 static evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `git diff --check` passed with `unsupported_gaps=0`.
