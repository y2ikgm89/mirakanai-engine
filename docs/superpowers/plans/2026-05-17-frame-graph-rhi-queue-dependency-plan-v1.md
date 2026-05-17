# Frame Graph RHI Queue Dependency Plan v1 - 2026-05-17

## Goal

Add a backend-neutral Frame Graph RHI queue dependency planning and wait-recording contract so future multi-queue execution can prove cross-queue pass ordering without exposing native queue, fence, semaphore, or command-list handles.

## Context

- `frame-graph-v1` already owns texture barriers, pass target states, transient aliasing barriers, render pass envelopes, package-streaming texture binding handoff, and primary/viewport/shadow pass envelope evidence.
- D3D12 official guidance synchronizes graphics/compute/copy command queues by signaling fences on producer queues and making consumer queues wait before dependent work.
- Vulkan official synchronization guidance uses queue submission synchronization plus queue-family ownership transfer acquire/release rules when resources move across queue families.
- This slice records only backend-neutral queue wait intent and `IRhiDevice::wait_for_queue` calls from already submitted pass fences. It does not split Frame Graph recording into native per-queue command lists.

## Official Practice Check

- Context7 / Microsoft Learn: Direct3D 12 multi-engine synchronization examples use per-queue fence `Signal` plus `Wait` before dependent rendering work.
- Context7 / Khronos Vulkan docs: cross-queue resource handoff requires ordered release/acquire synchronization, commonly ordered with semaphores, and queue-family ownership transfer remains backend-private.

## Constraints

- Clean break; do not add compatibility aliases, native handle exposure, or public D3D12/Vulkan/Metal queue objects.
- Keep `execute_frame_graph_rhi_texture_schedule` on one caller-owned command list in this slice.
- Do not claim async overlap, measured performance, Vulkan/Metal multi-queue readiness, production graph ownership, package streaming expansion, data inheritance/content preservation, or broad renderer readiness.
- Queue waits must be derived from scheduled Frame Graph barrier edges and explicit pass queue bindings, then recorded through existing backend-neutral `IRhiDevice::wait_for_queue`.

## Plan

- [x] Add RED renderer/RHI tests for cross-queue wait planning from scheduled pass barriers.
- [x] Add RED tests for queue wait recording from submitted producer pass fences, including missing/duplicate/wrong-queue fence diagnostics.
- [x] Add minimal backend-neutral queue binding, wait plan, and wait recording APIs in `frame_graph_rhi`.
- [x] Update docs, plan registry, manifest fragments/composed manifest, and agent-surface checks if durable guidance or AI-operable claims changed.
- [x] Run focused renderer/RHI validation, then full slice validation before commit/PR.

## Done When

- Cross-queue Frame Graph barriers produce deterministic consumer queue wait rows while same-queue barriers do not.
- Wait recording calls `IRhiDevice::wait_for_queue` only after matching submitted producer pass fences are supplied.
- Diagnostics reject empty/duplicate/unscheduled pass queue bindings and empty/duplicate/missing/wrong-queue submitted fences before recording waits.
- Docs/manifest/plan surfaces describe the narrow multi-queue foundation without broad readiness claims.
- Focused renderer/RHI tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or an exact host/tool blocker is recorded.

## Validation Evidence

- RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed before implementation because `FrameGraphRhiPassQueueBinding`, `FrameGraphRhiQueueWait`, `FrameGraphRhiSubmittedPassFence`, `plan_frame_graph_rhi_queue_waits`, and `record_frame_graph_rhi_queue_waits` were not defined.
- Focused GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` passed.
- Focused GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` passed.
- Focused GREEN evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/renderer/src/frame_graph_rhi.cpp,tests/unit/renderer_rhi_tests.cpp'` passed.
- Slice-closing validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Diagnostic-only host gates remained expected on this Windows host for Metal/Apple lanes.
