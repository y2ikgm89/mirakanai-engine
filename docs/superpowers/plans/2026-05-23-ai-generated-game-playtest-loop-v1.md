# 2026-05-23 AI Generated Game Playtest Loop v1

## Status

**Status:** Completed.

**Plan ID:** `ai-generated-game-playtest-loop-v1`

## Goal

Promote `ai-generated-game-playtest-loop-v1` from `candidate` to `implemented-1x-foundation` by making generated/sample 2D and 3D package manifests carry fail-closed `game.agent.json.aiWorkflow.generatedGamePlaytestLoop` rows for reviewed validation recipe execution, evidence capture, failure classification, and mutation-ledger remediation.

## Context

- `ai-game-design-spec-v1`, `ai-game-generation-orchestrator-v1`, `ai-safe-content-mutation-ledger-v1`, and `ai-placeholder-asset-pipeline-v1` are implemented.
- `run-validation-recipe` already provides reviewed dry-run/execute validation recipe surfaces.
- The projection ledger places playtest/remediation after placeholder assets and before generated-game quality rubric work.

## Constraints

- Descriptor-only for this slice: do not evaluate raw manifest command strings, execute arbitrary shell, weaken validation, delete evidence, bypass host gates, mutate cooked packages directly, edit engine internals, expose native handles, or claim broad game quality.
- Remediation must route through existing per-game mutation ledger rows and reviewed command surfaces.
- Edit `engine/agent/manifest.fragments/` and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.

## Done When

- `schemas/game-agent.schema.json` accepts and constrains `aiWorkflow.generatedGamePlaytestLoop`.
- `tools/check-json-contracts.ps1` fails closed when required 2D/3D package manifests omit or weaken generated-game playtest loop rows.
- `tools/new-game.ps1` generated 2D/3D package manifests and committed sample 2D/3D package manifests include playtest loop rows.
- Docs, skills, backlog, plan registry, manifest fragment, composed manifest, and AI integration needles are synchronized.
- Validation evidence is recorded, including RED static-check evidence and a final `tools/validate.ps1` pass or concrete blocker.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Expected FAIL | RED confirmed: the new static guard rejected missing `aiWorkflow.generatedGamePlaytestLoop` in required 2D/3D package manifests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Schema, ledger registration, sample manifests, and generated-game playtest loop descriptors are synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration needles cover docs, schema, templates, samples, static check, skill guidance, backlog, and manifest closeout surfaces. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Codex/Claude/Cursor skill guidance and tracked text formatting budgets are aligned after `tools/format-text.ps1` normalized generated JSON rewrites. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++/tracked text formatting checks pass for the touched docs, JSON, and PowerShell files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full slice gate passed; existing Apple/Metal host gates remain diagnostic-only on this Windows host, and CTest passed 73/73. |
