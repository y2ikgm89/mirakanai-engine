# Registered Source Cook Duplicate Selection Contract v1 (2026-05-09)

**Plan ID:** `registered-source-cook-duplicate-selection-contract-v1`  
**Status:** Completed  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md) (P2 — explicit selection diagnostics; does not close `asset-identity-v2`)

## Goal

Lock **`plan_registered_source_asset_cook_package`** fail-closed behavior when **`selected_asset_keys` lists the same key twice**, producing stable **`duplicate_selected_asset_key`** diagnostics. Supports Phase 5 milestone language that explicit selection paths stay reproducible and schema-clean.

## Context

- `validate_request_shape` in `registered_source_asset_cook_package_tool.cpp` rejects duplicates before cook planning.
- Milestone queue **P2** calls for regression tests on registered cook explicit paths.

## Constraints

- English-only plan text under `docs/superpowers/plans/` per `AGENTS.md`.
- No `engine/agent/manifest.json` gap promotion or `docs/current-capabilities.md` ready inflation.

## Done when

- `mirakana_tools_tests` contains the new case and passes under `ctest --preset dev -R mirakana_tools_tests`.

## Validation evidence

- Build: `cmake --build --preset dev --target mirakana_tools_tests`
- Run: `ctest --preset dev -R mirakana_tools_tests --output-on-failure`
- Repo: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

## Out of scope

- Registry-closure cook proofs (already covered elsewhere), importer codecs, or full Phase 5 milestone closure.
