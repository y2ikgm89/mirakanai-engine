# Upload Ring And Staging Pool Execution v1 Implementation Plan (2026-05-03)

> **For agentic workers:** Phase 3 master child `**upload-ring-and-staging-pool-execution-v1`**. Adds **device-backed** upload staging execution: a fence-tracked byte **ring** over one `copy_source` upload buffer, a fixed **staging buffer pool** for whole-chunk acquisitions, and **command-list recording** that pairs `RhiUploadStagingPlan` pending rows with explicit GPU destinations—without claiming async streaming, package integration, or native backend upload-queue specialization.

**Goal:** `RhiUploadRing` sub-allocates aligned byte ranges inside a device buffer, tracks in-flight byte spans until `release_completed`, and maps `RhiUploadAllocationHandle::id` to ring byte offsets for `record_upload_gpu_batch`. `RhiStagingBufferPool` hands out `copy_source` buffers from a fixed freelist. `validate_upload_gpu_batch` / `record_upload_gpu_batch` / `mark_pending_allocations_submitted` connect the existing value-only `RhiUploadStagingPlan` to `IRhiCommandList::copy_buffer` / `copy_buffer_to_texture` on the null backend and real backends that honor the same usage flags.

**Architecture:** Types and functions live in `mirakana/rhi/upload_staging.hpp` and `upload_staging.cpp` beside `RhiUploadStagingPlan`. No new `IRhiDevice` entry points; ring/pool own `BufferHandle` values created through `create_buffer` with `BufferUsage::copy_source` (upload). Destinations remain caller-owned buffers/textures with `copy_destination` as already enforced by null RHI.

**Tech Stack:** `engine/rhi/include/mirakana/rhi/upload_staging.hpp`, `engine/rhi/src/upload_staging.cpp`, `tests/unit/rhi_upload_staging_tests.cpp`, `engine/agent/manifest.json`, master plan + plan registry.

---

## Phases

### Phase v1（本スライス・完了）

- `RhiUploadRing` + `RhiStagingBufferPool` + GPU batch validation/recording helpers.
- `rhi_upload_staging_tests` end-to-end null path: reserve, `write_buffer`, enqueue, record, submit, wait, retire.
- Manifest `mirakana_rhi` purpose + `recommendedNextPlan` sync; master plan checkbox.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` evidence.

### Follow-ups

- Optional `copy_destination` on ring buffer for readback tooling; upload queue/async fences; package/runtime integration.

## Validation Evidence


| Command                                         | Result         | Date       |
| ----------------------------------------------- | -------------- | ---------- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`                              | `validate: ok` | 2026-05-03 |
| `ctest -C Debug -R mirakana_rhi_upload_staging_tests` | Passed         | 2026-05-03 |


## Done When

- Null RHI records buffer and texture uploads from a ring-backed staging plan and counters advance.
- Pool acquire/release is deterministic under exhaustion.

## Non-Goals

- Mesh/texture streaming, partial mip chains, compression metadata, or editor panels.

---

*Completed: 2026-05-03.*
