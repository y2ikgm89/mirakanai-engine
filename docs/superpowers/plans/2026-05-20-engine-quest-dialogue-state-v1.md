# Engine Quest Dialogue State v1 (2026-05-20)

**Plan ID:** `engine-quest-dialogue-state-v1`
**Status:** Completed.
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

**Status:** Completed.

### Goal

Define the smallest reusable quest/dialogue authoring contract for quest ids, objective ids, dialogue node ids, flags, prerequisites, and deterministic validation diagnostics.

### Done When

- RED tests fail first for missing quest/dialogue document validation behavior.
- Public value types validate duplicate ids, missing references, invalid prerequisites, unsupported reward/action ids, and unsafe localization keys without game-specific execution.
- Focused tests prove deterministic diagnostic ordering and fail-closed invalid documents.

## Phase 2: Runtime State Transition Contract

**Status:** Completed.

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
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_quest_dialogue_tests` failed before implementation because `mirakana/runtime/quest_dialogue.hpp` did not exist.
- Phase 1 focused GREEN: `MK_runtime_quest_dialogue_tests` builds and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_tests|MK_runtime_quest_dialogue_tests"` passes, proving deterministic diagnostics and fail-closed invalid quest/dialogue documents.
- Phase 1 static drift checks: `check-format`, focused `check-tidy.ps1 -Files engine/runtime/src/quest_dialogue.cpp`, `check-json-contracts`, `check-agents`, `check-ai-integration`, and `check-public-api-boundaries` pass after manifest fragment compose.
- Phase 1 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-05-20 with 66/66 CTest tests passing and only expected diagnostic-only host gates for Apple/Metal on this Windows host.
- Phase 2 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_quest_dialogue_tests` failed before implementation because `RuntimeQuestDialogueStateValidationResult`, `validate_runtime_quest_dialogue_state`, returned transition `reward_ids`, and dialogue follow-up action rows were missing.
- Phase 2 focused GREEN: `MK_runtime_quest_dialogue_tests` builds, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_quest_dialogue_tests"` passes, and `sample_2d_desktop_runtime_package --require-gameplay-systems` reports `gameplay_systems_quest_dialogue_ready=1`, `transition_rows=3`, `action_ids=2`, `reward_ids=2`, and `state_rows=3`.
- Phase 2 package evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` passed, including installed D3D12 package smoke with quest/dialogue action, reward, and save-state counters.
- Phase 2 static drift checks: `check-format`, `check-json-contracts`, `check-agents`, `check-ai-integration`, `check-public-api-boundaries`, and focused `check-tidy.ps1 -Files engine/runtime/src/quest_dialogue.cpp,games/sample_2d_desktop_runtime_package/main.cpp,tests/unit/runtime_quest_dialogue_tests.cpp` passed after manifest fragment compose.
- Phase 2 full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-05-20 with 66/66 CTest tests passing, `production-readiness-audit: unsupported_gaps=0`, and only expected diagnostic-only host gates for Apple/Metal on this Windows host.
