# Runtime Resource v2 Resident Package Replacement Commit v1 (2026-05-12)

**Status:** Completed
**Gap:** `runtime-resource-v2` foundation follow-up
**Parent:** [Production Completion Master Plan v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a host-independent Runtime Resource v2 resident package replacement commit that swaps one existing `RuntimeResidentPackageMountIdV2` to a candidate loaded package while preserving the mount slot/order, refreshing `RuntimeResidentCatalogCacheV2`, and leaving live state unchanged on invalid, missing, budget, or catalog failure.

## Context

The current burn-down already supports explicit resident mount sets, resident catalog caching, streaming resident mount commit, and resident unmount/cache refresh. Hot-reload or recook flows still need a safe-point-style mutation for replacing an already resident package without unmount/remount order drift. This slice provides that atomic catalog mutation boundary without file watching, broad/background streaming, renderer/RHI resource ownership, GPU upload/staging, eviction, or disk/VFS discovery.

## Constraints

- Keep the API in `MK_runtime` and independent from platform, filesystem watching, renderer/RHI, editor, and native handles.
- Use copy-then-commit semantics: projected mount-set replacement and catalog-cache refresh must pass before live state changes.
- Preserve the existing mount slot/order; do not implement replacement as public unmount followed by mount.
- Preflight the raw candidate package through existing catalog build validation so duplicate package records are rejected before overlay merge can hide them.
- Keep remaining `runtime-resource-v2` claims honest: renderer/RHI ownership, broad/background package streaming, disk/VFS discovery, upload/staging integration, arbitrary eviction, and enforced GPU budgets remain follow-up work.

## Done When

- `commit_runtime_resident_package_replace_v2` and result/status types are public in `resource_runtime.hpp`.
- Unit tests cover success, invalid id, missing id, raw duplicate-record failure, projected budget failure, and mount-order preservation for overlay semantics.
- Docs, registry, roadmap/current capabilities, manifest fragments/composed manifest, and static checks agree on the narrow claim.
- Focused runtime tests, public API checks, JSON/AI integration checks, production readiness audit, `validate.ps1`, and `build.ps1` have fresh evidence.

## Validation Evidence

- TDD RED: after adding `MK_runtime_resource_resident_replace_tests`, `cmake --build --preset dev --target MK_runtime_resource_resident_replace_tests` failed on the missing `commit_runtime_resident_package_replace_v2` / replacement status API symbols.
- Focused GREEN: `cmake --build --preset dev --target MK_runtime_resource_resident_replace_tests MK_runtime_resource_resident_unmount_tests MK_runtime_resource_resident_cache_tests MK_runtime_package_streaming_resident_mount_tests MK_asset_identity_runtime_resource_tests` passed.
- Focused tests: `ctest --preset dev --output-on-failure -R "^(MK_runtime_resource_resident_replace_tests|MK_runtime_resource_resident_unmount_tests|MK_runtime_resource_resident_cache_tests|MK_runtime_package_streaming_resident_mount_tests|MK_asset_identity_runtime_resource_tests)$"` passed, 5/5 tests.
- Static/API checks passed: `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_resource_resident_replace_tests.cpp`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-production-readiness-audit.ps1`, `tools/check-agents.ps1`, and `Invoke-ScriptAnalyzer` with `-Severity Error` on touched PowerShell checks.
- Review: `cpp-reviewer` subagent reported no findings for the slot-preserving replacement semantics, raw candidate catalog preflight, projected rollback paths, and stale-handle invalidation coverage.
- Full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 51/51 CTest tests; shader/Apple host checks remained diagnostic-only/host-gated as expected on this Windows host.
- Commit gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed.
