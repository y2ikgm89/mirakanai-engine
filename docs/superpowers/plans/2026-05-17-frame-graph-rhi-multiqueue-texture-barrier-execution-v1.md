# Frame Graph RHI Multi-Queue Texture Barrier Execution v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for behavior changes. Keep this slice in `frame-graph-v1`; do not broaden renderer readiness.

**Goal:** Let the backend-neutral Frame Graph RHI multi-queue executor optionally record scheduled texture barriers on the consumer pass command list after producer queue waits and before the consumer callback.

**Architecture:** Extend `FrameGraphRhiMultiQueueExecutionDesc` with opt-in imported `FrameGraphTextureBinding` rows and expose `barriers_recorded` in `FrameGraphRhiMultiQueueExecutionResult`. The executor keeps its pass command-list submission envelope, prevalidates texture barrier bindings before any command recording when bindings are supplied, records producer waits first, then records the consumer pass texture transition on that pass command list before invoking its callback. Native queues, fences, semaphores, and queue-family details stay behind `IRhiDevice`.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, NullRHI, PowerShell 7 validation wrappers.

---

## Official Practice Check

- Direct3D 12 queue synchronization uses producer queue signal plus consumer queue wait before dependent command lists execute.
- Vulkan cross-queue work requires submission synchronization plus backend-owned pipeline barriers and queue-family details when needed. This slice exposes only backend-neutral wait intent plus public `IRhiCommandList::transition_texture` recording.

## Context

- `Frame Graph RHI Queue Dependency Plan v1` derives cross-queue waits from scheduled barrier edges.
- `Frame Graph RHI Multi-Queue Executor v1` opens, closes, submits, and publishes one command list/fence per scheduled pass.
- Scheduled texture transition recording still lives in the single-command-list `execute_frame_graph_rhi_texture_schedule` path.

## Constraints

- Keep this opt-in: an empty `texture_bindings` span preserves the existing queue-wait-only multi-queue behavior.
- Do not migrate `RhiFrameRenderer`, postprocess, directional-shadow, viewport, or package-visible renderers in this slice.
- Do not claim async overlap, performance, Vulkan/Metal production multi-queue readiness, queue-family ownership handling, package-visible scheduling, or production render graph ownership.
- Preserve clean-break API shape; no compatibility aliases.

## Tasks

- [x] Add RED tests proving the multi-queue executor records a scheduled texture barrier on the consumer pass command list after the producer wait and before the consumer callback.
- [x] Add RED tests proving opt-in texture barrier validation fails before any command recording when bindings are supplied but the scheduled barrier has no matching texture binding.
- [x] Extend `FrameGraphRhiMultiQueueExecutionDesc` / `FrameGraphRhiMultiQueueExecutionResult` and implement prevalidation plus per-consumer-pass texture barrier recording.
- [x] Run focused renderer build/test and public API boundary checks.
- [x] Update docs, plan registry, manifest fragments/composed manifest, rendering skills/subagents, and static needles for the narrowed capability.
- [x] Close with full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `tools/build.ps1` before commit/PR.

## Done When

- `MK_renderer_tests` proves a copy/compute producer pass can publish a fence, a graphics consumer pass waits, and the consumer command list records the scheduled texture transition before callback execution.
- The result reports `command_lists_submitted`, `queue_waits_recorded`, `barriers_recorded`, and `pass_callbacks_invoked` for completed work.
- Invalid opt-in texture bindings fail before command list begin/submit/callback evidence.
- Durable docs/manifest/skills/static checks describe this as a foundation-only executor capability and keep production graph adoption, async overlap/performance, Vulkan/Metal production readiness, and renderer migration unsupported.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed before implementation because `FrameGraphRhiMultiQueueExecutionDesc` did not expose `texture_bindings`.
- GREEN build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests --parallel 1` passed. The prior parallel build hit transient MSVC `C1041` PDB contention, so the focused target was rerun serially.
- GREEN test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` passed.
- Static drift checks passed: `git diff --check`, `tools/check-json-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1`.
- Focused clang-tidy passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/frame_graph_rhi.cpp`.
- Full closeout validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- Standalone build passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
