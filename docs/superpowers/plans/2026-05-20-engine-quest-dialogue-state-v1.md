# Engine Quest Dialogue State v1 (2026-05-20)

**Plan ID:** `engine-quest-dialogue-state-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is developer-owned gameplay-family capability work, not a reopened Engine 1.0 production gap.

## Goal

Add first-party quest/dialogue state primitives so AI-generated narrative, tutorial, progression, adventure, and objective-driven games can validate quest/dialogue documents, advance deterministic objective and dialogue state, and persist game-owned progress without engine-specific rules or arbitrary scripting.

## Context

- `gameplay-authoring-foundation-v1` completed scene gameplay binding and basic interaction planning.
- `engine-save-settings-profile-v1`, `engine-ui-game-menu-hud-v1`, `engine-input-action-contexts-v1`, `engine-audio-gameplay-mixer-v1`, `engine-asset-placeholder-generation-v1`, 2D family enablers, behavior authoring, collision query, navmesh/crowd, and advanced physics controller milestones are completed.
- The developer-owned capability backlog lists `engine-quest-dialogue-state-v1` as the next unimplemented gameplay-family enabler for quests, branching dialogue, objectives, flags, rewards, and save-state integration.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If this becomes an Engine 1.0 blocker, stop.
- Keep story content, quest text, reward logic, combat effects, economy values, and game-specific progression rules in game-owned data/code.
- Keep the engine surface value-only and deterministic: validation rows, state-transition rows, and explicit diagnostics only.
- Do not add a script runtime, localization system, UI renderer, editor graph product, network service, or package format change in this milestone.
- Start behavior/API/regression-risk changes with a RED test or static guard.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the next active developer-owned gameplay-family capability after `engine-advanced-physics-controller-v1` closeout.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active milestone and records `engine-advanced-physics-controller-v1` as completed.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Quest Dialogue Document Contract

**Status:** Pending.

### Goal

Define the smallest reusable quest/dialogue authoring contract for quest ids, objective ids, dialogue node ids, flags, prerequisites, and deterministic validation diagnostics.

### Done When

- RED tests fail first for missing quest/dialogue document validation behavior.
- Public value types validate duplicate ids, missing references, invalid prerequisites, unsupported reward/action ids, and unsafe localization keys without game-specific execution.
- Focused tests prove deterministic diagnostic ordering and fail-closed invalid documents.

## Phase 2: Runtime State Transition Contract

**Status:** Pending.

### Goal

Add value-only quest/dialogue state transition helpers over validated documents so game-owned systems can advance objectives, flags, dialogue choices, win/loss/restart hooks, and save-state rows deterministically.

### Done When

- RED tests fail first for missing state transition behavior.
- Runtime state rows distinguish accepted, ignored, blocked, completed, and invalid transitions with deterministic diagnostics.
- Sample or package evidence demonstrates at least one quest/objective/dialogue progression path through public APIs only.
- Docs, manifest fragments, schemas/static checks, and agent surfaces are updated for durable AI-operable contract changes.
- Full `tools/validate.ps1` passes at the coherent phase gate, with only explicit host-gated diagnostics where applicable.

## Validation Evidence

- Phase 0 pointer sync selected this plan after `engine-advanced-physics-controller-v1` completed deterministic advanced-controller package counters in `sample_generated_desktop_runtime_3d_package`, while `unsupportedProductionGaps = []` stayed empty.
