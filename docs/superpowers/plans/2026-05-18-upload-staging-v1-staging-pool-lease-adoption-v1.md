# Upload Staging v1 Staging Pool Lease Adoption v1 - 2026-05-18

**Status:** Completed.

**Goal:** Let host-owned staging-pool chunks back `RhiUploadRing` uploads so selected package upload transactions can reuse pooled staging buffers without creating per-transaction upload buffers.

**Context:** `upload-staging-v1` already has `RhiStagingBufferPool`, ring-backed runtime texture/mesh/skinned/morph uploads, package texture/static/skinned/morph upload transactions, async upload batch submission, and upload queue waits. The remaining staging-pool blocker is that pool chunks are not first-class upload-ring leases, so package/runtime upload paths can only use rings that allocate their own backing buffer.

**Implementation surface:**
- Extend `RhiStagingBufferPool` in `engine/rhi/include/mirakana/rhi/upload_staging.hpp` and `engine/rhi/src/upload_staging.cpp` with an explicit lease row carrying the acquired `copy_source` buffer and chunk size.
- Extend `RhiUploadRingDesc` so a caller can bind a pre-acquired staging-pool buffer as the ring backing store while preserving the existing self-allocating ring path.
- Add RED-first tests in `tests/unit/rhi_upload_staging_tests.cpp` proving a pool lease backs async upload execution without allocating an extra ring buffer.
- Add RED-first runtime/package coverage in `tests/unit/runtime_rhi_tests.cpp` proving a package mesh upload transaction can use a pool-backed upload ring and return the pooled buffer in upload evidence.

**Constraints:**
- Keep ownership host-side: pool acquisition/release remains explicit, and runtime/package helpers receive only caller-owned `RhiUploadRing` pointers.
- Do not add background streaming, runtime-wide upload-ring ownership, allocator/GPU budget enforcement, renderer-owned residency, public native handles, selected package smoke readiness, or async-overlap/performance claims.
- Fail before command-list creation when pool-backed ring reservations cannot satisfy the staging allocation, preserving existing ring diagnostics.

**Done when:**
- RED tests fail for missing pool lease or pre-acquired ring backing-buffer API before implementation.
- `RhiUploadRing` can use a `RhiStagingBufferPool` lease buffer without calling `IRhiDevice::create_buffer` for the ring.
- Package mesh upload transactions using a pool-backed ring report caller-owned upload buffers, submitted fences, uploaded bytes, command-list counters, and graphics queue upload waits.
- Docs, manifest fragments, skills, and static guards describe staging-pool lease adoption while leaving selected 2D/3D package upload evidence, async-ready resource updates, allocator/GPU budgets, background streaming, public native handles, and async-overlap claims unsupported.
- Focused RHI/runtime RHI build/test/static checks pass, then full `tools/validate.ps1` passes for the coherent public RHI API slice.

**Validation evidence:**
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_upload_staging_tests MK_runtime_rhi_tests` failed before implementation because `RhiStagingBufferPool::try_acquire_lease` and `RhiUploadRingDesc::buffer` did not exist.
- Focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_upload_staging_tests MK_runtime_rhi_tests MK_runtime_scene_rhi_tests` passed.
- Focused tests: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_rhi_upload_staging_tests|MK_runtime_rhi_tests|MK_runtime_scene_rhi_tests"` passed.
- Focused static/format: `tools/check-tidy.ps1 -Files engine/rhi/src/upload_staging.cpp,tests/unit/rhi_upload_staging_tests.cpp,tests/unit/runtime_rhi_tests.cpp,tests/unit/runtime_scene_rhi_tests.cpp`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1` passed.
- Final full validation gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed.
