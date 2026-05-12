# Editor Prefab Instance Source-Link Review Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `editor-prefab-instance-source-link-review-v1`  
**Status:** Completed.  
**Goal:** Add durable, reviewed source-link metadata for scene nodes instantiated from prefabs so later nested prefab propagation and merge review can diagnose source identity without guessing.

**Architecture:** Store prefab source-link metadata as first-party `mirakana_scene` authoring data on instantiated `SceneNode` rows, then expose deterministic `mirakana_editor_core` review rows and retained `mirakana_ui` ids. Keep the slice review-only: it records source prefab path/name and source node index/name, renders diagnostics, and never applies automatic nested propagation, fuzzy matching, or merge/rebase resolution.

**Tech Stack:** C++23, `mirakana_scene`, `mirakana_editor_core`, `mirakana_ui`, Dear ImGui adapter in optional `mirakana_editor`, focused `mirakana_editor_core_tests`, `mirakana_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Context

- The active production gap is `editor-productization` in `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps`.
- `Editor Prefab Variant Base Refresh Merge Review v1` completed embedded base refresh for variants, but scene nodes instantiated from prefabs still do not carry durable source identity.
- `mirakana::PrefabInstance` currently returns instantiated scene node IDs only, and `make_scene_authoring_instantiate_prefab_action` takes a plain `PrefabDefinition`.
- This slice should establish source-link evidence only. Full nested prefab propagation, automatic merge, rebase, fuzzy matching, runtime prefab instance semantics, package streaming, renderer/RHI work, and dynamic game-module loading remain unsupported.

## Files

- Modify `engine/scene/include/mirakana/scene/scene.hpp`: add `ScenePrefabSourceLink` and optional source-link metadata on `SceneNode`.
- Modify `engine/scene/include/mirakana/scene/prefab.hpp`: add instantiate input metadata or equivalent path-aware overload.
- Modify `engine/scene/src/scene.cpp`: preserve source-link metadata through node ownership operations.
- Modify `engine/scene/src/scene_io.cpp`: serialize/deserialize source-link rows deterministically and reject incomplete or unsafe link data.
- Modify `engine/scene/src/prefab.cpp`: populate source-link metadata when instantiating a valid prefab.
- Modify `editor/core/include/mirakana/editor/scene_authoring.hpp`: add review row/model APIs and path-aware instantiate action.
- Modify `editor/core/src/scene_authoring.cpp`: build review rows, retained UI rows, and undoable source-linked instantiate actions.
- Modify `editor/src/main.cpp`: render selected-node and scene-wide prefab source-link review rows; pass the saved prefab or variant path into instantiate actions.
- Modify `tests/unit/core_tests.cpp`: cover scene serialization and prefab instantiation source-link metadata.
- Modify `tests/unit/editor_core_tests.cpp`: cover editor review rows, retained UI ids, undo/redo, stale diagnostics, and variant composed prefab source path.
- Update `docs/editor.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`, `engine/agent/manifest.json`, `docs/superpowers/plans/README.md`, and this plan.
- Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` so the new contract stays synchronized.

## Tasks

### Task 1: Source-Link Data Contract

- [x] Add failing `mirakana_core_tests` coverage proving prefab instantiation records source prefab name, optional source path, source node index, and source node name on every instantiated scene node.
- [x] Add failing `mirakana_core_tests` coverage proving `serialize_scene` / `deserialize_scene` round-trip source-link rows and reject incomplete link payloads.
- [x] Implement `ScenePrefabSourceLink`, validation, deterministic serialization, deserialization, and prefab instantiation population.
- [x] Run `cmake --build --preset dev --target mirakana_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_core_tests`.

### Task 2: Editor Review Model

- [x] Add failing `mirakana_editor_core_tests` coverage for `ScenePrefabInstanceSourceLinkModel` rows over a source-linked scene, including stable ids and diagnostics for stale/missing links.
- [x] Add failing retained `mirakana_ui` coverage for `scene_prefab_source_links` ids.
- [x] Implement `ScenePrefabInstanceSourceLinkRow`, `ScenePrefabInstanceSourceLinkModel`, `make_scene_prefab_instance_source_link_model`, `make_scene_prefab_instance_source_link_ui_model`, and a path-aware instantiate action.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`.

### Task 3: Visible Editor Adapter

- [x] Update `mirakana_editor` Scene panel to pass `current_prefab_path()` when instantiating the saved prefab and `prefab_variant_path_` when instantiating a composed variant.
- [x] Render source-link review rows in the Scene panel without mutation controls or automatic propagation actions.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Documentation And Static Sync

- [x] Update docs, skills, manifest, plan registry, and static checks to describe source-link review as implemented while keeping nested propagation unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and targeted `git diff --check` for touched files.

### Task 5: Slice Closeout

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence in this plan.
- [x] Update `docs/superpowers/plans/README.md`, master plan, and `engine/agent/manifest.json.aiOperableProductionLoop` to mark this child plan completed and return `currentActivePlan` to the master plan if no child slice remains active.

## Done When

- Instantiated prefab scene nodes carry durable source-link metadata with deterministic text IO.
- Editor core exposes review-only source-link rows and retained `scene_prefab_source_links` UI ids.
- The visible editor surfaces the review rows and passes saved prefab/variant source paths into instantiate actions.
- Tests and docs prove this is a narrow source-link review surface, not nested prefab propagation.
- Focused tests, API boundary check, GUI build, agent/schema checks, targeted diff check, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete host/tool blockers.

## Validation Evidence

| Command | Result |
| --- | --- |
| `cmake --build --preset dev --target mirakana_core_tests` before implementation | Failed as expected: missing `mirakana::PrefabInstantiateDesc`, `SceneNode::prefab_source`, and `mirakana::ScenePrefabSourceLink`. |
| `cmake --build --preset dev --target mirakana_editor_core_tests` before implementation | Failed as expected: missing path-aware instantiate action and source-link review/UI APIs. |
| `cmake --build --preset dev --target mirakana_core_tests` | Passed. |
| `cmake --build --preset dev --target mirakana_editor_core_tests` | Passed. |
| `ctest --preset dev --output-on-failure -R mirakana_core_tests` | Passed, 1/1. |
| `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` | Passed, 1/1. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed; desktop-gui CTest passed 46/46. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed after synchronizing the new source-link sentinel strings. |
| `git diff --check -- <touched source/docs/static-check files>` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed; production readiness audit still honestly reports 11 unsupported gaps, with `editor-productization` remaining `partly-ready`. |
