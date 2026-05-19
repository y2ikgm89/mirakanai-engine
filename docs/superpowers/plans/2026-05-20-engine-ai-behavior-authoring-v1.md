# Engine AI Behavior Authoring v1 (2026-05-20)

**Plan ID:** `engine-ai-behavior-authoring-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned gameplay/AI capability work, not a reopened Engine 1.0 production gap.

## Goal

Add a first-party AI behavior authoring foundation so generated games can describe reusable behavior documents, validate blackboard/perception dependencies, run deterministic traces, and surface package-visible AI counters without arbitrary scripting or game-specific behavior engines in core engine code.

## Context

- `gameplay-authoring-foundation-v1` closed scene gameplay binding and interaction primitives.
- `gameplay-physics-navigation-ai-foundation-v1` already proved selected physics, navigation, navmesh, local-avoidance, and AI runtime composition counters in 2D/3D packages.
- `sprite-animation-flipbook-v1` completed the 2D authoring family; the next user-prioritized gameplay/AI follow-up is durable AI behavior authoring rather than another renderer or 2D capability.

## Constraints

- Keep arbitrary script execution, external behavior middleware, native handles, editor productization, and game-specific combat/story logic out of engine-owned capability.
- Keep documents deterministic, data-oriented, versioned, and validated before runtime execution.
- Reuse existing `MK_ai` perception, blackboard, and behavior-tree execution surfaces where possible.
- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned gameplay/AI capability after `sprite-animation-flipbook-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, and the production master-plan index list this plan as the active developer-owned milestone.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Behavior Document Contract

**Status:** Completed.

### Goal

Define the smallest reusable behavior-authoring document contract over existing behavior-tree, blackboard, and perception concepts, with deterministic validation diagnostics and no script execution.

### Done When

- RED tests or static guards fail first for missing behavior authoring document validation.
- `MK_ai` exposes value-only document/validation/result rows for reusable generated-game behavior authoring.
- Focused tests prove invalid node ids, missing blackboard keys, unsupported action rows, duplicate behavior ids, and deterministic trace ordering.

## Phase 2: Package Trace Evidence

**Status:** Pending.

### Goal

Adopt the behavior authoring contract in selected generated gameplay package paths so package smokes can report deterministic behavior document validation/execution counters.

### Done When

- A committed sample or generated package path reports package-visible behavior authoring counters through existing validation recipes.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable workflow or AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync: this plan was selected after `sprite-animation-flipbook-v1` completed package-visible flipbook counters, while `unsupportedProductionGaps = []` stayed empty.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ai_tests` failed first because `BehaviorAuthoringDocument`, `BehaviorAuthoringValidationContext`, and `validate_behavior_authoring_document` did not exist.
- Phase 1 focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ai_tests` passed after the value-only behavior authoring contract was implemented.
- Phase 1 focused tests: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_ai_tests` passed.
- Phase 1 static checks: `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed.
- Phase 1 full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed.
