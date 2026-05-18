# Scene v2 Prefab Refresh Command Surface v1 - 2026-05-18

## Goal

Expose the stable-id Schema v2 prefab instance refresh apply as a reviewed `MK_tools` Scene/Prefab v2 authoring command so agents can dry-run or apply one selected authored `.scene` refresh through `plan_scene_prefab_authoring` / `apply_scene_prefab_authoring`.

## Context

The preceding Scene v2 prefab refresh slices added deterministic copied-row source provenance, non-mutating stable-id refresh planning, duplicate-source and ownership guards, and pure value `apply_scene_prefab_instance_refresh_v2`. The remaining foundation step for this slice is connecting that value apply to the existing shell-free `scene_prefab_authoring_tool` surface without widening generated-game descriptors, runtime package cooking, editor actions, or nested prefab propagation.

## Constraints

- Use `ScenePrefabAuthoringCommandKind::refresh_prefab_instance` and the existing `ScenePrefabAuthoringRequest` fields: safe `.scene` path, safe `.prefab` path, and selected `node_id`.
- Write only the validated authored `.scene` result through `IFileSystem`.
- Keep `apply_scene_prefab_instance_refresh_v2` as a value API; it remains independent of file IO and editor undo.
- Keep `prefabScenePackageAuthoringTargets` command rows at their existing five generated-package operations for this slice.
- Keep nested prefab propagation, local merge UX, runtime prefab semantics, broad/dependent package cooking, renderer/RHI residency, package streaming, editor productization, public native handles, and arbitrary shell unsupported.

## Done When

- `MK_tools` exposes dry-run/apply support for `refresh_prefab_instance`.
- `MK_tools_tests` proves the reviewed apply surface reads `.scene` / `.prefab`, refreshes by stable source ids, preserves author-owned matched component state, removes stale sourced rows, adds deterministic refresh rows, and writes only the selected `.scene`.
- `aiCommandSurfaces` includes ready `refresh-prefab-instance` with narrow request/result/policy text.
- Docs, manifest fragments, composed manifest, and static checks agree on the new command surface and remaining unsupported claims.
- Focused build/test/static checks and full `tools/validate.ps1` pass, or a concrete blocker is recorded.

## Implementation Notes

- RED: add `MK_tools_tests` coverage first, then watch `MK_tools_tests` fail because `ScenePrefabAuthoringCommandKind::refresh_prefab_instance` is missing.
- GREEN: add the enum value, command dispatch, path/input loading, schema diagnostic mapping for refresh diagnostics, and `plan_refresh_prefab_instance`.
- Drift sync: update `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, docs, and static guards; run `tools/compose-agent-manifest.ps1 -Write`.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` before implementation | RED: failed because `ScenePrefabAuthoringCommandKind::refresh_prefab_instance` was missing. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` after implementation | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS after running `tools/format.ps1` for C++ wrapping. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/scene/scene_prefab_authoring_tool.cpp,tests/unit/tools_tests.cpp` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |

## Remaining Gap

`scene-component-prefab-schema-v2` is still not closed for Engine 1.0. Production 2D atlas/tilemap/native GPU output, future 3D playable vertical-slice evidence, production editor scene editing, nested prefab propagation/merge resolution UX, and broad/dependent package cooking remain the manifest-level required-before-ready claims.
