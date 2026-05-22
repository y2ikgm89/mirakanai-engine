# AI Game Design Spec v1 (2026-05-23)

Plan ID: `ai-game-design-spec-v1`
Status: Completed
Capability row: `ai-game-design-spec-v1`

## Goal

Implement a fail-closed structured game design descriptor for AI-authored 2D/3D package work so generation starts from explicit gameplay family, template, camera, input, loop, assets, systems, package targets, validation recipes, quality gates, and unsupported claims.

## Context

- The production master plan is active with `unsupportedProductionGaps = []` and `recommendedNextPlan.id = next-production-gap-selection`.
- The canonical backlog listed `ai-game-design-spec-v1` as the next AI game-creation foundational unblocker before orchestration.
- Existing `game.agent.json` manifests already define package targets, runtime package files, scene validation targets, validation recipes, and optional AI handoff descriptors.

## Constraints

- Keep the contract descriptor-only: no file mutation, command execution, package mutation, game rule execution, native handles, middleware contracts, or engine-internal edits.
- Keep package and validation references same-manifest so static checks can reject stale or invented evidence ids.
- Keep generated game public contracts first-party and backend-neutral.
- Update schema, static checks, templates, sample manifests, docs, skills, and manifest fragments together because this changes the durable AI game workflow.

## Official Practice Check

- Context7 lookup for JSON Schema failed in this session with a transport deserialize error, so official JSON Schema Draft 2020-12 docs were used directly.
- JSON Schema Draft 2020-12 identifies `https://json-schema.org/draft/2020-12/schema` as the default dialect meta-schema and defines validation assertion keywords for object, array, enum, const, and string constraints: <https://json-schema.org/draft/2020-12> and <https://json-schema.org/draft/2020-12/json-schema-validation>.
- The schema uses `required`, `additionalProperties: false`, `const`, `enum`, `pattern`, `minItems`, and `uniqueItems` for fail-closed data shape checks, while repository PowerShell checks enforce same-manifest package/recipe references that JSON Schema alone cannot express.

## Phase Checklist

- [x] Select `ai-game-design-spec-v1` from the developer-owned AI game-creation backlog.
- [x] Add a failing static check requiring `aiWorkflow.gameDesignSpec` for 2D/3D package recipe manifests.
- [x] Add fail-closed JSON schema for `aiWorkflow.gameDesignSpec`.
- [x] Add 2D/3D sample manifest examples and 2D/3D template generation rows.
- [x] Update docs, backlog, manifest fragments, skills, and static integration needles.
- [x] Compose manifest and run focused static validation.
- [x] Run full `tools/validate.ps1`.

## Done When

- `schemas/game-agent.schema.json` exposes `aiWorkflow.gameDesignSpec` with required fields and explicit unsupported-claim rows.
- `tools/check-json-contracts.ps1` rejects 2D/3D package manifests without design specs and rejects undeclared package target or validation recipe references.
- `tools/new-game.ps1 -Template DesktopRuntime2DPackage` and `DesktopRuntime3DPackage` emit design specs.
- Durable docs, skills, backlog status, manifest fragments, and static needles describe the supported boundary without claiming autonomous game design.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Expected fail | RED failed on missing `games/sample_2d_desktop_runtime_package/game.agent.json aiWorkflow.gameDesignSpec`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass | Composed `engine/agent/manifest.json` from updated fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Verified AI design-spec needles across schema, templates, sample manifests, docs, manifest output, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass | Verified Codex/Claude/Cursor agent surface consistency after skill updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | Verified design-spec schema/static contract checks and manifest composition. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Verified tracked text and format checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Final coherent slice validation before commit. |
