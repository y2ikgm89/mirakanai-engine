# Scene v2 Prefab Refresh Integrity v1 (2026-05-18)

**Plan ID:** `scene-v2-prefab-refresh-integrity-v1`
**Status:** Completed
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `scene-component-prefab-schema-v2`

## Goal

Make the contract-only Scene/Prefab v2 prefab instance refresh planner fail closed when copied prefab provenance is ambiguous, so later editor/apply/productization work never has to guess which current node or component owns a stable source id.

## Context

`Scene v2 Stable-Id Prefab Refresh Plan v1` added non-mutating preserve/add/remove rows from selected prefab instance provenance to a refreshed `PrefabDocumentV2`. That plan is only safe as a review/apply foundation when each copied source node and component identity is unique within the selected instance subtree.

## Constraints

- Keep the work in `MK_scene`; do not add editor mutation, undo actions, visible UI, runtime prefab instance semantics, package commands, or package cooking.
- Fail closed before row generation when duplicate source node/component identities make refresh rows ambiguous.
- Keep diagnostics deterministic and attached to the duplicate current node/component provenance row.
- Keep nested prefab propagation, local-child merge policy, fuzzy merge/rebase, automatic conflict UX, broad/dependent package cooking, renderer/RHI residency, package streaming, and native handles unsupported.

## Done When

- `mirakana::plan_scene_prefab_instance_refresh_v2` returns an invalid, non-mutating, non-executing plan with no refresh rows when duplicate `source_node_id` or `source_component_id` provenance appears under the selected instance root for the same prefab path.
- `SceneSchemaV2DiagnosticCode::duplicate_prefab_source_identity` reports the ambiguous provenance.
- Focused tests prove the fail-closed behavior for node and component source identities.
- Docs, manifest fragments, composed manifest, and static checks describe the same narrow ready surface and remaining unsupported work.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` before implementation | RED: missing `duplicate_prefab_source_identity` diagnostic enum |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` after implementation | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_scene_schema_v2_tests$"` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/scene/src/schema_v2.cpp,tests/unit/scene_schema_v2_tests.cpp` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS; 65 tests passed; Metal/Apple host checks remained diagnostic-only/host-gated on Windows |


