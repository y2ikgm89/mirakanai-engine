# Renderer RHI Resource Foundation 1.0 Scope Closeout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close `renderer-rhi-resource-foundation` for the Engine 1.0 Windows-default renderer/RHI foundation surface without broadening frame graph, upload/staging, Metal, allocator-enforcement, native-handle, or renderer-quality claims.

**Architecture:** This is a docs/manifest/static-guard closeout over already landed RHI evidence. `MK_rhi` owns backend-neutral lifetime records, D3D12/Vulkan deferred native teardown, GPU markers/timestamps, and read-only `RhiDeviceMemoryDiagnostics`; `frame-graph-v1` and `upload-staging-v1` remain separate unsupported gap rows until their production ownership and native async/package integration claims are validated.

**Tech Stack:** C++23 repository contracts, `engine/agent/manifest.fragments`, `tools/check-ai-integration*.ps1`, `tools/check-json-contracts*.ps1`, docs/roadmap/current capabilities/testing/AI game-development guidance.

---

## Context

- Parent roadmap: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md).
- Registry state before this slice: `runtime-resource-v2` is closed and the next selected Phase 1 gap is `renderer-rhi-resource-foundation`.
- Implementation evidence already landed in:
  - [2026-05-01-renderer-rhi-resource-foundation-v1.md](2026-05-01-renderer-rhi-resource-foundation-v1.md)
  - [2026-05-03-native-rhi-resource-lifetime-migration-v1.md](2026-05-03-native-rhi-resource-lifetime-migration-v1.md)
  - [2026-05-03-gpu-marker-and-timestamp-adapter-v1.md](2026-05-03-gpu-marker-and-timestamp-adapter-v1.md)
  - [2026-05-03-allocator-and-residency-diagnostics-v1.md](2026-05-03-allocator-and-residency-diagnostics-v1.md)
  - [2026-05-07-editor-resource-panel-diagnostics-v1.md](2026-05-07-editor-resource-panel-diagnostics-v1.md)
  - [2026-05-07-editor-resource-capture-request-v1.md](2026-05-07-editor-resource-capture-request-v1.md)
  - [2026-05-07-editor-resource-capture-execution-evidence-v1.md](2026-05-07-editor-resource-capture-execution-evidence-v1.md)
  - [2026-05-09-rhi-resource-lifetime-retired-handle-invalid-contract-v1.md](2026-05-09-rhi-resource-lifetime-retired-handle-invalid-contract-v1.md)

## Constraints

- Do not add renderer/RHI gameplay APIs, native handles, compatibility aliases, or migration shims.
- Do not mark `frame-graph-v1`, `upload-staging-v1`, `3d-playable-vertical-slice`, `2d-playable-vertical-slice`, broad renderer quality, or Metal readiness as complete.
- Keep `engine/agent/manifest.json` composed output only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Update static checks in the same slice so stale unsupported-gap claims fail validation.

## Task 1: Close the manifest row

**Files:**
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Regenerate: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

- [x] Remove `renderer-rhi-resource-foundation` from `unsupportedProductionGaps`.
- [x] Update `recommendedNextPlan.completedContext` to name this closeout and the implemented evidence.
- [x] Update `recommendedNextPlan.reason` to select `frame-graph-v1` as the next Phase 1 foundation gap.
- [x] Update JSON/static guard checks to reject a lingering `renderer-rhi-resource-foundation` unsupported row and require the new closeout evidence text.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

## Task 2: Align current docs and agent guidance

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-05-11-phase1-foundation-gaps-coherent-slice-order-v1.md`
- Modify: `docs/superpowers/plans/2026-05-11-production-completion-gap-stream-plans-index-v1.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify if stale: `.codex/agents/engine-architect.toml`

- [x] Mark `renderer-rhi-resource-foundation` as a completed gap burn-down and `frame-graph-v1` as the active gap.
- [x] Replace stale "native backend destruction migration remains unsupported" wording with D3D12/Vulkan evidence and Metal host-gated limits where current docs are authoritative.
- [x] Keep upload/staging package integration, production render graph ownership, GPU allocator enforcement, Metal IRhiDevice parity, native handles, and general renderer quality in separate remaining rows or explicit non-goals.
- [x] Ensure generated-game guidance continues to forbid direct RHI/native handles and treats renderer/RHI readiness as host-owned infrastructure, not gameplay API surface.

## Task 3: Validate and publish

**Files:**
- Modify this plan's validation table.
- Commit and push this branch.
- Create a PR with validation evidence.

- [x] Run focused checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`
- [x] Run slice-closing checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`
- [x] Update this plan's validation table with exact results.
- [ ] Commit, push, and open a PR.

## Done When

- [x] `renderer-rhi-resource-foundation` is absent from `unsupportedProductionGaps`.
- [x] The next selected Phase 1 gap is `frame-graph-v1`.
- [x] Docs, manifest fragments, composed manifest, static checks, and agent guidance agree.
- [x] Full validation and build pass or a concrete environment blocker is recorded.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass; composed `engine/agent/manifest.json` regenerated | 2026-05-16 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass; `agent-manifest-compose: ok`, `json-contract-check: ok` | 2026-05-16 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass; `agent-config-check: ok` | 2026-05-16 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass; `ai-integration-check: ok` | 2026-05-16 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass; `unsupported_gaps=8`, `production-readiness-audit-check: ok` | 2026-05-16 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass; 65/65 CTest tests passed, diagnostic-only Apple/Metal host gates reported | 2026-05-16 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass; MSBuild completed with existing MSB8028 intermediate-directory warnings | 2026-05-16 |

## Non-Goals

- Production render graph ownership, transient allocation, aliasing, multi-queue scheduling, or renderer-wide manual-transition removal.
- Native async upload execution, package upload streaming integration, or GPU allocator/residency enforcement.
- Metal `IRhiDevice` parity or Apple-host readiness promotion.
- Public native/RHI handles in generated gameplay, editor core, or AI command surfaces.
