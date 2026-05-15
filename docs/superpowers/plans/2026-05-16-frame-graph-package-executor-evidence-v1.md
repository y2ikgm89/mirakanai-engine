# Frame Graph Package Executor Evidence v1 (2026-05-16)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `frame-graph-package-executor-evidence-v1`
**Status:** Completed.

**Goal:** Make package/runtime smoke output prove that completed postprocess and directional-shadow frame graph paths execute both pass callbacks and executor-owned texture barrier steps.

**Architecture:** Keep the existing renderer-owned RHI resources and backend-private synchronization. Extend the SDL desktop presentation quality gate and package smoke output to surface `RendererStats::framegraph_barrier_steps_executed` with exact expected counts for the already migrated `RhiPostprocessFrameRenderer` and `RhiDirectionalShadowSmokeFrameRenderer` paths. This is evidence plumbing only; it does not add production graph ownership, transient allocation, aliasing, multi-queue scheduling, package streaming, Metal parity, or renderer-wide migration.

**Tech Stack:** C++23, `MK_renderer`, `MK_runtime_host_sdl3`, sample desktop runtime games, PowerShell validation wrappers.

---

## Official Practice Check

- Microsoft D3D12 documentation shows explicit command-list resource barriers transitioning resources between pass uses and before presentation; the package evidence should prove our executor records those transitions through `IRhiCommandList`, not through gameplay or package code.
- Khronos Vulkan synchronization examples show image barriers and layout transitions between render passes, including color/depth attachment writes becoming shader reads; the package evidence should remain backend-neutral and count executor-owned transitions without exposing Vulkan or D3D12 handles.
- Engine implication: package stdout may expose backend-neutral counts (`framegraph_passes_executed`, `framegraph_barrier_steps_executed`) but native barrier objects, layouts, queue ownership, and render-pass bodies stay private to renderer/RHI implementations.

## Context

- `RhiPostprocessFrameRenderer` now routes scheduled scene-color/depth inter-pass transitions and scene-depth final-state restoration through `execute_frame_graph_rhi_texture_schedule`.
- `RhiDirectionalShadowSmokeFrameRenderer` now routes scheduled shadow-depth, scene-color, scene-depth inter-pass transitions plus scene-depth/shadow-depth final-state restoration through the same executor.
- `SdlDesktopPresentationReport` already carries `RendererStats`, but package stdout and quality gates currently prove frame graph pass count/executions more strongly than exact executor-owned barrier-step counts.

## Constraints

- Keep package evidence host-gated and backend-neutral. Do not expose native handles, D3D12/Vulkan barrier structs, or resource ownership details.
- Keep exact expectations narrow: postprocess-depth package path expects 2 frame graph passes and 3 executor barrier steps per frame; directional-shadow package path expects 3 passes and 5 executor barrier steps per frame.
- Update tests before implementation and keep designated aggregate initializers in declaration order.
- Update docs, manifest fragments/composed manifest, and static guards only where the package-visible evidence claim changes durable agent behavior.

## Tasks

- [x] Add RED `MK_runtime_host_sdl3` coverage proving the quality gate reports expected frame graph barrier steps and blocks stale barrier-step budgets.
- [x] Extend `SdlDesktopPresentationQualityGateReport` with expected barrier-step count and current-budget booleans, then compute exact postprocess/depth/shadow expectations from the existing request flags.
- [x] Print package-visible `framegraph_passes_executed`, `framegraph_barrier_steps_executed`, and quality barrier-step fields in the desktop runtime sample and generated 3D/cooked/material package samples that already print `framegraph_passes`.
- [x] Tighten package smoke failure checks so postprocess-depth and directional-shadow requirements fail when barrier-step totals do not match the expected per-frame executor counts.
- [x] Update docs/manifest/skills/subagent/static guards for the new package-visible executor evidence claim, then compose `engine/agent/manifest.json`.
- [x] Run focused `MK_runtime_host_sdl3_tests`/package-relevant checks first, then close with `tools/validate.ps1` and `tools/build.ps1`.

## Done When

- Package-visible output includes frame graph pass executions and executor-owned barrier-step executions for the affected package paths.
- Quality gates and smoke requirements validate exact barrier-step budgets for postprocess-depth and directional-shadow evidence.
- Agent-facing docs, manifest fragments, composed manifest, skills/subagents, and static checks agree on the narrow claim.
- Validation evidence is recorded below and the branch is published through a PR.

## Validation Evidence

- RED: `cmake --build --preset dev --target MK_runtime_host_sdl3_tests` failed before implementation because `SdlDesktopPresentationQualityGateReport` did not expose expected/current frame graph barrier-step budget fields.
- PASS: `cmake --build --preset dev --target MK_runtime_host_sdl3_tests sample_desktop_runtime_game sample_generated_desktop_runtime_3d_package sample_generated_desktop_runtime_cooked_scene_package sample_generated_desktop_runtime_material_shader_package`.
- PASS: `ctest --preset dev --output-on-failure -R MK_runtime_host_sdl3_tests`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` with installed D3D12 smoke output proving `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_barrier_steps_executed=10`, `renderer_quality_expected_framegraph_barrier_steps=5`, and `renderer_quality_framegraph_barrier_steps_ok=1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` on the completed plan state, including `65/65` CTest pass. Apple/Metal checks remained expected host-gated or diagnostic-only on this Windows host.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
