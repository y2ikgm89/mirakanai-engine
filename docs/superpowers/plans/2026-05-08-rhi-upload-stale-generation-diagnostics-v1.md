# RHI Upload Stale Generation Diagnostics v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Use the existing `RhiUploadDiagnosticCode::stale_generation` contract whenever upload staging handles match an allocation id but not its generation.

**Plan ID:** `rhi-upload-stale-generation-diagnostics-v1`

**Status:** Completed.

**Architecture:** Keep this inside `MK_rhi` upload staging. The slice does not add public API; it makes existing allocation generation checks precise across `RhiUploadStagingPlan`, `RhiUploadRing`, and upload GPU batch helpers while keeping native upload queues, package streaming, and async execution out of scope.

**Tech Stack:** C++23, `MK_rhi`, `NullRhiDevice`, `tests/unit/rhi_upload_staging_tests.cpp`.

---

## Context

- Target unsupported gap: `upload-staging-v1`, currently `implemented-foundation-only`.
- `RhiUploadAllocationHandle` already carries `id` and `generation`, and `RhiUploadDiagnosticCode::stale_generation` is public.
- Current implementation treats stale-generation handles as `invalid_allocation`, and `RhiUploadRing` maps ownership by id only.
- This slice narrows upload/staging correctness without changing the public API surface or claiming native async upload execution.

## Constraints

- Do not introduce native backend handles or GPU upload queues.
- Do not change package streaming, runtime upload ownership, or renderer ownership.
- Do not add compatibility shims or new public API names.
- Keep diagnostics deterministic and value-type only.
- Add RED tests before production code.

## Done When

- Unit tests prove stale-generation handles in buffer upload, texture upload, and submitted marking return `stale_generation`.
- Unit tests prove upload-ring reserve returns `stale_generation` for known id/wrong generation.
- Unit tests prove ring ownership and byte-offset lookup require both id and generation.
- Docs, manifest notes, plan registry, master plan, and static checks mention RHI Upload Stale Generation Diagnostics v1 while `upload-staging-v1` remains `implemented-foundation-only`.
- Focused RHI upload tests, targeted tidy, schema/agent checks, production-readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact blockers.

## File Plan

- Modify `tests/unit/rhi_upload_staging_tests.cpp`: add RED coverage for stale generation diagnostics and ring ownership.
- Modify `engine/rhi/include/mirakana/rhi/upload_staging.hpp`: store upload-ring span generation beside id in the private `Span`.
- Modify `engine/rhi/src/upload_staging.cpp`: distinguish unknown allocation ids from stale generations and make ring offset/ownership generation-aware.
- Modify `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/rhi.md`, `docs/superpowers/plans/README.md`, this master plan, and static checks to prevent drift.

## Tasks

### Task 1: RED Tests

- [x] Add `MK_TEST("rhi upload staging diagnoses stale allocation generations")`.
- [x] Allocate staging bytes, create a stale handle with the same `id` and a different `generation`, and assert buffer upload, texture upload, and `mark_submitted` return `RhiUploadDiagnosticCode::stale_generation`.
- [x] Add `MK_TEST("rhi upload ring ownership requires matching allocation generation")`.
- [x] Reserve a valid handle in `RhiUploadRing`, assert stale handles do not own the allocation and have no byte offset, then assert reserving a stale handle returns `stale_generation`.
- [x] Run `cmake --build --preset dev --target MK_rhi_upload_staging_tests` and record the expected assertion failure because stale handles currently return `invalid_allocation` or id-only ownership.

### Task 2: Implementation

- [x] Add a private helper in `upload_staging.cpp` that finds an allocation by `RhiUploadAllocationId` and reports whether the generation is stale.
- [x] Use that helper from `enqueue_buffer_upload`, `enqueue_texture_upload`, `mark_submitted`, and `RhiUploadRing::reserve_for_allocation`.
- [x] Add `allocation_generation` to `RhiUploadRing::Span`, store it on reserve, and make `byte_offset_for`, `owns_allocation`, and duplicate-reserve checks match both id and generation.
- [x] Preserve existing `invalid_allocation`, `invalid_copy_range`, `already_submitted`, `ring_exhausted`, and `allocation_already_bound` behavior for non-stale cases.
- [x] Run `cmake --build --preset dev --target MK_rhi_upload_staging_tests` and `ctest --preset dev --output-on-failure -R "^MK_rhi_upload_staging_tests$"`.

### Task 3: Docs And Static Contract

- [x] Update current-truth docs and manifest notes for stale-generation diagnostics.
- [x] Keep `upload-staging-v1` `implemented-foundation-only` and explicitly keep native GPU upload queues, native async upload execution, partial texture updates, compression metadata, allocator/residency budgets, package streaming, editor resource management/capture tooling execution, and 2D/3D playable vertical slices unsupported.
- [x] Add static checks for stale-generation tests, source/header implementation, docs, and completed plan status.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation

- [x] Run targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit the coherent slice after validation and build pass.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED `cmake --build --preset dev --target MK_rhi_upload_staging_tests` | PASS | Test binary compiled after adding RED coverage. |
| RED `ctest --preset dev --output-on-failure -R "^MK_rhi_upload_staging_tests$"` | FAIL expected | Failed on stale handles returning `invalid_allocation` and id-only ring ownership before implementation. |
| `cmake --build --preset dev --target MK_rhi_upload_staging_tests` | PASS | Rebuilt after implementation and after formatting. |
| `ctest --preset dev --output-on-failure -R "^MK_rhi_upload_staging_tests$"` | PASS | `MK_rhi_upload_staging_tests` passed. |
| Targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/src/upload_staging.cpp,tests/unit/rhi_upload_staging_tests.cpp -MaxFiles 2` | PASS | Passed with existing clang-tidy warning output. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Initial roadmap-name static assertion was fixed, then `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `unsupported_gaps=11`; `upload-staging-v1` remains `implemented-foundation-only`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after private public-header storage change. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format check passed. |
| `git diff --check` | PASS | Only local LF-to-CRLF warnings were reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; CTest 29/29, with Metal/Apple lanes reported as host-gated diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full development build passed after validation. |
