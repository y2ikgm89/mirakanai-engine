# Runtime Scene Package Validation Command Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed host-independent validation command surface that loads an explicit runtime `.geindex` package and instantiates an explicit scene asset through `mirakana_runtime` and `mirakana_runtime_scene`.

**Architecture:** Keep this non-mutating and renderer-free. Reuse `mirakana::runtime::load_runtime_asset_package` and `mirakana::runtime_scene::instantiate_runtime_scene`; put any AI-reviewed request/result wrapper in `mirakana_tools`. Do not create a broad package cook, package streaming, renderer/RHI residency, desktop runtime, editor, or source parsing surface.

**Tech Stack:** C++23 `mirakana_tools`, `mirakana_runtime`, `mirakana_runtime_scene`, focused tests, `engine/agent/manifest.json`, docs, and existing static checks.

---

## Goal

Make the validated authored-to-runtime workflow operationally checkable by agents after package mutation:

- dry-run/execute a reviewed `validate-runtime-scene-package` surface for a safe package index path and explicit scene asset key/id
- report deterministic diagnostics for package load failures, missing scene rows, wrong kinds, malformed scene payloads, missing or wrong referenced assets, duplicate node names when requested, unsafe paths, unsupported claims, and free-form edit attempts
- return structured package/scene summary rows suitable for docs, validation recipe guidance, and future editor diagnostics
- keep gameplay/runtime code on cooked package APIs only

## Context

- `scene-v2-registered-asset-runtime-workflow-validation-v1` proved the end-to-end sequence in tests.
- Agents still need a narrow reviewed validation surface for the final runtime package load and `mirakana_runtime_scene` instantiation check instead of inventing broad package cooking, renderer smokes, or runtime source parsing.
- Existing runtime APIs already own package hash/dependency validation and scene reference diagnostics.

## Constraints

- Do not add third-party dependencies.
- Do not mutate package, source, scene, prefab, material, manifest, or build files.
- Do not parse source assets at runtime.
- Do not execute arbitrary shell, optional external importers, renderer/RHI upload/residency, desktop runtime, editor UI, package streaming, material/shader graphs, live shader generation, Metal tooling, or native handle access.
- Keep this separate from `run-validation-recipe`; this is a C++ package/scene validation command surface, not a shell recipe runner.

## Done When

- A focused RED -> GREEN record exists in this plan.
- `validate-runtime-scene-package` exists as a reviewed non-mutating dry-run/execute surface.
- Tests cover success, package load failure, missing scene row, wrong scene kind, malformed scene payload, missing referenced asset, referenced kind mismatch, unsafe path, unsupported claims, and free-form edit rejection.
- Manifest, docs, static checks, and `agent-context` expose the new surface without broad renderer/package/editor claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Runtime Validation Contracts

**Files:**
- Read: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Read: `engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp`
- Read: `tests/unit/runtime_tests.cpp`
- Read: `tests/unit/runtime_scene_tests.cpp`
- Read: `tests/unit/tools_tests.cpp`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `docs/ai-game-development.md`
- Read: `docs/specs/generated-game-validation-scenarios.md`

- [x] Define request/result fields, command id, diagnostic codes, summary rows, and unsupported claim sentinels.
- [x] Decide the smallest `mirakana_tools` wrapper shape and test placement.
- [x] Define safe path and scene asset key/id validation.
- [x] Record non-goals before RED tests are added.

**Task 1 Inventory Decisions**

- Command id: `validate-runtime-scene-package`.
- C++ wrapper: `mirakana::plan_runtime_scene_package_validation` for reviewed dry-run shape checks, and `mirakana::execute_runtime_scene_package_validation` for non-mutating package read plus scene instantiation.
- Request fields: command kind, safe package-relative `.geindex` path, optional safe package-relative `content_root`, explicit `AssetKeyV2 scene_asset_key`, `validate_asset_references`, `require_unique_node_names`, and unsupported-claim sentinels.
- Result fields: package/scene summary, package record count, scene node count, reference summary rows, deterministic diagnostics, validation recipes, unsupported gap ids, and placeholder undo token.
- Diagnostic codes: unsafe package/content paths, invalid scene asset key, unsupported/free-form operation, runtime package load failures, missing scene asset, wrong asset kind, malformed scene payload, missing referenced asset, referenced asset kind mismatch, and duplicate node name.
- Safe path rules: package index must be repository/package-relative, segment-safe, and end in `.geindex`; content root may be empty or segment-safe package-relative; scene asset key uses the existing lowercase slash-separated `AssetKeyV2` policy.
- Non-goals: no mutation, package cooking, runtime source parsing, external importer execution, renderer/RHI residency, package streaming, desktop/editor work, material/shader graph authoring, live shader generation, Metal readiness, public native/RHI handles, or general production renderer quality claim.

### Task 2: RED Tests And Static Checks

**Files:**
- Modify: focused test files selected in Task 1
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: this plan

- [x] Add failing success and failure-mode tests for the reviewed validation surface.
- [x] Add failing static checks requiring the descriptor, helper files, result fields, and no renderer/package/editor/source claims.
- [x] Record RED evidence.

### Task 3: Production Implementation

**Files:**
- Modify: `engine/tools/include/mirakana/tools/runtime_scene_package_validation_tool.hpp`
- Modify: `engine/tools/src/runtime_scene_package_validation_tool.cpp`
- Modify: `engine/tools/CMakeLists.txt`
- Modify: focused tests

- [x] Implement the wrapper by reusing `load_runtime_asset_package` and `instantiate_runtime_scene`.
- [x] Keep execute non-mutating and deterministic.
- [x] Keep runtime source parsing, renderer/RHI ownership, and shell execution out.

### Task 4: Manifest, Docs, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-integration.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: this plan

- [x] Promote only the reviewed non-mutating runtime scene package validation surface to ready.
- [x] Keep broader renderer/package/editor claims planned or host-gated.
- [x] Run required validation commands and record evidence.
- [x] Mark this plan complete and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding `validate-runtime-scene-package` tests: `tests/unit/tools_tests.cpp(22,10): error C1083: include file 'mirakana/tools/runtime_scene_package_validation_tool.hpp' not found`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected after adding static checks: `engine/agent/manifest.json aiOperableProductionLoop must expose one ready validate-runtime-scene-package command surface`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding static checks: `engine manifest aiOperableProductionLoop must expose one ready validate-runtime-scene-package command surface`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed with 28/28 tests after adding `mirakana::plan_runtime_scene_package_validation` and `mirakana::execute_runtime_scene_package_validation`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after manifest/docs/static-check synchronization.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after manifest/docs/static-check synchronization.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and exposed the ready `validate-runtime-scene-package` command surface.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after adding the public `mirakana/tools/runtime_scene_package_validation_tool.hpp` header.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only: D3D12 DXIL, Vulkan SPIR-V, and DXC SPIR-V CodeGen were ready; Metal `metal`/`metallib` remained missing as host/toolchain blockers.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed and executed only the reviewed `agent-check` plus `schema-check` command plan.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 tests. Diagnostic-only gates remained Metal tools missing, Apple packaging blocked on non-macOS/Xcode, Android release signing/device smoke not fully configured, and tidy compile database availability.

## Completion

Completed on 2026-05-01. The next active focused plan is `docs/superpowers/plans/2026-05-01-game-manifest-runtime-scene-validation-targets-v1.md`.
