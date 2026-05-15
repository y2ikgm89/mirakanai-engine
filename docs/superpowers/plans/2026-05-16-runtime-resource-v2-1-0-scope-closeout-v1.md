# Runtime Resource v2 1.0 Scope Closeout Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the `runtime-resource-v2` 1.0 foundation follow-up by recording the implemented reviewed safe-point, residency, streaming, and hot-reload surfaces without broadening claims beyond evidence.

**Architecture:** `MK_runtime` remains the host-owned cooked package, resident mount/cache, and reviewed safe-point orchestration layer. Native watcher ownership, broad/background streaming, arbitrary eviction policy, renderer/RHI lifetime ownership, upload/staging pools, allocator/GPU enforcement, and native handles stay outside this runtime-resource closeout and remain covered by explicit renderer/RHI, upload/staging, or post-1.0 exclusions.

**Tech Stack:** C++23 MIRAIKANAI runtime/resource APIs, `MK_tools` hot-reload orchestration APIs, composed `engine/agent/manifest.json`, PowerShell validation scripts.

---

**Plan ID:** `runtime-resource-v2-1-0-scope-closeout-v1`

**Status:** Completed.

**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)

**Gap:** `runtime-resource-v2`

## Context

- The `runtime-resource-v2` burn-down now has focused evidence for generation-checked handles, safe-point package replacement/unload, selected package-streaming safe points, explicit resident mount ids, resident catalog cache reuse/rollback, resident unmount/replace, reviewed eviction planning/commit, reviewed `.geindex` discovery and selected candidate load, reviewed candidate/discovery resident mount/replace with optional reviewed evictions, and reviewed hot-reload recook-to-resident replacement composition.
- The latest child, [2026-05-16-runtime-hot-reload-registered-asset-watch-tick-v1.md](2026-05-16-runtime-hot-reload-registered-asset-watch-tick-v1.md), adds caller-owned registered asset scanning into the hot-reload tracker/scheduler and keeps retry rows intact on recook/runtime replacement failure.
- The remaining words in the old unsupported row are not all `runtime-resource-v2` blockers: renderer/RHI resource lifetime belongs to `renderer-rhi-resource-foundation`, upload rings/staging pools belong to `upload-staging-v1`, production render graph ownership belongs to `frame-graph-v1`, and broad native watcher/background streaming/productized hot reload is outside the Engine 1.0 runtime-resource ready claim unless a later roadmap explicitly accepts it.

## Constraints

- Do not change runtime C++ behavior in this closeout slice.
- Do not claim native file watcher ownership, automatic target inference, package scripts, background workers, arbitrary/LRU eviction, allocator/GPU budget enforcement, renderer/RHI ownership, upload/staging integration, native handles, or broad hot-reload productization.
- Keep selected host-owned package streaming, reviewed resident mount/cache operations, and reviewed hot-reload safe points described as explicit safe-point/controller surfaces, not autonomous middleware.
- Keep `renderer-rhi-resource-foundation`, `upload-staging-v1`, and `frame-graph-v1` unsupported gap rows honest after removing `runtime-resource-v2`.

## Done When

- `runtime-resource-v2` is removed from `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json.unsupportedProductionGaps`, and the composed manifest is regenerated.
- The manifest `recommendedNextPlan.completedContext` records the closeout and points the next Windows-default foundation gap to `renderer-rhi-resource-foundation`.
- The plan registry, master plan, phase-1 order ledger, roadmap, current capabilities, AI game-development guide, and testing notes agree that the runtime-resource 1.0 scope is closed without broadening native watcher, background streaming, renderer/RHI, upload/staging, allocator, or native-handle claims.
- Static checks prove the manifest is valid and the production readiness audit still lists the remaining unsupported rows.
- Full `validate.ps1` and `build.ps1` pass before publication, or an exact blocker is recorded.

## Tasks

- [x] **Task 1: Update the machine-readable runtime-resource closeout.**
  Remove the `runtime-resource-v2` unsupported gap row, keep the remaining renderer/RHI, upload/staging, and frame-graph rows unchanged except for any needed notes, and regenerate `engine/agent/manifest.json` from fragments.

- [x] **Task 2: Synchronize human-readable execution guidance.**
  Update the registry, master plan, phase-1 linked-slice ledger, roadmap, current capabilities, AI game-development guide, and testing notes so the next selected gap is `renderer-rhi-resource-foundation` and broad runtime-resource non-goals stay explicit.

- [x] **Task 3: Validate and publish the closeout.**
  Run manifest composition, JSON contracts, AI integration checks, agent checks, production-readiness audit, formatting, full validation, and build, then commit, push, and open a PR with the evidence.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass | Regenerated `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | Manifest compose and JSON contracts agree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Agent-facing needles agree after stale manifest guidance was restored to current fields. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass | Agent instruction, skill, subagent, Cursor pointer, and text-format contracts pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Reports `unsupported_gaps=9`; `runtime-resource-v2` is no longer listed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Text and source formatting pass after LF normalization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passes with 65/65 CTest tests; Metal/Apple checks remain host-gated diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Dev build succeeds; existing MSBuild shared-intermediate-directory warnings remain warnings only. |

## Agent Surface Drift Review

- Reviewed `AGENTS.md`, `CLAUDE.md`, `.agents/skills`, `.claude/skills`, `.cursor/skills`, `.codex/rules`, `.codex/agents`, `.claude/agents`, `docs/agent-operational-reference.md`, and `docs/workflows.md` for stale `runtime-resource-v2` closeout guidance.
- No durable updates were needed in `AGENTS.md`, skills, rules, or subagents for this closeout; the required changes were limited to manifest fragments, generated manifest, docs, plan registry, and static guards.
- Historical completed plans under `docs/superpowers/plans/2026-05-01-*` still mention the original Runtime Resource v2 foundation because they are preserved implementation evidence, not current execution guidance.

## Next Candidate After Validation

- Continue the Phase 1 foundation order with `renderer-rhi-resource-foundation`.
