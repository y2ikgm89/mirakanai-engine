# AI Gameplay Authoring Tools v1 (2026-05-22)

**Plan ID:** `ai-gameplay-authoring-tools-v1`
**Status:** Active.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while active. Keep `unsupportedProductionGaps = []`; this is a post-1.0 developer-owned high-freedom game creation capability, not a reopened Engine 1.0 production gap.

## Goal

Add a first-party AI-operable gameplay authoring review surface so Codex/Claude/Cursor can propose game rules, encounters, quests, regions, and progression against already supported engine capabilities, receive deterministic remediation rows, and produce an inspectable mutation ledger without editing engine internals or cooked packages directly.

## Context

- `gameplay-authoring-foundation-v1` is completed: runtime-scene gameplay binding and interaction rows are ready.
- Follow-up developer-owned gameplay milestones are completed through quest/dialogue state, inventory/items/crafting, construction placement, procedural generation, world-region streaming, entity scale/culling, scripting sandbox policy, networking foundation policy, and gameplay simulation orchestration package evidence.
- The high-freedom game creation track names `ai-gameplay-authoring-tools-v1` as the authoring/remediation stream that should bind these reusable systems into reviewed schemas and validation recipes.
- The current manifest has `unsupportedProductionGaps = []`; this plan must not add Engine 1.0 blockers.

## Constraints

- Keep the Phase 1 contract value-only and host-independent. No filesystem writes, cooked-package mutation, command execution, arbitrary script evaluation, network access, native handles, renderer/RHI ownership, editor dependency, or external service calls.
- Expose a small first-party tool API under `MK_tools` using existing public runtime/gameplay capability ids and validation recipe ids; game-specific content remains game-owned data.
- Use RED tests before behavior/API changes.
- Close each C++/runtime/public-contract phase with focused checks and one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- Update manifest fragments, docs, generated-game guidance, and static checks when the AI-operable contract changes.

## Phase 0: Selection and Pointer Sync

**Status:** Completed.

### Goal

Make the plan registry, master-plan ledger, backlog/high-freedom track, and manifest pointers agree that `ai-gameplay-authoring-tools-v1` is the active developer-owned capability after Gameplay Simulation Orchestration v1 and closeout PR #173.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` slice.
- The production-completion master plan and readiness ledger name this plan as the current developer-owned capability.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Focused docs/agent checks pass.

## Phase 1: Gameplay Authoring Capability Review Contract

**Status:** Completed.

### Goal

Add a small `MK_tools` public contract that reviews AI-authored gameplay feature requests against a caller-supplied supported-capability profile, returns deterministic accepted/blocked rows, emits remediation rows for unsupported or missing evidence, and records mutation-ledger rows for the reviewed plan without mutating files.

### Planned API Shape

- `GameplayAuthoringCapabilityProfile`: supported capability ids, validation recipe ids, and package evidence ids exposed to the authoring review.
- `GameplayAuthoringRequestedFeatureRow`: requested feature id, gameplay family, required capability ids, optional validation recipe ids, and source index.
- `GameplayAuthoringReviewRequest`: profile plus requested feature rows.
- `GameplayAuthoringReviewResult`: accepted feature rows, remediation rows, mutation ledger rows, diagnostics, and `succeeded()`.
- `review_gameplay_authoring_request`: fail-closed review helper over the value rows above.

### Completed Surface

- `engine/tools/include/mirakana/tools/gameplay_authoring_tool.hpp` exposes the value rows and `review_gameplay_authoring_request` under `MK_tools`.
- Valid rows return accepted feature rows and `gameplay-authoring:<feature_id>` mutation ledger rows in input order.
- Invalid rows return no accepted features and no mutation ledger rows; diagnostics/remediation cover duplicate or missing feature ids, missing gameplay families, missing or unsupported required capabilities, unsupported validation recipes, unsupported package evidence, and broad unreviewed authoring claims.
- The helper does not mutate files or cooked packages, execute commands, evaluate scripts, call external services, depend on editor/runtime hosts, or expose renderer/RHI/native handles.

### Done When

- RED tests prove valid feature requests produce deterministic accepted rows and mutation-ledger rows in input order.
- RED tests prove duplicate feature ids, missing feature ids, missing required capabilities, missing validation recipes, unsupported capability ids, and unsafe broad-authoring claims produce diagnostics and no accepted rows.
- Focused `MK_tools_tests` passes.
- Public API boundary, agent/static drift checks, composed manifest, docs, and full validation are updated.

## Phase 2: Generated Package Guidance and Evidence Promotion

**Status:** Planned.

### Goal

Promote the review contract into generated-game guidance and selected package-visible evidence so future game agents can see which authoring rows were reviewed and which remediation recipes remain required.

### Done When

- Generated-game guidance documents the reviewed authoring flow and rejected direct cooked-package mutation.
- A selected sample or generated package path reports package-visible authoring review counters without executing authoring fixes from the package runtime.
- Manifest recipes and static checks expose the capability without claiming autonomous commercial game design or unreviewed content generation.

## Validation Evidence

- Phase 0 pointer sync:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format-text.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
- Phase 1 gameplay authoring review contract:
  - RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` failed on the new tests because `mirakana/tools/gameplay_authoring_tool.hpp` did not exist.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed with `unsupported_gaps=0`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
