# UI Atlas Metadata Authoring Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `native-ui-atlas-package-metadata`. Do not append this work to the completed native overlay, textured sprite atlas, package metadata proof, 2D, 3D, renderer/RHI, or desktop runtime plans.

**Goal:** Make first-party cooked UI atlas metadata authorable and verifiable by AI/tooling without adding runtime source image decoding, production atlas packing, text/font shaping, accessibility bridges, public native/RHI handles, or broad renderer quality claims.

**Architecture:** Keep `mirakana_ui` renderer/RHI/platform independent. Put authoring and validation logic behind tools, `mirakana_assets`, `mirakana_runtime`, or `mirakana_ui_renderer` adapters. The tooling may write deterministic `.uiatlas` package payloads and package-index records for explicit texture pages and UV rects, but it must not decode source images or pack atlases.

**Tech Stack:** C++23, PowerShell 7 validation entrypoints under `tools/`, `mirakana_assets`, `mirakana_runtime`, `mirakana_ui_renderer`, `sample_desktop_runtime_game`, manifest/static checks, desktop runtime package validation.

---

## Goal

Build the smallest truthful authoring step for UI atlas metadata:

- deterministic tool/test path that creates or verifies first-party `GameEngine.UiAtlas.v1` metadata from explicit cooked texture page ids, asset URIs, resource ids, and UV rects
- package index registration checks for `AssetKind::ui_atlas` records and `ui_atlas_texture` dependencies
- clear diagnostics for missing texture pages, non-texture page references, duplicate resources/asset URIs, malformed UVs, and unsupported source decoding or production packing claims
- sample/package smoke remains driven by metadata authored in package files, not C++ hardcoded atlas rows

## Context

- `native-ui-atlas-package-metadata` completed runtime consumption of package-authored UI atlas metadata.
- The next AI-operable gap is safe metadata authoring/update: agents should be able to add or update a cooked UI atlas mapping and package index entry without hand-editing multiple files inconsistently.
- This slice is still a cooked-metadata proof, not a source image pipeline.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not decode PNG/JPEG or other source images at runtime or in production claims.
- Do not implement automatic atlas packing.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, `IRhiDevice`, descriptor sets, shader modules, swapchains, or native GPU handles through gameplay-facing APIs.
- Do not make `mirakana_ui` depend on `mirakana_renderer`, `mirakana_rhi`, platform, SDL3, Dear ImGui, or backend APIs.
- Do not claim production text shaping, font rasterization, glyph atlases, IME, OS accessibility bridges, package streaming, material/shader graph, Metal readiness, editor productization, or general production renderer quality.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A deterministic authoring/verification path exists for cooked UI atlas metadata and package index/dependency rows.
- Static checks keep `native-ui-atlas-package-metadata` limited to cooked metadata and reject source decoding or production packing claims.
- `sample_desktop_runtime_game` still validates D3D12 and strict Vulkan selected package lanes with `ui_atlas_metadata_status=ready`, positive metadata page/binding counts, and existing textured overlay fields.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected D3D12/Vulkan package validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Authoring Contract Selection

**Files:**
- Read: `engine/assets/include/mirakana/assets/asset_package.hpp`
- Read: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Read: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Read: `tools/`
- Read: `games/sample_desktop_runtime_game/runtime/`
- Read: `engine/agent/manifest.json`

- [x] Decide whether the authoring entrypoint belongs in a PowerShell tool, `mirakana_tools`, or a narrow shared validator first.
  - Decision: put the canonical `GameEngine.UiAtlas.v1` cooked metadata document validation/serialization in `mirakana_assets`, then expose deterministic author/write/verify helpers from `mirakana_tools`. Do not add a PowerShell mutator or broaden AI command apply surfaces in this slice.
- [x] Define the deterministic authoring inputs: metadata asset id/name, page texture asset ids/URIs, resource ids, image asset URIs, UV rects, and color.
  - Inputs are metadata `AssetId`, output path, source revision, explicit cooked texture page `AssetId`/asset URI rows, explicit image `resource_id`/asset URI rows, normalized UV rects, and RGBA color values. `source.decoding` and `atlas.packing` must remain `unsupported`.
- [x] Define the exact package index/dependency update rules.
  - A cooked metadata artifact must register as `AssetKind::ui_atlas`, list every page texture in `entry.dependencies`, and carry one `ui_atlas_texture` dependency edge per page whose owner is the atlas metadata asset and whose path is the atlas metadata output path.
- [x] Record which parts remain manual, planned, or blocked.
  - This slice stays metadata-only: no runtime source PNG/JPEG decoding, no production atlas packing, no source image dimension probing, no pixel-rect packing policy, no public native/RHI handles, and no command-surface apply readiness beyond existing package-file registration.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: focused `mirakana_assets`, `mirakana_runtime`, `mirakana_ui_renderer`, or tooling tests

- [x] Add failing checks for an authoring/verification entrypoint and docs that keep source decoding/packing unsupported.
- [x] Add failing tests for deterministic `.uiatlas` serialization/validation or package index registration.
- [x] Add failing tests for duplicate/malformed/non-texture page diagnostics.
- [x] Record RED evidence in this plan.

### Task 3: Authoring Tooling Implementation

**Files:**
- Modify only the selected tooling/runtime/assets modules from Task 1.

- [x] Implement deterministic cooked UI atlas metadata authoring or verification.
- [x] Implement deterministic package index/dependency update or validation for authored metadata.
- [x] Preserve the existing runtime consumption path and native textured overlay package proof.
- [x] Keep source image decoding, production packing, text/font, IME, accessibility, native handles, and broad renderer claims unavailable.

### Task 4: Manifest, Docs, Checks, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` only if top-level output needs new fields
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the validated authoring/tooling capability.
- [x] Update docs so agents know how to author cooked metadata without claiming source image decoding or atlas packing.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED 2026-05-01: after adding static expectations first, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected with `engine/agent/manifest.json aiOperableProductionLoop authoring surface ui-atlas-metadata-authoring-tooling-v1 must be ready as a cooked-metadata-only mirakana_assets/mirakana_tools surface`.
- RED 2026-05-01: after adding matching JSON contract expectations, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected with `engine manifest aiOperableProductionLoop authoring surface ui-atlas-metadata-authoring-tooling-v1 must be ready as a cooked-metadata-only mirakana_assets/mirakana_tools surface`.
- RED 2026-05-01: after adding focused C++ tests first, `cmake --build --preset dev --target mirakana_tools_tests mirakana_asset_identity_runtime_resource_tests` through `tools/common.ps1` failed as expected with missing `mirakana/tools/ui_atlas_tool.hpp`.
- GREEN 2026-05-01: focused C++ build through `tools/common.ps1` passed for `mirakana_tools_tests`, `mirakana_asset_identity_runtime_resource_tests`, `mirakana_runtime_tests`, and `mirakana_ui_renderer_tests`; the four focused executables then passed.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after manifest/static-check synchronization.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reported `aiOperableProductionLoop.currentActivePlan` as this plan during implementation.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after adding public `mirakana_assets` and `mirakana_tools` headers.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` completed diagnostic-only with D3D12 DXIL and Vulkan SPIR-V ready, and Metal `metal`/`metallib` still missing as an existing host gate.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed with 14/14 tests.
- GREEN 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed for the D3D12 selected package with `ui_atlas_metadata_status=ready`, `ui_atlas_metadata_pages=1`, `ui_atlas_metadata_bindings=1`, `ui_texture_overlay_status=ready`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`.
- GREEN 2026-05-01: strict Vulkan selected package validation passed with `--require-native-ui-overlay --require-native-ui-textured-sprite-atlas`, `renderer=vulkan`, `framegraph_passes=3`, and the same ready metadata/textured overlay fields.
- GREEN 2026-05-01: final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after this plan was marked complete and the next focused plan was created.
