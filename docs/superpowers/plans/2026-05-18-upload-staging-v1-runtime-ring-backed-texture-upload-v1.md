# Upload Staging v1 Runtime Ring-Backed Texture Upload v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for behavior changes. This is a phase under `upload-staging-v1`, not a separate production gap.

**Goal:** Add an opt-in runtime texture upload path that uses a caller-owned `RhiUploadRing` and `RhiUploadStagingPlan` rows instead of allocating one staging buffer per texture upload.

**Architecture:** `RuntimeTextureUploadOptions` gains a backend-neutral `RhiUploadRing*` pointer. `upload_runtime_texture` keeps the existing default path unchanged, but when the pointer is present it validates a staging allocation, reserves ring bytes before creating the texture, writes the staged bytes into the ring, records the copy through `record_upload_gpu_batch`, marks the allocation submitted with the returned fence, and releases completed ring spans after synchronous waits. `upload_runtime_package_streaming_frame_graph_texture_bindings` inherits the path through its existing per-source upload options.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_rhi` upload staging helpers, NullRHI focused tests, manifest/docs/static checks.

---

## Goal

Close the next `upload-staging-v1` foundation slice by proving runtime texture uploads and package texture streaming transactions can use the already-implemented upload ring/staging-plan execution path without exposing native handles, broad background streaming, or native async overlap claims.

## Context

- `RhiUploadRing`, `RhiStagingBufferPool`, `record_upload_gpu_batch`, and `mark_pending_allocations_submitted` already exist in `MK_rhi`.
- Runtime texture uploads currently allocate one `copy_source` buffer per upload.
- Package streaming texture transactions already pass `RuntimeTextureUploadOptions` through to `upload_runtime_texture`.

## Constraints

- Keep the default upload path compatible with existing tests and counters.
- Ring-backed uploads must fail before texture creation, command-list creation, or buffer writes when the ring cannot reserve the staging allocation.
- Do not claim native async upload execution, async overlap/performance, allocator/residency budget integration, partial texture updates, compression metadata, or broad/background streaming.
- Do not expose public native/RHI handles beyond the existing backend-neutral `RhiUploadRing` opt-in pointer.

## Done When

- RED tests prove a ring-backed runtime texture upload reuses the caller-owned ring buffer, preserves frame graph/fence counters, and avoids per-upload staging buffer allocation.
- RED tests prove a committed package streaming texture upload transaction can reuse one shared ring across multiple payloads through existing source upload options.
- RED tests prove a too-small ring fails closed before texture upload side effects.
- Focused `MK_runtime_rhi_tests` pass, agent-surface drift is reconciled, and the coherent C++/public-contract slice passes full `tools/validate.ps1`.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | RED failed as expected before implementation: `RuntimeTextureUploadOptions::upload_ring` was missing. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | Passed after implementation. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | Passed after moving texture creation behind successful ring reservation. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed after updating the upload-staging recommended-next-plan static needle. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed. | 2026-05-18 |
