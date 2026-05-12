# Runtime Resource v2 Resident Package Mount Set v1 (2026-05-12)

**Plan ID:** `runtime-resource-v2-resident-package-mount-set-v1`
**Status:** Completed.

## Goal

Add a deterministic `MK_runtime` resident package mount set for Runtime Resource v2 so hosts can name, replace, remove, and rebuild a resident cooked-package view from explicit package mounts instead of passing an anonymous vector at every call site.

## Context

- Parent: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md), selected gap `runtime-resource-v2`.
- Existing foundation: `RuntimeResourceCatalogV2`, `build_runtime_resource_catalog_v2_from_resident_mounts`, residency budget execution, residency hints execution, and safe-point package replacement/unload.
- Official C++ resource-management guidance favors explicit ownership and RAII/value ownership boundaries. This slice keeps package ownership inside first-party runtime value types and does not expose native, RHI, or raw owning handles.

## Constraints

- Keep the implementation inside `mirakana_runtime` and its focused tests.
- Do not add filesystem loading, async/background streaming, arbitrary eviction, renderer/RHI ownership, renderer/RHI teardown, allocator/GPU memory budget enforcement, hot reload, native handles, or source-registry parsing.
- Keep the API clean breaking and first-party; no compatibility shims around anonymous mount vectors beyond the existing helper.
- Preserve deterministic mount order and leave catalogs unchanged on rejected mount-set mutations or failed catalog builds.

## Done When

- `RuntimeResidentPackageMountSetV2` stores explicit non-zero mount ids with deterministic mount order and generation.
- Duplicate, zero, and missing mount ids return deterministic diagnostics and do not mutate the set.
- A catalog can be rebuilt from the mount set with existing first/last-mount overlay semantics, reporting mounted package count and mount-set generation.
- Removing a mount and rebuilding invalidates stale catalog handles through the existing catalog generation contract.
- Docs, manifest notes, and static checks record the narrowed resident package mount set while remaining unsupported `runtime-resource-v2` claims stay explicit.
- Focused tests, public API checks, targeted tidy, JSON/agent checks, production-readiness audit, `validate.ps1`, `build.ps1`, and a coherent commit complete or record exact host/tool blockers.

## File Plan

- Modify `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`: add mount id/status/diagnostic/result types, mount-set value type, and mount-set catalog build result/API.
- Modify `engine/runtime/src/resource_runtime.cpp`: implement deterministic mount, unmount, snapshot access, generation updates, and catalog rebuild from set.
- Modify `tests/unit/asset_identity_runtime_resource_tests.cpp`: add TDD coverage for overlay rebuild, duplicate/invalid/missing ids, and unmount-driven handle invalidation.
- Modify current-truth docs, plan registry, master-plan verdict, manifest fragment/composed output, and static validation checks for the new resident package mount-set surface.

## Tasks

- [x] Add RED tests for resident package mount set behavior and record the expected focused build failure.
- [x] Implement `RuntimeResidentPackageMountSetV2` and mount-set catalog rebuild API.
- [x] Run focused runtime tests and public API checks.
- [x] Synchronize docs, manifest, and static checks.
- [x] Run focused/static/full validation and commit the slice.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused runtime test build | PASS | `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests` failed before implementation on missing `RuntimeResidentPackageMountSetV2`, mount record/id/status types, and `build_runtime_resource_catalog_v2_from_resident_mount_set`. |
| Focused runtime test build | PASS | `cmake --build --preset dev --target MK_asset_identity_runtime_resource_tests`. |
| Focused runtime CTest | PASS | `ctest --preset dev --output-on-failure -R "^MK_asset_identity_runtime_resource_tests$"`. |
| Public API boundary check | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`. |
| Targeted tidy | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/asset_identity_runtime_resource_tests.cpp`; existing warning-only include-cleaner/const-correctness noise remains non-fatal. |
| JSON/agent/static checks | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and slice-scoped `git diff --check` passed. Full-tree `git diff --check` remains blocked by unrelated pre-existing whitespace in `docs/superpowers/plans/2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md`. |
| PSScriptAnalyzer | PASS WITH PRE-EXISTING WARNINGS | Analyzer ran on touched PowerShell files; reported existing repository warnings such as `Write-Host`, plural nouns, and ShouldProcess guidance outside this slice. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest 47/47 passed. Metal/Apple diagnostics remain host-gated/diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default `dev` build passed. |

## Non-Goals

- Broad package streaming, disk VFS mount discovery, file watching, hot reload, automatic eviction, renderer/RHI residency ownership, GPU upload/staging execution, allocator/VRAM budget enforcement, and 2D/3D vertical-slice ready claims.
