# 2D Desktop Runtime Package Proof v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after the source-tree `2d-playable-source-tree` recipe. Do not append this work to completed MVP or foundation plans.

**Goal:** Promote the validated source-tree 2D playable foundation into an honest host-gated desktop runtime/package proof, while keeping gameplay on first-party `mirakana::` APIs and native presentation behind engine host adapters.

**Architecture:** Reuse `mirakana::GameApp`, `mirakana_runtime_host`, `mirakana_runtime_host_sdl3`, `mirakana_runtime_host_sdl3_presentation`, `mirakana_scene`, `mirakana_scene_renderer`, `mirakana_ui`, `mirakana_ui_renderer`, `mirakana_audio`, and manifest-driven desktop package metadata. SDL3, Win32, D3D12, Vulkan, Metal, RHI devices, shader compilers, and package installation details stay in host/tool adapters, not gameplay code.

**Tech Stack:** C++23, CMake desktop runtime target metadata, existing first-party runtime/package files, `engine/agent/manifest.json`, schema/static checks, docs, and `tools/*.ps1` validation commands.

---

## Goal

Build the smallest visible/package 2D proof that can be validated without overstating renderer production readiness:

- a host-gated 2D desktop runtime package recipe or package proof status
- a registered sample game that composes the source-tree 2D gameplay path with the reusable SDL3 desktop game host
- manifest-derived package files for the sample, with package validation that proves installed config/assets are present
- deterministic fallback diagnostics when native D3D12/Vulkan/Metal presentation or shader artifacts are unavailable
- checks that still reject texture atlas, tilemap editor UX, runtime image decoding, production sprite batching, package streaming, 3D, editor productization, and public native/RHI handle claims

## Context

- `2d-playable-source-tree` is ready for source-tree validation only.
- Desktop runtime config/cooked-scene/material-shader package lanes are already host-gated.
- `sample_2d_playable_foundation` proves input, orthographic scene camera, visible sprite validation, HUD submission, audio cue rendering, and `NullRenderer` counters without native window/package claims.
- `sample_desktop_runtime_game` proves desktop runtime packaging and host-owned scene GPU binding for a mesh/material path, not a 2D sprite/HUD/audio game path.
- AI command surfaces remain descriptor-only except `register-runtime-package-files`.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not parse PNG, glTF, audio, or source scene files in runtime gameplay.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, RHI backend handles, or shader tool process handles to gameplay-facing APIs.
- Do not mark texture atlas cooking, tilemap editor UX, runtime image decoding, production sprite batching, package streaming, 3D playable, editor productization, or production renderer readiness as complete.
- Keep host-gated validation honest: visible package readiness requires the selected desktop runtime/package commands to pass on the current host, otherwise record concrete blockers.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A 2D desktop runtime/package proof is represented honestly in `engine/agent/manifest.json` as host-gated or ready only to the level validated.
- Static checks reject stale claims that the source-tree 2D recipe implies desktop package, atlas/tilemap, runtime image decoding, production renderer, or native handle readiness.
- Source-tree and selected desktop runtime/package validation recipes pass, or concrete host/toolchain blockers are recorded.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` if renderer/shader paths are touched, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Boundary Selection

**Files:**
- Read: `games/sample_2d_playable_foundation/`
- Read: `games/sample_desktop_runtime_game/`
- Read: `games/CMakeLists.txt`
- Read: `engine/runtime_host*/`
- Read: `engine/agent/manifest.json`
- Read: `tools/package-desktop-runtime.ps1`

- [x] Decide whether to add a new registered 2D desktop runtime sample, extend the source-tree sample into a desktop-runtime target, or add a package proof wrapper.
- [x] Identify the minimum runtime package files needed for the 2D sample without source asset parsing.
- [x] Record whether the proof is host-gated only or can be promoted to a ready recipe on the current host.

### Task 2: RED Checks And Tests

**Files:**
- Modify: focused tests under `tests/unit` or a registered sample under `games/`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `engine/agent/manifest.json`

- [x] Add failing checks for the selected 2D desktop/package recipe or host-gated proof id.
- [x] Add failing package/runtime proof for installed config/assets and finite desktop runtime smoke behavior.
- [x] Add stale-claim checks that reject atlas/tilemap/runtime-image/production-renderer/native-handle readiness.
- [x] Record RED evidence in this plan.

### Task 3: Desktop Runtime Package Proof

**Files:**
- Modify only the modules and samples selected in Task 1.

- [x] Implement the minimum 2D desktop runtime host/package sample.
- [x] Keep gameplay on `mirakana::GameApp`, `mirakana_scene`, `mirakana_ui`, `mirakana_audio`, and public renderer contracts.
- [x] Keep native presentation and backend handles behind existing runtime-host/renderer/RHI adapters.
- [x] Preserve deterministic `NullRenderer` fallback diagnostics when native presentation is unavailable.

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

- [x] Promote only the validated visible/package 2D capability; keep atlas/tilemap/runtime-image/production-renderer/3D/editor claims planned or host-gated.
- [x] Update docs and prompts so agents can distinguish source-tree 2D from desktop package 2D.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- Boundary selected: add a new registered `games/sample_2d_desktop_runtime_package` target instead of extending `sample_2d_playable_foundation`. The source-tree sample remains the ready headless `2d-playable-source-tree` proof; the new sample owns the optional SDL3 desktop runtime/package proof and remains `host-gated`.
- Minimum package files selected: runtime config, `.geindex`, first-party cooked texture, material, audio, and `GameEngine.Scene.v1` sprite scene payloads. Gameplay will load cooked runtime records through `mirakana_runtime` / `mirakana_runtime_scene`, not parse PNG, external audio, or source scene files.
- RED 2026-05-01: after adding static expectations first, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected with `engine/agent/manifest.json aiOperableProductionLoop missing recipe id: 2d-desktop-runtime-package`.
- RED 2026-05-01: after adding static expectations first, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected with `engine manifest aiOperableProductionLoop missing recipe id: 2d-desktop-runtime-package`.
- GREEN 2026-05-01: added `games/sample_2d_desktop_runtime_package` with a manifest-derived registered desktop-runtime target, runtime config, `.geindex`, cooked 2D scene, cooked sprite texture/material, cooked audio payload, finite smoke args, and installed package smoke args.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding the host-gated `2d-desktop-runtime-package` recipe, validation recipe mapping, runtime package file checks, and stale-claim checks.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after synchronizing `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, sample manifests, docs, and generated-game guidance.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` initially failed on sample compile/audio-smoke issues, then passed with 14/14 tests after changing the sample to pass `std::span<const mirakana::AudioClipSampleData>` and use deterministic `action_down`-gated cooked audio playback.
- GREEN 2026-05-01: `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` first hit the known sandbox-only vcpkg 7zip extraction blocker, then passed outside the sandbox. Installed smoke reported `frames=3`, `final_x=3`, `scene_sprites=3`, `hud_boxes=3`, `audio_commands=1`, `audio_underruns=0`, `package_records=4`, `package_scene_sprites=1`, and deterministic `NullRenderer` fallback through `presentation_selected=null`.
- GREEN 2026-05-01: created `docs/superpowers/plans/2026-05-01-3d-playable-vertical-slice-foundation-v1.md`, updated `docs/superpowers/plans/README.md`, and moved `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to that next C-phase slice.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and emitted the updated production loop with `currentActivePlan=docs/superpowers/plans/2026-05-01-3d-playable-vertical-slice-foundation-v1.md`.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with D3D12 DXIL ready, Vulkan SPIR-V ready, and Metal `metal`/`metallib` missing as an Apple-host-gated blocker.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 CTest tests. Diagnostic-only blockers remained Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing/device smoke not fully configured, and tidy compile database availability.
