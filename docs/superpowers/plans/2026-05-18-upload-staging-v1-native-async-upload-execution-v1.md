# Upload Staging v1 Native Async Upload Execution v1 - 2026-05-18

**Status:** Completed.

**Goal:** Add a first-party RHI helper that submits an upload staging GPU batch to a selected backend-neutral queue without performing a CPU wait.

**Context:** `upload-staging-v1` already has `RhiUploadStagingPlan`, `RhiUploadRing`, GPU batch recording helpers, ring-backed runtime texture and mesh/skinned/morph uploads, and a static mesh package upload binding transaction. Callers still assemble command-list begin/record/close/submit/mark-submitted manually, so native async upload execution remains unsupported in the production gap ledger.

**Constraints:**
- Keep the API in `MK_rhi`; do not expose public native handles, backend-native fences, semaphores, or queue objects.
- Do not claim async overlap or performance. The guarantee is no helper-owned CPU wait plus a returned backend-neutral `FenceValue`.
- Fail before command-list creation when target counts or ring ownership are invalid.
- Keep runtime/package adoption, staging-pool production use, skinned/morph package streaming, and selected 2D/3D package evidence as follow-up `upload-staging-v1` work.

**Done when:**
- RED tests prove a staged buffer upload can be submitted on `QueueKind::copy` through one helper call, returns the submitted fence, records one copy-queue submit, and performs no `wait` or `wait_for_queue`.
- RED tests prove target mismatch and unreserved staging are rejected before command-list creation or upload submission.
- Focused RHI build/test/tidy pass.
- Docs, manifest fragments, and static guard needles describe the narrowed capability without broadening background streaming, public native handles, staging-pool production adoption, package skinned/morph streaming, or allocator/GPU budget claims.
- Full `tools/validate.ps1` passes for the coherent public RHI API slice.

**Implementation evidence:**
- Added `mirakana::rhi::RhiUploadGpuBatchExecutionResult` and `execute_upload_gpu_batch_async` in `mirakana/rhi/upload_staging.hpp`.
- The helper prevalidates upload target counts and `RhiUploadRing` ownership before command-list creation, records pending copies, closes and submits on the selected `QueueKind`, marks pending allocations with the returned `FenceValue`, and performs no helper-owned CPU wait.
- Added `MK_rhi_upload_staging_tests` coverage for copy-queue submission without waits, target-count mismatch rejection before command-list creation, and unreserved staging rejection before command-list creation.

**Validation evidence:**
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_upload_staging_tests` failed on missing `execute_upload_gpu_batch_async`.
- Focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_upload_staging_tests` passed.
- Focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_rhi_upload_staging_tests` passed.
- Focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/src/upload_staging.cpp,tests/unit/rhi_upload_staging_tests.cpp` passed.
- Agent/static: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-format.ps1` passed.
- Full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed (`validate: ok`, 65/65 CTest tests passed).
