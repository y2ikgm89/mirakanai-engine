# Scene v2 Registered Asset Runtime Workflow Validation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the AI-operable authored-to-runtime workflow by chaining reviewed source asset registration, explicit registered source asset cook/package updates, Scene v2 runtime package migration, and runtime package loading/scene instantiation in focused tests and docs.

**Architecture:** Do not create a broad package-cook command. Keep each mutation on its reviewed surface: `register-source-asset`, `cook-registered-source-assets`, Scene/Prefab v2 authoring, `migrate-scene-v2-runtime-package`, and runtime package validation. The workflow proof should make the supported sequence obvious to agents without adding renderer/RHI residency, package streaming, optional external importer execution, editor productization, material/shader graphs, or public native handles.

**Tech Stack:** C++23 tests over `mirakana_assets`, `mirakana_tools`, `mirakana_runtime`, `mirakana_runtime_scene`, and existing docs/manifest/static checks.

---

## Goal

Make the current AI command surfaces usable as a coherent authored scene package workflow:

- define the minimal first-party fixture sequence for source texture/material/mesh rows, explicit cook/package, authored Scene v2 migration, runtime package load, and runtime scene instantiation
- add focused tests that prove the sequence is deterministic and rejects skipped prerequisites
- update docs/manifest guidance so agents choose this sequence instead of inventing broad package cooking or runtime source parsing
- keep the workflow host-independent and renderer-free; visible GPU/package smokes remain separate host-gated recipes

## Context

- `register-source-asset` records source identity/import intent.
- `cook-registered-source-assets` now cooks explicitly selected registered rows and updates `.geindex`.
- `migrate-scene-v2-runtime-package` bridges authored Scene v2 into the existing Scene v1 package update surface.
- `mirakana_runtime` / `mirakana_runtime_scene` already load cooked packages and instantiate runtime scene state.

## Constraints

- Do not add third-party dependencies.
- Do not introduce a catch-all package cooking command.
- Do not execute optional external importer adapters in the initial workflow proof.
- Do not claim renderer/RHI residency, package streaming, material/shader graph readiness, live shader generation, editor productization, Metal readiness, public native/RHI handles, or general production renderer quality.
- Keep runtime gameplay on cooked package APIs; no runtime source parsing.

## Done When

- Focused tests prove an authored Scene v2 plus explicitly selected registered source rows can become a runtime-loadable package and instantiate through runtime scene APIs.
- Tests cover skipped or stale prerequisites, unsafe paths, and unsupported broad-workflow claims.
- Docs and manifest guidance describe the supported sequence clearly.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory And Workflow Contract

**Files:**
- Read: `tests/unit/tools_tests.cpp`
- Read: `tests/unit/runtime_scene_tests.cpp`
- Read: `engine/tools/include/mirakana/tools/registered_source_asset_cook_package_tool.hpp`
- Read: `engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp`
- Read: `engine/runtime/include/mirakana/runtime/asset_package_runtime.hpp`
- Read: `engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp`
- Read: `docs/ai-game-development.md`
- Read: `docs/specs/generated-game-validation-scenarios.md`

- [x] Define the exact fixture sequence and runtime assertions.
- [x] Decide whether the proof belongs in existing tools/runtime tests or a new focused test case.
- [x] Define diagnostics for skipped cook/package or missing runtime package rows.
- [x] Record non-goals before RED tests are added.

Task 1 contract: the host-independent proof lives in `tests/unit/tools_tests.cpp` and chains existing reviewed helpers only: source registry rows plus first-party source payloads, `apply_registered_source_asset_cook_package`, `apply_scene_v2_runtime_package_migration`, `mirakana::runtime::load_runtime_asset_package`, and `mirakana::runtime_scene::instantiate_runtime_scene`. The runtime assertions are package load success, four records, Scene v2-derived Scene v1 name, and mesh/material/sprite reference validation. Skipped cook/package prerequisites must surface existing scene package diagnostics such as missing scene mesh/material/sprite package entries.

Task 1 non-goals: no new broad workflow/orchestrator command, no `cook-runtime-package` ready promotion, no runtime source parsing, no renderer/RHI residency, no package streaming, no `mirakana_scene_renderer`, no `mirakana_renderer`, no `mirakana_runtime_rhi`, no `mirakana_runtime_scene_rhi`, no desktop runtime, no editor productization, no material/shader graph, no live shader generation, no Metal readiness, and no public native/RHI handles.

### Task 2: RED Tests And Static Guidance

**Files:**
- Modify: focused test files selected in Task 1
- Modify: docs/static checks if guidance needs enforcement
- Modify: this plan

- [x] Add failing test for full source-registry cook -> Scene v2 migration -> runtime package load -> runtime scene instantiation.
- [x] Add failing tests for missing cooked dependency rows and skipped explicit cook step.
- [x] Add failing checks that keep broad workflow claims out of docs.
- [x] Record RED evidence.

### Task 3: Production Implementation

**Files:**
- Modify only the smallest helper/test/doc set required by RED evidence.

- [x] Reuse existing helpers instead of creating broad orchestration.
- [x] Add any narrow adapter only if a current API gap prevents the tested workflow.
- [x] Keep runtime source parsing and renderer/RHI ownership out of the workflow.

### Task 4: Manifest, Docs, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: this plan

- [x] Update guidance so agents use the validated sequence.
- [x] Keep broader rendering/package/editor claims planned or host-gated.
- [x] Run required validation commands and record evidence.
- [x] Mark this plan complete and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- 2026-05-01 RED:
  - Direct `cmake --build --preset dev --target mirakana_tools_tests` could not run from this PowerShell PATH because `cmake` was not found; the RED compile was rerun through the repository script.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: FAIL as expected after adding the integrated workflow test. `mirakana_tools_tests` failed to compile with `C1083` because `mirakana/runtime_scene/runtime_scene.hpp` was not reachable from the test target before `mirakana_tools_tests` linked `mirakana_runtime_scene`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: FAIL as expected after adding static workflow guidance checks. `tools/check-ai-integration.ps1` reported missing `validated authored-to-runtime workflow` text in `docs/ai-game-development.md`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: FAIL as expected after adding matching JSON/static workflow checks. `tools/check-json-contracts.ps1` reported missing `validated authored-to-runtime workflow` text in `docs/ai-game-development.md`.
- 2026-05-01 GREEN:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: PASS, 28/28 tests after linking `mirakana_tools_tests` with `mirakana_runtime_scene` and keeping the workflow proof on existing cook, migration, runtime package load, and `mirakana_runtime_scene` instantiation APIs.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS after docs/manifest guidance used the validated authored-to-runtime workflow and kept broader claims out.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS after matching JSON/static guidance checks were satisfied.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: PASS, diagnostic-only; Metal tools are still missing on this host.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 28/28 tests; diagnostic-only host gates remain Metal tools, Apple packaging on non-macOS, Android release signing/device smoke configuration, and tidy compile database availability before configure.
  - Next plan created: `docs/superpowers/plans/2026-05-01-runtime-scene-package-validation-command-tooling-v1.md`.
