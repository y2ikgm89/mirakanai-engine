# 2D Sprite Batch Package Telemetry v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Connect the host-independent sprite batch planner to the packaged 2D sample telemetry so package smokes can verify deterministic batch-plan counts without claiming native GPU batch execution.

**Architecture:** Keep `mirakana_scene` renderer-neutral and keep the existing `mirakana::plan_sprite_batches` execution-free. Add a small `mirakana_scene_renderer` adapter that maps `SceneRenderPacket::sprites` through `make_scene_sprite_command`, then expose the resulting batch-plan counters in `sample_2d_desktop_runtime_package` smoke output and package validation.

**Tech Stack:** C++23, `mirakana_scene_renderer`, `mirakana_renderer`, `sample_2d_desktop_runtime_package`, PowerShell static/package checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Goal

Add `2d-sprite-batch-package-telemetry-v1` as a narrow follow-up to `2d-sprite-batch-planning-contract`. The ready claim is limited to deterministic scene-to-sprite-batch telemetry for the selected packaged 2D smoke path.

## Post-2D Sprite Batch Planning Contract Review

- Latest completed slice: `docs/superpowers/plans/2026-05-02-2d-sprite-batch-planning-contract-v1.md`.
- Validation evidence from that slice records `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with `validate: ok` and 28/28 CTest entries passed.
- Host gates remain explicit: Apple/iOS/Metal requires macOS/Xcode/metal/metallib, strict clang-tidy remains diagnostic-only without a compile database for the active Visual Studio generator, and Vulkan package claims remain strict host/runtime/toolchain-gated outside verified hosts.
- Non-goals remain non-ready: production sprite batching execution, source/runtime image decoding, production atlas packing, tilemap editor UX, package streaming execution, renderer/RHI teardown execution, public native/RHI handles, Metal/iOS readiness, broad renderer quality, material/shader graphs, live shader generation, and production text/font/IME/accessibility.
- The next useful focused slice is not native batch execution. It is package-visible telemetry proving the existing planning contract can be applied to cooked 2D scene sprites through public renderer-neutral paths.

## Context

- `mirakana::plan_sprite_batches` already accepts `std::span<const SpriteCommand>` and reports batch, draw, texture-bind, textured-sprite, and diagnostic counts.
- `mirakana_scene_renderer` already owns the translation from `SceneRenderPacket::sprites` to `SpriteCommand` through `make_scene_sprite_command`.
- `sample_2d_desktop_runtime_package` already emits package smoke counters for scene sprite submission and native 2D overlay status.
- `tools/validate-installed-desktop-runtime.ps1` already enforces selected smoke report fields.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset formats, editor, SDL3, Dear ImGui, and native handles.
- Keep `mirakana_scene` independent from renderer and RHI APIs.
- Do not expose native OS/GPU/RHI handles through gameplay APIs.
- Do not introduce third-party dependencies.
- Do not sort sprites or change transparent draw ordering.
- Do not claim production sprite batching execution, production renderer quality, atlas packing, runtime/source image decoding, package streaming readiness, Metal/iOS readiness, material/shader graphs, live shader generation, production text/font/IME/accessibility, public native/RHI handles, or broad generated 2D production readiness.

## Done When

- [x] RED test/static check is added before implementation and fails for missing scene/package batch telemetry.
- [x] `mirakana_scene_renderer` exposes a host-independent `plan_scene_sprite_batches` helper over `SceneRenderPacket`.
- [x] Unit coverage proves scene sprite rows are converted into deterministic batch-plan counters and invalid sprite texture metadata diagnostics flow through.
- [x] `sample_2d_desktop_runtime_package` smoke output reports `sprite_batch_plan_*` counters for the loaded cooked scene.
- [x] Installed desktop runtime validation requires the new 2D package telemetry fields for `sample_2d_desktop_runtime_package`.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` are synchronized with the limited telemetry ready claim.
- [x] Static checks keep production sprite batching execution, atlas packing, image decoding, Metal, public native/RHI handle, and renderer quality claims non-ready.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has PASS evidence or a concrete environment blocker recorded here.
- [x] Registry and manifest are re-read after completion to select the next focused slice or stop on host-gated/broad-only work.

## Implementation Tasks

- [x] Add RED checks to `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` requiring `plan_scene_sprite_batches`, `sprite_batch_plan_draws`, `sprite_batch_plan_texture_binds`, and `2D Sprite Batch Package Telemetry v1` documentation text.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and record the expected failure.
- [x] Add `plan_scene_sprite_batches(const SceneRenderPacket&)` to `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp` and `engine/scene_renderer/src/scene_renderer.cpp`.
- [x] Add unit tests in `tests/unit/scene_renderer_tests.cpp` for scene packet batch telemetry and invalid texture metadata diagnostics.
- [x] Update `games/sample_2d_desktop_runtime_package/main.cpp` to accumulate and print the batch-plan counters.
- [x] Update `tools/validate-installed-desktop-runtime.ps1` to require the new fields for the 2D package smoke.
- [x] Update docs, registry, manifest, and static checks for the limited ready claim.
- [x] Run focused checks, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-step decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | RED FAIL (expected) | Before GREEN implementation, failed on `games/sample_2d_desktop_runtime_package/main.cpp missing native presentation smoke field: mirakana/renderer/sprite_batch.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS | CMake 3.31.6, CTest 3.31.6, MSBuild ready. |
| `cmake --preset dev` | PASS | Configured `out/build/dev`; `import std` remained gated for this generator/toolchain. |
| `cmake --build --preset dev` | HOST ENV BLOCKED | Direct shell build hit MSBuild `Path`/`PATH` duplicate environment key failure before compiling `mirakana_scene_renderer`; repository wrappers normalize process environment. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Configured and built dev preset with `mirakana_scene_renderer_tests` and samples. |
| `ctest --preset dev -R mirakana_scene_renderer_tests --output-on-failure` | PASS | 1/1 `mirakana_scene_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | PASS | Desktop runtime lane passed 16/16 CTest entries, including `sample_2d_desktop_runtime_package_smoke`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 28/28 CTest entries passed. Host gates remained diagnostic-only for Apple/iOS/Metal and strict clang-tidy compile database availability. |

## Non-Goals

- Native GPU sprite batch execution.
- A production 2D batching system or renderer quality claim.
- Sprite sorting, material sorting, transparency policy, or render-queue ownership.
- Source/runtime image decoding.
- Production atlas packing, glyph atlas packing, or tilemap editor UX.
- Renderer/RHI residency expansion, package streaming execution, or renderer/RHI teardown execution.
- Public gameplay access to native OS, GPU, or RHI handles.
- Metal/iOS readiness.
- Material/shader graphs, live shader generation, production text/font/IME/accessibility, or broad generated 2D production readiness.

## Next-Step Decision

Registry and manifest were re-read after validation. `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this completed telemetry slice and `recommendedNextPlan` is `post-2d-sprite-batch-package-telemetry-review`. The remaining manifest gaps are broad planned areas (`3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters`) plus Apple/iOS/Metal host gates. No next concrete dated focused slice is selected in the registry or manifest, so this run stops here instead of broadening into unplanned renderer, editor, importer, Metal, native-handle, or production sprite batching claims.
