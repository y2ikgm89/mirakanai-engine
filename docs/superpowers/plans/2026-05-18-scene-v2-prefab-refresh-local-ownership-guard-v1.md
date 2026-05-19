# Scene v2 Prefab Refresh Local Ownership Guard v1 - 2026-05-18

**Status:** Completed

## Goal

Make `plan_scene_prefab_instance_refresh_v2` fail closed when the selected prefab instance subtree contains local author-owned child nodes or components that have no matching selected-prefab source provenance.

## Context

`scene-component-prefab-schema-v2` is still the selected production gap. Stable-id prefab refresh planning is contract-only, non-mutating, and non-executing. It already rejects duplicate copied source identities and descendant nested prefab roots before producing preserve/add/remove rows.

Local child preservation and local component merge policy are not part of the Schema v2 planner contract. Until a reviewed merge/apply surface exists, silently ignoring those local rows would make future apply semantics ambiguous.

## Constraints

- Keep the planner non-mutating and non-executing.
- Do not add apply behavior, editor productization, nested propagation, runtime prefab semantics, broad/dependent package cooking, renderer/RHI residency, package streaming, or native handles.
- Preserve the existing stable-id preserve/add/remove row contract for fully sourced instance subtrees.
- Keep manifest truth in `engine/agent/manifest.fragments/` and regenerate `engine/agent/manifest.json`.

## Done When

- `MK_scene_schema_v2_tests` proves local child nodes and local components under the selected instance root return invalid plans with no rows.
- New diagnostics are surfaced as `unsupported_local_prefab_child` and `unsupported_local_prefab_component`.
- Current-truth docs, manifest fragments, generated manifest, and static checks describe the same contract.
- Focused build/test/static checks and one full `tools/validate.ps1` pass, or a concrete blocker is recorded.

## Validation Evidence

| Check | Command | Result |
| --- | --- | --- |
| RED build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` | Failed before implementation because `unsupported_local_prefab_child` and `unsupported_local_prefab_component` did not exist. |
| Focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_schema_v2_tests` | Passed after implementation. |
| Focused test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_scene_schema_v2_tests$"` | Passed. |
| Format | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed after applying repository format. |
| Public API boundaries | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| JSON contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| Agent surfaces | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| AI integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| Targeted tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/scene/src/schema_v2.cpp,tests/unit/scene_schema_v2_tests.cpp` | Passed. |
| Full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed; 65 tests passed. Metal shader tools and Apple host evidence remain diagnostic-only/host-gated on this Windows host. |
