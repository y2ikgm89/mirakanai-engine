# 2026-05-18 Frame Graph Multi-Queue Render Pass Execution v1

**Plan ID:** `frame-graph-multiqueue-render-pass-execution-v1`
**Status:** Completed.

## Goal

Advance `frame-graph-v1` by letting the backend-neutral multi-queue executor own render-pass envelopes and writer target-state preparation for scheduled graphics passes.

## Context

Recent `frame-graph-v1` work already proves render-pass envelopes and writer target-state preparation in the single command-list texture executor, plus queue waits, command-list submission, and texture barriers in the multi-queue executor. The gap is that the multi-queue path cannot yet wrap scheduled graphics pass callbacks in `FrameGraphRhiRenderPassDesc` envelopes or prepare declared writer target states before those callbacks.

## Constraints

- Keep the API backend-neutral and do not expose native queues, fences, semaphores, render pass objects, or backend handles.
- Keep swapchain acquire/present, viewport readback/display, native overlay preparation, package residency, Vulkan/Metal memory alias allocation, data inheritance/content preservation, async overlap/performance, and public native handles out of this slice.
- Do not claim broad production graph ownership or broad renderer readiness; this is executor capability evidence only.
- Use RED/GREEN `MK_renderer_tests` before production code and focused renderer validation during implementation.
- Update manifest fragments and compose output; do not hand-edit `engine/agent/manifest.json`.

## Official Practice Check

- Microsoft Direct3D 12 command-list guidance keeps work recording in command lists and cross-queue dependency synchronization explicit through queues/fences.
- Microsoft Direct3D 12 resource barrier guidance keeps per-resource state tracking and write-to-read transitions as application responsibilities.
- Microsoft Direct3D 12 render pass guidance declares render-pass output bindings and requires pass commands to be nested between begin/end render-pass calls; render passes do not replace correct resource state tracking.

Sources: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/executing-and-synchronizing-command-lists>, <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>, <https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-render-passes>.

## Phases

- [x] Phase 1: Multi-queue render-pass target-state and envelope execution.
  - Extend `FrameGraphRhiMultiQueueExecutionDesc` with pass target access rows, pass target-state rows, and render-pass envelope rows.
  - Extend `FrameGraphRhiMultiQueueExecutionResult` with pass target-state and render-pass counters.
  - Prevalidate missing/duplicate/unknown render-pass and target-state rows before command recording.
  - Record target-state transitions after producer queue waits and before render-pass begin/callback execution.
  - Record render-pass begin/end around the selected scheduled pass callback on that pass command list.
  - Preserve the existing multi-queue order: producer submit fence -> consumer queue wait -> consumer texture barriers -> consumer target-state preparation -> render-pass envelope -> callback -> close/submit.

## Done When

- RED/GREEN tests prove the multi-queue executor records target-state barriers and render-pass envelopes before callbacks, reports the new counters, and rejects invalid rows before command-list creation.
- `MK_renderer_tests` focused build/test passes.
- Public API, docs, plan registry, manifest fragments, composed manifest, and static integration checks describe the new executor evidence without broadening ready claims.
- Full `tools/validate.ps1` passes at the C++/runtime public-contract phase gate, unless a concrete host/tool blocker is recorded.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` failed before the new multi-queue descriptor/result fields existed.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` failed before multi-queue render-pass target-state execution and prevalidation were implemented.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_renderer_tests --output-on-failure` passed.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, with expected diagnostic-only host gates for Metal/Apple packaging on this Windows host.
- Agent-surface drift follow-up: plan registry, current capabilities, roadmap, manifest fragment, and composed manifest were updated without broadening `frame-graph-v1` beyond `implemented-foundation-only`.
