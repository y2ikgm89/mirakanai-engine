# Game Spec Template

## Goal

Describe the game in one sentence.

## Core Loop

Describe what the player does repeatedly.

## Current Engine Constraints

- Runtime: headless by default; use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name> -Template DesktopRuntimePackage` when a new game should start with an SDL3 desktop runtime package scaffold, `runtime/<game_name>.config`, manifest `runtimePackageFiles`, and `PACKAGE_FILES_FROM_MANIFEST` registration. Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name> -Template DesktopRuntimeCookedScenePackage` when the scaffold should also include a deterministic first-party cooked scene package, `.geindex`, renderer-neutral scene submission smoke, and manifest-derived package files without shader/GPU claims. The optional `desktop-game-runtime` lane can open an SDL3 desktop window through reusable `SdlDesktopGameHost` plus host-owned `SdlDesktopPresentation` with build-output D3D12 DXIL, build-output Vulkan SPIR-V on ready hosts, and deterministic `NullRenderer` fallback; the `desktop-runtime-release` lane defaults to the validated sample shell through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` and can package a registered desktop runtime game target with `tools/package-desktop-runtime.ps1 -GameTarget <target>` when the target declares `GAME_MANIFEST`, package smoke metadata, and bundled runtime config/assets in `runtimePackageFiles` through `PACKAGE_FILES_FROM_MANIFEST` or intentionally mirrored CMake `PACKAGE_FILES`; mobile lifecycle/touch/orientation/safe-area/storage/permission contracts exist, while generated shader artifacts, scene GPU binding, Metal game-window presentation, and host-validated mobile package builds are toolchain-gated or follow-up work.
- API: public `mirakana::` C++ headers only.
- Dependencies: no new third-party dependencies without license review.
- Backend readiness: select only entries from `engine/agent/manifest.json` `runtimeBackendReadiness` whose status is validated or optional for the intended host.
- Asset import: runtime code consumes cooked artifacts only; external PNG, glTF, and common audio source files are imported only through optional `mirakana_tools` adapters behind the `asset-importers` feature.
- Packaging: list only supported `packagingTargets` from the engine manifest; Android/iOS targets are template-toolchain-gated and require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` plus the relevant SDK build script on a configured host.

## Entities and Components

List the entities and their components.

## Win/Lose or Completion Condition

Describe how the game ends or how progress is measured.

## Validation

State what executable output, test, or behavior proves the game works.

Mirror that proof in `game.agent.json` `validationRecipes`.
