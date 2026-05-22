# 2026-05-23 AI Validation Remediation Recipes v1

## Status

**Status:** Completed.

**Plan ID:** `ai-validation-remediation-recipes-v1`

## Goal

Promote `ai-validation-remediation-recipes-v1` from `candidate` to `implemented-1x-foundation` by making generated/sample 2D and 3D package manifests carry fail-closed `game.agent.json.aiWorkflow.validationRemediationRecipes` rows that map playtest failure classifications to reviewed mutation-ledger actions without weakening validation.

## Context

- `ai-generated-game-playtest-loop-v1` already records selected validation recipes, evidence roots, failure classification rows, and mutation-ledger remediation policy.
- `ai-safe-content-mutation-ledger-v1` already records reviewed game-local command surfaces and remediation actions.
- This slice connects those two descriptors with machine-readable remediation recipes for common generated-game failures.

## Constraints

- Descriptor-only for this slice: do not execute remediation, evaluate raw manifest commands, mutate cooked packages directly, weaken validation, delete evidence, bypass host gates, edit engine internals, expose native handles, or claim broad generated-game quality.
- Remediation recipes must reference existing `aiWorkflow.generatedGamePlaytestLoop.failureClassifications`, `aiWorkflow.contentMutationLedger.remediationActions`, and reviewed command surfaces.
- Edit `engine/agent/manifest.fragments/` and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.

## Done When

- `schemas/game-agent.schema.json` accepts and constrains `aiWorkflow.validationRemediationRecipes`.
- `tools/check-json-contracts.ps1` fails closed when required 2D/3D package manifests omit or weaken validation remediation recipes.
- `tools/new-game.ps1` generated 2D/3D package manifests and committed sample 2D/3D package manifests include validation remediation recipe rows.
- Docs, skills, backlog, plan registry, manifest fragment, composed manifest, and AI integration needles are synchronized.
- Validation evidence is recorded, including RED static-check evidence and a final `tools/validate.ps1` pass or concrete blocker.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Expected FAIL | RED confirmed: the new static guard rejected missing `aiWorkflow.validationRemediationRecipes` in required package manifests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Schema, ledger registration, sample manifests, and validation remediation recipe descriptors are synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | AI integration needles cover docs, schema, templates, samples, static check, skill guidance, backlog, and manifest surfaces. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Codex/Claude/Cursor skill guidance and tracked text formatting budgets are aligned. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++/tracked text formatting checks pass for the touched docs, JSON, and PowerShell files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full slice gate passed; existing Apple/Metal host gates remain diagnostic-only on this Windows host, and CTest passed 73/73. |
