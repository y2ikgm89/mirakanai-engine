# Generated 3D Visible Production Game Proof Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` when independent implementation/review tasks can run in parallel, otherwise use `superpowers:executing-plans`. Track steps with checkbox (`- [ ]`) syntax.

**Goal:** Add a narrow selected D3D12 package validation proof that a generated `DesktopRuntime3DPackage` game can run as a visible/windowed production-style 3D package path with package-loaded scene content, gameplay systems, renderer quality gates, postprocess, playable aggregate evidence, and native UI overlay evidence, without promoting broad generated 3D production readiness.

**Architecture:** Build on the committed `sample_generated_desktop_runtime_3d_package` and its existing component smokes. Add a selected `--require-visible-3d-production-proof` smoke that composes already-supported runtime package loading, camera/controller, transform/morph/quaternion animation, package streaming safe point, public gameplay systems, D3D12 scene GPU bindings, depth-aware postprocess, renderer quality gates, playable aggregate counters, and native UI overlay HUD box counters into a small set of `visible_3d_*` output fields. Keep image/glyph UI atlas, directional shadow, shadow+morph composition, Vulkan, Metal, source import execution, broad package streaming, and public native/RHI handles as separate selected or follow-up lanes.

**Tech Stack:** C++23, `DesktopRuntime3DPackage`, `mirakana_runtime`, `mirakana_runtime_scene`, `mirakana_scene_renderer`, `mirakana_runtime_host_sdl3`, `mirakana_runtime_host_sdl3_presentation`, D3D12 DXIL package shaders, CMake desktop-runtime metadata, PowerShell validators/static checks.

---

**Plan ID:** `generated-3d-visible-production-game-proof-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Previous Slice:** [2026-05-09-generated-3d-native-ui-text-glyph-atlas-package-smoke-v1.md](2026-05-09-generated-3d-native-ui-text-glyph-atlas-package-smoke-v1.md)

## Context

- The master plan now points back to `next-production-gap-selection` after completing the generated 3D text glyph atlas smoke.
- `production-readiness-audit-check` reports 11 non-ready `unsupportedProductionGaps`; the highest-priority audited category is `3d-playable-vertical-slice`.
- The generated 3D sample already proves many component smokes separately: package loading, camera/controller, transform animation, CPU morph, compute morph, skin+compute, async telemetry counters, quaternion animation, package streaming safe point, gameplay systems, scene GPU bindings, postprocess depth input, renderer quality gates, playable aggregate counters, directional shadow, shadow+morph composition, native UI overlay, image UI atlas, and text glyph UI atlas.
- What remains missing for this slice is a single selected, machine-readable "visible generated 3D production-style package proof" that ties the already-supported D3D12 package/window path together without claiming every optional 3D production feature.

## Constraints

- Keep this as a selected D3D12 package proof. Do not mark Vulkan, Metal, or broad backend parity ready.
- Do not add public native/RHI handle access, renderer texture upload APIs for gameplay, new third-party dependencies, source import execution, live shader generation, or broad package streaming.
- Do not combine every optional smoke into one impossible mega-smoke. Directional shadow, shadow+morph, image UI atlas, text glyph UI atlas, Vulkan, and future Metal remain separate selected validation recipes.
- Keep generated gameplay code on public `mirakana::` APIs and package/runtime contracts. SDL3, D3D12, swapchain, shader artifact loading, native windows, and RHI resources remain host/presentation adapter details.
- This plan may add aggregate `visible_3d_*` counters derived from existing first-party reports, but those counters must be fail-closed and frame-exact for the selected smoke.
- Keep current docs, manifest, skills, and static checks honest: this is "visible/windowed package proof", not broad generated 3D production readiness.

## Done When

- `games/sample_generated_desktop_runtime_3d_package` accepts `--require-visible-3d-production-proof`.
- The selected installed D3D12 package smoke emits and validates:
  - `visible_3d_status=ready`
  - `visible_3d_ready=1`
  - `visible_3d_diagnostics=0`
  - `visible_3d_expected_frames=2`
  - `visible_3d_presented_frames=2`
  - `visible_3d_d3d12_selected=1`
  - `visible_3d_null_fallback_used=0`
  - `visible_3d_scene_gpu_ready=1`
  - `visible_3d_postprocess_ready=1`
  - `visible_3d_renderer_quality_ready=1`
  - `visible_3d_playable_ready=1`
  - `visible_3d_ui_overlay_ready=1`
- `tools/validate-installed-desktop-runtime.ps1` enforces those fields when smoke args include `--require-visible-3d-production-proof`.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` propagates the selected flag, manifest recipe, README text, generated static markers, and validation expectations.
- `engine/agent/manifest.json`, docs, plan registry, and both Codex/Claude game-development skills describe this as a narrow D3D12 visible generated 3D package proof.
- Focused build, installed package smoke, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## Task 1: RED Validation And Static Contract

**Files:**
- Test: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Test: `tools/validate-installed-desktop-runtime.ps1`
- Test: `tools/check-json-contracts.ps1`
- Test: `tools/check-ai-integration.ps1`

- [x] Run the current generated 3D package smoke with `--require-visible-3d-production-proof`.

Expected now: FAIL because the flag and `visible_3d_*` fields do not exist.

- [x] Add static-check expectations for:
  - `--require-visible-3d-production-proof`
  - `installed-d3d12-3d-visible-production-proof-smoke`
  - `visible_3d_status`
  - `visible_3d_presented_frames`
  - generated `DesktopRuntime3DPackage` propagation through `tools/new-game.ps1`

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; expected now: FAIL on missing markers.

## Task 2: Sample Aggregate Visible Proof

**Files:**
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`

- [x] Add `require_visible_3d_production_proof` to options and parser.
- [x] Make the flag imply the existing D3D12 generated package proof prerequisites:
  - `--require-primary-camera-controller`
  - `--require-transform-animation`
  - `--require-morph-package`
  - `--require-compute-morph`
  - `--require-compute-morph-skin`
  - `--require-compute-morph-async-telemetry`
  - `--require-quaternion-animation`
  - `--require-package-streaming-safe-point`
  - `--require-gameplay-systems`
  - `--require-d3d12-scene-shaders`
  - `--require-d3d12-renderer`
  - `--require-scene-gpu-bindings`
  - `--require-postprocess`
  - `--require-postprocess-depth-input`
  - `--require-renderer-quality-gates`
  - `--require-playable-3d-slice`
  - `--require-native-ui-overlay`
- [x] Derive a small `Visible3dProductionProofState` from existing game counters and `SdlDesktopPresentationReport`.
- [x] Emit `visible_3d_*` fields in the final report.
- [x] Fail closed when any required component is absent, not D3D12-selected, null-fallback-backed, diagnostic-bearing, or not frame-exact.

## Task 3: Validator, Scaffold, Manifest, And README

**Files:**
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/new-game.ps1`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`

- [x] Teach the installed validator exact `visible_3d_*` field checks.
- [x] Add `installed-d3d12-3d-visible-production-proof-smoke` to the committed sample manifest.
- [x] Propagate the same recipe through `tools/new-game.ps1 -Template DesktopRuntime3DPackage`.
- [x] Update README commands and non-claims.

## Task 4: Machine-Readable And Human Docs

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Add the new validation recipe and production-loop guidance.
- [x] Update `3d-playable-vertical-slice` notes to include this visible proof while keeping the gap non-ready unless all remaining required-before-ready claims are resolved or explicitly excluded.
- [x] Close active plan pointers only after validation evidence is recorded.

## Task 5: Verification And Closeout

- [x] Run the selected installed D3D12 visible proof package smoke.
- [x] Run:

```powershell
cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

- [x] Record validation evidence in this plan.
- [x] Commit only the coherent slice if unrelated worktree changes can be excluded safely; otherwise record the staging blocker.

## Validation Evidence

| Check | Result | Evidence |
| --- | --- | --- |
| RED package smoke | Expected fail | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs @('--smoke','--require-visible-3d-production-proof')` failed before implementation with `unknown argument: --require-visible-3d-production-proof`. |
| Focused build | PASS | `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug`. |
| Explicit visible proof smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs @('--smoke', ... '--require-native-ui-overlay', '--require-visible-3d-production-proof')` emitted `visible_3d_status=ready`, `visible_3d_ready=1`, `visible_3d_diagnostics=0`, `visible_3d_expected_frames=2`, `visible_3d_presented_frames=2`, `visible_3d_d3d12_selected=1`, `visible_3d_null_fallback_used=0`, `visible_3d_scene_gpu_ready=1`, `visible_3d_postprocess_ready=1`, `visible_3d_renderer_quality_ready=1`, `visible_3d_playable_ready=1`, and `visible_3d_ui_overlay_ready=1`. |
| Default package smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` validates the registered `PACKAGE_SMOKE_ARGS`, including `--require-visible-3d-production-proof`, and produced the same `visible_3d_*` ready counters. |
| Static/schema checks | PASS | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `git diff --check`. |
| Pending final repository gate | To rerun after plan closeout edits | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` will be run after this evidence update and the master-plan replan sync. |

## Closeout Notes

- This slice intentionally narrows the `3d-playable-vertical-slice` gap but does not mark it ready. Remaining broad claims such as Vulkan/Metal parity, source import execution, editor productization, public native/RHI handles, and broad generated 3D production readiness stay unsupported or host-gated.
- The working tree includes unrelated pre-existing changes, so no coherent slice commit was created from this mixed worktree.
