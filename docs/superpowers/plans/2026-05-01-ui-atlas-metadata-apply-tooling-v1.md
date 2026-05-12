# UI Atlas Metadata Apply Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `ui-atlas-metadata-authoring-tooling-v1`. Do not append this work to the completed native overlay, textured sprite atlas, package metadata, or cooked metadata authoring plans.

**Goal:** Add a reviewed dry-run/apply tooling surface that updates cooked `.uiatlas` metadata and matching `.geindex` package rows together, using the validated `GameEngine.UiAtlas.v1` author/verify contracts without source image decoding or production atlas packing.

**Architecture:** Keep the canonical cooked metadata contract in `mirakana_assets` and deterministic author/verify logic in `mirakana_tools`. The apply surface may be a narrow repository tool or command descriptor that edits explicit package files, but it must validate before writing, preserve package hash/dependency consistency, and keep gameplay-facing APIs free of SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, RHI, or native handles.

**Tech Stack:** C++23, PowerShell 7 validation entrypoints under `tools/`, `mirakana_assets`, `mirakana_tools`, `engine/agent/manifest.json`, schema/static checks, sample desktop runtime package validation.

---

## Goal

Make the cooked UI atlas metadata authoring path usable by AI/tooling end to end:

- dry-run a `.uiatlas` and `.geindex` update from explicit texture page ids/URIs, image resource ids/asset URIs, UV rects, and colors
- apply the update only after validating duplicate identities, malformed UVs, missing or non-texture pages, content hashes, `AssetKind::ui_atlas`, and `ui_atlas_texture` dependency rows
- emit deterministic diagnostics and changed-file lists for command-surface consumers
- keep the sample package metadata-driven and keep D3D12/Vulkan selected package proof stable

## Context

- `ui-atlas-metadata-authoring-tooling-v1` added `mirakana::UiAtlasMetadataDocument`, deterministic `GameEngine.UiAtlas.v1` serialization/validation, and `mirakana::author_cooked_ui_atlas_metadata` / `mirakana::verify_cooked_ui_atlas_package_metadata`.
- The remaining AI-operable gap is a reviewed apply/update surface: agents still need a safe way to mutate `.uiatlas` and `.geindex` together rather than hand-editing multiple files.
- This slice is still cooked-metadata only. It is not a source image importer or atlas packer.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not decode PNG/JPEG or other source images at runtime or in production claims.
- Do not implement automatic atlas packing or image dimension probing.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, `IRhiDevice`, descriptor sets, shader modules, swapchains, or native GPU handles through gameplay-facing APIs.
- Do not make `mirakana_ui` depend on `mirakana_renderer`, `mirakana_rhi`, platform, SDL3, Dear ImGui, or backend APIs.
- Do not claim production text shaping, font rasterization, glyph atlases, IME, OS accessibility bridges, package streaming, material/shader graph, Metal readiness, editor productization, or general production renderer quality.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A deterministic dry-run/apply path exists for cooked `.uiatlas` plus matching package index/dependency rows.
- Static checks keep the apply surface narrow, reviewed, and limited to cooked metadata with unsupported decode/packing claims explicit.
- `sample_desktop_runtime_game` still validates D3D12 and strict Vulkan selected package lanes with `ui_atlas_metadata_status=ready` and existing textured overlay fields.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected D3D12/Vulkan package validation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Apply Contract Selection

**Files:**
- Read: `engine/assets/include/mirakana/assets/ui_atlas_metadata.hpp`
- Read: `engine/tools/include/mirakana/tools/ui_atlas_tool.hpp`
- Read: `engine/assets/include/mirakana/assets/asset_package.hpp`
- Read: `tools/register-runtime-package-files.ps1`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `games/sample_desktop_runtime_game/runtime/`
- Read: `engine/agent/manifest.json`

- [x] Decide whether the apply entrypoint belongs in a PowerShell tool, `mirakana_tools` helper, or AI command surface descriptor first.
- [x] Define dry-run and apply request/result shapes, including changed files, diagnostics, package hash updates, and dependency rows.
- [x] Define exact overwrite, duplicate, missing-page, non-texture-page, and package-external path policies.
- [x] Record which parts remain manual, planned, or blocked.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: focused tooling tests or PowerShell validation tests

- [x] Add failing checks for a reviewed dry-run/apply surface and docs that keep source decoding/packing unsupported.
- [x] Add failing tests for deterministic dry-run output and changed-file diagnostics.
- [x] Add failing tests for package hash/dependency updates and rejection of inconsistent `.geindex` rows.
- [x] Record RED evidence in this plan.

### Task 3: Apply Tooling Implementation

**Files:**
- Modify only the selected tooling modules from Task 1.

- [x] Implement deterministic dry-run for cooked UI atlas metadata/package index updates.
- [x] Implement apply only after validation passes.
- [x] Reuse `mirakana::author_cooked_ui_atlas_metadata` and `mirakana::verify_cooked_ui_atlas_package_metadata` rather than duplicating metadata rules.
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

- [x] Promote only the validated apply capability.
- [x] Update docs so agents know when to use dry-run/apply instead of hand-editing cooked UI atlas package rows.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed after adding the new static guard because `engine/agent/manifest.json` did not yet expose one ready `update-ui-atlas-metadata-package` command surface.
- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed with the same missing ready `update-ui-atlas-metadata-package` command surface.
- RED, 2026-05-01: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_tools_tests` failed after adding tests because `mirakana::CookedUiAtlasPackageUpdateDesc`, `mirakana::CookedUiAtlasPackageApplyDesc`, `mirakana::plan_cooked_ui_atlas_package_update`, and `mirakana::apply_cooked_ui_atlas_package_update` were not implemented yet.
- GREEN, 2026-05-01: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_tools_tests` passed after implementing the `mirakana_tools` plan/apply helpers.
- GREEN, 2026-05-01: `.\out\build\dev\Debug\mirakana_tools_tests.exe` passed, including `ui atlas apply tooling dry-runs atlas and package index changes`, `ui atlas apply tooling applies only after validation passes`, and `ui atlas apply tooling rejects unsafe paths and inconsistent page rows`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reported `aiOperableProductionLoop.currentActivePlan.path` as `docs/superpowers/plans/2026-05-01-ui-atlas-metadata-apply-tooling-v1.md`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with the narrow ready `update-ui-atlas-metadata-package` command surface.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with the same command-surface contract.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the public `mirakana_tools` header changes.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with D3D12 DXIL and Vulkan SPIR-V ready; Metal tools remain missing host gates.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed 14/14.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed for the D3D12 selected package with `ui_atlas_metadata_status=ready`, `ui_atlas_metadata_pages=1`, `ui_atlas_metadata_bindings=1`, `ui_texture_overlay_status=ready`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2`.
- GREEN, 2026-05-01: strict Vulkan selected package validation passed with `renderer=vulkan`, `framegraph_passes=3`, `ui_atlas_metadata_status=ready`, `ui_atlas_metadata_pages=1`, `ui_atlas_metadata_bindings=1`, and the same ready textured overlay fields.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 tests. Diagnostic-only host gates remain Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing/device smoke not fully configured, and tidy compile database availability.
- COMPLETED, 2026-05-01: this slice created `docs/superpowers/plans/2026-05-01-material-instance-apply-tooling-v1.md` as the next dated focused plan.
