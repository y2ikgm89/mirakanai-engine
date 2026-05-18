# Upload Staging v1 Runtime Buffer Ring-Backed Uploads v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for behavior changes. This is a phase under `upload-staging-v1`, not a separate production gap.

**Goal:** Add opt-in caller-owned upload-ring staging for runtime mesh, skinned mesh, and morph mesh buffer uploads.

**Architecture:** The existing runtime texture `upload_ring` option becomes the pattern for buffer-backed runtime uploads. `RuntimeMeshUploadOptions`, `RuntimeSkinnedMeshUploadOptions`, and `RuntimeMorphMeshUploadOptions` gain a backend-neutral `RhiUploadRing*`; each upload function validates and reserves all ring staging bytes before creating destination buffers, writes payload bytes into the shared ring, records buffer copies through `record_upload_gpu_batch`, marks submitted allocations with the returned fence, and releases completed spans after synchronous waits.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_rhi` upload staging helpers, NullRHI focused tests, manifest/docs/static checks.

---

## Goal

Narrow `upload-staging-v1` by proving runtime mesh, skinned mesh, and morph mesh uploads can reuse a caller-owned `RhiUploadRing` / `RhiUploadStagingPlan` path instead of one `copy_source` upload buffer per stream.

## Context

- Runtime texture uploads already support `RuntimeTextureUploadOptions::upload_ring`.
- Runtime mesh, skinned mesh, and morph mesh uploads already route copy commands through `execute_frame_graph_rhi_multi_queue_schedule`.
- `RuntimeSceneGpuUploadOptions` already carries the runtime upload option structs, so the new option can pass through without a separate runtime-scene API.

## Constraints

- Preserve default per-upload-buffer behavior when no ring is provided.
- Ring-backed uploads must reserve all required staging spans before destination buffer creation, command-list creation, or payload writes.
- Keep submitted fence, queue, Frame Graph command-list, queue-wait, barrier, and callback counters unchanged.
- Do not claim native async upload execution, async overlap/performance, package mesh streaming, staging-pool production adoption, allocator/residency budgets, renderer-owned residency, or public native handles.

## Done When

- RED tests prove mesh uploads can use one caller-owned ring buffer for vertex and index staging.
- RED tests prove skinned mesh uploads can use the ring for vertex, index, and padded joint palette staging.
- RED tests prove morph mesh uploads can use the ring for position, optional normal/tangent, and padded weight staging.
- RED tests prove ring exhaustion fails before runtime buffer-upload side effects.
- Focused runtime RHI tests pass, agent-surface drift is reconciled, and the coherent C++/public-contract slice passes full `tools/validate.ps1`.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` after RED tests | Failed as expected because `RuntimeMeshUploadOptions::upload_ring`, `RuntimeSkinnedMeshUploadOptions::upload_ring`, and `RuntimeMorphMeshUploadOptions::upload_ring` did not exist yet. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_tests` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_runtime_rhi_tests|MK_runtime_scene_rhi_tests)$"` | Passed: 2/2 tests. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_rhi/src/runtime_upload.cpp,engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp,tests/unit/runtime_rhi_tests.cpp,tests/unit/runtime_scene_rhi_tests.cpp` | Passed: 4 files. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: `validate: ok`; CTest 65/65 passed. Host-gated Apple and Metal diagnostics remained diagnostic-only. | 2026-05-18 |
