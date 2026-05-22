# 2026-05-23 AI Placeholder Asset Pipeline v1

## Status

**Status:** Completed.

**Plan ID:** `ai-placeholder-asset-pipeline-v1`

## Goal

Promote `ai-placeholder-asset-pipeline-v1` from `foundation-ready` to `implemented-1x-foundation` by making generated/sample 2D and 3D package manifests carry fail-closed `game.agent.json.aiWorkflow.placeholderAssetPipeline` rows that connect design asset requests to first-party placeholder asset planning and reviewed cook/package handoff.

## Context

- `engine-asset-placeholder-generation-v1` already provides `mirakana::PlaceholderAssetBundleRequest`, `mirakana::plan_placeholder_asset_bundle`, `mirakana::PlaceholderAssetCookPackageRequest`, and `mirakana::plan_placeholder_asset_cook_package`.
- `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, and `ai-safe-content-mutation-ledger-v1` are implemented; generated game manifests now have design specs and mutation ledgers.
- `05-projections-and-scenarios.md` places placeholder assets before playtest/remediation and generated-game quality rubric work.

## Constraints

- Descriptor-only for this slice: do not add arbitrary shell execution, external asset downloads, image/audio generation services, native handles, renderer/RHI residency, runtime source parsing, or broad art-direction claims.
- Keep mutation game-local under `games/<game_name>/` and route package handoff through existing reviewed first-party tooling.
- Edit `engine/agent/manifest.fragments/` and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.

## Done When

- `schemas/game-agent.schema.json` accepts and constrains `aiWorkflow.placeholderAssetPipeline`.
- `tools/check-json-contracts.ps1` fails closed when required 2D/3D package manifests omit or weaken placeholder pipeline rows.
- `tools/new-game.ps1` generated 2D/3D package manifests and committed sample 2D/3D package manifests include placeholder pipeline rows.
- Docs, skills, backlog, plan registry, manifest fragment, composed manifest, and AI integration needles are synchronized.
- Validation evidence is recorded, including RED static-check evidence and a final `tools/validate.ps1` pass or concrete blocker.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Expected FAIL | RED: placeholder pipeline static guard should reject missing `aiWorkflow.placeholderAssetPipeline` in required package manifests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | GREEN after adding schema, generated/scaffolded 2D/3D package rows, committed sample manifest rows, and fail-closed placeholder pipeline static checks. |
