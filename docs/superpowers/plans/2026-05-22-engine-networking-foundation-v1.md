# Engine Networking Foundation v1 (2026-05-22)

**Plan ID:** `engine-networking-foundation-v1`
**Status:** Active.
**Current pointer rule:** `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this milestone while active. Keep `unsupportedProductionGaps = []`; this is a developer-owned optional-adapter foundation, not a reopened Engine 1.0 production gap.

## Goal

Add the first AI-operable networking foundation so generated games can describe reviewed multiplayer session intent, trust boundaries, transport requirements, replication channels, deterministic replay prerequisites, and security diagnostics without opening sockets, adding transport middleware, exposing native network handles, or claiming broad multiplayer readiness.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- The developer-owned backlog lists `engine-networking-foundation-v1` as the remaining optional-adapter row after `engine-scripting-sandbox-v1`. It requires a separate architecture/security plan, transport abstraction, threat-model evidence, deterministic simulation prerequisites, and no broad multiplayer ready claim from local gameplay.
- The 2D/3D capability matrix classifies multiplayer/networking as optional-adapter work, not an Engine 1.0 blocker.
- Existing runtime foundations already cover deterministic session/profile documents, input contexts, quest/dialogue state, inventory/crafting, construction placement, procedural generation, world-region streaming, entity scale/culling, and scripting sandbox policy. This milestone should begin with value-only networking policy and diagnostics that compose with those systems rather than executing network I/O.
- Engine Scripting Sandbox v1 closed through PR #170 with hosted checks and merge commit `d17ac0ac`, keeping `unsupportedProductionGaps = []`.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If networking must become an Engine 1.0 blocker to proceed, stop.
- Do not open sockets, create threads, use platform networking APIs, add third-party transport dependencies, expose native handles, perform encryption/authentication handshakes, or claim production multiplayer, rollback, prediction, matchmaking, NAT traversal, or transport security readiness in this milestone.
- Keep Phase 1 public contracts value-only, deterministic, dependency-free, and safe for generated games to inspect before deciding whether networking is unsupported, host-gated, or game-owned.
- Treat security-sensitive networking behavior as opt-in future work that needs explicit threat model evidence, transport dependency/legal records if external code is introduced, and host gates before any I/O path.
- Start behavior/API/regression-risk changes with RED tests.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the active developer-owned networking foundation after the merged scripting sandbox milestone.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, the master-plan index, and `docs/roadmap.md` list this plan as the active milestone and record Engine Scripting Sandbox v1 as merged historical evidence.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Network Session Policy and Threat Boundary Contract

**Status:** Completed.

### Goal

Add the smallest deterministic `MK_runtime` contract for reviewed network session topology, local role, trust boundary, transport requirement, replication channel, replay prerequisite, and fail-closed security diagnostics.

### Done When

- RED tests fail first for missing networking foundation contracts.
- `MK_runtime` exposes value-only rows for session topology, host/client roles, trusted/untrusted peer assumptions, required transport capabilities, replication channel declarations, deterministic replay prerequisites, and diagnostics.
- Focused tests prove deterministic ordering, duplicate/missing id diagnostics, unsupported transport capability rejection, insecure default diagnostics, replay prerequisite validation, and no socket/platform/network/native-handle mutation.

## Phase 2: Package Evidence and Agent Surface Closeout

**Status:** Planned.

### Goal

Expose selected networking foundation counters in a package or sample lane and close the AI-operable contract surfaces for the supported narrow claim.

### Done When

- Selected smokes report deterministic networking foundation counters for session rows, channel rows, rejected unsafe transport requirements, replay prerequisite rows, security diagnostics, and clean reviewed-policy diagnostics.
- Docs, manifest fragments, schemas/static checks, skills/rules/subagents, and generated-game guidance are checked for drift and updated only where durable behavior or workflow changed.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with `unsupportedProductionGaps = []`.

## Validation Evidence

- Phase 0 starts from `main` at merge commit `d17ac0ac` after PR #170 merged Engine Scripting Sandbox v1. `currentActivePlan=docs/superpowers/plans/2026-05-21-engine-scripting-sandbox-v1.md`, `recommendedNextPlan.id=engine-scripting-sandbox-v1`, no open PRs, and `unsupportedProductionGaps = []`.
- Phase 0 pointer sync composed `engine/agent/manifest.json` from fragments and passed `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-production-readiness-audit.ps1` (`unsupported_gaps=0`), `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1`.
- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_networking_foundation_tests` failed because `mirakana/runtime/networking_foundation.hpp` did not exist.
- Phase 1 focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_networking_foundation_tests`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure --tests-regex MK_runtime_networking_foundation_tests` passed after adding the value-only `MK_runtime` contract.
- Phase 1 focused/static checks passed: `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files engine/runtime/src/networking_foundation.cpp,tests/unit/runtime_networking_foundation_tests.cpp`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-production-readiness-audit.ps1` (`unsupported_gaps=0`), `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1`.
- Phase 1 runtime/public-contract gate passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed successfully with 72 tests passed and `unsupported_gaps=0`.
