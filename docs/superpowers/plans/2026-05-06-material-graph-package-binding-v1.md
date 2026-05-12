# Material Graph Package Binding v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed, host-independent package update surface that lowers explicit `GameEngine.MaterialGraph.v1` source text into the existing runtime-loadable `GameEngine.Material.v1` material package row.

**Status:** Completed

**Architecture:** Keep `GameEngine.MaterialGraph.v1` as an authoring document in `mirakana_assets`, and reuse the existing `MaterialDefinition` runtime package format for cooked/runtime consumption. `mirakana_tools` gets a narrow `plan_material_graph_package_update` / `apply_material_graph_package_update` surface that reads or accepts explicit graph text, lowers it through `lower_material_graph_to_definition`, validates referenced texture package rows, updates one `.material` file plus the `.geindex`, and keeps shader graph generation, live shader compilation, renderer/RHI residency, package streaming, public native handles, and broad material pipeline readiness out of scope.

**Tech Stack:** C++23, `mirakana_assets` material graph/material contracts, `mirakana_tools`, `mirakana_platform::IFileSystem`, `AssetCookedPackageIndex`, focused `mirakana_tools_tests`, docs/manifest/static AI guidance sync.

---

## Context

- `2026-05-04-material-graph-authoring-v1.md` already added deterministic `GameEngine.MaterialGraph.v1` text IO, validation, editor-core authoring, and lowering to `MaterialDefinition`.
- `2026-05-04-shader-graph-and-generation-pipeline-v1.md` already added a reviewed shader-export planning bridge, but package/runtime material rows still cannot be produced from `.materialgraph` through a reviewed command surface.
- `docs/current-capabilities.md` still explicitly says automatic `.materialgraph` cook/package ingestion is not ready.
- The master plan lists material graph cook/runtime binding as a preferred post-Phase-6 production slice.

## Constraints

- Do not add third-party dependencies.
- Do not execute shader compilers, shell commands, importers, or package scripts from this surface.
- Do not add runtime source parsing; gameplay and runtime package loading continue to consume `GameEngine.Material.v1` rows.
- Do not change renderer/RHI bindings or expose native handles.
- Do not broaden `registered-source-asset-cook-package`, Scene v2 migration, or source asset registration claims in this slice.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Material Graph Package Tests

**Files:**
- Modify: `tests/unit/tools_tests.cpp`

- [x] Add a test named `material graph package tooling lowers graph into runtime material package row`.
- [x] Build an index with two texture entries and no material graph material row.
- [x] Build a valid `MaterialGraphDesc` with base-color and emissive factors plus base-color/emissive texture nodes.
- [x] Call `plan_material_graph_package_update` with explicit graph text, package index content, output `.material` path, and non-zero source revision.
- [x] Assert the plan succeeds, returns `serialize_material_definition(lower_material_graph_to_definition(graph))`, returns exactly two changed files, updates the material package entry, and writes `material_texture` dependency edges for texture nodes only.
- [x] Add an apply test proving `apply_material_graph_package_update` reads graph/index text and writes the `.material` and `.geindex` only after validation passes.
- [x] Add rejection coverage for unsafe paths, invalid graph text, missing texture package rows, wrong texture kinds, existing non-material rows, output path collisions, zero source revision, shader graph, live shader generation, renderer/RHI residency, and package streaming claims.
- [x] Run `cmake --build --preset dev --target mirakana_tools_tests` and confirm RED because the new API does not exist.

### Task 2: Package Binding API

**Files:**
- Modify: `engine/tools/include/mirakana/tools/material_tool.hpp`
- Modify: `engine/tools/src/material_tool.cpp`

- [x] Add `MaterialGraphPackageUpdateDesc`, `MaterialGraphPackageApplyDesc`, and `MaterialGraphPackageUpdateResult` value types reusing the existing changed-file and failure row shapes.
- [x] Add public functions:
  - `plan_material_graph_package_update(const MaterialGraphPackageUpdateDesc& desc)`
  - `apply_material_graph_package_update(IFileSystem& filesystem, const MaterialGraphPackageApplyDesc& desc)`
- [x] Parse `desc.material_graph_content` through `deserialize_material_graph`.
- [x] Lower with `lower_material_graph_to_definition` and serialize the result through `serialize_material_definition`.
- [x] Extract texture dependencies from the lowered material, sort/dedupe them, and validate each dependency exists as `AssetKind::texture`.
- [x] Update or add the `AssetKind::material` package entry with the lowered material content hash and source revision.
- [x] Replace dependency edges for that material with deterministic `AssetDependencyKind::material_texture` rows.
- [x] Keep unsupported sentinels for shader graph, live shader generation, renderer/RHI residency, and package streaming.

### Task 3: GREEN Tools Tests

**Files:**
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `engine/tools/include/mirakana/tools/material_tool.hpp`
- Modify: `engine/tools/src/material_tool.cpp`

- [x] Run `cmake --build --preset dev --target mirakana_tools_tests` until the new tests pass.
- [x] Run `ctest --preset dev -R "mirakana_tools_tests" --output-on-failure`.
- [x] Refactor only while the focused tests stay green.

### Task 4: Docs, Manifest, Skills, And Static Guidance

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `tools/check-ai-integration.ps1`

- [x] Document that authored material graphs can now be lowered into runtime `.material` package rows through the reviewed `mirakana_tools` surface.
- [x] Keep ready claims narrow: no shader graph codegen promotion, live shader generation, shader compiler execution, renderer/RHI residency, package streaming, public native/RHI handles, Metal readiness, or broad renderer quality.
- [x] Update manifest `mirakana_tools` purpose, command/package surfaces, generated-game guidance, unsupported gap notes, and `recommendedNextPlan` text.
- [x] Update static AI checks so the new API and non-goals stay synchronized.
- [x] Return `currentActivePlan` to the master plan after validation passes.

### Task 5: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run focused `mirakana_tools_tests` build/CTest commands from Task 3.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and move this plan to Recent Completed in the registry.

## Done When

- `plan_material_graph_package_update` accepts explicit `GameEngine.MaterialGraph.v1` text, lowers it to `GameEngine.Material.v1`, and returns deterministic `.material` plus `.geindex` changed-file rows.
- `apply_material_graph_package_update` performs the same validation before writing through `IFileSystem`.
- Texture dependencies from graph texture nodes are reflected as material package dependencies and `material_texture` edges.
- Unsupported shader graph, live shader generation, renderer/RHI residency, package streaming, native handle, and broad renderer quality claims remain rejected or documented out of scope.
- Docs, manifest, Codex/Claude skills, and static checks state the new boundary honestly.
- Focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete local-tool blocker is recorded.

## Validation Results

- `cmake --build --preset dev --target mirakana_tools_tests`: direct host invocation was blocked by duplicate `Path` / `PATH` environment entries before the compiler ran; the sanitized equivalent below was used for actual RED/GREEN evidence.
- `cmd.exe /D /S /C "set Path=%PATH%&& set PATH=&& cmake --build --preset dev --target mirakana_tools_tests"`: RED before implementation on missing `MaterialGraphPackage*` API; PASS after implementation.
- `ctest --preset dev -R "mirakana_tools_tests" --output-on-failure`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS after syncing the mirrored `check-json-contracts.ps1` apply-ready allowlist for `create-material-from-graph`; final run reports `validate: ok`.
