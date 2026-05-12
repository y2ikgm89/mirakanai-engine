# Material Instance Apply Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `ui-atlas-metadata-apply-tooling-v1`. Do not append this work to completed material, package, UI atlas, generated desktop package, or editor authoring plans.

**Goal:** Add a reviewed dry-run/apply tooling surface for first-party material instance authoring that can create or update explicit `.material` content and matching cooked `.geindex` `AssetKind::material` rows with texture dependency edges, without claiming material graphs, shader graphs, live shader generation, renderer residency, or package streaming.

**Architecture:** Keep material data contracts in `mirakana_assets` and deterministic mutation helpers in `mirakana_tools`. The command surface should promote the existing `create-material-instance` descriptor only after tests prove dry-run diagnostics, validated writes, package row hashing, texture dependency rows, and unsupported-claim rejection. Gameplay-facing APIs must stay free of SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, RHI, descriptor, shader module, swapchain, or native handles.

**Tech Stack:** C++23, `mirakana_assets`, `mirakana_tools`, PowerShell 7 validation entrypoints under `tools/`, `engine/agent/manifest.json`, schema/static checks, desktop runtime sample package validation.

---

## Goal

Make material instance edits AI-operable through a narrow reviewed path:

- dry-run a material instance `.material` payload from an explicit base material id, material instance id/name, factor overrides, texture overrides, output path, and package index path
- validate stable asset ids, finite material factors, supported texture slots, duplicate texture slots, missing base material rows, missing or non-texture texture dependencies, package-external paths, package hash updates, and dependency rows before writing
- apply the `.material` and `.geindex` updates only after the same dry-run validation succeeds
- emit deterministic diagnostics and changed-file lists for command-surface consumers
- keep source texture decoding, material graph/shader graph, live shader generation, renderer/RHI residency, package streaming, and editor productization unavailable

## Context

- `create-material-instance` is currently descriptor-only in `engine/agent/manifest.json`.
- `mirakana_assets` already owns `MaterialDefinition`, `MaterialInstanceDefinition`, serialization, validation, composition, and texture dependency data.
- The UI atlas apply tooling slice established the preferred pattern: C++ `mirakana_tools` dry-run/apply helpers, manifest/static-check promotion only after RED/GREEN tests, and selected package validation to prove existing runtime lanes are unchanged.
- This slice is material metadata/package-row authoring only. It is not a shader compiler, material graph, shader graph, runtime source parser, renderer residency system, or editor product feature.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not decode PNG/JPEG or other source images.
- Do not implement material graphs, shader graphs, live shader generation, shader hot reload, shader compilation execution, or runtime shader authoring.
- Do not implement renderer/RHI residency, package streaming, GPU upload ownership, descriptor allocation, or native pipeline creation.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, `IRhiDevice`, descriptor sets, shader modules, swapchains, or native GPU handles through gameplay-facing APIs.
- Do not make `mirakana_assets` or `mirakana_ui` depend on renderer, RHI, platform, SDL3, Dear ImGui, or backend APIs.
- Keep existing `sample_desktop_runtime_game` and generated material/shader package proofs metadata-driven and host-gated.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A deterministic dry-run/apply path exists for material instance `.material` plus matching package index/dependency rows.
- Static checks keep the promoted command surface narrow, reviewed, and limited to material metadata/package rows with unsupported graph/shader/runtime claims explicit.
- Existing desktop runtime and selected package validations still pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected package validation where relevant, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record blockers.

## Implementation Tasks

### Task 1: Inventory And Contract Selection

**Files:**
- Read: `engine/assets/include/mirakana/assets/material.hpp`
- Read: `engine/assets/include/mirakana/assets/asset_package.hpp`
- Read: `engine/tools/include/mirakana/tools/ui_atlas_tool.hpp`
- Read: `engine/tools/src/ui_atlas_tool.cpp`
- Read: `tests/unit/tools_tests.cpp`
- Read: `engine/agent/manifest.json`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `games/sample-generated-desktop-runtime-material-shader-package/`
- Read: `games/sample_desktop_runtime_game/runtime/`

- [x] Decide whether to add a new `mirakana_tools` material helper or reuse an existing tooling module.
- [x] Define dry-run and apply request/result shapes, including changed files, diagnostics, package hash updates, texture dependency rows, and unsupported claims.
- [x] Define exact overwrite, duplicate texture slot, missing base material, missing/non-texture texture dependency, and package-external path policies.
- [x] Record which parts remain manual, planned, blocked, or host-gated.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: focused tooling tests

- [x] Add failing checks for a reviewed `create-material-instance` dry-run/apply surface and docs that keep material/shader graph and live shader generation unsupported.
- [x] Add failing tests for deterministic dry-run output and changed-file diagnostics.
- [x] Add failing tests for package hash/dependency updates and rejection of inconsistent `.geindex` rows.
- [x] Add failing tests for duplicate texture slots, missing base material rows, missing texture rows, non-texture texture rows, unsafe paths, and unsupported graph/shader/runtime claims.
- [x] Record RED evidence in this plan.

### Task 3: Apply Tooling Implementation

**Files:**
- Modify only the selected tooling modules from Task 1.

- [x] Implement deterministic dry-run for material instance `.material` and package index updates.
- [x] Implement apply only after validation passes.
- [x] Reuse `mirakana::serialize_material_instance_definition`, `mirakana::deserialize_material_instance_definition`, `mirakana::is_valid_material_instance_definition`, and package index serialization rather than duplicating material or package rules.
- [x] Preserve existing runtime material payload consumption and desktop package proof.
- [x] Keep graph/shader/live compilation, source image decoding, renderer/RHI residency, package streaming, native handles, and broad renderer claims unavailable.

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

- [x] Promote only the validated material instance apply capability.
- [x] Update docs so agents know when to use dry-run/apply instead of hand-editing material package rows.
- [x] Run focused checks and default validation.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed after adding static checks because `engine/agent/manifest.json` did not yet expose one ready `create-material-instance` command surface.
- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed with the same missing ready `create-material-instance` command surface.
- RED, 2026-05-01: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_tools_tests` failed after adding focused tests because `mirakana/tools/material_tool.hpp` and the material instance plan/apply API did not exist yet.
- GREEN, 2026-05-01: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_tools_tests` passed after adding `mirakana_tools` material instance plan/apply helpers.
- GREEN, 2026-05-01: `.\out\build\dev\Debug\mirakana_tools_tests.exe` passed, including `material instance apply tooling dry-runs material and package index changes`, `material instance apply tooling applies only after validation passes`, and `material instance apply tooling rejects unsafe paths inconsistent rows and unsupported claims`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after promoting only the narrow ready `create-material-instance` command surface.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with the same command-surface contract.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reported `aiOperableProductionLoop.currentActivePlan.path` as `docs/superpowers/plans/2026-05-01-material-instance-apply-tooling-v1.md`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the public `mirakana_tools` header addition.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with D3D12 DXIL, Vulkan SPIR-V, and DXC SPIR-V codegen ready; Metal `metal`/`metallib` tools remain missing host diagnostics.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed, 14/14 tests.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed for the selected D3D12 package with `ui_atlas_metadata_status=ready`, `ui_texture_overlay_status=ready`, and `ui_texture_overlay_draws=2`.
- GREEN, 2026-05-01: strict Vulkan selected package validation passed with `renderer=vulkan`, `framegraph_passes=3`, `ui_atlas_metadata_status=ready`, `ui_texture_overlay_status=ready`, and `ui_texture_overlay_draws=2`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, 28/28, with the existing diagnostic-only host gates for Metal tools, Apple packaging, Android release signing/device smoke, and tidy compile database availability.
