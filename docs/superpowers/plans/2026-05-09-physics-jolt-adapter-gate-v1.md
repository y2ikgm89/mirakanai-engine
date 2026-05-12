# Physics Jolt Adapter Gate Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Decide and encode the Physics 1.0 Jolt/native adapter gate after the first-party collision/query/contact/CCD/character/joint/determinism surface is proven.

**Architecture:** Keep the default Physics 1.0 ready surface first-party and deterministic. This phase must either add a fail-closed adapter readiness contract with dependency/legal/vcpkg/notice coverage, or explicitly exclude Jolt/native middleware from the 1.0 ready claim with machine-readable manifest evidence.

**Tech Stack:** C++23, `mirakana_physics`, `engine/agent/manifest.json`, dependency/legal docs, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-license.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

**Plan ID:** `physics-jolt-adapter-gate-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `physics-1-0-collision-system` Phase P3.  
**Previous Slice:** [2026-05-09-physics-benchmark-determinism-gates-v1.md](2026-05-09-physics-benchmark-determinism-gates-v1.md)

## Context

- First-party Physics 1.0 now has package-authored collision rows, exact current-primitive queries, deterministic contact manifolds, opt-in CCD rows, character/dynamic policy rows, explicit distance-joint solving, and count-based determinism gates.
- The advanced backend evaluation keeps Jolt as the preferred future optional adapter candidate, but no middleware backend is integrated into the current ready surface.
- This phase is the decision gate before the final `physics-1-0-collision-system` closeout.

## Constraints

- Do not add Jolt or any third-party dependency unless `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` are updated and validated.
- Do not expose native physics handles, middleware types, or backend-specific ownership through public gameplay APIs.
- If the adapter is excluded from Physics 1.0, record that exclusion in `engine/agent/manifest.json`, docs, skills, and static checks without weakening first-party ready evidence.
- Keep dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, persistent joint assets, vehicles, ragdolls, oriented boxes, mesh/convex casts, editor physics debugging UX, and broad middleware parity outside this plan unless explicitly accepted by updated requirements and tests.

## Done When

- The repository has a reviewed, machine-readable Jolt/native adapter decision for Physics 1.0: either a fail-closed adapter gate with dependency/legal validation, or an explicit 1.0 exclusion.
- `physics-1-0-collision-system` remains non-ready until the closeout plan synchronizes the final ready surface.
- Docs, manifest, skills, registry, and static checks describe the same adapter decision and remaining unsupported physics scope.
- Relevant focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-license.ps1` if dependencies change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## Task 1: Adapter Decision Contract

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/specs/2026-04-27-physics-backend-evaluation.md`
- Modify as needed: `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, `vcpkg.json`

- [x] Decide whether Physics 1.0 includes an optional Jolt/native adapter gate or explicitly excludes middleware/native backends from the ready surface.
- [x] If adding a dependency, update dependency, legal, vcpkg, and notice records before any ready claim.
- [x] If excluding the adapter, record the exclusion as an intentional 1.0 scope decision in manifest notes and docs.

## Task 2: Tests And Static Checks

**Files:**
- Modify as needed: `tools/check-ai-integration.ps1`
- Modify as needed: `tools/check-json-contracts.ps1`
- Modify as needed: `tests/unit/core_tests.cpp`

- [x] Add or update tests/static checks so the adapter decision cannot drift from the manifest and docs.
- [x] Keep native handles and middleware types out of public gameplay APIs unless an explicit adapter API is accepted and validated.
- [x] Run the smallest focused checks that cover the selected decision.

## Task 3: Docs, Manifest, Skills, And Validation

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

- [x] Update capability language only after the adapter decision is validated.
- [x] Move the active plan pointer to `physics-1-0-collision-system-closeout-v1` only after this P3 decision is complete.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Record validation evidence and mark this plan completed.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; verifies one active child plan, P3 completed, P4 active, adapter decision drift checks, and no dry-run game leftovers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`; verifies `physicsBackendAdapterDecisions` schema and P4 active-plan drift. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; `unsupported_gaps=12`, `physics-1-0-collision-system` remains `partly-ready` with two required closeout claims. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 29/29 CTest tests passed. Windows host keeps Metal/iOS checks diagnostic or host-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default `dev` build completed successfully. |
| `git diff --check -- ...` | PASS | No whitespace errors for the touched agent, docs, manifest, plan, and static-check files; Git reported only CRLF normalization warnings. |

## Self-Review

- Scope is adapter readiness or explicit exclusion, not broad middleware parity.
- The default ready surface must remain first-party and dependency-free unless the dependency/legal gate is intentionally opened.
- The active pointer must remain synchronized with `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
