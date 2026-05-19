# Scene v2 Stable-Id Prefab Refresh Plan v1 (2026-05-18)

**Plan ID:** `scene-v2-stable-id-prefab-refresh-plan-v1`
**Status:** Completed
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `scene-component-prefab-schema-v2`

## Goal

Add a non-mutating Scene/Prefab v2 prefab instance refresh planning contract that compares an existing selected prefab instance subtree against a refreshed `PrefabDocumentV2` through stable source node/component ids.

## Context

`Scene v2 Prefab Source Provenance v1` added deterministic `SceneNodePrefabSourceV2` / `SceneComponentPrefabSourceV2` rows for copied prefab instance nodes and components. The next foundation step is a schema-level review plan that can say which source rows would be preserved, added, or removed after a prefab refresh without relying on editor-only source-name/index matching.

## Constraints

- Keep the contract in `MK_scene`; do not add editor mutation, undo actions, visible UI, runtime prefab instance semantics, or package commands.
- Use selected-root review through `instance_root_node` so multiple instances of the same prefab path do not collapse into one path-wide comparison.
- Compare only stable `source_node_id` / `source_component_id` rows from matching `prefab_source` provenance.
- Keep nested prefab propagation, local-child merge policy, fuzzy merge/rebase, automatic conflict UX, broad/dependent package cooking, renderer/RHI residency, package streaming, and native handles unsupported.

## Done When

- `mirakana::plan_scene_prefab_instance_refresh_v2` returns a valid, non-mutating, non-executing `ScenePrefabInstanceRefreshPlanV2` for a selected root with deterministic preserve/add/remove rows and node/component counters.
- The plan derives its prefab path from the selected root's `SceneNodePrefabSourceV2` row.
- Focused tests prove stable source id preserve/add/remove planning for both nodes and components.
- Docs, manifest fragments, composed manifest, and static checks describe the same narrow ready surface and remaining unsupported work.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` before implementation | RED: missing `plan_scene_prefab_instance_refresh_v2` and `ScenePrefabInstanceRefreshRowKindV2` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` after implementation | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_scene_schema_v2_tests$"` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/scene/src/schema_v2.cpp,tests/unit/scene_schema_v2_tests.cpp` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS; 65 tests passed; Metal/Apple host checks remained diagnostic-only/host-gated on Windows |


