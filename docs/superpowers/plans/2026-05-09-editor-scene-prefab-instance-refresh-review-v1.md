# Editor Scene Prefab Instance Refresh Review Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed, undoable editor refresh path for scene prefab instances that already carry `ScenePrefabSourceLink` metadata.

**Architecture:** Keep the behavior in `mirakana_editor_core` as a GUI-independent review/apply model over `SceneAuthoringDocument`. The optional `mirakana_editor` shell may call the model from the Scene panel, but editor core must not execute package scripts, validation recipes, renderer/RHI work, native handles, or arbitrary file operations. The first refresh slice is intentionally conservative: it maps instance nodes by unique `source_node_name`, preserves existing scene node state for retained nodes, adds new source nodes from the refreshed prefab, removes reviewed stale source nodes, and blocks ambiguous or mixed local-child cases.

**Tech Stack:** C++23, `mirakana_scene`, `mirakana_editor_core`, retained `mirakana_ui` model rows, `mirakana_editor` Dear ImGui adapter, CMake/CTest, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Status

Completed.

## Context

The selected production gap is `editor-productization`. The previous slices added reviewed prefab variant base refresh/merge rows and scene prefab source-link diagnostics. This slice turns source-link diagnostics into an explicit, host-independent refresh review/apply operation for scene prefab instances. It does not claim full nested prefab propagation, fuzzy merge/rebase UX, runtime prefab instance semantics, package scripts, validation recipe execution, renderer/RHI uploads, native handles, or package streaming.

## Constraints

- Use TDD: add failing `mirakana_editor_core_tests` coverage before production code.
- Keep `editor/core` GUI-independent.
- Preserve existing scene node state for retained source nodes; do not infer local override intent from unavailable historical source data.
- Block refresh when the selected root is not a valid linked prefab root, when source names are ambiguous, or when the subtree contains unlinked/different-link local children that would be deleted.
- Apply only through an explicit undoable action.
- Update docs, manifest, skills, and static checks after focused tests are green.

## Files

- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`

## Tasks

### Task 1: Red Tests

- [x] Add `editor scene prefab instance refresh review preserves linked nodes and applies with undo` to `tests/unit/editor_core_tests.cpp`.
- [x] The test must instantiate a prefab with `source_path`, edit one retained node in the scene, refresh against a prefab with one added node and one removed node, verify retained state is preserved, added nodes use refreshed prefab data, removed source nodes are gone, source links are updated, retained `scene_prefab_instance_refresh` UI rows exist, and undo restores the original scene.
- [x] Add `editor scene prefab instance refresh review blocks unsafe mappings` to `tests/unit/editor_core_tests.cpp`.
- [x] The test must cover a non-root linked node, duplicate refreshed source names, and an unlinked local child under the instance root.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests`; expected RED is missing `ScenePrefabInstanceRefresh*`, `plan_scene_prefab_instance_refresh`, `apply_scene_prefab_instance_refresh`, `make_scene_prefab_instance_refresh_action`, or `make_scene_prefab_instance_refresh_ui_model`.

### Task 2: Editor-Core Contract

- [x] Add refresh status, row-kind, row, plan, and result structs to `editor/core/include/mirakana/editor/scene_authoring.hpp`.
- [x] Add declarations for `scene_prefab_instance_refresh_status_label`, `scene_prefab_instance_refresh_row_kind_label`, `plan_scene_prefab_instance_refresh`, `apply_scene_prefab_instance_refresh`, `make_scene_prefab_instance_refresh_action`, and `make_scene_prefab_instance_refresh_ui_model`.
- [x] Implement the conservative source-name review in `editor/core/src/scene_authoring.cpp`.
- [x] Implement scene rebuild/apply by replacing the linked root subtree, preserving outside nodes, preserving retained linked node name/transform/components, adding new refreshed nodes, removing reviewed stale nodes, and selecting the refreshed counterpart or refreshed root.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`; expected GREEN.

### Task 3: Visible Editor Adapter

- [x] Wire the Scene panel to show a `Refresh Prefab Instance` control only for selected linked root nodes with a safe prefab path.
- [x] Load the refreshed prefab through the existing project text store and call `make_scene_prefab_instance_refresh_action`; log a warning on blocked review or load failure.
- [x] Keep file path conversion, loading, and ImGui state in `mirakana_editor`; do not move I/O or Dear ImGui into `editor/core`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, and Static Checks

- [x] Update editor docs, current capabilities, roadmap, testing docs, master plan, registry, manifest, and Codex/Claude editor skills with the narrow ready boundary.
- [x] Add static check sentinels for the new API names, retained `scene_prefab_instance_refresh` rows, tests, docs, manifest guidance, and unsupported boundary text.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

### Task 5: Closeout Validation

- [x] Run `git diff --check` on touched files.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence in this plan.
- [x] Update this plan status to `Completed`, point `currentActivePlan` back to the master plan, update the plan registry latest completed slice, and keep `recommendedNextPlan.id=next-production-gap-selection`.

## Done When

- Focused editor-core tests prove RED then GREEN for review/apply and blockers.
- Visible `mirakana_editor` can explicitly refresh a selected linked prefab root from its reviewed source path.
- Docs, skills, manifest, and static checks agree that this is a narrow reviewed scene instance refresh surface, not full nested prefab propagation or broad merge/rebase UX.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes, or a concrete host/tool blocker is recorded here.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_editor_core_tests` | Passed | RED before implementation failed on the missing `ScenePrefabInstanceRefresh*` API surface; the focused target then built successfully after implementation. |
| `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` | Passed | Focused editor-core test run passed, including refresh apply/undo and unsafe-mapping blockers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | Visible `mirakana_editor` adapter and desktop GUI lane built; desktop-gui CTest completed 46/46. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public editor-core header additions accepted by the API boundary check. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest/static JSON contracts passed after docs/manifest synchronization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration sentinels passed after docs, manifest, skills, plan registry, and static-check updates. |
| `git diff --check -- .agents/skills/editor-change/SKILL.md .claude/skills/gameengine-editor/SKILL.md docs/editor.md docs/current-capabilities.md docs/roadmap.md docs/testing.md docs/superpowers/plans/README.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md docs/superpowers/plans/2026-05-09-editor-scene-prefab-instance-refresh-review-v1.md editor/core/include/mirakana/editor/scene_authoring.hpp editor/core/src/scene_authoring.cpp editor/src/main.cpp engine/agent/manifest.json tests/unit/editor_core_tests.cpp tools/check-ai-integration.ps1 tools/check-json-contracts.ps1` | Passed | Whitespace check passed; Git reported line-ending normalization warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed. `production-readiness-audit-check` still reports 11 non-ready unsupported gaps, including `editor-productization` as `partly-ready`; Metal/Apple diagnostics remain host-gated. |
