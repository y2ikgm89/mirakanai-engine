# AI Command Surface Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as a focused production-loop slice. Do not append this work to historical MVP or completed foundation plans.

**Goal:** Add the first backend-neutral AI command surface contract so agents can dry-run and report safe authoring/package/game mutations without scraping prose or bypassing engine capability gates.

**Architecture:** Keep the contract machine-readable and host-independent. Add value/schema/check/docs support for command descriptors, request modes, dry-run result diagnostics, validation recipe references, capability gates, and undo-token placeholders. Do not wire broad editor UI actions, execute package builds, mutate CMake, or claim 2D/3D playable readiness in this slice.

**Tech Stack:** C++23 only if a small value API is needed, otherwise JSON schema/docs/tools, `engine/agent/manifest.json`, PowerShell checks, and `tools/*.ps1` validation commands.

---

## Goal

Move the AI-operable production loop beyond recipe discovery by making safe command surfaces explicit:

- command descriptors list ids, schema versions, request/result shapes, required modules, capability gates, host gates, and validation recipes
- dry-run/apply modes are represented as data, but this slice may implement dry-run contract checks before broad apply plumbing
- unsupported capabilities remain explicit diagnostics instead of being silently accepted
- generated-game and engine-production guidance points agents to supported commands only

## Context

- `engine/agent/manifest.json.aiOperableProductionLoop.commandSurfaces` exists but is mostly descriptive.
- Scene/Component/Prefab Schema v2, Asset Identity v2, Runtime Resource v2, Renderer/RHI Resource Foundation v1, and Frame Graph/Upload-Staging Foundation v1 are now foundation-only data contracts.
- Future 2D/3D playable recipes remain planned until their own implementation and validation land.

## Constraints

- Keep `engine/core` unchanged unless a separate accepted design requires shared diagnostics.
- Do not expose editor, Dear ImGui, SDL3, RHI backend handles, native platform handles, or tool process handles through command contracts.
- Do not execute package builds or source asset importers from a command contract without a reviewed apply/execution slice.
- Do not mark 2D/3D/editor/product renderer work ready.

## Done When

- The manifest exposes command descriptor fields that static checks validate.
- Schema/check tooling rejects missing command ids, missing dry-run/result diagnostics fields, unknown validation recipes, unknown capability gates, and unsupported ready claims.
- Docs and prompt guidance explain how agents choose commands without treating planned recipes as ready.
- Validation evidence is recorded here.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Contract Inventory And RED Checks

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `engine/agent/manifest.json`

- [x] Add failing checks for required command descriptor ids and fields.
- [x] Require command descriptors to reference known validation recipes and known unsupported gap ids.
- [x] Record RED evidence in this plan.

### Task 2: Manifest And Schema Contract

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: schemas if needed

- [x] Add or normalize command descriptor rows for initial dry-run-safe operations.
- [x] Keep apply/execution host gates explicit where mutation or tools are not implemented.

### Task 3: Agent Context And Docs

**Files:**
- Modify: `tools/agent-context.ps1` if top-level command output is incomplete.
- Modify: `docs/ai-integration.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`

- [x] Ensure agents can inspect command descriptors from `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`.
- [x] Update docs to keep planned 2D/3D and broad editor/product renderer claims unsupported.

### Task 4: Validation And Next Plan

**Files:**
- Modify: this plan
- Modify: `docs/superpowers/plans/README.md`

- [x] Run focused checks and default validation.
- [x] Record evidence.
- [x] Mark this plan complete and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- 2026-05-01 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after strengthening command surface descriptor checks. `tools/check-json-contracts.ps1` reported `engine manifest aiOperableProductionLoop commandSurfaces missing required property: schemaVersion`.
- 2026-05-01 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected after strengthening agent-facing command surface checks. `tools/check-ai-integration.ps1` reported `engine/agent/manifest.json aiOperableProductionLoop command surface missing required property: schemaVersion`.
- 2026-05-01 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok` after replacing legacy command surface fields with schema-versioned descriptors and updating `schemas/engine-agent.schema.json`.
- 2026-05-01 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok` after docs/static checks were synchronized for `requestModes`, `capabilityGates`, `undoToken`, and `register-runtime-package-files` as the only ready apply command.
- 2026-05-01 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and exposed the descriptor-shaped `aiCommandSurfaces` plus `2d-playable-vertical-slice-foundation-v1` as the next active/recommended plan.
- 2026-05-01 FINAL: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; CTest reported 26/26 tests passing. Diagnostic-only gates remain unchanged: Metal `metal`/`metallib` are missing on this host, Apple packaging requires macOS/Xcode, Android release signing/device smoke is not fully configured, and strict tidy analysis needs a compile database.
- 2026-05-01 REVIEW: `cpp-reviewer` found that the ready package-file registration descriptor referenced `-WhatIf` instead of the actual `-DryRun` switch, checks did not lock ready apply to `register-runtime-package-files`, and generated-game scenario docs omitted `requiredModules`. The manifest, schema/static checks, and docs were updated so `register-runtime-package-files` must keep `dry-run` and `apply` ready, all other command surfaces fail if `apply` becomes ready in this slice, and command-surface docs mention `requiredModules`.
- 2026-05-01 FINAL REVIEW FIX: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the review fixes; CTest reported 26/26 tests passing.
