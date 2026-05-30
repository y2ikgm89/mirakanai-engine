# Sandbox World Package Validation And Performance Budgets v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the generic 2D sandbox production lane through selected package smoke flags, package-visible counters, and deterministic budget gates.

**Architecture:** Keep the proof in the existing `sample_2d_desktop_runtime_package` and repository validation wrappers. Add no middleware, no SDL3, no native-handle exposure, and no broad online/modding/renderer-quality claim; package evidence stays a first-party Win32/D3D12/WASAPI-hosted smoke over already implemented value rows.

**Tech Stack:** C++23, `MK_runtime`, `MK_renderer`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, `MK_audio_wasapi`, CMake/CTest/CPack through repository PowerShell 7 wrappers, Context7 official CMake/vcpkg docs.

---

**Plan ID:** `sandbox-world-package-validation-performance-budgets-v1`

**Status:** Completed.

Closeout status: implemented and published through PR #313 / merge commit `04cc3b0f9cb1331f1ea21b15f91f116eb3fec8cf` after local full validation and hosted `PR Gate`, `Windows MSVC`, Linux, macOS Metal CMake, iOS smoke, full repository static analysis shards, `Agent Static Guards`, and CodeQL checks succeeded.

## Official Evidence

- Context7 `/websites/cmake_cmake_help` on 2026-05-30: CTest tests should be registered through official CMake `add_test(NAME ... COMMAND ...)`; source-tree negative probes use `set_tests_properties(... PROPERTIES WILL_FAIL TRUE)` after the test exists.
- Context7 `/microsoft/vcpkg`: optional dependencies belong in manifest features; `VCPKG_MANIFEST_INSTALL=OFF` keeps dependency installation outside CMake configure. This phase adds no vcpkg dependency or feature.
- Microsoft Learn Direct3D 12 graphics on 2026-05-30: D3D12 remains the selected Windows presentation lane behind `MK_runtime_host_win32_presentation`; the smoke requires evidence, not public native handles.
- Microsoft Learn WASAPI on 2026-05-30: WASAPI remains inside `MK_audio_wasapi`; the package smoke checks existing review counters and zero device-IO side effects rather than opening a live device from gameplay code.
- Microsoft Learn Win32 API reference on 2026-05-30: Win32 host evidence stays behind first-party runtime host adapters; gameplay/runtime package code consumes value fields only.

## Context

- Parent milestone: `docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md`.
- Predecessor child: `docs/superpowers/plans/2026-05-30-sandbox-world-network-modding-gate-v1.md`.
- Phase 9 landed through PR #312 and merge commit `0fa26b1823d6bb4b68837f19b4040ede781e09b9`.
- Phase 10 starts after Phase 1-9 behavior exists. It is a package/validation proof, not a new gameplay, renderer backend, transport, or mod execution feature.

## Constraints

- SDL3 stays absent from source, package artifacts, docs, and manifest claims.
- Do not add dependency shims, compatibility aliases, or third-party package features.
- Do not mutate runtime package files during review or smoke execution.
- Do not claim broad Terraria-level readiness; only claim exact counters and recipes proven by validation.
- Keep ENet, Vulkan, Metal, Android, and Apple lanes explicitly host-gated where applicable.

## Task 1 - Phase 9 Closeout And Active Plan Selection

**Files:**
- Modify: `docs/superpowers/plans/2026-05-30-sandbox-world-network-modding-gate-v1.md`
- Modify: `docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [x] Mark Phase 9 child closeout tasks complete with PR #312, merge commit `0fa26b1823d6bb4b68837f19b4040ede781e09b9`, hosted `PR Gate`, `Windows MSVC`, Linux, macOS Metal CMake, iOS smoke, static analysis, and CodeQL success evidence.
- [x] Change the plan registry current active row from Phase 9 to this Phase 10 child plan.
- [x] Update parent Phase 9 status from pending PR publication/hosted review to completed; keep Phase 10 unchecked until implemented.
- [x] Update `currentActivePlan` and `recommendedNextPlan` to this file while keeping `unsupportedProductionGaps = []`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

## Task 2 - Source Tree Package Smoke Flags

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/CMakeLists.txt`

- [x] Add CLI options `--require-win32-runtime-host`, `--require-win32-d3d12-presentation`, `--require-wasapi-audio`, and `--require-sandbox-package-budgets`.
- [x] Wire `--require-win32-d3d12-presentation` to require D3D12 shaders and D3D12 presentation, without introducing any SDL3 or backend-native public handle path.
- [x] Treat `--require-wasapi-audio` as a package evidence gate over existing audio production/WASAPI review counters and zero device IO side-effect fields; do not open a live device in the sample.
- [x] Add the new flags to `SMOKE_ARGS` and `PACKAGE_SMOKE_ARGS` only after the underlying Phase 1-9 evidence remains green.
- [x] Add a CTest negative smoke through CMake `add_test(NAME ... COMMAND ...)` that passes `--force-sandbox-package-budget-overflow` and is marked `WILL_FAIL TRUE`.

## Task 3 - Package Budget Counter Rows

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`

- [x] Add a small value-only budget evaluation helper inside the sample for the selected lane counters already emitted by the sample:
  - sandbox rows and diagnostics from `sandbox_world_*`
  - dirty region and chunk byte evidence from sandbox world/persistence/streaming probes
  - tile renderer visible/draw/light/dirty rebuild rows
  - sprite batch and texture-bind budget rows
  - package records and package scene sprite counts
  - side-effect counters for package IO, native handles, arbitrary shell, external downloads, broad multiplayer, backend submission, native texture ownership, and renderer/RHI residency
- [x] Print deterministic fields under a stable prefix:
  - `sandbox_package_budget_status=ready`
  - `sandbox_package_budget_rows`
  - `sandbox_package_budget_diagnostics=0`
  - `sandbox_package_budget_row_limit`
  - `sandbox_package_budget_byte_limit`
  - `sandbox_package_budget_chunk_bytes`
  - `sandbox_package_budget_dirty_chunks`
  - `sandbox_package_budget_renderer_draw_rows`
  - `sandbox_package_budget_tile_draw_calls`
  - `sandbox_package_budget_light_rows`
  - `sandbox_package_budget_streaming_load_rows`
  - `sandbox_package_budget_streaming_unload_rows`
  - `sandbox_package_budget_replay_hash`
  - zero unsupported side-effect counters
- [x] Make `--require-sandbox-package-budgets` fail closed if status is not ready, diagnostics are nonzero, replay hash is zero, required positive counters are absent, or any unsupported side-effect counter is nonzero.

## Task 4 - Over-Budget Negative Probe

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `tools/validate-installed-desktop-runtime.ps1`

- [x] Add `--force-sandbox-package-budget-overflow` for a deterministic negative probe using intentionally tiny row/byte budgets.
- [x] When the overflow flag is present, print `sandbox_package_budget_status=budget_limited`, nonzero `sandbox_package_budget_diagnostics`, and return a nonzero exit code after the status line.
- [x] In installed validation, require the positive `sandbox_package_budget_*` fields when package smoke args include `--require-sandbox-package-budgets`.
- [x] Keep the negative probe in source-tree CTest only; do not make installed package validation run an intentional failing smoke.

## Task 5 - Game Manifest And Validation Recipe Sync

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify static checks only if new recipe ids or machine-readable literals require needles.

- [x] Add `installed-2d-sandbox-package-budget-smoke` to the game manifest recipe ids and quality gates.
- [x] Update the `desktop-runtime-2d-package-proof` recipe purpose to include the new package-budget flags and counters.
- [x] Add a reviewed dry-run recipe plan for the new installed smoke only if `tools/run-validation-recipe-plans.ps1` requires explicit allowlisting.
- [x] Keep the runner fail-closed: no raw manifest command execution, package scripts, or arbitrary shell expansion.

## Task 6 - Docs, Manifest, Static Checks, And Validation

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`

- [x] Update docs with exact supported package-budget proof and explicit non-claims.
- [x] Compose manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
```

- [x] Run targeted drift checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

- [x] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Task 7 - Candidate Publication

**Files:**
- All task-owned files above.

- [x] Commit a verified candidate on `codex/generic-2d-sandbox-package-validation-budgets-v1`.
- [x] Push and open a reviewable PR with local validation evidence.
- [x] Wait for hosted checks including `PR Gate` before merge/cleanup.

## Done When

- The selected 2D package smoke has explicit Win32 runtime host, D3D12 presentation, WASAPI evidence, sandbox-world runtime/persistence/streaming, production tile renderer, sandbox authoring review, and sandbox package budget gates.
- The sample prints deterministic package-budget counters and fails closed on an intentional over-budget negative probe.
- Installed validation checks the positive package-budget fields through metadata-selected package smoke args.
- Docs, manifest fragments, generated manifest, validation recipes, static checks, and plan registry match exactly what the code proves.
- Local full validation and hosted PR evidence exist for the candidate.

## Validation Evidence

- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package"`; source-tree positive and `--force-sandbox-package-budget-overflow` negative smokes passed.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package`; installed validation accepted `sandbox_package_budget_status=ready` and deterministic positive counters.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `git diff --check`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; full static checks, build, tidy smoke, and 84/84 `dev` CTests passed. Apple/Metal and mobile Apple lanes remained host-gated diagnostic-only on this Windows host.
- PASS: PR #313 hosted checks completed for `PR Gate`, `Windows MSVC`, Linux CMake, macOS Metal CMake, iOS Simulator smoke, full repository static analysis shards, `Agent Static Guards`, and CodeQL; Linux Coverage, Linux Clang ASan/UBSan, and Windows C++23 Release Evaluation were skipped by PR validation-tier selection.
- MERGED: PR #313 merged to `main` as merge commit `04cc3b0f9cb1331f1ea21b15f91f116eb3fec8cf`; implementation commit `b473bfd772bc163b83cc5b522c1d3fd5985abec2` is contained in `origin/main`.
