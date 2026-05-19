# Runtime Skinned Mesh Frame Graph Command Evidence v1 (2026-05-17)

**Status:** Completed
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `frame-graph-v1`

## Goal

Move byte-backed `upload_runtime_skinned_mesh` vertex, index, and joint-palette copy command recording plus command-list submission evidence under the existing Frame Graph RHI multi-queue executor, while keeping the slice limited to one skinned mesh upload pass and one submitted command list.

## Context

- `Runtime Upload Frame Graph Transition Evidence v1` moved texture upload copy/final texture state transitions through `execute_frame_graph_rhi_texture_schedule`.
- `Runtime Mesh Frame Graph Command Evidence v1` moved byte-backed `upload_runtime_mesh` vertex/index copy command recording and submit evidence through `execute_frame_graph_rhi_multi_queue_schedule`.
- `upload_runtime_skinned_mesh` still recorded its vertex/index/joint-palette buffer copies directly into a caller-created command list and submitted it directly.

## Official Practice Check

- Direct3D 12 official documentation keeps GPU work recorded into command lists, submits command lists through queues, and uses fence signal/wait patterns for command allocator/list reuse and cross-engine ordering.
- Vulkan synchronization examples keep staging-to-device copies in command buffers, submit them to transfer or unified queues, and require barriers/queue synchronization before later vertex or shader reads. This slice keeps buffer barriers and queue-family ownership outside scope because the current first-party RHI exposes copy commands, submitted fences, and queue waits but not buffer resource states.

## Constraints

- Do not add public native handles or backend-specific queue/fence/semaphore objects.
- Do not add a buffer barrier API in this slice.
- Do not move morph mesh, material-factor uploads, staging rings, broad/background package streaming, renderer-owned residency, native async upload execution, or production graph ownership into scope.
- Preserve `RuntimeSkinnedMeshUploadOptions::queue` and `wait_for_completion` behavior while making the Frame Graph executor own the submitted pass command list.

## Done When

- `RuntimeSkinnedMeshUploadResult` reports Frame Graph command evidence for byte-backed skinned mesh upload:
  - one submitted command list,
  - zero queue waits,
  - zero texture barriers,
  - one pass callback.
- Existing vertex/index/joint-palette bytes, copy regions, submitted fence, queue selection, wait behavior, and descriptor-set binding path stay covered.
- Docs, roadmap/current capability notes, manifest fragments plus composed manifest, and agent-surface drift checks agree that only runtime skinned mesh upload command ownership evidence moved.
- Focused runtime/renderer static checks and one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass before commit/PR.

## Validation Evidence

| Command | Status | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS | Linked worktree toolchain and vcpkg links ready before implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS | Configured clean linked worktree baseline. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Baseline build before RED test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Baseline test before RED test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | EXPECTED FAIL | RED test failed because `RuntimeSkinnedMeshUploadResult` did not yet expose Frame Graph command-list, queue-wait, barrier, or pass-callback evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Runtime skinned mesh upload now records vertex/index/joint-palette copies through one `execute_frame_graph_rhi_multi_queue_schedule` pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Skinned mesh upload test covers submitted fence behavior, selected queue, descriptor binding path, and new Frame Graph command evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | Repository formatter left tracked text and C++ formatting clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Rebuilt after docs/manifest formatting and compose. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest fragments compose cleanly into `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary guard accepts the new first-party skinned upload evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++ and tracked text formatting remain clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Re-ran focused runtime RHI tests after docs/manifest updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing docs/manifest checks agree with the runtime skinned mesh upload command-evidence scope. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent instruction budgets and skill parity remain valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/runtime_rhi/src/runtime_upload.cpp,tests/unit/runtime_rhi_tests.cpp'` | PASS | Focused tidy for the changed runtime RHI implementation and test passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Fresh full slice gate passed in the linked worktree; diagnostic-only host gates remain Metal/Apple host evidence on this Windows host. |


