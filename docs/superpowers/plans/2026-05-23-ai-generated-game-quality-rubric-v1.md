# 2026-05-23 AI Generated Game Quality Rubric v1

## Status

**Status:** Completed.

## Goal

Promote `ai-generated-game-quality-rubric-v1` from `candidate` to `implemented-1x-foundation` by adding fail-closed, machine-readable generated-game quality rubric rows for selected generated/sample 2D and 3D package manifests.

## Context

- The AI game creation workflow now has structured design specs, reviewed generation, mutation ledgers, placeholder asset pipeline rows, playtest loops, and validation remediation recipes.
- The backlog row asks for minimum quality gates beyond compile: objective, controls, feedback, fail/restart, deterministic package smoke, and budget evidence.
- This slice is a manifest/schema/static-check foundation only. It does not claim subjective commercial quality, autonomous balancing, broad production readiness, or renderer/backend quality beyond selected package evidence.

## Constraints

- Keep the public game-facing contract first-party and backend-neutral; do not expose native handles, middleware types, Dear ImGui, SDL3, renderer, RHI, or backend internals.
- Use clean breaking rows only: no compatibility aliases, duplicate API, deprecated fields, or migration layer.
- Edit `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- Use only supported PowerShell entrypoints under `tools/*.ps1`; do not use `bun run`.
- Keep generated-game mutation under reviewed game-local manifest rows and static checks.

## Done When

- A failing static contract proves generated/sample 2D/3D package manifests must carry `aiWorkflow.generatedGameQualityRubric`.
- The schema, generated templates, committed sample manifests, scaffold smokes, docs, skills, backlog row, plan registry, and manifest fragment are synchronized.
- Rubric rows cover objective, controls, feedback, fail/restart, deterministic package smoke, and budget evidence, and include explicit unsupported rows for unmet/broad quality claims.
- Focused checks and final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete blocker is recorded.

## Implementation Plan

1. Add `tools/check-json-contracts-065-generated-game-quality-rubric.ps1`, register it in `tools/static-contract-ledger.ps1`, and verify RED failure on missing `aiWorkflow.generatedGameQualityRubric`.
2. Add `generatedGameQualityRubric` schema under `schemas/game-agent.schema.json`.
3. Add rubric template helpers to `tools/new-game-templates.ps1` and attach the object to `DesktopRuntime2DPackage` and `DesktopRuntime3DPackage` manifests.
4. Regenerate/update the committed generated/sample 2D/3D package manifests with rubric rows.
5. Extend scaffold/static smoke needles and docs/skills/roadmap/backlog/plan registry/manifest fragment, then compose the manifest.
6. Run focused static checks and the full validation gate.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | RED | Failed as expected: `games/sample_2d_desktop_runtime_package/game.agent.json aiWorkflow.generatedGameQualityRubric missing for gameplay recipe '2d-desktop-runtime-package'`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Schema/static guard accepts rubric rows and composed manifest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-surface, scaffold-smoke, manifest, docs, and skill needles are synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Codex/Claude/Cursor skill and instruction parity remain valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Text and PowerShell formatting checks pass after `tools/format-text.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full slice gate passed: static checks, build, tidy smoke, and CTest 73/73; Metal/Apple diagnostics remain host-gated. |
