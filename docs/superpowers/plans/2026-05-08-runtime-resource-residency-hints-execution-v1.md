# Runtime Resource Residency Hints Execution v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enforce selected `packageStreamingResidencyTargets` residency hints against a loaded runtime package before safe-point package streaming commits it.

**Architecture:** Keep the feature inside `mirakana_runtime` as deterministic host-independent validation over an already-loaded `RuntimeAssetPackageLoadResult`. The helper checks required preload assets, allowed resident resource kinds, and selected package count before staging or committing, preserving the active package/catalog on failure and leaving broad async streaming, eviction, allocator/GPU enforcement, renderer/RHI teardown, and native handles unsupported.

**Tech Stack:** C++23, `mirakana_runtime`, first-party runtime asset package/resource catalog contracts, focused C++ unit tests, existing docs/manifest/static validation.

---

## Goal

- Add optional C++ execution fields corresponding to manifest `packageStreamingResidencyTargets` hints:
  - required preload asset ids
  - allowed resident `AssetKind` values
  - max resident package count for the selected one-package safe-point lane
- Reject loaded packages that do not satisfy these hints before `RuntimeAssetPackageStore::stage_if_loaded`.
- Record deterministic counters and diagnostics in `RuntimePackageStreamingExecutionResult`.

## Context

- Manifest/schema already validate optional `preloadAssetKeys`, `residentResourceKinds`, and `maxResidentPackages` descriptor fields.
- `execute_selected_runtime_package_streaming_safe_point` currently enforces runtime scene validation evidence, package load success, positive resident byte budget, byte budget intent, staging, and safe-point catalog replacement.
- `runtime-resource-v2` remains foundation-only; this slice narrows only the selected safe-point execution surface.

## Constraints

- Do not parse `game.agent.json` in C++ or add file IO/shell execution.
- Do not add async/background streaming, arbitrary eviction, resident caches, allocator/GPU budget enforcement, renderer/RHI ownership, renderer/RHI teardown, public native/RHI handles, hot reload, or editor migration claims.
- Keep `engine/core` untouched and keep changes inside runtime tests/docs/manifest/static checks.

## Done When

- Missing required preload assets and disallowed resident kinds fail with `RuntimePackageStreamingExecutionStatus::residency_hint_failed`.
- Hint failure leaves the active store package and existing catalog handles unchanged.
- Valid hints commit through the existing safe-point replacement path and report hint counters.
- Docs, manifest notes, and static checks record Runtime Resource Residency Hints Execution v1 while `runtime-resource-v2` remains `implemented-foundation-only`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, focused runtime tests, targeted tidy, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact blockers.

## File Plan

- Modify `engine/runtime/include/mirakana/runtime/package_streaming.hpp`: add hint fields, result counters, and `residency_hint_failed` status.
- Modify `engine/runtime/src/package_streaming.cpp`: validate hints after package load success and before budget/stage/commit.
- Modify `tests/unit/asset_identity_runtime_resource_tests.cpp`: add missing-preload, disallowed-kind, and valid-hints commit coverage.
- Modify `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, and the master plan: document the narrowed runtime-resource surface.
- Modify `tools/check-ai-integration.ps1` and/or `tools/check-json-contracts.ps1` as needed so docs/manifest cannot drift.

## Tasks

### Task 1: RED Tests

- [x] Add a missing required preload asset test beside the existing package streaming tests:
  - descriptor sets `required_preload_assets` to an asset absent from the loaded package
  - result status is `residency_hint_failed`
  - diagnostic code is `preload-asset-missing`
  - active package/catalog handle remain unchanged
- [x] Add a disallowed resident kind test:
  - descriptor permits only `AssetKind::texture`
  - loaded package includes a `material`
  - result status is `residency_hint_failed`
  - diagnostic code is `resident-resource-kind-disallowed`
- [x] Add a valid hints test using texture/material preload and kind rows that commits successfully and reports non-zero hint counters.
- [x] Run the focused test build and record the expected compile failure before implementation.

### Task 2: Runtime Contract

- [x] Add `residency_hint_failed` to `RuntimePackageStreamingExecutionStatus`.
- [x] Add `std::vector<AssetId> required_preload_assets`, `std::vector<AssetKind> resident_resource_kinds`, and `std::uint32_t max_resident_packages` to `RuntimePackageStreamingExecutionDesc`.
- [x] Add `required_preload_asset_count`, `resident_resource_kind_count`, and `resident_package_count` to `RuntimePackageStreamingExecutionResult`.
- [x] Implement hint validation without mutating the store or catalog on failure.
- [x] Run focused runtime tests.

### Task 3: Docs And Static Contract

- [x] Update current docs and manifest notes for Runtime Resource Residency Hints Execution v1.
- [x] Keep remaining unsupported runtime-resource claims explicit.
- [x] Add static evidence checks for the new status/diagnostics/docs if the existing checks do not cover them.
- [x] Run schema, agent, production-readiness, API boundary, and targeted tidy gates.

### Task 4: Final Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit the coherent slice after validation/build pass.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused runtime test build | PASS | `cmake --build --preset dev --target mirakana_asset_identity_runtime_resource_tests` failed before implementation on missing `required_preload_assets`, `resident_resource_kinds`, `max_resident_packages`, `residency_hint_failed`, and result counter members. |
| Focused runtime tests | PASS | `ctest --preset dev --output-on-failure -R "^mirakana_asset_identity_runtime_resource_tests$"` passed after implementation. |
| Desktop-runtime sample build | PASS | `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package` passed; existing Vulkan source warning C4819 remains non-fatal. |
| Source-tree package streaming smoke | PASS | `sample_generated_desktop_runtime_3d_package.exe --smoke --require-config runtime/sample_generated_desktop_runtime_3d_package.config --require-scene-package runtime/sample_generated_desktop_runtime_3d_package.geindex --require-package-streaming-safe-point --max-frames 1` reported `package_streaming_status=committed`, `package_streaming_required_preload_assets=1`, `package_streaming_resident_resource_kinds=8`, `package_streaming_resident_packages=1`, and `package_streaming_diagnostics=0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public runtime header change passed `public-api-boundary-check: ok`. |
| Targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/package_streaming.cpp,tests/unit/asset_identity_runtime_resource_tests.cpp -MaxFiles 2` passed. The desktop-runtime sample is not present in the `dev` compile database; it was validated by the desktop-runtime build above. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `unsupported_gaps=11`; `runtime-resource-v2` remains `implemented-foundation-only`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok` after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `git diff --check` | PASS | Only CRLF working-copy warnings; no whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest passed 29/29; Metal/Apple host diagnostics remain host-gated/diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default `dev` build passed. |
