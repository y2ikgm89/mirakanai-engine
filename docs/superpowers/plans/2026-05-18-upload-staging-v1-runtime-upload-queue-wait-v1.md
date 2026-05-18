# Upload Staging v1 Runtime Upload Queue Wait v1 - 2026-05-18

**Status:** Completed.

**Goal:** Add runtime/package upload queue-consumption evidence so async copy-queue uploads can be made visible to graphics consumers through backend-neutral GPU-side queue waits.

**Context:** RHI Native Async Upload Execution v1 added `execute_upload_gpu_batch_async` for caller-owned upload staging batches without helper-owned CPU waits. Runtime/package mesh upload transactions can already return submitted upload fences, but the package static-mesh binding transaction does not yet record a graphics queue wait for copy-queue uploads consumed by renderer mesh bindings.

**Implemented surface:** `wait_for_runtime_uploads_on_queue`, `RuntimeUploadQueueWaitResult`, and `RuntimePackageStreamingMeshUploadBindingResult::upload_queue_waits_recorded`.

**Constraints:**
- Keep the API in `MK_runtime_rhi` over backend-neutral `QueueKind` and `FenceValue` values.
- Do not expose native queue/fence/semaphore handles.
- Do not claim async overlap or performance; the guarantee is deterministic GPU-side wait intent through `IRhiDevice::wait_for_queue`.
- Keep staging-pool production adoption, package skinned/morph streaming, selected 2D/3D package evidence, and allocator/GPU budgets as follow-up work.

**Done when:**
- RED tests prove a package static mesh transaction with `QueueKind::copy`, caller-owned upload ring, and `wait_for_completion=false` returns a copy-queue upload fence, records a graphics queue wait for that fence, and performs no CPU fence wait.
- A reusable runtime upload queue-wait helper reports diagnostics and queue-wait counts without native handle exposure.
- Focused runtime RHI build/test/tidy pass.
- Docs, manifest fragments, and static guard needles describe runtime/package async upload queue consumption without broadening background streaming, public native handles, staging-pool production adoption, package skinned/morph streaming, or allocator/GPU budget claims.
- Full `tools/validate.ps1` passes for the coherent public runtime RHI API slice.

**Validation evidence:**
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` failed on missing `RuntimePackageStreamingMeshUploadBindingResult::upload_queue_waits_recorded`.
- Focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` passed.
- Focused test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` passed.
- Focused tidy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_rhi/src/runtime_upload.cpp,engine/runtime_rhi/src/package_streaming_frame_graph.cpp,tests/unit/runtime_rhi_tests.cpp` passed.
- Agent/static drift: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, and `tools/check-public-api-boundaries.ps1` passed after docs/manifest/static sync.
- Format: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after docs/manifest/static sync.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 65/65 CTest tests; Metal/Apple lanes remained diagnostic host-gated on Windows.
