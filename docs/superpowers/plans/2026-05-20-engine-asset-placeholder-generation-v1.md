# Engine Asset Placeholder Generation v1 (2026-05-20)

**Plan ID:** `engine-asset-placeholder-generation-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is a developer-owned capability milestone, not a reopened Engine 1.0 production gap.

## Goal

Add deterministic first-party placeholder asset primitives so AI-generated games can legally stand up missing sprites, simple meshes, materials, UI imagery, and audio cue references with provenance rows, stable ids, package/update evidence, and replacement guidance.

## Context

- `gameplay-authoring-foundation-v1`, `engine-save-settings-profile-v1`, `engine-ui-game-menu-hud-v1`, `engine-input-action-contexts-v1`, and `engine-audio-gameplay-mixer-v1` are completed developer-owned milestones.
- The developer-owned backlog lists `engine-asset-placeholder-generation-v1` as the next foundational unblocker after gameplay audio.
- Existing asset/tooling contracts already include first-party source documents, cooked package indexes, material definitions, source asset registration, and deterministic validation. This plan should add only reusable placeholder generation primitives, not game-specific art direction.

## Constraints

- Keep placeholders deterministic, first-party, and license-safe. Do not download, copy, or embed external images, meshes, fonts, sounds, or AI-generated third-party-looking assets.
- Keep game-specific style, balance, names, and final art replacement in game-owned code/data.
- Use existing `MK_assets` / `MK_tools` package and provenance patterns where possible.
- Do not add new third-party dependencies unless a separate dependency/legal plan explicitly accepts them.
- Preserve `unsupportedProductionGaps = []`. If this work requires reopening an Engine 1.0 production gap, stop.
- Use RED tests before behavior/API changes.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Make the manifest pointers, master-plan ledger, and plan registry agree that `engine-asset-placeholder-generation-v1` is the active developer-owned capability after audio gameplay mixer closeout.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` slice and records `engine-audio-gameplay-mixer-v1` as completed.
- The production master plan and readiness ledger name this milestone as developer-owned capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Placeholder Asset Surface Review And First Contract

**Status:** Pending.

### Goal

Review existing `MK_assets` / `MK_tools` source/cooked asset contracts and select the smallest reusable placeholder-generation primitive that can feed generated games without external assets.

### Done When

- Existing asset source document, material, package, and scaffold flows are reviewed before implementation.
- RED tests describe deterministic placeholder generation, stable ids, provenance/license rows, and invalid-request diagnostics.
- Focused asset/tool build/test and relevant public/agent static checks pass.

## Phase 2: Generated Game Or Package Adoption Evidence

**Status:** Pending.

### Goal

Apply the selected placeholder primitive to one generated-game, sample-game, or package-authoring path so missing art/audio references can be represented legally and replaced later.

### Done When

- At least one existing generated-game/sample/package flow uses the placeholder primitive before package or runtime consumption.
- Focused build/test/package or source-tree validation proves the adoption.
- Docs, manifest fragments, and static checks describe the placeholder workflow while keeping `unsupportedProductionGaps = []`.

## Validation Evidence

- Phase 0 pointer sync: plan registry, production master-plan index, readiness ledger, manifest fragments, and composed manifest point `currentActivePlan` / `recommendedNextPlan` at this plan while keeping `unsupportedProductionGaps = []`.
