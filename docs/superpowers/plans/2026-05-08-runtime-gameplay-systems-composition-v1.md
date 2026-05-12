# Runtime Gameplay Systems Composition v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-gameplay-systems-composition-v1`  
**Status:** Complete.  
**Goal:** Promote `sample_gameplay_foundation` into a headless generated-game composition proof that uses physics authored collision/controller movement, navigation path planning, AI perception/blackboard behavior, audio device streaming, animation, and the game lifecycle together without native handles or middleware.

**Architecture:** Keep this as a source-tree headless sample proof over existing public `mirakana::` APIs. The sample owns explicit rows for physics collision, navigation grids, perception targets, behavior-tree leaf statuses, and audio PCM samples; engine subsystems stay decoupled and do not gain scene/physics/AI hidden integration.

**Tech Stack:** C++23, `mirakana_core`, `mirakana_animation`, `mirakana_audio`, `mirakana_physics`, `mirakana_navigation`, `mirakana_ai`, existing `dev` preset.

---

## Context

- Master plan runtime-system row still lists generated gameplay composition evidence as remaining after Audio Device Streaming Baseline v1, Physics Character Controller Collision Authoring v1, Navigation Production Path v1, and AI Perception Services v1.
- `sample_gameplay_foundation` currently proves only deterministic `PhysicsWorld3D`, `AnimationStateMachine`, `HeadlessRunner`, and `Registry`.
- `sample_ai_navigation` separately proves AI perception + behavior tree + navigation, but no single sample currently composes the latest runtime-system baselines.

## Constraints

- Do not add new engine APIs, dependencies, native handles, SDL3, Dear ImGui, editor code, renderer/RHI code, asset importers, scene integration, middleware, async jobs, or package/runtime-host claims.
- Keep the evidence honest: this proves headless source-tree composition only, not generated package shipping, navmesh/crowd, physics joints/exact casts/CCD, scene/physics perception integration, audio codec streaming, or persistent/async AI services.
- Use existing public headers from game code.
- Use TDD: first make the sample's executable assertions require the new composition evidence and verify the focused build fails before adding implementation fields.

## Done When

- `sample_gameplay_foundation` exits successfully only after one `HeadlessRunner` loop proves all of these in one game object:
  - deterministic dynamic physics body motion and animation transition still match the existing sample expectations;
  - `build_physics_world_3d_from_authored_collision_scene` creates reviewed collision rows;
  - `move_physics_character_controller_3d` grounds a capsule controller against authored collision;
  - `plan_navigation_grid_agent_path` creates a ready path and `update_navigation_agent` reaches its destination;
  - `build_ai_perception_snapshot_2d`, `write_ai_perception_blackboard`, and `evaluate_behavior_tree` drive movement from explicit target/path facts;
  - `render_audio_device_stream_interleaved_float` renders bounded PCM frames from an `AudioMixer` without device/native handles.
- `games/sample_gameplay_foundation/game.agent.json` and README describe the expanded public API composition and preserve unsupported boundaries.
- `engine/agent/manifest.json`, current-truth docs, plan registry, master plan, and static checks agree that this is source-tree composition evidence only.
- Focused sample build/CTest, schema/static checks, format check, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing.

## Files

- Modify: `games/sample_gameplay_foundation/main.cpp`
- Modify: `games/sample_gameplay_foundation/game.agent.json`
- Modify: `games/sample_gameplay_foundation/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`

## Tasks

### Task 1: Red Composition Assertion

- [x] Add `sample_gameplay_foundation` executable assertions and output fields for runtime-system composition evidence before implementation fields exist.
- [x] Run focused sample build and record the expected compile failure.

### Task 2: Headless Composition Implementation

- [x] Add public API includes and small helper/status-name functions in `sample_gameplay_foundation/main.cpp`.
- [x] Build authored collision world rows and run conservative controller grounding.
- [x] Plan a deterministic navigation path, publish AI perception facts into a blackboard, evaluate a behavior tree, and tick the navigation agent to destination.
- [x] Render a bounded audio device stream from deterministic PCM sample data.
- [x] Preserve existing physics body, animation, entity cleanup, and deterministic exit checks.

### Task 3: Documentation And Contract Checks

- [x] Update the sample manifest and README.
- [x] Update current-truth docs, master plan, plan registry, and `engine/agent/manifest.json` with narrow source-tree composition evidence and explicit unsupported boundaries.
- [x] Add static check assertions for the expanded sample evidence.

### Task 4: Validation And Commit

- [x] Run focused sample build and CTest.
- [x] Run schema/static checks touched by docs and manifests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Stage only this slice and commit as `feat: add runtime gameplay systems composition proof`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target sample_gameplay_foundation` | Expected RED failure | Failed before implementation because `SampleGameplayFoundationGame` did not yet expose the runtime composition evidence getters required by `main`. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target sample_gameplay_foundation; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "^sample_gameplay_foundation$"` | Passed | Focused sample build and CTest passed after implementation. |
| `out/build/dev/games/Debug/sample_gameplay_foundation.exe` | Passed | Output included `frames=4`, `authored_collision_bodies=2`, `controller_grounded=1`, `nav_points=3`, `behavior_nodes=4`, and `audio_frames=2`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest/schema validation passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Static AI/game-development guidance passed after adding `sample_gameplay_foundation` composition assertions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Unsupported production gap audit remained green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Initial run found clang-format drift in `sample_gameplay_foundation/main.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed with 29/29 CTest; Apple/Metal host gates remained diagnostic-only and changed-file tidy reported `tidy-check: ok (1 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit precondition build completed after validation. |

## Status

**Status:** Complete.
