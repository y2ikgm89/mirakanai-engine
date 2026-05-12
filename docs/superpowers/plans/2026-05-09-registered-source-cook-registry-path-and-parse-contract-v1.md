# Registered Source Cook Registry Path and Parse Contract v1 (2026-05-09)

**Plan ID:** `registered-source-cook-registry-path-and-parse-contract-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Lock `plan_registered_source_asset_cook_package` failures for **`unsafe_source_registry_path`** (repository path not ending in `.geassets`) and **`invalid_source_registry`** (registry document parse failure).

## Context

`registered_source_asset_cook_package_tool.cpp` validates paths before parsing; malformed registry content is diagnosed during `parse_source_registry`.

## Constraints

- `mirakana_tools_tests` only; no importer productization or manifest gap promotion.

## Done when

- Tests assert both diagnostic codes and representative message substrings.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Configure/build | `cmake --preset dev` → `cmake --build --preset dev --target mirakana_tools_tests` | OK |
| Tests | `ctest --preset dev -R mirakana_tools_tests --output-on-failure` | OK |
| Slice close | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
