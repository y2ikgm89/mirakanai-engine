# Generated 3D Gameplay Systems Package Smoke v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `generated-3d-gameplay-systems-package-smoke-v1`  
**Status:** Completed.  
**Goal:** Promote the existing headless runtime gameplay systems composition proof into the committed generated `DesktopRuntime3DPackage` smoke so a generated 3D package reports validated physics, navigation, AI, audio, animation, package, and renderer evidence in one host-gated loop.

**Architecture:** Keep the proof inside `games/sample_generated_desktop_runtime_3d_package` as deterministic public-API gameplay code. The sample owns explicit in-memory physics collision rows, navigation grid rows, AI perception/blackboard rows, and audio PCM rows; it does not add hidden scene/physics/AI coupling or any native handles.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_audio`, `mirakana_ai`, `mirakana_navigation`, `mirakana_physics`, `mirakana_runtime`, `mirakana_runtime_scene`, `mirakana_scene_renderer`, SDL3 desktop runtime validation.

---

## Context

- `runtime-gameplay-systems-composition-v1` proves physics authored collision/controller, navigation, AI perception/blackboard, audio device streaming, animation, registry, and lifecycle composition only in `sample_gameplay_foundation`.
- The master plan still lists generated package composition evidence as remaining for the runtime systems row.
- `sample_generated_desktop_runtime_3d_package` already reports generated 3D package evidence for camera/controller, animation, morph, quaternion animation, package streaming, scene GPU, postprocess, renderer quality, directional shadows, and selected compute morph paths.
- This slice adds package-visible runtime systems evidence without broad generated 3D production readiness, scene/physics perception integration, navmesh/crowd, middleware, async AI services, codec streaming, HRTF/DSP, native audio devices, joints/CCD, exact shape sweeps, editor productization, Metal readiness, or native/RHI handles.

## Constraints

- Do not add dependencies, source importers, native handles, editor code, Dear ImGui, SDL3 APIs in gameplay code beyond the existing desktop host boundary, package scripts, broad dependency cooking, or broad package streaming claims.
- Use only public `mirakana::` headers from generated game code.
- Add tests/evidence before implementation where feasible by making the focused sample build fail on missing fields.
- Keep validation honest: source-tree smoke can use deterministic fallback; installed package smoke remains host-gated and selected by existing desktop runtime packaging recipes.

## Done When

- `sample_generated_desktop_runtime_3d_package` supports `--require-gameplay-systems`.
- The smoke output includes deterministic fields for `gameplay_systems_status`, `gameplay_systems_ready`, physics authored collision/controller, navigation path/tick, AI perception/behavior, audio stream, and animation/lifecycle evidence.
- Source-tree smoke args and installed package smoke args require gameplay systems evidence.
- `game.agent.json`, docs, master plan, plan registry, and static checks agree that this is generated package composition evidence only.
- Focused build/CTest, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing.

## Files

- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`
- Modify: `games/CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`

## Tasks

### Task 1: RED Gameplay Systems Gate

- [x] Add `--require-gameplay-systems` to the generated 3D sample smoke args and package smoke args.
- [x] Add executable assertions or references that require `gameplay_systems_*` output before the implementation exists.
- [x] Run focused build and record the expected smoke failure.

### Task 2: Generated Package Gameplay Systems Implementation

- [x] Add public API includes for `mirakana_ai`, `mirakana_audio`, `mirakana_navigation`, and `mirakana_physics`.
- [x] Add a small deterministic gameplay systems probe owned by `GeneratedDesktopRuntime3DPackageGame`.
- [x] Build authored physics collision rows and run conservative controller grounding.
- [x] Plan and tick a deterministic navigation agent to destination.
- [x] Project AI perception rows into a blackboard and evaluate a behavior tree action gated by physics/audio readiness.
- [x] Render a bounded audio device stream from deterministic PCM samples.
- [x] Expose status/counter getters and include the counters in sample output.
- [x] Make `--require-gameplay-systems` fail when any required counter is missing.

### Task 3: Docs, Manifest, And Static Checks

- [x] Update game manifest, README, current-truth docs, master plan, and generated-game validation scenarios.
- [x] Move `currentActivePlan` to this plan while active, then return it to the master plan after validation.
- [x] Update static checks so markers for `--require-gameplay-systems` and `gameplay_systems_ready` cannot drift.
- [x] Keep non-goals explicit in docs/manifest: no native handles, scene/physics perception integration, middleware, async AI services, codec streaming, device hotplug, navmesh/crowd, joints/CCD, broad generated 3D readiness, Metal, or editor productization.

### Task 4: Validation And Commit

- [x] Run focused CMake build for `sample_generated_desktop_runtime_3d_package`.
- [x] Run focused CTest for `sample_generated_desktop_runtime_3d_package_smoke`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` if needed.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add generated 3d gameplay systems package smoke`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset desktop-runtime; Invoke-CheckedCommand $tools.CMake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package; Invoke-CheckedCommand $tools.CTest --preset desktop-runtime --output-on-failure -R "^sample_generated_desktop_runtime_3d_package_smoke$"` | Expected RED failure | Build passed, then focused CTest failed because the sample rejected unknown `--require-gameplay-systems`. |
| Focused build/CTest | Passed | Re-ran after formatting; target build succeeded and `sample_generated_desktop_runtime_3d_package_smoke` passed 1/1. |
| Direct generated 3D package smoke | Passed | Manual smoke returned exit 0 with `gameplay_systems_status=ready`, `gameplay_systems_ready=1`, `gameplay_systems_navigation_plan_status=ready`, `gameplay_systems_blackboard_status=ready`, `gameplay_systems_behavior_status=success`, and `gameplay_systems_audio_status=ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | Passed | Re-ran after formatting; desktop-runtime validation passed 18/18 CTest cases. MSVC still reports the pre-existing C4819 warning in `engine/rhi/vulkan/src/vulkan_backend.cpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `production-readiness-audit-check: ok`; 11 unsupported gaps remain explicitly classified. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Initial run failed on `main.cpp`; ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, then `format-check: ok`. |
| `git diff --check` | Passed | Exit 0; Git reported only CRLF normalization warnings for touched text files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Exit 0. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Exit 0 through `tools/build.ps1` and the `dev` preset. |
