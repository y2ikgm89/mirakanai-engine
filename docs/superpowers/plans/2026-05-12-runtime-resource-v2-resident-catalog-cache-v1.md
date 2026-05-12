# Runtime Resource v2 Resident Catalog Cache v1 (2026-05-12)

**Plan ID:** `runtime-resource-v2-resident-catalog-cache-v1`

**Status:** Completed

## Goal

Add a deterministic Runtime Resource v2 resident catalog cache in `MK_runtime` so hosts can reuse a catalog built from a `RuntimeResidentPackageMountSetV2` when the mount generation, overlay policy, and residency budget are unchanged, while preserving the previous catalog on budget or catalog-build failure.

## Context

- Parent: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md), selected gap `runtime-resource-v2`.
- Previous slice: [2026-05-12-runtime-resource-v2-resident-package-mount-set-v1.md](2026-05-12-runtime-resource-v2-resident-package-mount-set-v1.md) added explicit loaded-package mount ids and deterministic catalog rebuilds from mount sets.
- Existing foundation: `RuntimeResourceCatalogV2`, `RuntimeResidentPackageMountSetV2`, `RuntimeResourceResidencyBudgetV2`, and budgeted catalog rebuilds from anonymous mount vectors.

## Constraints

- Keep this host-independent and inside `MK_runtime`.
- Do not add filesystem loading, disk/VFS mount discovery, async/background streaming, arbitrary eviction, renderer/RHI ownership, renderer/RHI teardown, allocator/GPU memory budget enforcement, hot reload, native handles, or 2D/3D vertical-slice ready claims.
- Keep catalog replacement atomic: budget failure or catalog-build failure must leave the cached catalog live.
- Add tests before production code.

## Done When

- A public `RuntimeResidentCatalogCacheV2` can refresh from `RuntimeResidentPackageMountSetV2`, overlay policy, and `RuntimeResourceResidencyBudgetV2`.
- Cache refresh reports `rebuilt`, `cache_hit`, `budget_failed`, or `catalog_build_failed` with deterministic counters and diagnostics.
- Cache hits do not rebuild or advance catalog generations.
- Mount-generation or budget changes force reevaluation; failed budget reevaluation preserves the previous catalog and live handles.
- Docs, manifest notes, and static checks record the narrowed resident catalog cache surface while remaining unsupported `runtime-resource-v2` claims stay explicit.

## Task Checklist

- [x] Create the focused plan.
- [x] Add RED tests for resident catalog cache reuse, generation-change rebuild, and budget-change failure preservation.
- [x] Implement the cache API in `resource_runtime`.
- [x] Run focused build/test and record RED/GREEN evidence.
- [x] Update current-truth docs, plan registry, master-plan verdict, manifest fragment/composed output, and static checks.
- [x] Run focused static checks, `validate.ps1`, `build.ps1`, and commit the slice.

## Validation Evidence

| Gate | Result | Evidence |
| --- | --- | --- |
| RED focused runtime resource resident cache build | PASS | `cmake --build --preset dev --target MK_runtime_resource_resident_cache_tests` failed before implementation on missing `RuntimeResidentCatalogCacheV2` and `RuntimeResidentCatalogCacheStatusV2`. |
| GREEN focused runtime resource resident cache build | PASS | `cmake --build --preset dev --target MK_runtime_resource_resident_cache_tests` built `MK_runtime_resource_resident_cache_tests.exe`. |
| Focused runtime resource resident cache CTest | PASS | `ctest --preset dev --output-on-failure -R "^(MK_runtime_resource_resident_cache_tests|MK_asset_identity_runtime_resource_tests)$"` passed 2/2 tests. |
| Public API boundary check | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` reported `public-api-boundary-check: ok`. |
| Runtime source tidy | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_resource_resident_cache_tests.cpp` reported `tidy-check: ok (2 files)`. |
| JSON contracts / AI integration / production readiness audit | PASS | `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-production-readiness-audit.ps1`, and `tools/check-agents.ps1` passed. Production readiness audit still reports 10 known unsupported gap rows. |
| Full validation | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` reported `validate: ok`; CTest passed 48/48 tests. Host-gated Apple/mobile diagnostic evidence remains explicitly unavailable on this Windows host. |
| Build | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` configured `dev` and built all dev targets including `MK_runtime_resource_resident_cache_tests.exe`. |
