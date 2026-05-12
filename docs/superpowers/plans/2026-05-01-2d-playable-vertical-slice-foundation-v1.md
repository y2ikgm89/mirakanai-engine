# 2D Playable Vertical Slice Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the first C-phase slice after the AI-operable command surface contract. Do not append this work to historical MVP or completed foundation plans.

**Goal:** Promote the planned 2D generated-game path toward a validated, AI-operable playable vertical slice using first-party C++23 engine contracts, without exposing SDL3, Dear ImGui, native OS handles, graphics API handles, or RHI backend handles to gameplay code.

**Architecture:** Keep gameplay on public `mirakana::` APIs. Use `mirakana_scene`, `mirakana_assets`, `mirakana_runtime`, `mirakana_renderer`, `mirakana_ui`, and runtime-host/package contracts where they already exist. Renderer/RHI execution, native presentation, shader artifacts, and package files stay behind engine adapters and validation recipes. Prefer clean recipe/manifest renames over preserving stale `future-*` naming once the implementation is proven.

**Tech Stack:** C++23, CMake targets/tests, existing first-party runtime/package formats, `engine/agent/manifest.json`, schema/static checks, docs, and `tools/*.ps1` validation commands.

---

## Goal

Build the smallest honest 2D playable path that an AI agent can select, implement, and validate:

- a machine-readable 2D recipe that is not named `future-*` once it is actually implemented
- deterministic source-tree validation for 2D gameplay, sprite/camera intent, HUD/menu model, and package assumptions
- a host-gated desktop runtime/package proof only when local validation recipes can prove visible behavior
- command-surface and unsupported-gap diagnostics that still reject 3D/editor/product renderer readiness claims

## Context

- `headless-gameplay`, `ai-navigation-headless`, and `runtime-ui-headless` are ready recipes.
- Desktop runtime package recipes are host-gated and already prove config/cooked-scene/material-shader package plumbing.
- `future-2d-playable-vertical-slice` and `future-3d-playable-vertical-slice` remain planned.
- Frame Graph, Upload/Staging, Runtime Resource, Asset Identity, and Scene Schema are foundation-only contracts, not production renderer/package streaming claims.
- AI Command Surface Foundation v1 makes command descriptors inspectable, but only `register-runtime-package-files` has a ready apply mode.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not parse PNG, glTF, audio, or source scene files in runtime gameplay.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, RHI backend handles, or tool process handles to gameplay-facing APIs.
- Do not mark 3D, editor productization, production render graph scheduling, native GPU upload execution, package streaming, atlas tooling, or tilemap editor UX as ready.
- If a recipe id is renamed from `future-2d-playable-vertical-slice`, update manifest, schemas/checks, generated-game docs, prompt pack, and handoff together.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A 2D generated-game recipe is represented honestly in `engine/agent/manifest.json`.
- Static checks reject stale `future-*` ready claims, unsupported 3D/editor claims, and docs that imply native handles or source asset runtime parsing.
- Source-tree and any selected package/visible validation recipes pass or record concrete host blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` when public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` when renderer/shader paths are touched, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Boundary Selection

**Files:**
- Read: `engine/agent/manifest.json`
- Read: `docs/ai-game-development.md`
- Read: `docs/specs/generated-game-validation-scenarios.md`
- Read: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Read: `engine/scene/include/mirakana/scene/scene.hpp`
- Read: `engine/ui/include/mirakana/ui/ui.hpp`
- Read: `games/`

- [x] Identify the smallest existing 2D sprite/camera/HUD/package path that can be validated without new dependencies.
- [x] Decide whether the first proof is source-tree only, desktop-runtime host-gated, or both.
- [x] Record the selected boundary before editing production code.

### Task 2: RED Checks And Tests

**Files:**
- Modify: focused tests under `tests/unit` or a sample game under `games/`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `engine/agent/manifest.json`

- [x] Add failing tests/checks for the selected 2D recipe id, required modules, validation recipes, and unsupported claims.
- [x] Add failing behavior proof for the selected 2D gameplay/render/HUD/package path.
- [x] Record RED evidence in this plan.

### Task 3: 2D Foundation Implementation

**Files:**
- Modify only the modules selected in Task 1.
- Add public headers only when the existing API surface cannot express the 2D proof cleanly.

- [x] Implement the minimum first-party 2D gameplay/render/HUD/package path.
- [x] Keep native presentation and backend handles behind existing runtime-host/renderer/RHI adapters.
- [x] Keep fallback diagnostics honest when the host lacks visible desktop validation.

### Task 4: Manifest, Docs, Checks, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `schemas/engine-agent.schema.json` if recipe shape changes
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` if top-level output needs new fields
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the validated 2D capability; keep 3D/editor/product renderer claims planned or host-gated.
- [x] Update docs and prompts so agents can select the 2D recipe without scraping prose.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

### Boundary Selection

- Selected proof boundary: source-tree first, with a deterministic headless 2D playable foundation.
- The proof will use public first-party `mirakana::` APIs only: `mirakana_core` runner/registry, `mirakana_runtime` input actions,
  `mirakana_scene` orthographic camera plus sprite nodes, `mirakana_scene_renderer` sprite packet submission, `mirakana_ui`/`mirakana_ui_renderer`
  HUD/menu renderer-intent payloads, `mirakana_audio` device-independent cue rendering, and `mirakana_renderer::NullRenderer`.
- Desktop runtime/window/package visibility remains host-gated and is not promoted in this slice.
- New production API surface is limited to backend-neutral `mirakana_scene` 2D scene validation so agents can reject stale
  3D/editor/native-handle claims before treating a generated scene as a 2D recipe candidate.

### RED Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed as expected after adding `mirakana_playable_2d_tests`: MSVC reported
  `fatal error C1083: cannot open include file: 'mirakana/scene/playable_2d.hpp': No such file or directory`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after tightening recipe checks:
  `engine manifest aiOperableProductionLoop missing recipe id: 2d-playable-source-tree`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected for the same missing `2d-playable-source-tree` recipe.

### GREEN Evidence

- Added `mirakana/scene/playable_2d.hpp` and backend-neutral `mirakana_scene` validation for the source-tree 2D recipe:
  primary orthographic camera, single-primary-camera diagnostics, visible sprite diagnostics, visible mesh rejection,
  invalid component/sprite diagnostics, and configurable missing primary/sprite requirements.
- Added `mirakana_playable_2d_tests` covering happy path, missing camera, visible 3D mesh rejection, multiple primary cameras,
  perspective primary camera diagnostics, missing visible sprite diagnostics, invalid sprite diagnostics, and desc toggles.
- Added `games/sample_2d_playable_foundation` as the source-tree proof for `2d-playable-source-tree`, composing
  `mirakana::runtime::RuntimeInputActionMap`, `mirakana::Scene`, `mirakana::validate_playable_2d_scene`, `mirakana_scene_renderer`,
  `mirakana_ui`/`mirakana_ui_renderer`, `mirakana_audio`, and `mirakana::NullRenderer` without native handles or desktop package claims.
- Updated `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`,
  generated-game docs, prompt pack, roadmap, handoff prompt, plan registry, and `games/sample_2d_playable_foundation/game.agent.json`
  so `2d-playable-source-tree` is ready and stale `future-2d-playable-vertical-slice` recipe claims are rejected.
- Review follow-up: added `multiple_primary_cameras`, installed `sample_2d_playable_foundation` with other runtime sample
  targets, and strengthened static unsupported-claim checks for `runtime image decoding`, `production sprite batching`,
  and `native GPU output`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS, 28/28 tests including `mirakana_playable_2d_tests` and `sample_2d_playable_foundation`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` PASS as diagnostic-only; D3D12 DXIL and Vulkan SPIR-V are ready, Metal tools remain missing.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS, 28/28 tests. Diagnostic-only blockers remain explicit: missing Metal tools, Apple packaging
  requires macOS/Xcode, Android release signing/device smoke not fully configured, and tidy compile database availability.

### Next Plan

- Created `docs/superpowers/plans/2026-05-01-2d-desktop-runtime-package-proof-v1.md`.
- Updated `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` and plan registry to make that the next active slice.
