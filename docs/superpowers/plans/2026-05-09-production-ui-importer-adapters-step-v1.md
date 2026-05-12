# Production UI Importer Platform Adapters Step v1 (2026-05-09)

**Plan ID:** `production-ui-importer-adapters-step-v1`  
**Parent wave:** [2026-05-09-phase-4-5-continuation-wave-1-v1.md](2026-05-09-phase-4-5-continuation-wave-1-v1.md)  
**Status:** Completed (governance slice).

## Goal

Hold **`production-ui-importer-platform-adapters`** at **`planned`** for Wave 1 while documenting the **legal/deps gate** any future adapter slice must pass.

## Constraints

- No new third-party dependencies in Wave 1 without `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, `THIRD_PARTY_NOTICES.md`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`.

## Done when

- Plan file records the constraint and references manifest gap notes; Wave 1 introduces **no** new importer adapters.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Docs/registry | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
