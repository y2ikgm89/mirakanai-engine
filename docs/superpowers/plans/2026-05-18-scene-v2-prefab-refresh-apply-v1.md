# Scene v2 Prefab Refresh Apply v1 (2026-05-18)

**Plan ID:** `scene-v2-prefab-refresh-apply-v1`
**Status:** Completed

## Goal

Add the first Schema v2 stable-id prefab refresh apply contract in `MK_scene`: a pure value API that turns a reviewed `plan_scene_prefab_instance_refresh_v2`-valid selected instance root plus refreshed `PrefabDocumentV2` into an updated `SceneDocumentV2`.

## Context

The preceding Scene v2 prefab refresh slices added deterministic copied-row source provenance, non-mutating stable-id preserve/add/remove review rows, duplicate source identity diagnostics, nested prefab root rejection, and local selected-subtree ownership rejection. The next foundation step is to prove the same stable-id rows can be applied inside the Schema v2 value layer without making editor actions, AI commands, runtime prefab semantics, nested propagation, package cooking, renderer/RHI residency, or native handles ready.

## Constraints

- Keep `apply_scene_prefab_instance_refresh_v2` a pure value API over already loaded documents.
- Preserve matched current node/component ids and author-owned state.
- Add only deterministic `instance_root/refresh/<source-id>` node/component ids for new source rows.
- Remove stale sourced rows from the selected instance subtree.
- Return source-to-result mappings and structured diagnostics.
- Do not write files, create editor undo actions, expose an AI command surface, implement runtime prefab instance semantics, or propagate nested prefab refreshes.

## Done When

- `ScenePrefabInstanceRefreshResultV2` and `apply_scene_prefab_instance_refresh_v2` are public `MK_scene` APIs.
- `MK_scene_schema_v2_tests` covers stable-id value apply for preserve/add/remove nodes and components plus source-to-result mappings.
- Docs, manifest fragments, composed manifest, and static guards describe the value apply boundary consistently.
- Focused build/test/static checks and one full `tools/validate.ps1` pass for the C++/public-contract phase.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` failed because `mirakana::apply_scene_prefab_instance_refresh_v2` was missing.
- Focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` passed.
- Focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_scene_schema_v2_tests` passed.
- Focused/static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after `tools/format.ps1`.
- Focused/static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Focused/static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- Focused/static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- Focused/static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Focused/static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/scene/src/schema_v2.cpp,tests/unit/scene_schema_v2_tests.cpp` passed.
- Full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It reported 65/65 CTest tests passed; Metal shader tools and Apple host evidence remained diagnostic/host-gated on this Windows host.
