# RHI Resource Lifetime Retired Handle Invalid Contract v1 (2026-05-09)

**Plan ID:** `rhi-resource-lifetime-retired-handle-invalid-contract-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Lock `RhiResourceLifetimeRegistry` behavior after `retire_released_resources` removes a deferred-release record: operations with the prior `RhiResourceHandle` fail with `invalid_resource` (native teardown completion analog at the registry layer).

## Context

`engine/rhi/src/resource_lifetime.cpp` erases retired rows; identifiers are not recycled in the current implementation, but handles must not be usable after retirement.

## Constraints

- Unit tests in `mirakana_rhi_resource_lifetime_tests` only; no manifest gap promotion for `renderer-rhi-resource-foundation`.

## Done when

- Tests prove `set_debug_name` and `release_resource_deferred` after retire yield `invalid_resource`.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Configure/build | `cmake --preset dev` → `cmake --build --preset dev --target mirakana_rhi_resource_lifetime_tests` | OK |
| Tests | `ctest --preset dev -R mirakana_rhi_resource_lifetime_tests --output-on-failure` | OK |
| Slice close | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
