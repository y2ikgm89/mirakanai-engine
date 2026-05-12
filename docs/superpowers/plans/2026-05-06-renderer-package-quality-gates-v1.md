# Renderer Package Quality Gates v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow package-visible renderer quality gate for the installed `sample_desktop_runtime_game` proof by evaluating existing scene GPU, postprocess, directional-shadow, and framegraph counters as one deterministic readiness surface.

**Status:** Completed on 2026-05-06

**Architecture:** Keep the gate in the optional SDL3 desktop runtime presentation contract because the inputs are already `SdlDesktopPresentationReport` value fields and backend-neutral `RendererStats`. The helper evaluates existing first-party counters and status fields only; it does not run renderers, execute package scripts, expose native D3D12/Vulkan/Metal handles, add timing/performance claims, or generalize the sample smoke into a production renderer stack.

**Tech Stack:** C++23, `mirakana_runtime_host_sdl3` presentation reports, `sample_desktop_runtime_game` smoke output, PowerShell installed desktop runtime validation, focused `runtime_host_sdl3_tests`, docs/manifest/static AI guidance sync.

---

## Context

- The master plan lists `renderer-package-quality-gates-v1` as the remaining renderer/material quality follow-up after material graph package binding and shader hot-reload/cache-index reconciliation.
- `sample_desktop_runtime_game` already prints package-visible `scene_gpu_*`, `postprocess_*`, `directional_shadow_*`, `framegraph_passes`, and renderer stat counters.
- `tools/validate-installed-desktop-runtime.ps1` already checks many individual fields when scene GPU, postprocess, and directional shadow are requested.
- This slice adds a single explicit quality gate surface so operators and validation scripts can reason about the package renderer proof without interpreting many unrelated fields ad hoc.

## Constraints

- Do not add native handle getters, `IRhiDevice` exposure, descriptor handles, swapchain frame handles, GPU timestamp data, raw backend stats, or framegraph internals to gameplay/package APIs.
- Do not create new renderer/RHI execution paths, shader artifacts, package files, package streaming, broad performance budgets, or production renderer quality claims.
- Keep the gate deterministic and derived only from existing `SdlDesktopPresentationReport` and `RendererStats` fields.
- Keep Vulkan and Metal claims gated honestly; the new gate is backend-neutral over the selected package smoke report, not a cross-backend parity claim.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Quality Gate Tests

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_public_api_compile.cpp`

- [x] Add a test named `sdl desktop presentation quality gate accepts ready package renderer counters`.
- [x] Build a `SdlDesktopPresentationReport` with ready scene GPU, postprocess, postprocess depth input, directional shadow, fixed PCF filtering, `framegraph_passes=3`, two finished frames, six executed framegraph passes, and two postprocess passes.
- [x] Assert the new gate reports `ready`, `diagnostics_count=0`, `expected_framegraph_passes=3`, and every requested sub-gate as ready.
- [x] Add a failing-budget test proving missing directional shadow/filtering or stale framegraph pass counts block readiness and increment diagnostics.
- [x] Update the public API compile smoke to reference the new status enum, report type, status-name helper, and evaluator.
- [x] Run the focused `desktop-runtime` target build and confirm RED because the new API does not exist.

### Task 2: Quality Gate API

**Files:**
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`

- [x] Add `SdlDesktopPresentationQualityGateStatus`, `SdlDesktopPresentationQualityGateDesc`, and `SdlDesktopPresentationQualityGateReport`.
- [x] Add `sdl_desktop_presentation_quality_gate_status_name`.
- [x] Add `evaluate_sdl_desktop_presentation_quality_gate(const SdlDesktopPresentationReport&, const SdlDesktopPresentationQualityGateDesc&)`.
- [x] Derive expected package framegraph passes from selected requirements: 3 when directional shadow is requested, 2 when postprocess is requested, and 0 otherwise.
- [x] Mark readiness false and increment diagnostics for missing scene GPU bindings, missing postprocess/depth input, missing directional shadow/filtering, wrong framegraph pass count, or executed pass counts that do not match `expected_frames * expected_framegraph_passes`.

### Task 3: Package-Visible Sample Output

**Files:**
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/CMakeLists.txt`

- [x] Add `--require-renderer-quality-gates`.
- [x] When requested, require scene GPU bindings, postprocess, postprocess depth input, directional shadow, and directional shadow filtering.
- [x] Print `renderer_quality_status`, `renderer_quality_ready`, `renderer_quality_diagnostics`, `renderer_quality_expected_framegraph_passes`, `renderer_quality_framegraph_passes_ok`, `renderer_quality_framegraph_execution_budget_ok`, `renderer_quality_scene_gpu_ready`, `renderer_quality_postprocess_ready`, `renderer_quality_postprocess_depth_input_ready`, `renderer_quality_directional_shadow_ready`, and `renderer_quality_directional_shadow_filter_ready` on the main status line.
- [x] Fail smoke mode when `--require-renderer-quality-gates` is selected and the quality gate is not ready.
- [x] Add `--require-renderer-quality-gates` to the installed `sample_desktop_runtime_game` package smoke args.

### Task 4: Installed Validation And Static Guidance

**Files:**
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`

- [x] Validate the new `renderer_quality_*` fields when `--require-renderer-quality-gates` appears in smoke args.
- [x] Update static checks so manifest/docs cannot drop the new gate wording.
- [x] Keep ready claims narrow: no native handles, no GPU timings, no broad performance budgets, no package streaming, no cross-backend parity, and no production renderer quality claim.
- [x] Return `currentActivePlan` to the master plan after validation passes.

### Task 5: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run focused runtime host build/CTest commands.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and move this plan to Recent Completed in the registry.

## Done When

- A deterministic quality gate evaluates existing selected package renderer fields into one package-visible readiness result.
- `sample_desktop_runtime_game` exposes `renderer_quality_*` fields and can require the gate during installed package smoke.
- Installed desktop runtime validation checks the quality gate fields when the new smoke arg is present.
- Docs, manifest, Codex/Claude skills, and static checks state the new boundary honestly.
- Focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete local-tool blocker is recorded.

## Validation Results

- RED: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` failed before implementation because `SdlDesktopPresentationQualityGateDesc`, `evaluate_sdl_desktop_presentation_quality_gate`, and `SdlDesktopPresentationQualityGateStatus` did not exist.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Focused GREEN: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests`, `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile`, `cmake --build --preset desktop-runtime --target sample_desktop_runtime_game`, and `ctest --preset desktop-runtime -R "mirakana_runtime_host_sdl3_tests|mirakana_runtime_host_sdl3_public_api_compile" --output-on-failure` passed.
- Package GREEN: the strict build-output D3D12 smoke passed with `--require-renderer-quality-gates`, `--require-gpu-skinning`, and `--require-quaternion-animation`, reporting `renderer_quality_status=ready`, `renderer_quality_ready=1`, `renderer_quality_diagnostics=0`, `renderer_quality_framegraph_execution_budget_ok=1`, positive skinned GPU upload/resolve counters, and positive GPU skinning draw/descriptor counters.
- Package GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed with `installed-desktop-runtime-validation: ok (sample_desktop_runtime_game)` and `desktop-runtime-package: ok (sample_desktop_runtime_game)`.
- Final repository gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed.
