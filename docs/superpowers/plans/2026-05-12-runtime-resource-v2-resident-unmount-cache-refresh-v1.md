# Runtime Resource v2 Resident Unmount Cache Refresh v1 (2026-05-12)

**Plan ID:** `runtime-resource-v2-resident-unmount-cache-refresh-v1`

**Status:** Completed

## Goal

Add a host-independent Runtime Resource v2 helper that removes an explicit resident package mount and refreshes `RuntimeResidentCatalogCacheV2` as one safe-point-style operation, preserving the previous mount set and cached catalog when preflight fails.

## Context

- Parent: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md), selected gap `runtime-resource-v2`.
- Previous slices added explicit resident mount ids, cached resident catalogs, and package-streaming resident mount commit.
- Current callers can manually call `RuntimeResidentPackageMountSetV2::unmount` followed by `RuntimeResidentCatalogCacheV2::refresh`; this leaves atomicity and diagnostic rows to each caller.

## Constraints

- Keep the slice inside `MK_runtime`; no renderer/RHI ownership, GPU/allocator budgets, filesystem/VFS discovery, background streaming, hot reload, native handles, or package eviction policy.
- Add focused tests first in a new test target to avoid unrelated dirty historical test files.
- Validate projected remaining resident packages and catalog cache refresh before mutating the live mount set.
- Invalid or missing mount ids, projected budget failures, and projected catalog build failures must preserve the previous mount set generation and cached catalog.
- Successful unmount advances the mount-set generation, refreshes the resident catalog cache, and invalidates stale catalog handles through normal catalog generation changes.

## Done When

- A public `MK_runtime` helper can unmount a `RuntimeResidentPackageMountIdV2` and refresh `RuntimeResidentCatalogCacheV2` with deterministic result rows.
- Failure paths report mount, budget, or catalog diagnostics without mutating the live mount set or cached catalog.
- Tests cover success, missing mount id preservation, and projected remaining-budget preservation.
- Docs, registry, manifest fragments/composed output, and static checks record the narrowed resident unmount/cache-refresh surface while remaining `runtime-resource-v2` gaps stay explicit.

## Task Checklist

- [x] Create the focused plan.
- [x] Add RED tests for resident unmount/cache refresh success, missing id failure, and projected-budget preservation.
- [x] Implement the resident unmount/cache refresh helper in `resource_runtime`.
- [x] Run focused build/test and record RED/GREEN evidence.
- [x] Update current-truth docs, plan registry, master-plan verdict, manifest fragment/composed output, and static checks.
- [x] Run focused static checks, `validate.ps1`, `build.ps1`, and commit the slice.

## Validation Evidence

| Gate | Result | Evidence |
| --- | --- | --- |
| RED focused resident unmount/cache build | PASS | `cmake --preset dev; cmake --build --preset dev --target MK_runtime_resource_resident_unmount_tests` failed before implementation because `commit_runtime_resident_package_unmount_v2` / `RuntimeResidentPackageUnmountCommitStatusV2` were missing. Copy-then-commit refinement RED failed `MK_runtime_resource_resident_unmount_tests` on `result.mounted_package_count == 2` before the projected-cache implementation. |
| GREEN focused resident unmount/cache build | PASS | `cmake --build --preset dev --target MK_runtime_resource_resident_unmount_tests MK_runtime_resource_resident_cache_tests MK_runtime_package_streaming_resident_mount_tests MK_asset_identity_runtime_resource_tests` built all four targets. |
| Focused resident unmount/cache CTest | PASS | `ctest --preset dev --output-on-failure -R "^(MK_runtime_resource_resident_unmount_tests|MK_runtime_resource_resident_cache_tests|MK_runtime_package_streaming_resident_mount_tests|MK_asset_identity_runtime_resource_tests)$"` passed 4/4 tests. |
| Public API boundary check | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` reported `public-api-boundary-check: ok`. |
| Runtime source tidy | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_resource_resident_unmount_tests.cpp` reported `tidy-check: ok (2 files)`. |
| JSON contracts / AI integration / production readiness audit | PASS | `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-production-readiness-audit.ps1`, and `tools/check-agents.ps1` reported ok; `Invoke-ScriptAnalyzer -Severity Error` on edited static-check scripts emitted no errors. |
| Full validation | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` reported `validate: ok`; CTest passed 50/50 tests. Diagnostic-only Metal/Apple host gates remained blocked on this Windows host as expected. |
| Build | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` completed the `dev` configure/build. |
