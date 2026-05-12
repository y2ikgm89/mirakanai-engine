# Runtime RHI Upload Submission Fence Rows v1 Implementation Plan (2026-05-08)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve submitted `mirakana::rhi::FenceValue` rows from Runtime RHI upload helpers, carry actual scene upload submit order, and aggregate them in runtime-scene GPU upload reports.

**Architecture:** Keep the work in `MK_runtime_rhi` and `MK_runtime_scene_rhi` as backend-neutral first-party telemetry over already-submitted upload command lists. The slice reports fence identity for texture, mesh, skinned mesh, morph mesh, compute-morph base mesh, and material-factor uploads without replacing the existing upload path with a staging ring, changing backend ownership, or claiming async/background package streaming.

**Tech Stack:** C++23, public RHI/runtime RHI result structs, NullRHI unit tests, existing docs/manifest/static validation.

---

## Goal

- Add a `submitted_fence` field to successful runtime upload result rows that call `IRhiDevice::submit`.
- Keep metadata-only texture uploads at the zero fence value because they create no upload command list.
- Preserve actual scene upload submit order in `RuntimeSceneGpuBindingResult::submitted_upload_fences`.
- Aggregate submitted upload fence rows in `RuntimeSceneGpuUploadExecutionReport` with:
  - `submitted_upload_fence_count`
  - `last_submitted_upload_fence`
- Preserve existing `wait_for_completion` behavior while making non-waiting upload submissions observable to host/package code.

## Context

- Target unsupported gap: `upload-staging-v1`, currently `implemented-foundation-only`.
- `MK_rhi` already exposes `FenceValue`, per-queue NullRHI fence timelines, and upload staging/ring planning contracts.
- `MK_runtime_rhi` upload helpers currently submit command lists and optionally wait, but the submitted fence is discarded.
- `MK_runtime_scene_rhi` reports upload counts/bytes but cannot prove which submitted upload rows were issued.

## Constraints

- Do not expose native queue/fence handles, command lists, backend objects, or descriptor handles to gameplay.
- Do not replace runtime uploads with `RhiUploadStagingPlan`, upload rings, staging pools, native async upload execution, package streaming, eviction, allocator/GPU budgets, or renderer ownership changes in this slice.
- Keep public API names in `mirakana::` style and update API-boundary checks because public headers change.
- Treat `NullRhiDevice` as plumbing proof only; do not claim real backend async overlap from NullRHI tests.

## Done When

- Unit tests prove texture, mesh, skinned mesh, morph mesh, and material-factor uploads retain submitted fence values.
- Runtime-scene report tests prove actual submitted upload fence order, submitted upload fence count, and last fence include package texture, mesh, material, morph, and compute-morph base uploads.
- Docs, manifest notes, plan registry, master plan, and static checks mention Runtime RHI Upload Submission Fence Rows v1 while `upload-staging-v1` remains `implemented-foundation-only`.
- Focused runtime tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, targeted tidy, schema/agent checks, production-readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact blockers.

## File Plan

- Modify `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`: add `rhi::FenceValue submitted_fence` to upload result structs and material binding rows.
- Modify `engine/runtime_rhi/src/runtime_upload.cpp`: assign returned fences immediately after `device.submit`; leave no-submit metadata texture rows at zero.
- Modify `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp`: add ordered submitted-fence rows and submitted-fence report counters.
- Modify `engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp`: record non-zero submitted fences as uploads are submitted, then derive report fields from that order.
- Modify `tests/unit/runtime_rhi_tests.cpp` and `tests/unit/runtime_scene_rhi_tests.cpp`: add RED coverage first.
- Modify `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/rhi.md`, `docs/superpowers/plans/README.md`, this master plan, and static checks to prevent drift.

## Tasks

### Task 1: RED Tests

- [x] Add assertions to existing texture and mesh upload tests:

```cpp
MK_REQUIRE(result.submitted_fence.value != 0);
MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
```

- [x] Add a metadata-only texture assertion:

```cpp
MK_REQUIRE(result.submitted_fence.value == 0);
```

- [x] Add assertions to existing skinned, morph, and material binding tests:

```cpp
MK_REQUIRE(upload.submitted_fence.value != 0);
MK_REQUIRE(binding.submitted_fence.value != 0);
```

- [x] Add runtime-scene report assertions:

```cpp
MK_REQUIRE(execution.report.submitted_upload_fence_count == 3);
MK_REQUIRE(execution.report.last_submitted_upload_fence.value != 0);
MK_REQUIRE(execution.bindings.submitted_upload_fences.size() == 3);
```

- [x] Run the focused build before implementation and record the expected compile failure on missing members.

### Task 2: Runtime RHI Contract

- [x] Add `rhi::FenceValue submitted_fence` to `RuntimeTextureUploadResult`, `RuntimeMeshUploadResult`, `RuntimeSkinnedMeshUploadResult`, `RuntimeMorphMeshUploadResult`, and `RuntimeMaterialGpuBinding`.
- [x] Populate `submitted_fence` from every successful `device.submit` call in runtime upload helpers.
- [x] Keep failed results and no-submit metadata texture rows at default zero fence.
- [x] Run focused runtime RHI tests.

### Task 3: Runtime Scene Report

- [x] Add `submitted_upload_fences` to `RuntimeSceneGpuBindingResult`.
- [x] Add `submitted_upload_fence_count` and `last_submitted_upload_fence` to `RuntimeSceneGpuUploadExecutionReport`.
- [x] Record non-zero upload fences from mesh, skinned mesh, morph mesh, compute-morph base mesh, texture, and material binding submit points.
- [x] Derive report count and last fence from `submitted_upload_fences` so compute-morph base uploads cannot be reordered behind morph uploads.
- [x] Run focused runtime scene RHI tests.

### Task 4: Docs And Static Contract

- [x] Update manifest and current-truth docs for the new fence-row evidence.
- [x] Keep native async upload, staging-ring execution, package streaming, renderer ownership, and backend parity explicitly unsupported.
- [x] Add or update static checks for the new public fields, implementation strings, tests, docs, and completed plan status.

### Task 5: Final Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit the coherent slice after validation and build pass.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused runtime test build | EXPECTED FAIL | `cmake --build --preset dev --target MK_runtime_rhi_tests MK_runtime_scene_rhi_tests` failed before implementation on missing submitted-fence members. |
| Focused runtime RHI tests | PASS | `ctest --preset dev --output-on-failure -R "^(MK_runtime_rhi_tests\|MK_runtime_scene_rhi_tests)$"` passed after implementation; `MK_runtime_rhi_tests` passed. |
| Focused runtime scene RHI tests | PASS | Same focused CTest command passed; `MK_runtime_scene_rhi_tests` passed with ordered `submitted_upload_fences` coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| Targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | PASS | `tidy-check: ok (4 files)` for runtime upload, runtime scene RHI, and focused tests; existing warning noise only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; dry-run generated games were cleaned. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gaps remain `11` and `upload-staging-v1` remains `implemented-foundation-only`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `git diff --check` | PASS | No whitespace errors; Git reported expected LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest reported 29/29 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full dev preset build passed. |
