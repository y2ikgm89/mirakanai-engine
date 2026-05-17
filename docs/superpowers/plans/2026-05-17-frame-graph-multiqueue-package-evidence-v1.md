# Frame Graph Multi-Queue Package Evidence v1 (2026-05-17)

**Plan ID:** `frame-graph-multiqueue-package-evidence-v1`
**Status:** Completed slice under `frame-graph-v1`.

## Goal

Make the completed backend-neutral Frame Graph multi-queue executor package-visible in the selected desktop runtime package smoke, without claiming production render graph adoption, async overlap/performance, Vulkan/Metal production multi-queue readiness, native queue/fence/semaphore exposure, or broad renderer quality.

## Context

`frame-graph-v1` already has internal executor evidence for `execute_frame_graph_rhi_multi_queue_schedule`, queue wait planning/recording, and optional texture barrier recording. The master plan recommends the next coherent foundation slice as package-visible multi-queue scheduling evidence.

## Official Practice Check

- Direct3D 12 official guidance uses queue-owned command-list submission plus fences and queue `Wait`/`Signal` to order copy/compute/graphics work across engines. This slice keeps those details behind `IRhiDevice` and exposes only first-party queue/fence evidence counters.
- Khronos Vulkan synchronization examples use queue submissions plus `vkCmdPipelineBarrier2KHR`/`VkDependencyInfo` for memory/image/buffer synchronization and queue ownership transfers where needed. This slice keeps Vulkan native synchronization private and reports only backend-neutral executor evidence.

## Constraints

- Keep gameplay and package manifests free of native RHI handles.
- Do not migrate high-level renderers to production multi-queue graph scheduling in this slice.
- Do not claim async overlap or performance from same-frame queue waits.
- Keep the package proof narrow and fail-closed when requested by smoke args.

## Done When

- A focused renderer/RHI test proves the new package evidence helper submits producer/consumer pass command lists, records the consumer queue wait, records the scheduled texture barrier, and reports clean diagnostics.
- `sample_desktop_runtime_game` accepts `--require-framegraph-multiqueue-evidence`, emits `framegraph_multiqueue_*` status fields, and fails smoke when requested evidence is not ready.
- Installed desktop runtime validation checks those fields when the new smoke arg is present.
- Game manifest/docs/plan registry/manifest fragments are synchronized, composed manifest is regenerated, and agent-surface drift is checked.
- Focused tests/package smoke pass, followed by one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

- [x] Add RED renderer/RHI test for package evidence helper.
- [x] Implement the backend-neutral evidence helper over `IRhiDevice`.
- [x] Wire `sample_desktop_runtime_game` package smoke output and fail-closed arg.
- [x] Update installed package validation and selected package smoke args.
- [x] Synchronize docs, manifest fragments, registry, and static checks.
- [x] Run focused validation, package smoke, agent checks, and full validation.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | RED, expected failure | Failed before implementation because `execute_frame_graph_rhi_multi_queue_package_evidence` was not declared. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | Passed | Focused renderer/RHI test target after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` | Passed | `frame graph rhi multi queue package evidence reports submitted waits and texture barriers` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_desktop_runtime_game MK_renderer_tests` | Passed | Focused Debug build for the changed sample and renderer tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` | Blocked before selected smoke | Existing broad `desktop-runtime-release` full build failure outside this slice: MSVC generated-file errors in unrelated runtime package tests and generated-sample shader custom build `VCEnd` label failures. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime-release --target sample_desktop_runtime_game` | Passed | Target-scoped Release package build for the changed sample. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --install out\build\desktop-runtime-release --config Release --prefix out\install\desktop-runtime-release` | Passed | Installed selected desktop runtime package artifacts. |
| `& .\tools\validate-installed-desktop-runtime.ps1 -InstallPrefix out\install\desktop-runtime-release -GameTarget sample_desktop_runtime_game -SmokeArgs ... --require-framegraph-multiqueue-evidence ... -RequireD3d12Shaders` | Passed | Installed D3D12 smoke proved `framegraph_multiqueue_status=ready`, two submitted command lists, one queue wait, one texture barrier, two submitted pass fences, positive queue submit/wait counters, and `framegraph_multiqueue_graphics_waited_for_copy=1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime-release --output-on-failure -R sample_desktop_runtime_game_smoke` | Passed | Existing source-tree sample smoke still passes in the desktop-runtime-release preset. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting/text checks after docs/manifest updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest compose and JSON contract checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent config/instruction surface checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration and manifest surface checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full slice gate, including static checks, focused tidy, full dev build, and 65 CTest tests. |
