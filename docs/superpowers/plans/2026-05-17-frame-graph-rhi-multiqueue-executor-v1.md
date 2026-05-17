# Frame Graph RHI Multi-Queue Executor v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for behavior changes. Keep this slice in `frame-graph-v1`; do not broaden renderer readiness.

**Goal:** Add a backend-neutral Frame Graph RHI executor that opens native RHI command lists on declared pass queues, submits each pass, and records cross-queue waits only after producer pass fences exist.

**Architecture:** Build on `plan_frame_graph_rhi_queue_waits` and `record_frame_graph_rhi_queue_waits`. The new executor owns pass command-list lifetime and submission order, while callbacks receive the active `IRhiCommandList&` so pass code records against the queue that will be submitted. Native D3D12/Vulkan/Metal queue objects, semaphores, and handles stay behind `IRhiDevice`.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, NullRHI/D3D12/Vulkan/Metal `IRhiDevice` contracts, PowerShell 7 validation wrappers.

---

## Official Practice Check

- Microsoft Direct3D 12 queue synchronization guidance: producer queues `Signal` fences after submitted work; consumer queues `Wait` on that fence before dependent work.
- Khronos Vulkan synchronization guidance: cross-queue work requires queue submission synchronization plus appropriate barriers/ownership handling in the backend; this slice exposes only backend-neutral queue/fence intent and does not surface semaphores or queue-family ownership APIs.

## Context

- `Frame Graph RHI Queue Dependency Plan v1` already derives deterministic wait intent from scheduled barrier edges and records waits through `IRhiDevice::wait_for_queue`.
- `execute_frame_graph_rhi_texture_schedule` remains the single-command-list texture/barrier executor.
- This slice adds a separate native command-list submission envelope for pass queues. It is not an async-overlap/performance claim and does not yet migrate production renderer pass bodies.

## Constraints

- Keep all native handles backend-private.
- Do not expose D3D12 fences, Vulkan semaphores, Metal events, or queue-family ownership details through public renderer APIs.
- Do not claim Vulkan/Metal native multi-queue readiness beyond what their current `IRhiDevice` implementations support.
- Preserve clean-break API shape; no compatibility aliases.

## Tasks

- [x] Add RED tests for multi-queue pass execution: pass callbacks receive command lists for their declared queues, each pass is closed/submitted, and a graphics pass waits on the submitted compute/copy producer fence.
- [x] Add RED tests for invalid inputs: null device, duplicate pass binding, missing scheduled callback, callback failure, begin/submit/wait exceptions.
- [x] Add backend-neutral executor API to `frame_graph_rhi.hpp` / `frame_graph_rhi.cpp`.
- [x] Keep diagnostics deterministic and preserve partial evidence: submitted pass fences and recorded wait counts must reflect work that actually completed before failure.
- [x] Run focused renderer test/build/tidy checks.
- [x] Perform agent-surface drift check and update docs/skills/manifest/static checks only if durable behavior changed.
- [x] Close with full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Done When

- `MK_renderer_tests` proves pass queue command-list ownership, pass fence publication, and consumer-queue waits.
- The API records waits only after producer pass fences have been submitted.
- Hosted/backend readiness claims remain honest: no async overlap/performance, no public native handles, no Vulkan/Metal production multi-queue claim.
- Docs, skills, manifest fragments, composed manifest, and static checks match the shipped contract.

## Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed before implementation because `FrameGraphRhiPassCommandBinding`, `FrameGraphRhiMultiQueueExecutionDesc`, and `execute_frame_graph_rhi_multi_queue_schedule` were missing.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests`.
- GREEN focused test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests`.
- GREEN focused tidy: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/renderer/src/frame_graph_rhi.cpp,tests/unit/renderer_rhi_tests.cpp'`.
- Agent-surface drift: updated `docs/architecture.md`, rendering skill guidance, manifest fragments/composed manifest, plan registry, and scoped static guard needles for the new backend-neutral multi-queue executor contract.
- GREEN full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
