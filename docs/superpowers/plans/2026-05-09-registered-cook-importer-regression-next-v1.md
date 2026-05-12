# Registered Cook + Importer Regression Next v1 (2026-05-09)

**Plan ID:** `registered-cook-importer-regression-next-v1`  
**Parent wave:** [2026-05-09-phase-4-5-continuation-wave-1-v1.md](2026-05-09-phase-4-5-continuation-wave-1-v1.md)  
**Status:** Completed (orientation slice).

## Goal

Point Wave 1 **asset-identity-v2** / **runtime-resource-v2** follow-up at the existing explicit cook + importer contract tests under `mirakana_tools_tests`.

## Regression catalog (non-exhaustive)

- Registered source cook: `tests/unit/tools_tests.cpp` plans dated `2026-05-09-*-contract-v1.md`.
- External import adapters: PNG/glTF/audio routing tests in the same translation unit.

## Next bounded targets

- Add new cook/import failures only through **dated focused plans** with matching diagnostics codes.

## Done when

- Wave milestone links this catalog; no manifest gap promotion.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Docs/registry | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
