# Simulation Persistence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a value-only `MK_runtime` simulation persistence planner that reviews loaded save/profile documents for stable world snapshot evidence, entity state rows, save-slot coherence, schema migration steps, and corrupted-save remediation without claiming cloud saves, binary compatibility, replication, or automatic migration execution.

**Plan ID:** `simulation-persistence-v1`

**Status:** Local validation complete; PR hosted checks and merge pending.

**Gap:** `simulation-persistence-v1`

**Architecture:** Candidate Backlog Burn-down v1 child. Reuse `RuntimeSaveData`, `RuntimeSessionProfileDocumentLoadResult`, and existing fail-closed session profile document rows. Add a pure planner in `session_services` that turns reviewed key/value save evidence into deterministic snapshot, entity, migration, and remediation rows; filesystem mutation and real migration execution remain outside this API.

**Tech Stack:** C++23, `MK_runtime`, `tests/unit/runtime_tests.cpp`, repository PowerShell validation scripts, composed engine agent manifest fragments.

## Official Docs Review

- Use repository PowerShell 7 entrypoints and existing value-planning conventions.
- Use C++23 standard-library value types only; no third-party persistence dependency is introduced.
- Use existing `RuntimeSessionProfileDocumentStatus::failed_corrupt` and `failed_unsupported_version` as fail-closed inputs for remediation/migration planning.

## Non-Goals

- Cloud saves, cross-device sync, multiplayer replication, binary compatibility policy, platform user-directory resolution, background migration execution, filesystem mutation, package mutation, native handles, or game-specific component serialization beyond reviewed save key evidence.

## Tasks

- [x] Write failing `runtime simulation persistence plan accepts stable snapshot entity rows and save slot evidence` in `tests/unit/runtime_tests.cpp` using loaded `RuntimeSessionProfileDocuments` with `save.slot`, `world.id`, `snapshot.id`, `world.tick`, and `entity.<id>.(type|region|state_hash)` keys.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests --parallel 4` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_tests" --timeout 120 --parallel 1` to verify the new test fails on missing API.
- [x] Add `RuntimeSimulationPersistenceRequest`, `RuntimeSimulationPersistencePlan`, entity row, migration step, diagnostic, remediation, and status types to `engine/runtime/include/mirakana/runtime/session_services.hpp`.
- [x] Implement `plan_runtime_simulation_persistence` in `engine/runtime/src/session_services.cpp` with deterministic entity sorting, fail-closed missing/malformed key diagnostics, schema migration-chain review, and corrupt-save remediation recommendations.
- [x] Add failing/passing tests for supported migration chains, reachable migration path selection, missing migration gaps, malformed entity rows, settings-corrupt save recovery, corrupt save remediation, and unsupported save reset remediation.
- [x] Update docs, manifest fragments, composed manifest, and static guards for the new `runtime-simulation-persistence-v1` authoring surface.
- [x] Run focused public API, tidy, format, agent, JSON, AI integration, and `MK_runtime_tests` gates.
- [x] Run full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [ ] Commit, push, open PR, wait for hosted checks, merge, and clean the linked worktree through repository wrappers.

## Validation Evidence

| Gate | Status | Evidence |
| --- | --- | --- |
| TDD red | pass | `MK_runtime_tests` first failed on missing `RuntimeSimulationPersistence*` types and `plan_runtime_simulation_persistence`; reviewer hardening tests then failed for reachable migration-path selection and settings-corrupt save recovery before fixes. |
| Focused runtime tests | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests --parallel 4`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_tests" --timeout 120 --parallel 1`. |
| C++ review | pass after fixes | `cpp-reviewer` found greedy migration path selection, non-save document over-blocking, and missing unsupported-save remediation coverage; all were addressed with tests and implementation changes. |
| Focused static | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/session_services.cpp -Jobs 2`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`. |
| Agent-surface audit | pass after fixes | `agent-surface-auditor` found missing static protection for plan prose pointers and `currentRuntimeSimulationPersistence`; both are guarded in `tools/check-json-contracts-010-engine-manifest.ps1` and `tools/check-ai-integration-020-engine-manifest.ps1`. |
| Full validation | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with 76/76 CTest passing; Apple/Metal host gates remain diagnostic-only/host-gated as before. |

## Done When

- `MK_runtime` exposes deterministic value-only simulation persistence planning for ready snapshots, migration-required saves, fail-closed malformed state, and corrupt-save remediation.
- Tests prove the external behavior without mirroring implementation details.
- Backlog row, plan registry, docs, manifest fragments, composed manifest, and static guards are synchronized.
- Focused validation and full `tools/validate.ps1` pass; PR is merged.
