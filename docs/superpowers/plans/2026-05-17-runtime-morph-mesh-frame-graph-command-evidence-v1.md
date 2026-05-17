# Runtime Morph Mesh Frame Graph Command Evidence v1 (2026-05-17)

**Status:** Completed
**Parent:** [Production Completion Master Plan v1](2026-05-03-production-completion-master-plan-v1.md)
**Gap:** `frame-graph-v1`

## Goal

Move byte-backed `upload_runtime_morph_mesh_cpu` POSITION, optional NORMAL, optional TANGENT, and morph-weight copy command recording plus command-list submission evidence under the existing Frame Graph RHI multi-queue executor, while keeping the slice limited to one morph mesh upload pass and one submitted command list.

## Context

- `Runtime Upload Frame Graph Transition Evidence v1` moved texture upload copy/final texture state transitions through `execute_frame_graph_rhi_texture_schedule`.
- `Runtime Mesh Frame Graph Command Evidence v1` moved byte-backed `upload_runtime_mesh` vertex/index copy command recording and submit evidence through `execute_frame_graph_rhi_multi_queue_schedule`.
- `Runtime Skinned Mesh Frame Graph Command Evidence v1` moved byte-backed `upload_runtime_skinned_mesh` vertex/index/joint-palette copy command recording and submit evidence through the same multi-queue executor.
- `upload_runtime_morph_mesh_cpu` still recorded morph delta and weight buffer copies directly into a caller-created command list and submitted it directly.

## Official Practice Check

- Direct3D 12 official documentation keeps GPU work recorded into command lists, submits command lists through queues, and uses fence signal/wait patterns for command allocator/list reuse and cross-engine ordering.
- Vulkan synchronization examples keep staging-to-device copies in command buffers, submit them to transfer or unified queues, and require barriers/queue synchronization before later vertex or shader reads. This slice keeps buffer barriers and queue-family ownership outside scope because the current first-party RHI exposes copy commands, submitted fences, and queue waits but not buffer resource states.

## Constraints

- Do not add public native handles or backend-specific queue/fence/semaphore objects.
- Do not add a buffer barrier API in this slice.
- Do not move material-factor uploads, staging rings, broad/background package streaming, renderer-owned residency, native async upload execution, or production graph ownership into scope.
- Preserve `RuntimeMorphMeshUploadOptions::queue` and `wait_for_completion` behavior while making the Frame Graph executor own the submitted pass command list.

## Done When

- `RuntimeMorphMeshUploadResult` reports Frame Graph command evidence for byte-backed morph mesh upload:
  - one submitted command list,
  - zero queue waits,
  - zero texture barriers,
  - one pass callback.
- Existing POSITION delta, optional NORMAL/TANGENT deltas, morph weights, copy regions, submitted fence, queue selection, wait behavior, and morph descriptor binding path stay covered.
- Docs, roadmap/current capability notes, manifest fragments plus composed manifest, and agent-surface drift checks agree that only runtime morph mesh upload command ownership evidence moved.
- Focused runtime/renderer static checks and one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass before commit/PR.

## Validation Evidence

| Command | Status | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS | Linked worktree toolchain and vcpkg links ready before implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS | Configured clean linked worktree baseline. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Baseline build before RED test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Baseline test before RED test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | EXPECTED FAIL | RED test failed because `RuntimeMorphMeshUploadResult` did not yet expose Frame Graph command-list, queue-wait, barrier, or pass-callback evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Runtime morph mesh upload now records morph delta and weight copies through one `execute_frame_graph_rhi_multi_queue_schedule` pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Morph mesh upload test covers submitted fence behavior, selected queue, descriptor binding path, and new Frame Graph command evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | Repository formatter left tracked text and C++ formatting clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Rebuilt after docs/manifest formatting and compose. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Re-ran focused runtime RHI tests after docs/manifest updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest fragments compose cleanly into `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary guard accepts the new first-party morph upload evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++ and tracked text formatting remain clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing docs/manifest checks agree with the runtime morph mesh upload command-evidence scope. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent instruction budgets and skill parity remain valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/runtime_rhi/src/runtime_upload.cpp,tests/unit/runtime_rhi_tests.cpp'` | PASS | Focused tidy for the changed runtime RHI implementation and test passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Fresh full slice gate passed in the linked worktree; diagnostic-only host gates remain Metal/Apple host evidence on this Windows host. |
| `cpp-reviewer` subagent read-only review | PASS | No blocking C++ ownership/API/lifetime issues; low test-evidence gaps were addressed with morph buffer readback and selected-queue/no-wait regression assertions. |
| `rendering-auditor` subagent read-only audit | PASS | No blocking Frame Graph/RHI issues; low current-capabilities boundary wording was addressed by explicitly leaving buffer barriers unsupported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | Post-review formatting left tracked text and C++ clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | PASS | Post-review runtime RHI test build passed after morph upload readback and compute-queue/no-wait assertions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Post-review formatting guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | PASS | Post-review morph upload regressions passed, including optional normal/tangent counters, destination readback, selected compute queue, and no forced wait. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'tests/unit/runtime_rhi_tests.cpp'` | PASS | Focused tidy for the post-review test changes passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-facing docs/current capability wording remains consistent with the narrow morph upload command-evidence scope. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Fresh post-review full slice gate passed in the linked worktree; diagnostic-only host gates remain Metal/Apple host evidence on this Windows host. |
