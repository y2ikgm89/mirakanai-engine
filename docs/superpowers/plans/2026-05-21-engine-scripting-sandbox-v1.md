# Engine Scripting Sandbox v1 (2026-05-21)

**Plan ID:** `engine-scripting-sandbox-v1`
**Status:** Completed.
**Historical pointer note:** This completed plan was selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` only while active; current pointer truth is the composed manifest and plan registry.

## Goal

Add the first AI-operable scripting sandbox foundation so generated games can describe reviewed game-local script modules, requested host permissions, entrypoints, execution budgets, deterministic replay seeds, and package-visible validation evidence without shipping a scripting runtime, native plugin ABI, filesystem/network/process access, or arbitrary engine mutation.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- The developer-owned backlog lists `engine-scripting-sandbox-v1` as an optional-adapter for high-freedom game-local scripts or mods, gated by restricted host APIs, execution budgets, deterministic diagnostics, and no filesystem/network/process access by default.
- The high-freedom track lists `scripting-and-mod-sandbox-v1` as a post-1.0 / 1.x stream. It requires first-party host API boundaries before any Lua/WASM or other runtime adapter can be considered.
- Existing runtime foundations already cover deterministic session documents, quest/dialogue transitions, inventory/crafting, construction placement, procedural generation, world-region streaming, entity scale/culling, and package-visible sample counters. This milestone should compose with those value contracts rather than execute game-specific script logic.

## Constraints

- Preserve `unsupportedProductionGaps = []`. If scripting must become an Engine 1.0 blocker to proceed, stop.
- Do not introduce Lua, WASM, JIT, bytecode execution, interpreter dependencies, dynamic libraries, native handles, filesystem/network/process access, or unrestricted engine mutation in this milestone.
- Keep the public contract value-only and deterministic. Games may use the rows to decide whether their own script package is allowed to run, but the engine does not execute script code here.
- Start behavior/API/regression-risk changes with RED tests.
- Keep optional runtime adapters, dependency/legal records, host SDK gates, performance claims, multiplayer-safe modding, and replay-perfect interpreted execution as future work.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Select this plan as the active developer-owned optional-adapter foundation after the current zero-gap production pointer state.

### Done When

- `docs/superpowers/plans/README.md`, the readiness ledger, the master-plan index, and `docs/roadmap.md` list this plan as the active milestone.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.
- Static JSON/agent integration checks pass for the pointer transition.

## Phase 1: Sandbox Policy and Script Plan Contract

**Status:** Completed.

### Goal

Add the smallest deterministic `MK_runtime` contract for reviewed script module documents, declared entrypoints, requested permissions, host API allowlists, execution budgets, deterministic replay seeds, and fail-closed diagnostics.

### Done When

- RED tests fail first for missing scripting sandbox policy contracts.
- `MK_runtime` exposes first-party value rows for script modules, entrypoints, permissions, denied capabilities, budget limits, replay seed rows, and diagnostics.
- Focused tests prove deterministic ordering, duplicate/missing module and entrypoint diagnostics, default-denied filesystem/network/process capabilities, unsupported host API rejection, over-budget rejection, and no filesystem/package/renderer/RHI/native-handle mutation.

## Phase 2: Package Evidence and Agent Surface Closeout

**Status:** Completed.

### Goal

Expose selected scripting sandbox policy counters in a desktop runtime package lane and close the AI-operable contract surfaces for the supported narrow claim.

### Done When

- Selected package smokes report deterministic scripting sandbox counters for allowed rows, denied permission rows, rejected unsafe capability rows, budget rows, replay seed evidence, and clean diagnostics.
- Docs, manifest fragments, schemas/static checks, skills/rules/subagents, and generated-game guidance are checked for drift and updated only where durable behavior or workflow changed.
- Full `tools/validate.ps1` passes at the coherent runtime/public-contract gate, with `unsupportedProductionGaps = []`.

## Validation Evidence

- Phase 0 starts from `main` at merge commit `b5b6c185` with no open PRs, `currentActivePlan=docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, `recommendedNextPlan.id=next-production-gap-selection`, and `unsupportedProductionGaps = []`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests` failed with `C1083` for missing `mirakana/runtime/scripting_sandbox.hpp` after adding the test target.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests` passed after adding `RuntimeScriptSandboxPolicyDesc`, `RuntimeScriptSandboxPlan`, and `plan_runtime_script_sandbox`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scripting_sandbox_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after repository formatting.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/scripting_sandbox.cpp,tests/unit/runtime_scripting_sandbox_tests.cpp` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed with `unsupportedProductionGaps = []`.
- Review RED: after C++ review, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scripting_sandbox_tests` failed for missing deterministic diagnostic ordering and aggregate budget overflow coverage.
- Review GREEN: after saturating aggregate budget totals and sorting diagnostics by module, entrypoint, source, diagnostic code, permission, and host API id, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scripting_sandbox_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scripting_sandbox_tests` passed.
- Review GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/scripting_sandbox.cpp,tests/unit/runtime_scripting_sandbox_tests.cpp` passed after review fixes.
- Full gate GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after Phase 1, including `production-readiness-audit: unsupported_gaps=0`, static checks, dev build, tidy smoke, and `71/71` CTest entries.
- Phase 2 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R sample_2d_desktop_runtime_package_smoke` failed after adding `--require-scripting-sandbox-policy` to the smoke arguments before the package executable recognized the flag.
- Phase 2 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R sample_2d_desktop_runtime_package_smoke` passed after adding selected scripting sandbox policy counters to `sample_2d_desktop_runtime_package`.
- Phase 2 package evidence GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` passed and the installed package smoke reported `scripting_sandbox_status=planned`, `scripting_sandbox_ready=1`, `scripting_sandbox_entrypoint_rows=2`, `scripting_sandbox_allowed_permission_rows=7`, `scripting_sandbox_denied_permission_rows=5`, `scripting_sandbox_rejected_unsafe_capability_rows=6`, `scripting_sandbox_budget_diagnostics=2`, `scripting_sandbox_replay_seed_rows=2`, and `scripting_sandbox_diagnostics=0`.
- Phase 2 focused static GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files games/sample_2d_desktop_runtime_package/main.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed with `unsupportedProductionGaps = []`.
- Phase 2 full gate GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after code, package evidence, docs, manifest fragments, static checks, skills, and composed manifest updates, including `production-readiness-audit: unsupported_gaps=0` and `71/71` CTest entries.
- Hosted GREEN: PR #170 passed PR Gate, Windows MSVC, Linux CMake, Linux Coverage, Linux Clang ASan/UBSan, Full Repository Static Analysis shards 0-3, CodeQL, iOS Simulator smoke, and macOS Metal CMake, then merged into `main` as `d17ac0ac`.
