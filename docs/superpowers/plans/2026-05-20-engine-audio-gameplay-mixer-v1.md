# Engine Audio Gameplay Mixer v1 (2026-05-20)

**Plan ID:** `engine-audio-gameplay-mixer-v1`
**Status:** Completed.
**Current pointer rule:** This milestone is closed. Do not leave `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` pointing at this completed plan. Keep `unsupportedProductionGaps = []`; this was a developer-owned capability milestone, not a reopened Engine 1.0 production gap.

## Goal

Add reusable gameplay-facing audio event and mixer primitives so AI-generated games can trigger music, SFX, ambience, volume groups, spatial one-shots, and pause/fade behavior through first-party contracts without native audio handles or codec assumptions.

## Context

- `gameplay-authoring-foundation-v1` completed runtime scene gameplay binding and interaction planning.
- `engine-save-settings-profile-v1` completed reviewed runtime profile path/document primitives.
- `engine-ui-game-menu-hud-v1` completed a runtime menu/HUD intent model.
- `engine-input-action-contexts-v1` completed runtime input context stack planning and source-tree 2D adoption evidence.
- The developer-owned backlog lists `engine-audio-gameplay-mixer-v1` as the next foundational unblocker for music, SFX, volume groups, spatial one-shots, looping ambience, pause/fade behavior, and missing-codec diagnostics.

## Constraints

- Keep game-specific audio event names, balance, clip ids, and music rules in game-owned code/data.
- Promote only reusable first-party audio event/mixer primitives into engine APIs.
- Keep native audio devices, SDL3, AAudio, codecs, and platform handles behind existing adapters or host gates.
- Do not add new third-party codec/dependency records unless a scoped dependency plan intentionally selects them.
- Preserve `unsupportedProductionGaps = []`. If this work requires reopening an Engine 1.0 production gap, stop.
- Use RED tests before behavior/API changes.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Make the manifest pointers, master-plan ledger, and plan registry agree that `engine-audio-gameplay-mixer-v1` is the active developer-owned capability after input action contexts, while keeping `unsupportedProductionGaps = []`.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` slice and records `engine-input-action-contexts-v1` as completed.
- The production master plan and readiness ledger name this milestone as developer-owned capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Gameplay Audio Event Review And First Contract

**Status:** Completed.

### Goal

Review existing `MK_audio` mixer/event/streaming APIs and select the smallest missing reusable primitive for generated-game gameplay audio.

### Done When

- Existing `MK_audio` public APIs, tests, and sample usage are reviewed before implementation.
- RED audio tests describe the selected missing behavior.
- Focused audio build/test and relevant public/agent static checks pass.

## Phase 2: Generated Game Adoption Evidence

**Status:** Completed.

### Goal

Apply the selected gameplay audio primitive to one or more generated-game or sample-game paths so source-tree or package-visible gameplay audio behavior is evidenced beyond unit tests.

### Done When

- At least one existing generated-game/sample flow uses the new gameplay audio primitive before direct mixer rendering or playback decisions.
- Focused build/test/package or source-tree validation proves the adoption.
- Docs, manifest fragments, and static checks describe the adopted generated-game flow while keeping `unsupportedProductionGaps = []`.

## Validation Evidence

- Phase 0 pointer sync: plan registry, production master-plan index, readiness ledger, manifest fragments, and composed manifest point `currentActivePlan` / `recommendedNextPlan` at this plan while keeping `unsupportedProductionGaps = []`.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed after adding `plan_gameplay_audio_mix` tests because the new public audio gameplay mix API did not exist yet.
- Phase 1 GREEN: `AudioGameplayMixRequest`, `AudioGameplayMixPlan`, `AudioGameplayBusMixDesc`, `AudioGameplayCueDesc`, `AudioGameplayCueTrigger`, fail-closed diagnostics, and `plan_gameplay_audio_mix` were added to `MK_audio`; focused `MK_core_tests` build and `ctest -R MK_core_tests` passed.
- Phase 2 RED: `tools/check-ai-integration.ps1` failed after adding the `games/sample_2d_playable_foundation/main.cpp` needle for `plan_gameplay_audio_mix`.
- Phase 2 GREEN: `sample_2d_playable_foundation` routes jump SFX through `plan_gameplay_audio_mix`, reports `audio_cues=1`, and passes focused build/test plus `tools/check-ai-integration.ps1`.
- Closeout sync: docs, game manifest, manifest fragments, generated-game guidance, skills, and static guards describe Audio Gameplay Mix Planner v1 while `unsupportedProductionGaps = []` remains unchanged.
