# Runtime Resident Package Eviction Plan v1 (2026-05-15)

**Plan ID:** `runtime-resident-package-eviction-plan-v1`

## Goal

Add a host-independent resident package eviction planner that produces an explicit reviewed unmount order from caller-supplied candidate mount ids when the current resident package view exceeds a target residency budget.

## Context

`runtime-resource-v2` now supports explicit resident mounts, resident catalog cache refresh, safe resident unmount/replace commits, and package-streaming mount/replace/unmount execution. The remaining gap is not arbitrary eviction or background streaming; the next clean slice is a pure value-type planner that lets a host/editor/controller review which explicit resident mounts would need to be removed before it calls the already validated unmount commit path.

Official practice check:

- The design stays value/result oriented and RAII-friendly, aligned with C++ Core Guidelines R.1: no raw/native handles, no ownership leaks, and no hidden mutation.
- The API accepts explicit candidate/protected mount ids and returns deterministic diagnostics; it does not infer LRU policy, scan disk/VFS, spawn workers, or touch renderer/RHI resources.
- The project is greenfield, so the v2 planner is added directly without compatibility aliases or migration layers.

## Constraints

- Keep the planner in `MK_runtime` and independent from renderer/RHI, editor, platform, filesystem discovery, package loading, and background streaming.
- Do not mutate `RuntimeResidentPackageMountSetV2` or `RuntimeResidentCatalogCacheV2`; this slice returns a plan only.
- Reject invalid, duplicate, missing, or protected candidate mount ids deterministically before producing a partial plan.
- Stop as soon as the projected mount set satisfies the target budget; report `budget_unreachable` when reviewed candidates are insufficient.
- Keep arbitrary eviction, automatic LRU scoring, allocator/GPU memory enforcement, upload/staging integration, hot reload execution, and renderer/RHI teardown as explicit non-goals.

## Implementation Checklist

- [x] Add failing tests in `tests/unit/runtime_resource_resident_unmount_tests.cpp` for no-op, planned multi-step eviction, protected candidate rejection, missing/duplicate candidate rejection, and budget-unreachable preservation.
- [x] Add public value types and `plan_runtime_resident_package_evictions_v2` to `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`.
- [x] Implement the pure planner in `engine/runtime/src/resource_runtime.cpp` using projected `RuntimeResidentPackageMountSetV2` copies and `RuntimeResidentCatalogCacheV2::refresh`.
- [x] Run focused build/test for `MK_runtime_resource_resident_unmount_tests`.
- [x] Reconcile docs, plan registry, manifest fragments + composed manifest, and static checks for the new AI-operable contract.
- [x] Run the slice-closing validation gate.

## Done When

- A caller can ask for an explicit reviewed unmount order that satisfies a target resident byte/record budget when the supplied candidates are sufficient.
- The planner returns no-op success when the current resident view already satisfies the target budget.
- Invalid, duplicate, missing, and protected candidate ids fail before returning a partial plan.
- Insufficient candidates return deterministic budget diagnostics without mutating live mount/cache state.
- Validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` | Passed | Linked worktree dependency junctions and local build roots were prepared. |
| `cmake --build --preset dev --target MK_runtime_resource_resident_unmount_tests` | RED failed as expected, then Passed | RED failed before `RuntimeResidentPackageEvictionPlan*` types and `plan_runtime_resident_package_evictions_v2` existed; GREEN passed after implementation. |
| `ctest --preset dev --output-on-failure -R MK_runtime_resource_resident_unmount_tests` | Passed | 1/1 passed. |
| `cmake --build --preset dev --target MK_runtime_resource_resident_unmount_tests MK_runtime_resource_resident_cache_tests MK_runtime_resource_resident_replace_tests MK_runtime_package_streaming_resident_mount_tests` | Passed | Resident resource regression targets rebuilt. |
| `ctest --preset dev --output-on-failure -R "^(MK_runtime_resource_resident_unmount_tests\|MK_runtime_resource_resident_cache_tests\|MK_runtime_resource_resident_replace_tests\|MK_runtime_package_streaming_resident_mount_tests)$"` | Passed | 4/4 passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed |  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Ran `tools/format.ps1` once first. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/resource_runtime.cpp,tests/unit/runtime_resource_resident_unmount_tests.cpp` | Passed | 2 files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Includes manifest compose verification. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed |  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed |  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | 51/51 CTest tests passed; Apple/Metal diagnostics remain host-gated as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Standalone post-validate build gate passed. |
