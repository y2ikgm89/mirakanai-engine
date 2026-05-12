# Registered Source Cook Zero Source Revision Contract v1 (2026-05-09)

**Plan ID:** `registered-source-cook-zero-source-revision-contract-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Lock `plan_registered_source_asset_cook_package` validation when `source_revision` is zero (`invalid_source_revision`), matching Phase 5 registered-source cook diagnostics.

## Context

`registered_source_asset_cook_package_tool.cpp` rejects `request.source_revision == 0` before planning.

## Constraints

- Tools-only unit test; no manifest gap promotion.

## Done when

- `mirakana_tools_tests` asserts failure with `invalid_source_revision` and message substring `non-zero`.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Configure/build | `cmake --preset dev` → `cmake --build --preset dev --target mirakana_tools_tests` | OK |
| Tests | `ctest --preset dev -R mirakana_tools_tests --output-on-failure` | OK |
| Slice close | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
