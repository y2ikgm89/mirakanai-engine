# 2D Native RHI Sprite Package Proof v1 (2026-05-02)

## Goal

Move `sample_2d_desktop_runtime_package` one focused step closer to a shippable 2D desktop runtime package by proving that its cooked 2D scene sprite, sprite material/texture, and HUD submission can render through the host-owned native RHI game-window path.

This slice selects Windows D3D12 as the primary validation lane. Vulkan remains a strict host/toolchain-gated lane using the same public runtime package and renderer contracts.

## Context

- `docs/superpowers/plans/2026-05-02-desktop-runtime-shippable-rhi-window-v1.md` is complete and already has PASS evidence for D3D12/Vulkan RHI-backed game-window presentation smoke.
- `sample_2d_desktop_runtime_package` already ships cooked config, `.geindex`, 2D scene, sprite texture, sprite material, audio, tilemap, runtime package metadata, D3D12/Vulkan shader artifact metadata, and package validation lanes.
- The current package smoke proves host-owned RHI-backed presentation selection but does not prove that packaged 2D sprite texture/material data feeds a native 2D RHI draw path.
- Existing contracts to reuse: `mirakana_runtime_host`, `mirakana_runtime_host_sdl3`, `mirakana_runtime_host_sdl3_presentation`, `mirakana_scene_renderer`, `mirakana_runtime_scene`, `mirakana_runtime_scene_rhi`, `mirakana_ui_renderer`, `mirakana_renderer`, desktop-runtime package metadata, `runtimePackageFiles`, `runtimeSceneValidationTargets`, and `desktop-runtime-games.json`.

## Constraints

- Keep `engine/core` independent from OS, GPU, editor, asset-format, and UI middleware details.
- Do not expose native OS handles, GPU handles, RHI handles, or backend objects through gameplay public APIs.
- Do not introduce new external dependencies.
- Do not claim production sprite batching, atlas packing, runtime image decoding, production text/font/IME/accessibility, broad package cooking, broad/background streaming, Metal/iOS, material graph, shader graph, live shader generation, or broad renderer quality readiness.
- Preserve the C++23/CMake/public `mirakana::` style and existing dependency direction.

## Done When

- [x] The completed desktop runtime RHI window plan is reflected as completed in the plan registry, roadmap/current-capabilities, and engine manifest surfaces.
- [x] `sample_2d_desktop_runtime_package` package smoke can require native 2D sprite rendering through host-owned D3D12 RHI without exposing native/RHI handles to gameplay code.
- [x] The host-owned RHI renderer consumes packaged 2D scene sprite texture/material identity through existing scene/runtime package contracts and records native 2D sprite/HUD diagnostics in the package smoke status line.
- [x] Vulkan remains available only through the existing strict host/toolchain-gated package lane.
- [x] RED -> GREEN checks cover the new package smoke flag, shader artifacts, status diagnostics, renderer sprite texture submission, and installed package validation.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.json`, and schema/static checks are synchronized.
- [x] Required validation has PASS evidence or records concrete host/toolchain blockers with the failing command.

## Implementation Tasks

- [x] Add RED static checks and unit tests for the native 2D sprite package proof contract before implementation.
- [x] Make scene sprite submission preserve packaged sprite texture identity in `mirakana_scene_renderer` without exposing native/RHI handles.
- [x] Add a host-owned native 2D sprite overlay path to the simple RHI frame renderer using existing renderer/RHI/UI overlay primitives.
- [x] Wire D3D12 and strict-gated Vulkan simple presentation descriptors to upload the cooked sprite texture and render scene/HUD sprites through the native overlay path.
- [x] Update `sample_2d_desktop_runtime_package` CLI, status line, package smoke metadata, shader artifacts, and README for `--require-native-2d-sprites`.
- [x] Update installed package validation to require native 2D sprite diagnostics when package metadata requests it.
- [x] Synchronize docs, manifest, and plan registry.
- [x] Run required validation and update Validation Evidence.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | RED: failed before implementation because `games/sample_2d_desktop_runtime_package/game.agent.json` did not yet expose `native 2D sprite package proof`; GREEN: `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`; generated-game dry-run scaffolds still pass for Headless and desktop package templates, including `DesktopRuntime2DPackage`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` | PASS | Generated machine-readable context includes `currentActivePlan=docs/superpowers/plans/2026-05-02-2d-native-rhi-sprite-package-proof-v1.md`, the updated 2D package recipe notes, and `post-2d-native-rhi-sprite-package-review` as the recommended next plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | PASS | Debug desktop-runtime lane configured/built and ran 16 CTest entries with `100% tests passed, 0 tests failed out of 16`. |
| `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` | PASS | Initial GREEN attempt exposed the separate-swapchain-pass bug with `sample_2d_desktop_runtime_package.exe ... --require-native-2d-sprites` exiting `-1073740791`; after moving native overlay draws into the active pass with upload-capable vertex buffers, installed validation passed with D3D12 `native_2d_sprites_status=ready`, `native_2d_sprites_submitted=6`, `native_2d_textured_sprites_submitted=3`, `native_2d_texture_binds=3`, `native_2d_draws=3`, `native_2d_textured_draws=3`, and `desktop-runtime-package: ok (sample_2d_desktop_runtime_package)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 28/28 dev CTest entries passed. Diagnostic-only host gates remain: Metal shader/library tools and Apple packaging are missing on this Windows host; strict clang-tidy reported missing compile database before configure and remained diagnostic-only. |

## Non-Goals

- Production sprite batching or atlas packing.
- Runtime image decoding beyond the existing cooked texture package path.
- Production text/font/IME/accessibility.
- Broad renderer quality readiness.
- Public gameplay access to native OS, GPU, or RHI handles.
- Metal/iOS readiness.
