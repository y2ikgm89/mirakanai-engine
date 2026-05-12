# Native UI Atlas Package Metadata Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `native-ui-textured-sprite-atlas`. Do not append this work to the completed native overlay, textured sprite atlas proof, 2D, 3D, renderer/RHI, or desktop runtime plans.

**Goal:** Move the validated native UI textured sprite proof from sample-hardcoded atlas page/UV binding to a first-party cooked package metadata contract that an AI agent can inspect, update, package, and validate without adding runtime source image decoding, production atlas packing, text/font shaping, accessibility bridges, or public native/RHI handles.

**Architecture:** Keep `mirakana_ui` renderer/RHI/platform independent. Keep gameplay-facing UI image references as stable resource ids or asset URIs. Add any atlas metadata parsing and binding construction behind `mirakana_ui_renderer`, `mirakana_runtime`, `mirakana_runtime_host_sdl3`, or a narrow tools/runtime package adapter so renderer backends receive only backend-neutral sprite texture regions and private RHI texture/sampler ownership.

**Tech Stack:** C++23, `mirakana_assets`, `mirakana_runtime`, `mirakana_ui`, `mirakana_ui_renderer`, `mirakana_renderer`, `mirakana_runtime_host_sdl3_presentation`, `sample_desktop_runtime_game`, manifest/static checks, desktop runtime package validation.

---

## Goal

Build the smallest truthful package metadata step for runtime UI image sprites:

- first-party cooked UI atlas metadata carried in the selected desktop runtime package
- deterministic mapping from UI image `resource_id` or `asset_uri` to atlas page and UV rect
- sample smoke fields proving metadata was loaded and used before the native textured overlay draw
- D3D12 selected package proof and strict Vulkan selected package proof on ready hosts
- honest diagnostics when metadata is missing, malformed, references a non-texture asset, or requests unsupported source decoding/packing behavior

## Context

- `native-ui-textured-sprite-atlas` validates renderer-owned sampled UI sprite drawing on D3D12 and strict Vulkan selected package lanes.
- That proof intentionally uses a narrow sample binding for one cooked texture page and does not make package-authored atlas metadata, source image decoding, or production atlas packing ready.
- The next useful step is a package/runtime metadata contract that removes hardcoded sample atlas bindings while preserving the existing public `mirakana_ui` and `mirakana_renderer` boundaries.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not decode PNG/JPEG or other source images at runtime.
- Do not implement production atlas packing.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, `IRhiDevice`, descriptor sets, shader modules, swapchains, or native GPU handles through gameplay-facing APIs.
- Do not make `mirakana_ui` depend on `mirakana_renderer`, `mirakana_rhi`, platform, SDL3, Dear ImGui, or backend APIs.
- Do not claim production font/text shaping, bidirectional text, glyph atlases, image decoding, automatic atlas packing, IME, OS accessibility bridges, material graph/shader graph, package streaming, skeletal animation, GPU skinning, Metal readiness, editor productization, or general production renderer quality.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- Package atlas metadata is represented by first-party deterministic data, not sample-only hardcoded constants.
- Static checks reject stale claims that source image decoding, production atlas packing, text/font/accessibility, public native handles, or general production UI renderer quality are ready.
- `sample_desktop_runtime_game` installed package validation proves the metadata-loaded UI image sprite path for D3D12 and strict Vulkan where the host is ready.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected D3D12/Vulkan package validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Metadata Contract Selection

**Files:**
- Read: `engine/assets/include/mirakana/assets/asset_package.hpp`
- Read: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Read: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Read: `engine/runtime_host/sdl3/`
- Read: `games/sample_desktop_runtime_game/`
- Read: `engine/agent/manifest.json`

- [x] Decide whether atlas metadata belongs in a package record payload, a sidecar first-party metadata file, or a narrow runtime adapter.
  - Decision: use a first-party cooked package record payload with `AssetKind::ui_atlas`, parsed by `mirakana_runtime`, then adapted into `UiRendererImagePalette` by `mirakana_ui_renderer`.
- [x] Decide the capability/recipe id before editing production code.
  - Decision: use `native-ui-atlas-package-metadata` as the recipe/capability id.
- [x] Identify the minimum sample status fields for metadata load/use diagnostics.
  - Decision: add `ui_atlas_metadata_status`, `ui_atlas_metadata_pages`, and `ui_atlas_metadata_bindings` beside the existing `ui_texture_overlay_*` fields.
- [x] Record whether this remains host-gated desktop-package-only.
  - Decision: this remains host-gated to the selected desktop runtime package lanes; source image decoding, production packing, Metal, and broader renderer quality remain planned or host-gated.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: focused runtime/UI renderer tests or `games/sample_desktop_runtime_game`

- [x] Add failing static checks for the selected atlas metadata capability id, validation recipes, unsupported claims, and sample status fields.
- [x] Add a failing behavior proof that UI image bindings can be built from package metadata instead of hardcoded sample constants.
- [x] Add malformed/missing metadata diagnostics for deterministic failure paths.
- [x] Record RED evidence in this plan with exact failing commands and diagnostic lines.

### Task 3: Package Metadata Implementation

**Files:**
- Modify only the metadata/runtime-host/sample modules selected in Task 1.
- Add public headers only if existing public `mirakana::` APIs cannot express the selected proof cleanly.

- [x] Implement deterministic atlas metadata parsing or package payload lookup.
- [x] Build `UiRendererImagePalette` / `UiRendererImageBinding` rows from package metadata.
- [x] Preserve the validated native textured overlay draw path and colored placeholder fallback behavior.
- [x] Keep backend resources, descriptors, command lists, swapchains, and native handles private.
- [x] Keep source image decoding, atlas packing, text/font, IME, and accessibility bridge work unavailable.

### Task 4: Manifest, Docs, Checks, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `schemas/engine-agent.schema.json` only if capability shape changes
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` if top-level output needs new fields
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the validated package atlas metadata capability.
- [x] Update docs and prompts so agents can distinguish hardcoded atlas proof from package-authored atlas metadata readiness.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED static check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation with `engine/agent/manifest.json aiOperableProductionLoop missing recipe id: native-ui-atlas-package-metadata`.
- RED schema check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed before implementation with `engine manifest aiOperableProductionLoop missing recipe id: native-ui-atlas-package-metadata`.
- RED focused C++ build attempt: `cmake --build --preset dev --target mirakana_runtime_tests mirakana_ui_renderer_tests` could not reach compile RED because this PowerShell environment does not have `cmake` on `PATH`: `The term 'cmake' is not recognized as a name of a cmdlet, function, script file, or executable program.`
- GREEN agent context: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and confirmed `aiOperableProductionLoop.currentActivePlan=docs/superpowers/plans/2026-05-01-native-ui-atlas-package-metadata-foundation-v1.md` before implementation.
- GREEN schema/static contract: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok`.
- GREEN agent contract: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok`.
- GREEN API boundary: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed with `public-api-boundary-check: ok`.
- GREEN shader toolchain diagnostics: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with D3D12 DXIL, Vulkan SPIR-V, DXC SPIR-V CodeGen, and `spirv-val` ready; Metal `metal` and `metallib` remain missing host/toolchain blockers.
- GREEN desktop runtime: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed with 14/14 desktop runtime tests.
- GREEN D3D12 selected package: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed with `desktop-runtime-package: ok (sample_desktop_runtime_game)`, `renderer=d3d12`, `ui_atlas_metadata_status=ready`, `ui_atlas_metadata_pages=1`, `ui_atlas_metadata_bindings=1`, `ui_texture_overlay_status=ready`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`.
- GREEN strict Vulkan selected package: the first `-File ... -SmokeArgs @(...)` attempt failed because PowerShell does not pass array literals through `-File`; rerunning with `-Command "& .\tools\package-desktop-runtime.ps1 ... -SmokeArgs @(...)"` passed with `desktop-runtime-package: ok (sample_desktop_runtime_game)`, `renderer=vulkan`, `framegraph_passes=3`, `ui_atlas_metadata_status=ready`, `ui_atlas_metadata_pages=1`, `ui_atlas_metadata_bindings=1`, and the same ready textured overlay fields.
- GREEN default validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and 28/28 CTest tests passed. Diagnostic-only blockers remained Metal tools, Apple packaging on non-macOS/Xcode hosts, Android release signing/device smoke configuration, and strict clang-tidy compile database availability.
- GREEN next-plan handoff: created `docs/superpowers/plans/2026-05-01-ui-atlas-metadata-authoring-tooling-v1.md`, updated the plan registry, roadmap, handoff prompt, and `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
- GREEN post-handoff agent context: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and emitted `currentActivePlan=docs/superpowers/plans/2026-05-01-ui-atlas-metadata-authoring-tooling-v1.md`.
- GREEN post-handoff schema/static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok`.
- GREEN post-handoff default validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and 28/28 CTest tests passed after the plan registry, roadmap, handoff prompt, and manifest active-plan switch.

## Completion

Completed on 2026-05-01 as the host-gated `native-ui-atlas-package-metadata` proof. The package path now carries first-party cooked `ui_atlas` metadata, `mirakana_runtime` parses `RuntimeUiAtlasPayload`, `mirakana_ui_renderer` builds `UiRendererImagePalette` / `UiRendererImageBinding` rows from package metadata, and `sample_desktop_runtime_game` reports metadata load/use fields before the existing native textured overlay draw. Runtime source PNG/JPEG decoding, production atlas packing, text shaping, font rasterization, glyph atlases, IME, OS accessibility bridges, Metal readiness, public native/RHI handles, and general production UI renderer quality remain planned or host-gated.
