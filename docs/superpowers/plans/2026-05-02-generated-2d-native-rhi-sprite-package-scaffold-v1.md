# Generated 2D Native RHI Sprite Package Scaffold v1 (2026-05-02)

## Goal

Extend the completed 2D native RHI sprite package proof from the committed sample into the generated `DesktopRuntime2DPackage` scaffold so new AI-created packaged 2D games can select the same host-gated native sprite package smoke without exposing native handles or broadening renderer claims.

## Context

- `docs/superpowers/plans/2026-05-02-2d-native-rhi-sprite-package-proof-v1.md` completed the committed `sample_2d_desktop_runtime_package` proof with `--require-native-2d-sprites` and D3D12 installed package evidence.
- `tools/new-game.ps1 -Template DesktopRuntime2DPackage` already generates cooked sprite/material/audio/orthographic scene payloads, runtime package files, runtime scene validation targets, atlas tilemap descriptors, package streaming residency descriptors, and static scaffold checks.
- The generated 2D scaffold currently remains a reusable package workflow proof and does not claim native GPU sprite output. This slice narrows that gap by emitting generated shader source, D3D12 package smoke args, shader artifact metadata, and manifest recipe rows for the existing host-owned native RHI sprite overlay path.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset format, editor, SDL3, Dear ImGui, and native handles.
- Keep gameplay public APIs free of native OS/GPU/RHI handles; the native sprite overlay remains host-owned under the desktop runtime presentation layer.
- Do not introduce third-party dependencies or new package managers.
- Do not claim production sprite batching, atlas packing, runtime image decoding, package streaming execution, Metal/iOS readiness, material/shader graphs, live shader generation, production text/font/IME/accessibility, public native/RHI handles, or broad renderer quality.
- Prefer existing desktop runtime package metadata, `PACKAGE_FILES_FROM_MANIFEST`, and validation recipe patterns.

## Done When

- [x] RED static checks fail because generated `DesktopRuntime2DPackage` does not yet emit native 2D sprite shader source, D3D12 package smoke args, shader artifact metadata, or manifest recipe rows.
- [x] `tools/new-game.ps1 -Template DesktopRuntime2DPackage` emits `shaders/runtime_2d_sprite.hlsl`, D3D12 package smoke args with `--require-native-2d-sprites`, `REQUIRES_D3D12_SHADERS`, and generated 2D sprite shader artifact registration.
- [x] Generated 2D manifests describe host-gated native 2D sprite output, include `installed-native-2d-sprite-smoke`, and keep unsupported production claims explicit.
- [x] `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` enforce the generated scaffold contract without relying on the committed sample alone.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` are synchronized.
- [x] Required validation has PASS evidence or concrete host/toolchain blockers recorded here.

## Implementation Tasks

- [x] Add RED static assertions for generated 2D shader source, generated CMake package smoke args, generated manifest native sprite recipe rows, and generated unsupported-claim text.
- [x] Add a reusable CMake helper for generated 2D sprite shader artifacts or equivalent target-scoped metadata that preserves the existing sample artifact names and install behavior.
- [x] Update `tools/new-game.ps1` to emit the shader source, package smoke args, `REQUIRES_D3D12_SHADERS`, helper call, manifest recipe, README validation text, and planned dry-run files for `DesktopRuntime2DPackage`.
- [x] Synchronize docs, manifest, and registry with the new generated-scaffold capability and remaining host gates.
- [x] Run focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Re-read the registry and manifest after completion to decide the next recommended focused slice or stop on host-gated/broad-only work.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | RED: failed before implementation with `Desktop runtime 2D package scaffold did not create shaders/runtime_2d_sprite.hlsl`. GREEN: `ai-integration-check: ok` after generated `DesktopRuntime2DPackage` emitted shader source, D3D12 native sprite smoke args, helper call, manifest recipe, and README/static scaffold evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | RED: failed before implementation with `tools/new-game.ps1 missing generated 2D native sprite scaffold contract: shaders/runtime_2d_sprite.hlsl`. GREEN: `json-contract-check: ok` after `tools/new-game.ps1` and `games/CMakeLists.txt` exposed the generated native sprite scaffold contract. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | PASS | Desktop runtime Debug lane configured, built, and ran 16 CTest entries with `100% tests passed, 0 tests failed out of 16`, confirming the new helper is valid during desktop runtime configure/build. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 28/28 dev CTest entries passed. Diagnostic-only host gates remain: Metal shader/library tools and Apple packaging are missing on this Windows host; strict clang-tidy reported missing compile database before configure and remained diagnostic-only. |

## Non-Goals

- Production sprite batching.
- Source image decoding or production atlas packing.
- Tilemap editor UX.
- Package streaming execution or renderer/RHI teardown execution.
- Public gameplay access to native OS, GPU, or RHI handles.
- Metal/iOS readiness.
- Broad renderer quality or production renderer readiness.
