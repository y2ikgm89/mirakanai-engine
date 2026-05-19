# Runtime Mesh Frame Graph Command Evidence v1 (2026-05-17)

**Status:** Completed
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `frame-graph-v1`

## Goal

Move byte-backed `upload_runtime_mesh` command-list creation, copy command recording, and submit evidence under the existing Frame Graph RHI multi-queue executor, while keeping the slice limited to one mesh upload pass and one submitted command list.

## Context

- `Runtime Upload Frame Graph Transition Evidence v1` moved texture upload copy/final texture state transitions through `execute_frame_graph_rhi_texture_schedule`.
- `execute_frame_graph_rhi_multi_queue_schedule` already owns per-pass command-list begin/close/submit, producer fence evidence, queue waits, and optional texture barriers.
- `upload_runtime_mesh` still records its vertex/index buffer copies directly into a caller-created command list and submits it directly.

## Official Practice Check

- Direct3D 12 official documentation shows command lists record GPU commands, command allocators/lists are synchronized through fences before reuse, texture uploads transition copied resources before shader use, and copy/render/compute engines coordinate with fence signal/wait patterns before downstream reads.
- Vulkan synchronization examples show staging-to-device copies recorded in command buffers, submitted to transfer or unified queues, then synchronized before vertex/shader consumption through barriers and queue synchronization. This slice keeps buffer barriers and queue-family ownership outside scope because the current first-party RHI exposes copy commands and queue waits but not buffer resource states.

## Constraints

- Do not add public native handles or backend-specific queue/fence objects.
- Do not add a buffer barrier API in this slice.
- Do not move skinned mesh, morph mesh, material-factor uploads, staging rings, broad/background package streaming, or renderer-owned residency into scope.
- Preserve the existing `RuntimeMeshUploadOptions::queue` and `wait_for_completion` behavior while making the Frame Graph executor own the submitted pass command list.

## Done When

- `RuntimeMeshUploadResult` reports Frame Graph command evidence for byte-backed mesh upload:
  - one submitted command list,
  - zero queue waits,
  - zero texture barriers,
  - one pass callback.
- Existing vertex/index bytes, copy regions, submitted fence, queue selection, and wait behavior stay covered.
- Docs, roadmap/current capability notes, manifest fragments plus composed manifest, and agent-surface drift checks agree that only runtime mesh upload command ownership evidence moved.
- Focused runtime/renderer static checks and one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass before commit/PR.

## Validation Evidence

| Command | Status | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS | Linked worktree toolchain and vcpkg links ready before implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS | Configured clean linked worktree baseline. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Baseline build before RED test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Baseline test before RED test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | EXPECTED FAIL | RED test failed because `RuntimeMeshUploadResult` did not yet expose Frame Graph command-list, queue-wait, barrier, or pass-callback evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Runtime mesh upload now records vertex/index copies through one `execute_frame_graph_rhi_multi_queue_schedule` pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Mesh upload test covers uploaded bytes, copy regions, submitted fence behavior, selected queue, and new Frame Graph command evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | Applied repository formatting before static drift checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest fragments compose cleanly into `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary guard accepts the new first-party runtime upload evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++ and tracked text formatting remain clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing docs/manifest checks agree with the runtime mesh upload command-evidence scope. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent instruction budgets and skill parity remain valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/runtime_rhi/src/runtime_upload.cpp,tests/unit/runtime_rhi_tests.cpp'` | PASS | Focused tidy for the changed runtime RHI implementation and test passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_generated_desktop_runtime_material_shader_package_shaders` | PASS | After moving the linked worktree to `.worktrees/rmfg-ce`, the previously path-length-gated DXIL custom build passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Fresh full slice gate passed in the shortened linked worktree; diagnostic-only host gates remain Metal/Apple host evidence on this Windows host. |
