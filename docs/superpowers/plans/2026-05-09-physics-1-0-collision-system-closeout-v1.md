# Physics 1.0 Collision System Closeout Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the `physics-1-0-collision-system` production gap by synchronizing the final first-party Physics 1.0 ready surface, explicit exclusions, manifest status, docs, skills, and validation checks.

**Architecture:** Keep the ready surface first-party, deterministic, dependency-free, and limited to the implemented public `mirakana::` physics contracts. Do not add middleware, native handles, or broad unsupported physics features during closeout.

**Tech Stack:** C++23, `engine/agent/manifest.json`, docs/static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.

---

**Plan ID:** `physics-1-0-collision-system-closeout-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `physics-1-0-collision-system` Phase P4.  
**Previous Slice:** [2026-05-09-physics-jolt-adapter-gate-v1.md](2026-05-09-physics-jolt-adapter-gate-v1.md)

## Context

- Physics Scene Package Collision Authoring, broader exact casts/sweeps, contact manifold stability, CCD foundation, character/dynamic policy, joints foundation, benchmark determinism gates, and the Jolt/native adapter exclusion decision are complete.
- Jolt/native middleware is intentionally excluded from the Physics 1.0 ready surface and remains a future optional adapter gate.
- The closeout decision is that dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, oriented boxes, mesh/convex casts, persistent joint assets, vehicles, ragdolls, timing benchmarks, editor physics debugging UX, and Jolt/native middleware stay outside the first-party Physics 1.0 ready surface.

## Constraints

- Do not broaden the ready surface beyond implemented, validated first-party APIs.
- Do not add dependencies, native handles, middleware types, timing benchmark claims, or editor physics debugging UX in this closeout.
- Keep all docs, skills, manifest rows, and static checks aligned with the same ready/excluded boundary.
- If the gap remains non-ready, record the exact blocker instead of claiming closeout.

## Done When

- The Physics 1.0 ready surface and exclusions are explicitly recorded in `engine/agent/manifest.json`, docs, skills, and static checks.
- `production-readiness-audit-check` no longer reports `physics-1-0-collision-system` as a non-ready row, or records a concrete blocker if closeout cannot be accepted.
- The master plan and plan registry move to the next production gap only after this closeout evidence is validated.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## Task 1: Ready Surface Closeout

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`

- [x] Decide whether the implemented first-party Physics 1.0 surface is ready with explicit exclusions.
- [x] Remove or reclassify the `physics-1-0-collision-system` `unsupportedProductionGaps` row only when the ready/exclusion boundary is fully synchronized.
- [x] Keep native/middleware backends, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, oriented boxes, mesh/convex casts, persistent joint assets, vehicles, ragdolls, timing benchmarks, and editor physics debugging UX out of the 1.0 ready claim unless separate evidence exists.

## Task 2: Docs, Skills, And Registry

**Files:**
- Modify: `docs/architecture.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Update docs and skills to describe the same ready surface and excluded features.
- [x] Move the active plan pointer to the next production gap only after closeout validation passes.
- [x] Preserve completed child plans as historical evidence.

## Task 3: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Record validation evidence and mark this plan completed.

## Self-Review

- This plan is closeout only; it must not add new physics behavior.
- The closeout claim must be backed by manifest/static-check evidence, not prose alone.
- If any ready-surface requirement remains ambiguous, keep the gap non-ready and record the blocker.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; dry-run game scaffolds were cleaned up. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `unsupported_gaps=11`; `physics-1-0-collision-system` is no longer reported. |
| `git diff --check -- ...` | PASS | No whitespace errors; Git reported CRLF normalization warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest passed 29/29. Metal/iOS remain Windows host-gated diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev preset configure/build completed with MSBuild 17.14.23. |
