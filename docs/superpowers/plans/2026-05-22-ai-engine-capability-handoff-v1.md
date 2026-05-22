# AI Engine Capability Handoff v1 (2026-05-22)

Plan ID: `ai-engine-capability-handoff-v1`
Status: Completed
Capability row: `ai-engine-capability-handoff-v1`

## Goal

Implement a first-party, backend-neutral handoff contract for AI game-creation agents that hit a reusable but unsupported engine capability. The contract must capture the requested canonical capability row, blocked feature, current workaround, affected game files, desired public API/data contract, and required evidence before developer-owned engine work can begin.

## Context

- The production master plan is active with `unsupportedProductionGaps = []` and `recommendedNextPlan.id = next-production-gap-selection`.
- The canonical backlog marks `ai-engine-capability-handoff-v1` as a `foundational-unblocker` candidate.
- Existing AI gameplay authoring review in `MK_tools` already returns value-only remediation rows and diagnostics without mutating files, executing commands, or editing engine internals.

## Constraints

- Keep the handoff value-only: no file mutation, command execution, package mutation, network access, editor dependency, renderer/RHI/native handle exposure, or middleware types.
- Require canonical backlog row ids so game-creation agents do not invent engine-internal work.
- Keep public contracts first-party and backend-neutral.
- Update agent-facing docs, schemas, manifest fragments, and static checks only where this durable contract becomes supported.

## Official Practice Check

- JSON Schema Draft 2020-12 official guidance supports fail-closed object contracts through `required`, `properties`, `additionalProperties: false`, `enum`, array `items`, `minItems`, and `uniqueItems`; this plan uses those keywords for descriptor rows rather than accepting free-form handoff data.
- Repository AI/operator command-surface guidance requires typed diagnostics and no broad shell permissions; this plan adds a value-only review API and descriptor schema, not an execution surface.

## Phase Checklist

- [x] Add failing unit tests for accepted and rejected engine capability handoff rows.
- [x] Implement `MK_tools` value types and review function for deterministic handoff rows.
- [x] Add game manifest descriptor schema rows for `aiWorkflow.engineCapabilityHandoffs`.
- [x] Update AI game development docs, current capabilities, backlog status, manifest fragments, and static checks.
- [x] Run focused validation for the touched target/static surfaces.
- [x] Run full `tools/validate.ps1`.

## Done When

- `review_engine_capability_handoff_request` returns deterministic handoff rows for valid rows and fails closed for missing canonical row ids, missing required text, missing affected files/evidence, duplicate handoff ids, and native/middleware/backend contract leakage.
- `schemas/game-agent.schema.json` exposes a fail-closed `aiWorkflow.engineCapabilityHandoffs` descriptor.
- Durable docs/manifest/static checks describe the supported boundary without broad game creation or engine-internal mutation claims.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` | Expected fail, then pass | RED failed on missing handoff API before implementation; post-implementation focused build passed. |
| Subagent C++ review | Findings fixed | Added fail-closed checks for non-`games/` affected paths and lowercase backend/native contract terms. |
| Subagent agent-surface audit | Pass | No blocking schema/docs/manifest/static-check drift found. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests` | Pass | Focused unit test loop passed after review fixes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | Schema and manifest contract check passed after manifest compose. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Agent-surface drift check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass | Codex/Claude/Cursor skill parity check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Formatting/static text check passed after review fixes and `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/asset/gameplay_authoring_tool.cpp` | Pass | Focused C++ static analysis passed after review fixes. Header-only input was rejected by the wrapper, so the implementation translation unit was used. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Slice-closing validation passed; Apple/Metal host checks remained diagnostic-only host gates on Windows. |
