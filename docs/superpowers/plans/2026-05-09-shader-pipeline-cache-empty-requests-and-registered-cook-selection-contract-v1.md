# Shader Pipeline Cache Empty Requests and Registered Cook Selection Contract v1 (2026-05-09)

**Plan ID:** `shader-pipeline-cache-empty-requests-and-registered-cook-selection-contract-v1`  
**Status:** Completed  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md) (Phase 4 shader hot reload / Phase 5 registered cook — regression lock-in)

## Goal

Lock **host-independent** contracts for:

1. `build_shader_pipeline_cache_plan` / `reconcile_shader_pipeline_cache_index` when the compile request list is **empty** (fail-closed `plan.ok`, diagnostic message, no cache writes).
2. `plan_registered_source_asset_cook_package` when **no asset keys** are selected (`missing_selected_asset_keys`).

This does **not** claim shader hot reload production readiness, importer parity, or Phase 4–5 milestone closure.

## Context

- Phase 4–5 milestone queue lists **P2** work for registered cook + shader pipeline surfaces (`upload-staging-v1` adjacent tooling lives elsewhere).
- Empty-request early return in `shader_compile_action.cpp` is easy to regress silently into “successful no-op” behavior.

## Constraints

- No production-ready wording changes in `engine/agent/manifest.json` or `docs/current-capabilities.md`.
- English-only plan text under `docs/superpowers/plans/` per `AGENTS.md`.

## Done when

- `mirakana_tools_tests` covers both behaviors with stable diagnostic substrings / codes.

## Validation evidence

- Build: `cmake --build --preset dev --target mirakana_tools_tests`
- Run: `ctest --preset dev -R mirakana_tools_tests --output-on-failure` (or `out/build/dev/Debug/mirakana_tools_tests.exe` with matching filter)

## Out of scope

- Host-gated Vulkan/Metal editor preview parity (milestone **P0**).
- Broader registered cook E2E or `game.agent.json` schema changes.
