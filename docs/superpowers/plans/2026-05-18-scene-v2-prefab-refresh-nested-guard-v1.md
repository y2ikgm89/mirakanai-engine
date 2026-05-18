# Scene v2 Prefab Refresh Nested Guard v1 (2026-05-18)

**Plan ID:** `scene-v2-prefab-refresh-nested-guard-v1`
**Status:** Completed
**Parent:** [Production Completion Master Plan v1](2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `scene-component-prefab-schema-v2`

## Goal

Make the contract-only Scene/Prefab v2 prefab instance refresh planner fail closed when the selected instance subtree contains a nested prefab root, so Schema v2 does not imply nested prefab propagation or merge behavior.

## Context

`Scene v2 Prefab Refresh Integrity v1` made duplicate copied source identities invalid before refresh row generation. The next safety boundary is nested prefab provenance: a descendant `SceneNodePrefabSourceV2` with a different `prefab_path` requires explicit nested propagation/merge policy, which remains outside the Schema v2 contract-only planner.

## Constraints

- Keep the work in `MK_scene`; do not add editor mutation, undo actions, visible UI, runtime prefab instance semantics, package commands, or package cooking.
- Fail closed before row generation when a selected instance subtree contains descendant prefab provenance for a different prefab path.
- Keep diagnostics deterministic and attached to the nested prefab root provenance row.
- Keep nested prefab propagation, local-child merge policy, fuzzy merge/rebase, automatic conflict UX, broad/dependent package cooking, renderer/RHI residency, package streaming, and native handles unsupported.

## Done When

- `mirakana::plan_scene_prefab_instance_refresh_v2` returns an invalid, non-mutating, non-executing plan with no refresh rows when a selected instance subtree contains a nested prefab root from a different prefab path.
- `SceneSchemaV2DiagnosticCode::unsupported_nested_prefab_instance` reports the unsupported nested prefab root.
- Focused tests prove the fail-closed behavior.
- Docs, manifest fragments, composed manifest, and static checks describe the same narrow ready surface and remaining unsupported work.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` before implementation | RED: missing `unsupported_nested_prefab_instance` diagnostic enum |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` after implementation | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_scene_schema_v2_tests$"` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/scene/src/schema_v2.cpp,tests/unit/scene_schema_v2_tests.cpp` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS; 65 tests passed; Metal/Apple host checks remained diagnostic-only/host-gated on Windows |
