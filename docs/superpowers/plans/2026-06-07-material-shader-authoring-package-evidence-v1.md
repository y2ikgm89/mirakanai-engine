# 2026-06-07 Material Shader Authoring Package Evidence v1 Implementation Plan

**Plan ID:** `material-shader-authoring-package-evidence-v1`

**Status:** Completed.

**Goal:** Strengthen the AI-operable material/shader package descriptor contract so generated game manifests fail closed before package smoke evidence is claimed.

**Context:** `Material Shader Authoring Loop v1` already has package smoke evidence for reviewed material source inputs, material graph package lowering, D3D12 DXIL artifacts, strict Vulkan SPIR-V artifact gates, and package-visible counters. This slice does not add runtime shader compilation or renderer residency; it closes the manifest/schema/static-contract gap between the already documented descriptor fields and the installed SDK schema shipped to game creators.

**Official Source Check:** JSON Schema Draft 2020-12 official docs were checked for object `properties`, `required`, `additionalProperties`, array `items`, and `enum` usage. Context7 was attempted first, but the connector returned OAuth authorization required in this session.

**Constraints:**

- Keep the material/shader target rows descriptor-only; no command strings, shell args, native handles, renderer/RHI handles, package streaming commands, or runtime source parsing.
- Require D3D12 and Vulkan artifact path descriptors for material/shader authoring targets, because the loop only becomes package evidence after host-built artifacts are reviewed.
- Keep material graph fields typed and fail-closed without forcing fixed-HLSL-only 3D package targets to claim material graph authoring.
- Do not change C++ runtime behavior, shader sources, package payloads, CMake shader compilation, vcpkg dependencies, or public renderer/RHI APIs.

## Tasks

### Task 1: Add Static Contract Coverage

**Files:**

- Modify: `tools/check-json-contracts-010-engine-manifest.ps1`

- [x] Require the `materialShaderAuthoringTargets.items` schema to expose `required`, `additionalProperties`, and `properties`.
- [x] Require schema properties for material graph descriptor fields, compile targets, unsupported boundaries, shader source paths, D3D12 artifact paths, Vulkan artifact paths, and validation flags.
- [x] Require common descriptor fields plus D3D12/Vulkan artifact path lists without forcing optional material graph descriptor fields onto fixed-HLSL package rows.
- [x] Verify the check fails before schema updates.

### Task 2: Harden `game.agent.json` Schema

**Files:**

- Modify: `schemas/game-agent.schema.json`

- [x] Add JSON Schema entries for `sourceMaterialGraphPath`, `shaderExportPath`, `reviewedHlslSourcePath`, `compileRequestTargets`, and `unsupportedBoundaries`.
- [x] Make `d3d12ShaderArtifactPaths` and `vulkanShaderArtifactPaths` required non-empty arrays.
- [x] Use safe game-relative path patterns, `.materialgraph`, `.shader_export`, `.hlsl`, `.dxil`, and `.spv` suffixes, and enum-closed compile targets and unsupported boundaries.
- [x] Verify `tools/check-json-contracts.ps1` passes after the schema update.

### Task 3: Sync Documentation And Manifest

**Files:**

- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `games/sample_generated_desktop_runtime_material_shader_package/main.cpp`
- Compose: `engine/agent/manifest.json`

- [x] Document the schema/static-contract evidence without expanding runtime claims.
- [x] Keep `currentActivePlan` on the production-completion master plan and `recommendedNextPlan.id = next-production-gap-selection`.
- [x] Add the missing package-visible `postprocess_policy_fog_effect=0` report field required by installed validation.
- [x] Compose the manifest after fragment edits.

### Task 4: Validate And Publish

**Files:**

- Validate: static/agent/docs plus package smoke where the host supports it.

- [x] Run `tools/check-json-contracts.ps1`.
- [x] Run `tools/check-ai-integration.ps1`.
- [x] Run `tools/check-agents.ps1`.
- [x] Run `tools/check-format.ps1` and `git diff --check`.
- [x] Run `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package` unless a concrete host/tool blocker appears.
- [x] Run targeted `tools/check-tidy.ps1` for `games/sample_generated_desktop_runtime_material_shader_package/main.cpp`.
- [x] Run `tools/validate.ps1` for the slice closeout if package/static edits invalidate broader repository contracts.
- [ ] Run publication preflight before staging, push, PR, merge, and cleanup.

## Done When

- The schema and static checks agree on all material/shader descriptor fields.
- Fixed-HLSL package rows remain valid without claiming material graph authoring.
- Material graph package rows are typed for source graph, shader export, reviewed HLSL, compile targets, unsupported boundaries, and artifact paths.
- Docs, registry, manifest fragments, composed manifest, and static checks describe the same ready/non-ready truth.
- Validation evidence is recorded below before closeout.

## Non-Claims

- No shader graph execution.
- No live shader generation.
- No runtime shader compiler execution.
- No native shader cache handles.
- No renderer/RHI shader residency.
- No package streaming execution.
- No public native/RHI handles.
- No Metal library generation or Metal readiness.
- No general renderer quality claim.

## Validation Evidence

- `tools/check-json-contracts.ps1`: RED before schema update because `sourceMaterialGraphPath` was missing from `materialShaderAuthoringTargets.items.properties`; GREEN after schema update.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package`: RED before report fix because installed validation required `postprocess_policy_fog_effect=0`; GREEN after adding the package-visible field, with installed smoke reporting `postprocess_policy_fog_effect=0`, material graph authoring counters ready, D3D12 shader evidence ready, Vulkan shader evidence host-gated, and `desktop-runtime-package: ok`.
- `tools/check-json-contracts.ps1`: `json-contract-check: ok`.
- `tools/check-ai-integration.ps1`: `ai-integration-check: ok`.
- `tools/check-agents.ps1`: `agent-config-check: ok`.
- `tools/check-format.ps1`: `format-check: ok`.
- `git diff --check`: no whitespace errors.
- `tools/check-tidy.ps1 -Preset desktop-runtime-release -Configuration Release -Files games/sample_generated_desktop_runtime_material_shader_package/main.cpp -ReuseExistingFileApiReply`: `tidy-check: ok (1 files)`.
- `tools/validate.ps1`: `validate: ok`; CTest `107/107` passed. Apple/Metal checks remained host-gated diagnostic-only on this Windows host.
