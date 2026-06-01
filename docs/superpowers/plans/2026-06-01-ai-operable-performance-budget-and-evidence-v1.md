# 2026-06-01 AI-Operable Performance Budget And Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an AI-readable performance budget and evidence contract before broader optimization work spreads through generated games, renderer paths, package workflows, and CPU/GPU execution lanes.

**Architecture:** Keep performance readiness descriptor-first. `game.agent.json` declares selected budgets, validation recipes, evidence rows, host gates, and unsupported claims; engine agent manifest exposes the production loop. No runtime allocator, scheduler, renderer/RHI, CUDA/HIP, Metal, or broad optimized-game claim becomes ready from this slice.

**Tech Stack:** JSON Schema draft 2020-12, PowerShell 7 repository wrappers, composed `engine/agent/manifest.json`, `game.agent.json`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and retained official-research spec `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.

---

**Plan ID:** `ai-operable-performance-budget-and-evidence-v1`

**Status:** Active.

Selected on 2026-06-01 after the optimization foundation research concluded that budget/evidence contracts must precede broad CPU/GPU/memory optimization implementation.

## Official Practice Check

- Context7 JSON Schema draft 2020-12 guidance was rechecked for object `properties`, `required`, and `additionalProperties: false` so new machine-readable budget rows fail closed.
- The retained optimization research spec records official anchors for Intel and AMD CPU optimization manuals, NVIDIA/AMD/Intel GPU performance guidance, D3D12/Vulkan/Metal documentation, C++ Core Guidelines, PGO/LTO documentation, sanitizers/profilers, and memory/allocator practice. This plan turns that research into an enforceable contract rather than claiming implementation.

## Context

- Parent selection gate: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- Retained analysis record: `docs/specs/2026-06-01-engine-performance-optimization-foundation-v1.md`.
- Existing narrow evidence already includes sandbox package budget counters, renderer quality rows, package upload staging counters, GPU memory/residency diagnostics, and package streaming residency descriptors. This plan does not broaden those rows into an optimized-game claim.

## Constraints

- Keep `unsupportedProductionGaps = []`; this is a focused capability selection, not a reopened production gap.
- Do not implement broad thread scheduling, job-system work stealing, allocator enforcement, GPU-driven rendering, CUDA/HIP paths, or renderer/RHI residency execution in this slice.
- Do not generalize D3D12 evidence to Vulkan, Metal, AMD, Intel, or NVIDIA parity without fresh host evidence.
- Do not expose native handles or public backend objects.
- Preserve clean-break policy; add the current contract directly with no compatibility aliases.

## Task 1 - Game Manifest Budget Contract

**Files:**
- Modify: `schemas/game-agent.schema.json`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `tools/new-game-templates.ps1`
- Modify: `tools/check-json-contracts-020-game-contracts.ps1`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`

- [x] Add optional root `performanceBudgets` to `game.agent.json` schema with `budgetRows`, `evidenceRows`, `validationRecipeIds`, `hostGateId`, and explicit `unsupportedClaims`.
- [x] Add selected 2D and 3D sample budget rows that reference existing package smoke and residency evidence without claiming broad runtime performance.
- [x] Update 2D/3D game templates so new generated package games carry descriptor-first budget/evidence rows.
- [x] Add static checks for required fields, referenced budget ids, referenced validation recipe ids, and required unsupported claims.
- [x] Document how AI agents should use `performanceBudgets` before claiming an optimized game.

## Task 2 - Engine Agent Production Loop Visibility

**Files:**
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `schemas/engine-agent/ai-operable-production-loop.schema.json`
- Generate: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`

- [x] Select this active plan in `currentActivePlan` and `recommendedNextPlan` while preserving `unsupportedProductionGaps = []`.
- [x] Add `performanceBudgetEvidenceLoops` to the engine manifest schema and composed manifest.
- [x] Add AI-integration needles so budget/evidence docs and manifest rows cannot drift.
- [x] Update plan registry and roadmap to point at the active child plan.

## Task 3 - Validation And Publication

**Files:**
- Verify: `tools/compose-agent-manifest.ps1 -Write`
- Verify: `tools/check-text-format.ps1`
- Verify: `tools/check-json-contracts.ps1`
- Verify: `tools/check-ai-integration.ps1`
- Verify: `tools/check-agents.ps1`
- Verify: `tools/validate.ps1`
- Verify: `git diff --check`

- [x] Compose the engine agent manifest.
- [x] Run focused static checks and full repository validation because schema/manifest public contracts change.
- [ ] Commit the validated candidate, run publication preflight, push the branch, and open a PR with validation evidence.

Validation evidence on 2026-06-01: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-text-format.ps1`, `git diff --check`, and `tools/validate.ps1` passed in the candidate worktree.

## Done When

- `game.agent.json.performanceBudgets` is schema-backed, documented, sampled, template-generated, and statically checked.
- `engine/agent/manifest.json.aiOperableProductionLoop.performanceBudgetEvidenceLoops` exposes the AI production loop.
- AI agents can distinguish ready narrow budget evidence from host-gated trace/profiler evidence and unsupported broad optimization claims.
- Validation evidence is recorded before PR publication.
