# Sandbox World Network And Modding Gate v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add sandbox-world-specific network mutation replication and reviewed modding policy gates without promoting broad online multiplayer, broad modding, or legacy desktop middleware readiness.

**Architecture:** Keep the new surface value-only in `MK_runtime`. Sandbox mutation commands and snapshot deltas extend production replication review rows; modding policy rows extend scripting sandbox policy review. ENet remains optional behind the existing `network-enet` host-gated adapter, and SDL3 remains absent.

**Tech Stack:** C++23, `MK_runtime`, existing production network replication, existing scripting sandbox, optional `MK_runtime_network_enet`, CMake/CTest through repository PowerShell 7 wrappers, Context7 official documentation for ENet/CMake/vcpkg.

**Plan ID:** `sandbox-world-network-modding-gate-v1`

**Status:** Completed through PR #312 / merge commit `0fa26b1823d6bb4b68837f19b4040ede781e09b9`.

---

## Context

- Parent milestone: `docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md`.
- Prior completed slice: `Generic 2D Sandbox Gameplay Integration Rows v1` through PR #311 / merge commit `9562c3ae3844467c3ed82aff33b16ad8691c83b2`.
- This child plan narrows the old Phase 9 into a reviewable candidate because network replication/security and scripting/modding policy are distinct enough from completed generic gameplay rows.
- `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps` remains `[]`; this work is a selected post-ready evidence slice, not a reopened unsupported 1.0 gap.

## Official Evidence

- Context7 `/lsalzman/enet` on 2026-05-30: ENet initialization/deinitialization, host lifecycle, service loop, reliability flags, and packet lifetime stay inside the optional adapter.
- Context7 `/websites/cmake_cmake_help` on 2026-05-30: use target-based `add_test(NAME ... COMMAND ...)` and existing CTest wrappers.
- Context7 `/microsoft/vcpkg` on 2026-05-30: optional dependencies stay in manifest features and bootstrap remains outside CMake configure.
- Existing specs:
  - `docs/specs/2026-05-24-networking-and-multiplayer-v1-threat-model.md`
  - `docs/specs/2026-05-26-networking-production-security-threat-model.md`
  - `docs/specs/2026-05-30-sandbox-world-network-security-threat-model.md`

## Constraints

- No SDL3, legacy desktop middleware, compatibility shim, native handle exposure, public socket/peer/packet objects, or backend-specific API in public runtime rows.
- No encryption, authentication, accounts, anti-cheat, NAT traversal, matchmaking, relay, cloud service, or internet-facing server claim.
- No actual authoritative world mutation execution, rollback/prediction execution, filesystem IO, package mutation, renderer/RHI residency, editor calls, threads, process spawning, native plugins, or runtime source parsing.
- ENet changes are optional. If untouched, do not run ENet validation as a required proof; keep host-gated recipe text honest.

## Tasks

### Task 1 - Select Focused Child Plan

**Files:**
- Create: `docs/superpowers/plans/2026-05-30-sandbox-world-network-modding-gate-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [x] Create this child plan and record why old Phase 9 is not direct continuation from the selection gate.
- [x] Update the plan registry active row to point at this child plan.
- [x] Update `currentActivePlan` / `recommendedNextPlan` to this child plan while keeping `unsupportedProductionGaps = []`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 2 - Sandbox Network Mutation Replication

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/production_network_replication.hpp`
- Modify: `engine/runtime/src/production_network_replication.cpp`
- Modify: `tests/unit/runtime_production_network_replication_tests.cpp`

- [x] Add RED tests for sandbox mutation command rows, snapshot delta rows, sequence replay rejection, authority diagnostics, delta budget rejection, and unknown command references.
- [x] Verify RED by building `MK_runtime_production_network_replication_tests` and observing missing API compile failures.
- [x] Add `RuntimeNetworkSandboxMutationCommandRow` and `RuntimeNetworkSandboxSnapshotDeltaRow` public value rows.
- [x] Validate ids, command-id uniqueness, channel authority, monotonic ticks, per-player/channel sequence uniqueness, positive byte counts, nonzero payload/state hashes, delta command references, and shared snapshot/delta byte budget.
- [x] Keep outputs empty on invalid requests and expose deterministic counts/replay hash when valid.
- [x] Run focused build/test:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_production_network_replication_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_production_network_replication"
```

### Task 3 - Reviewed Modding Policy Rows

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/scripting_sandbox.hpp`
- Modify: `engine/runtime/src/scripting_sandbox.cpp`
- Modify: `tests/unit/runtime_scripting_sandbox_tests.cpp`

- [x] Add RED tests for deterministic reviewed modding adapter rows and denied unsafe capability requests.
- [x] Add `RuntimeScriptModdingPolicy*` value-only policy review rows, diagnostics, and `plan_runtime_script_modding_policy`.
- [x] Deny filesystem, network, process, native plugin, and package mutation capability requests by default.
- [x] Keep reviewed deterministic adapters sorted and fail closed when unreviewed, nondeterministic, missing replay seed, duplicated, or unsafe.
- [x] Run focused build/test:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_scripting_sandbox"
```

### Task 4 - Agent Surface And Documentation Drift

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`
- Modify static checks only for new machine-readable literals.

- [x] Update docs with exact narrow claims and explicit non-claims.
- [x] Add or update validation recipe docs for `network-production-security` if the reviewed recipe surface is stale.
- [x] Add static needles for new public row names/counters/diagnostics only where agent-operable contracts depend on them.
- [x] Compose manifest and run targeted drift checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

### Task 5 - Slice Closeout

**Files:**
- All task-owned files above.

- [x] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] Commit a verified candidate on `codex/generic-2d-sandbox-network-modding-gate-v1`.
- [x] Push and open a reviewable PR with validation evidence.
- [x] Wait for hosted checks including `PR Gate` before merge/cleanup.

## Done When

- Sandbox-world mutation commands and snapshot deltas are reviewed by production network replication with deterministic rows, counts, diagnostics, and replay evidence.
- Modding policy review produces deterministic value rows and denied capability rows without executing filesystem, network, process, native plugin, package mutation, or source parsing behavior.
- Docs, manifest fragments, generated manifest, static checks, and validation recipes describe only the proven narrow surface.
- Full local validation and hosted PR evidence exist for the candidate.

## Validation Evidence

- Focused build passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_production_network_replication_tests MK_runtime_scripting_sandbox_tests MK_runtime_network_production_security_tests`.
- Focused CTest passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_production_network_replication|runtime_scripting_sandbox|runtime_network_production_security"` with 3/3 tests passed.
- Validation recipe dry-runs passed for `network-production-security`; `network-enet` remains host-gated by `network-enet-vcpkg`.
- Targeted drift checks passed: `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `git diff --check`.
- Full validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` with `validate: ok` and 84/84 CTest tests passed. Apple/Metal and Apple packaging evidence remained diagnostic host gates on this Windows host; Android packaging preflight reported ready.
- PR #312 merged at merge commit `0fa26b1823d6bb4b68837f19b4040ede781e09b9` after hosted `PR Gate`, `Windows MSVC`, `Agent Static Guards`, `Linux CMake`, `Linux Coverage`, `Linux Clang ASan/UBSan`, full repository static analysis shards, `macOS Metal CMake`, `iOS Simulator smoke`, and CodeQL checks succeeded for head `7f039e13d1f28d05e5b68eaf7d414eb87862217a`.
