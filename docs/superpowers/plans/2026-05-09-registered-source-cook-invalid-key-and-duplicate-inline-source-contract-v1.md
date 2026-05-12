# Registered Source Cook Invalid Key and Duplicate Inline Source Contract v1 (2026-05-09)

**Plan ID:** `registered-source-cook-invalid-key-and-duplicate-inline-source-contract-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Lock `plan_registered_source_asset_cook_package` request-shape validation for **`invalid_selected_asset_key`** (malformed `AssetKeyV2` text) and **`duplicate_source_file`** (duplicate inline `source_files` paths).

## Context

`registered_source_asset_cook_package_tool.cpp` validates selection keys and inline source payloads before planning imports; explicit cook paths remain foundation-only per Phase 5 milestone scope.

## Constraints

- `mirakana_tools_tests` only; no manifest gap promotion.

## Done when

- Tests fail closed with the expected diagnostic codes/messages.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Configure/build | `cmake --preset dev` → `cmake --build --preset dev --target mirakana_tools_tests` | OK |
| Tests | `ctest --preset dev -R mirakana_tools_tests --output-on-failure` | OK |
| Slice close | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
