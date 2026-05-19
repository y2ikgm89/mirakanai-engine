# Scene v2 Prefab Source Provenance v1 (2026-05-18)

**Plan ID:** `scene-v2-prefab-source-provenance-v1`
**Status:** Completed
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `scene-component-prefab-schema-v2`

## Goal

Add deterministic Scene/Prefab v2 prefab instance source provenance so `instantiate-prefab` preserves the source prefab path and copied source node/component ids in the authored `.scene` document.

## Context

`Scene v2 Command-Authored Runtime Workflow Validation v1` proved reviewed Scene/Prefab v2 authoring commands can feed source registration, selected registered cooking, Scene v2 runtime migration, and runtime scene package validation. The remaining Scene/Prefab v2 foundation gap still lacks a schema-level source link between copied instance rows and their source prefab rows, which blocks future editor refresh/propagation work from using the same authored-source contract.

## Constraints

- Keep Scene/Prefab v2 source provenance in `MK_scene` / `MK_tools` contracts only; do not claim runtime prefab instance semantics.
- Do not add broad/dependent package cooking, renderer/RHI residency, package streaming, public native/RHI handles, or editor productization.
- Preserve deterministic line-oriented `GameEngine.Scene.v2` / `GameEngine.Prefab.v2` text IO.
- Keep nested prefab propagation, fuzzy merge/rebase, and automatic conflict UX unsupported.

## Done When

- `GameEngine.Scene.v2` serializes and deserializes deterministic `prefab_source` rows for instantiated nodes and components.
- `mirakana::plan_scene_prefab_authoring` / `mirakana::apply_scene_prefab_authoring` `instantiate_prefab` records source prefab path plus source node/component ids for copied rows.
- Focused tests prove the provenance rows and the existing command-authored runtime workflow still validate.
- Docs, manifest fragments, composed manifest, and static checks describe the same narrow ready surface and remaining unsupported work.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_tools_tests$"` before implementation | RED: missing `node.1.prefab_source.prefab_path=source/prefabs/enemy.prefab` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_tools_tests$"` after implementation | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/scene/src/schema_v2.cpp,engine/tools/scene/scene_prefab_authoring_tool.cpp,tests/unit/tools_tests.cpp` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS; 65 tests passed; Metal/Apple host checks remained diagnostic-only/host-gated on Windows |


