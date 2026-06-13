# 2026-06-13 Environment Commercial Excellence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` after the operator explicitly selects this plan for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `environment-commercial-excellence-v1`

**Status:** Active. Phase 0 selected the milestone, Phase 1 added the readiness taxonomy, Phase 2 added optional OpenEXR dependency/legal/bootstrap gates, Phase 3 slice 1 added descriptor-level asset/cook metadata contracts, Phase 3 slice 2 added optional-gated OpenEXR environment source metadata review APIs only, and Phase 3 slice 3 adds the hosted Windows `asset-importers` validation lane. This does not claim strict Vulkan aggregate readiness, Metal host aggregate readiness, backend parity, all-platform readiness, broad optimization, OpenEXR/KTX/Basis asset-pipeline readiness, AAA preset-library readiness, physical weather simulation readiness, or artist-workflow readiness.

**Goal:** Promote the completed selected D3D12-primary environment aggregate into a commercial environment capability set with strict Vulkan aggregate readiness, Apple-host Metal aggregate readiness, backend parity, exact all-platform readiness rows, measured broad optimization evidence, a licensed AAA preset asset library, OpenEXR/KTX/Basis production asset pipeline, physically modeled weather simulation, and advanced artist workflow support. Every claim must be backed by backend-local package-visible counters, validation recipes, official-source constraints, dependency/legal records, and hosted or host-gated evidence. No broad claim may be inferred from another backend, host, sample, asset, or adjacent row.

**Architecture:** Keep `MK_environment` as the backend-neutral value, profile, validation, weather, quality, asset-reference, and readiness-contract layer. Keep content import and cooking in `MK_tools` under `engine/tools/{asset,scene,shader}` with public headers in `engine/tools/include/mirakana/tools/`. Keep renderer policy in `MK_renderer` and GPU execution in private backend implementations for `MK_rhi_d3d12`, `MK_rhi_vulkan`, and Apple-host-gated `MK_rhi_metal`. Keep first-party editor workflow in `MK_editor_core` and the visible `MK_editor` shell. Put all optional third-party codec dependencies behind vcpkg manifest features and `tools/bootstrap-deps.ps1`. Expose no public native OS, GPU, texture, command queue, or dependency handles. Use clean-break schemas only; do not add compatibility parsers, deprecated aliases, or migration shims for older environment profile or asset-pipeline formats.

**Tech Stack:** C++23, PowerShell 7 repository tools, CMake presets through repository wrappers, vcpkg manifest features, `MK_environment`, `MK_renderer`, `MK_rhi`, `MK_tools`, `MK_editor_core`, `MK_editor`, HLSL, DXC DXIL, DXC SPIR-V CodeGen, SPIR-V validation, Vulkan 1.3 synchronization2 and dynamic rendering, Direct3D 12 barriers/descriptors/debug-layer/GPU-based-validation evidence, Apple Metal resource/texture/pipeline/synchronization evidence on Apple hosts, OpenEXR for HDR scene-linear source images, KTX2/Basis Universal texture containers/transcoding, first-party asset-library metadata, and repository package/runtime validation recipes.

---

## Current Starting Point

Use these current facts as the baseline before implementing this plan:

- `environment-production-excellence-v1` is completed through selected D3D12-primary `environment_ready` aggregate evidence in PR #588 / merge commit `32f053be6ac3bba1948a1d40edf0132120e7adc7`.
- The current aggregate proves `environment_ready=1`, `environment_ready_profile_v2=1`, `environment_ready_d3d12_primary=1`, `environment_ready_vulkan_strict=0`, `environment_ready_metal_host=0`, `environment_ready_backend_parity=0`, `environment_ready_broad_optimization_claimed=0`, `environment_ready_native_handle_access=0`, and `environment_ready_diagnostics=0`.
- Current evidence does not claim strict Vulkan aggregate readiness, Metal host aggregate readiness, backend parity, all-platform readiness, broad optimization, OpenEXR/KTX/Basis production asset import, AAA preset asset-library readiness, physically complete weather simulation, or complete real-production artist workflow.
- `unsupportedProductionGaps = []` remains the 1.0 truth. This plan is a post-1.0 candidate milestone and must not reopen completed 1.0 readiness rows.
- The active plan stack must stay shallow. Selecting this plan later requires replacing the active selection through manifest fragments rather than appending work to completed historical plans.

## Official Source Baseline

Re-open and refresh the exact source set before implementing the phase that depends on it. Record the refresh date, selected Context7 library id, official URL, and implementation implication in this plan or the phase PR before editing code.

Product-scope sources:

- Unreal Engine Sky Atmosphere: <https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-in-unreal-engine?lang=en-US>
- Unreal Engine Environmental Light with Fog, Clouds, Sky, and Atmosphere: <https://dev.epicgames.com/documentation/unreal-engine/environmental-light-with-fog-clouds-sky-and-atmosphere-in-unreal-engine?lang=en-US>
- Unity HDRP Visual Environment 17: <https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/Override-Visual-Environment.html>
- Unity HDRP Volumes: <https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/understand-volumes.html>
- Godot Environment and post-processing: <https://docs.godotengine.org/en/4.4/tutorials/3d/environment_and_post_processing.html>
- Godot volumetric fog and fog volumes: <https://docs.godotengine.org/en/latest/tutorials/3d/volumetric_fog.html>

Implementation-source sources:

- Khronos Vulkan specification and guide through Context7 `/khronosgroup/vulkan-docs`
- Khronos Vulkan synchronization chapter: <https://docs.vulkan.org/spec/latest/chapters/synchronization.html>
- Khronos Vulkan synchronization2 guide: <https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html>
- Khronos Vulkan dynamic rendering: <https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/03_Render_passes.html>
- Khronos Vulkan validation layers: <https://docs.vulkan.org/guide/latest/layers.html>
- Microsoft Direct3D 12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- Microsoft Direct3D 12 debug layer and GPU-based validation: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation>
- Microsoft Direct3D 12 upload heaps: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources>
- Microsoft Direct3D 12 readback heaps: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps>
- Apple Metal resource fundamentals: <https://developer.apple.com/documentation/metal/resource-fundamentals>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
- Apple Metal `synchronize(resource:)`: <https://developer.apple.com/documentation/metal/mtlblitcommandencoder/synchronize%28resource%3A%29>
- Apple Metal capabilities: <https://developer.apple.com/metal/capabilities/>
- OpenEXR documentation through Context7 `/academysoftwarefoundation/openexr`
- OpenEXR Technical Introduction: <https://openexr.com/en/latest/TechnicalIntroduction.html>
- OpenEXR Scene Linear: <https://openexr.com/en/latest/SceneLinear.html>
- Khronos KTX-Software through Context7 `/khronosgroup/ktx-software`
- Khronos KTX overview: <https://www.khronos.org/ktx/>
- Vulkan Samples Basis Universal texture compression: <https://docs.vulkan.org/samples/latest/samples/performance/texture_compression_basisu/README.html>
- vcpkg manifest reference: <https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json>
- vcpkg manifest mode: <https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode>

Source implications:

- Unreal, Unity, and Godot are product-scope references only. They justify capability shape, authoring concepts, quality controls, and renderer support boundaries; they are not implementation code sources.
- Vulkan readiness must use explicit image usage flags, image layouts, `VkImageMemoryBarrier2KHR`, `VkDependencyInfoKHR`, `vkCmdPipelineBarrier2KHR`, queue ownership transfer evidence where required, SPIR-V validation, validation layers, and package-visible synchronization/readback counters.
- Metal readiness must be proven on Apple hosts with real command queues, command buffers, resource options, texture usage, render or compute pipelines, blit synchronization where required, readback evidence, and zero public native-handle exposure. Windows-side declaration rows cannot promote Metal readiness.
- D3D12 readiness must track descriptor heap/table use, SRV/UAV/RTV/DSV view shape, resource states, barriers, upload/readback heaps, fences, debug-layer diagnostics, and GPU-based validation status.
- OpenEXR import must treat source HDR images as scene-linear data with explicit channel, pixel type, data window, chromaticity, multipart/deep/tiled handling decisions, metadata preservation policy, and failure diagnostics.
- KTX2/Basis import must validate container metadata, decide whether transcoding is required, choose a GPU format from backend/device capabilities, and record compression/transcoding decisions in cooked metadata.
- vcpkg changes must be optional manifest features installed only by `tools/bootstrap-deps.ps1`; CMake configure must not restore, download, or install packages.

## Non-Negotiable Claim Rules

- Do not claim `environment_commercial_ready` until every selected row in this plan is ready, package-visible, and guarded by validation and static checks.
- Do not claim backend parity from D3D12 evidence alone, Vulkan evidence alone, Metal evidence alone, a common policy test, or a product-doc comparison.
- Do not claim all-platform readiness as a single unconditional row. Use per-platform and per-backend rows such as `windows_d3d12`, `linux_vulkan`, `macos_metal`, `ios_metal`, and `android_vulkan`.
- Do not claim broad optimization from a single sample, a single frame, synthetic counters without before/after measurements, or code inspection.
- Do not claim AAA asset-library readiness until every shipped asset has first-party ownership or recorded redistribution license, source provenance, authored metadata, package budget, memory budget, and sample consumption evidence.
- Do not claim complete weather simulation from authored weather states, particle precipitation, fog, clouds, or visual effects alone.
- Do not mutate source art, environment profiles, cooked packages, or editor state without reviewed command ids, dry-run output, revision checks, and deterministic apply reports.
- Do not expose public native handles or public third-party dependency types.
- Do not add Dear ImGui, SDL3, Qt, Slint, RmlUi, UI middleware, compatibility shims, deprecated aliases, or migration layers.

## Target Claim Matrix

| Claim id | Required exact evidence | Forbidden inference |
| --- | --- | --- |
| `environment_commercial_ready` | All selected rows pass on their required hosts, package-visible counters are positive where expected, static checks enforce non-claims, docs/manifest/plan registry are synced, and PR evidence records hosted or host-gated validation. | Any single backend aggregate, product-doc comparison, or editor-only readiness. |
| `environment_vulkan_strict_aggregate_ready` | Linux or Windows Vulkan strict recipe with validation layers, SPIR-V validation, synchronization2 barriers, dynamic-rendering layout proof, descriptor proof, selected environment features rendered or computed, readback checksums, and zero diagnostics/native handles. | D3D12 aggregate, Vulkan unit tests without a strict recipe, or selected feature-only Vulkan lanes. |
| `environment_metal_host_aggregate_ready` | macOS Apple-host recipe with compiled Metal libraries, real Metal pipelines, texture/cube/HDR resources, synchronization/readback proof, feature rows, and zero diagnostics/native handles. | Windows declaration rows, CMake compile-only proof, or non-Apple host evidence. |
| `environment_backend_parity_ready` | D3D12, strict Vulkan, and Apple-host Metal rows share the same declared feature set, quality-budget class, rendered output tolerance class, package counters, and unsupported-row list. | Policy-only tests, two-backend parity, or comparing screenshots without backend counter parity. |
| `environment_platform_windows_d3d12_ready` | Windows hosted/local D3D12 recipe with package smoke, runtime execution, debug-layer or GBV evidence, and package counters. | Generic Windows build success. |
| `environment_platform_linux_vulkan_ready` | Linux Vulkan recipe with required Vulkan SDK tools, validation layers, SPIR-V validation, strict runtime package smoke, and readback counters. | Windows Vulkan evidence or Linux compile-only evidence. |
| `environment_platform_macos_metal_ready` | macOS Apple-host Metal recipe with Xcode tools, Metal compile, runtime execution, synchronization/readback, and package counters. | iOS simulator evidence or Windows host declarations. |
| `environment_platform_ios_metal_ready` | iOS simulator or device smoke with selected Metal environment features, signing/toolchain gate status, package counters, and host-gated blockers recorded when absent. | macOS desktop Metal evidence. |
| `environment_platform_android_vulkan_ready` | Android GameActivity or selected Android runtime recipe with Vulkan feature/device checks, package smoke, and toolchain evidence. | Desktop Vulkan evidence. |
| `environment_broad_optimization_ready` | Measured CPU/GPU/memory before/after traces for selected workloads, exact host/GPU/driver/tool versions, regression budgets, package counters, and static checks preventing narrow evidence from broad claims. | Single metric, synthetic-only benchmark, or "optimized" code review. |
| `environment_asset_pipeline_openexr_ktx_basis_ready` | Optional dependency feature gates, legal records, importer/cooker tests, sample assets, cooked metadata, device-format selection, package smoke, and fail-closed unsupported format diagnostics. | Existing placeholder assets, source-image metadata only, or compile-only dependencies. |
| `environment_aaa_preset_library_ready` | First-party or licensed preset pack with art direction, provenance, package and memory budgets, sample scenes, editor browsing, package install smoke, and third-party notices. | Procedural placeholders, unlicensed assets, or engine feature rows. |
| `environment_physical_weather_simulation_ready` | Numerical model contract, deterministic CPU reference or explicit nondeterminism contract, stability constraints, conservation/error bounds, CPU/GPU budget, fallback path, validation datasets/images, package counters, and profiler evidence. | Rain/snow/fog/cloud rendering, authored weather timelines, or particle systems. |
| `environment_artist_workflow_ready` | First-party editor commands, asset browser, profile graph editing, import/cook/preview/package loop, dry-run/apply reports, validation remediation, revision safety, and production sample walkthrough evidence. | Editor-core rows without visible workflow or manual file editing. |

## Phase 0: Select The Candidate Milestone

**Goal:** Select this plan intentionally without changing runtime behavior or making readiness claims.

**Files:**

- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- composed `engine/agent/manifest.json`
- `docs/superpowers/plans/README.md`
- `docs/roadmap.md`
- `docs/current-capabilities.md`
- static checks only when new selected-plan literals need enforcement

- [x] Confirm no unrelated local edits will be staged.
- [x] Set `currentActivePlan` to `docs/superpowers/plans/2026-06-13-environment-commercial-excellence-v1.md`.
- [x] Set `recommendedNextPlan.id` to `environment-commercial-excellence-v1`.
- [x] Keep `unsupportedProductionGaps = []` unless a specific newly selected gap is deliberately represented.
- [x] Record that this selection does not claim strict Vulkan, Metal, backend parity, all-platform, broad optimization, asset-pipeline, AAA asset library, physical weather simulation, or artist-workflow readiness.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

**Done when:** The manifest and registry point to this plan, no runtime behavior changed, non-claims are explicit, and static checks pass.

**Phase 0 local evidence:** This selection slice updates only manifest/docs/registry surfaces and composed manifest output. It keeps `unsupportedProductionGaps = []` and does not modify runtime, renderer, backend, asset-pipeline, performance, simulation, platform, or editor implementation files.

## Phase 1: Readiness Taxonomy And Static Guard Foundation

**Goal:** Define the machine-readable readiness taxonomy before implementing features.

**Files:**

- `engine/agent/manifest.fragments/009-validationRecipes.json`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- composed `engine/agent/manifest.json`
- `tools/check-ai-integration-030-runtime-rendering.ps1`
- `tools/check-json-contracts-030-tooling-contracts.ps1`
- `tools/check-json-contracts-040-agent-surfaces.ps1`
- `docs/current-capabilities.md`
- `docs/roadmap.md`
- this plan and `docs/superpowers/plans/README.md`

- [x] Add manifest rows for every target claim id in the Target Claim Matrix with `ready`, `host-gated`, or `unsupported` state and exact validation recipe id.
- [x] Add unsupported adjacent rows for unconditional all-platform parity, broad renderer quality, public native handles, unlicensed assets, unreviewed content mutation, compatibility parsers, and default optional-codec dependencies.
- [x] Add static checks that fail if broad readiness rows are set without their required backend/platform/asset/performance evidence rows.
- [x] Add package counter naming rules for every new claim id.
- [x] Add docs that explain exact readiness versus host-gated readiness.
- [x] Run focused JSON and AI integration checks.

Phase 1 implementation notes:

- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` now owns `environmentCommercialClaimMatrix` and `environmentCommercialUnsupportedAdjacentClaims`.
- `schemas/engine-agent/ai-operable-production-loop.schema.json` requires the new taxonomy rows and keeps `ready`, `host-gated`, and `unsupported` as a dedicated clean-break claim-state enum.
- `tools/check-json-contracts-010-engine-manifest.ps1` now fails closed on missing/duplicate claim ids, unknown validation recipes, invalid state, non-canonical package counters, unknown dependencies, broad ready claims before dependency readiness, adjacent non-claim drift, and accidental `unsupportedProductionGaps` repopulation.
- `engine/agent/manifest.fragments/014-gameCodeGuidance.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `docs/superpowers/plans/README.md` explain exact readiness versus host-gated or unsupported readiness.
- Focused validation passed on 2026-06-13: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-text-format.ps1`, `tools/check-agents.ps1`, and `git diff --check`.

**Done when:** The repository can represent every planned claim and non-claim before implementation starts, and static checks fail closed for overbroad readiness.

## Phase 2: Dependency, License, And Bootstrap Gate For OpenEXR/KTX/Basis

**Goal:** Add optional dependency gates without changing default configure behavior.

**Files:**

- `vcpkg.json`
- `engine/tools/CMakeLists.txt`
- `tools/bootstrap-deps.ps1`
- `CMakePresets.json` only if a repository-supported optional preset is required
- `docs/dependencies.md`
- `docs/legal-and-licensing.md`
- `THIRD_PARTY_NOTICES.md`
- `docs/testing.md`
- `tools/check-dependency-policy.ps1`
- `tools/check-ai-integration-030-runtime-rendering.ps1`
- `tools/check-json-contracts-030-tooling-contracts.ps1`

- [x] Use `license-audit` before selecting package names or redistributable assets.
- [x] Add optional vcpkg manifest features for OpenEXR and KTX/Basis support, or record an explicit official-source reason if a dependency is not available through vcpkg.
- [x] Keep CMake configure side-effect-free with `VCPKG_MANIFEST_INSTALL=OFF`; dependency installation remains only in `tools/bootstrap-deps.ps1`.
- [x] Record license, version, registry baseline, notice, redistribution, patent/compression, and binary-distribution implications.
- [x] Add validation that default builds do not require optional codec packages.
- [x] Add validation that selected optional codec builds fail with a clear missing-dependency blocker when dependencies are not bootstrapped.
- [x] Run dependency/legal/static checks and a focused default configure/build lane.

Phase 2 implementation notes:

- `vcpkg.json` now keeps default `dependencies` empty and adds `openexr` to the existing optional `asset-importers` feature alongside libspng, fastgltf, KTX Software, and miniaudio.
- `engine/tools/CMakeLists.txt` requires `find_package(OpenEXR CONFIG REQUIRED)` only when `MK_ENABLE_ASSET_IMPORTERS=ON`; it does not link OpenEXR into public or installed Mirakanai package config targets yet.
- `THIRD_PARTY_NOTICES.md`, `docs/dependencies.md`, and `docs/legal-and-licensing.md` record OpenEXR 3.4.12, Imath 3.2.2, libdeflate 1.25, and OpenJPH 0.27.2 port 1 from vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476`.
- `tools/check-dependency-policy.ps1` now fails closed when the OpenEXR dependency, docs, legal policy, notices, or CMake optional configure gate drift.
- Local validation passed on 2026-06-13: `tools/check-dependency-policy.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-text-format.ps1`, `tools/check-agents.ps1`, `git diff --check`, `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev`, and `tools/ctest.ps1 --preset dev --output-on-failure`.
- `tools/bootstrap-deps.ps1` was attempted locally but the session command policy rejected it with approval required while approvals were unavailable. Existing `vcpkg_installed` did not contain OpenEXR, and `tools/build-asset-importers.ps1` failed at optional configure with a missing bootstrapped package blocker before any CMake-side package installation.

**Done when:** Optional dependencies are legally recorded, bootstrap-owned, default-off, fail-closed, and documented without importing or cooking assets yet.

## Phase 3: Production Asset Pipeline For OpenEXR, KTX2, And Basis

**Goal:** Implement import, validation, and cooking for HDR and compressed texture assets.

**Files:**

- `engine/tools/include/mirakana/tools/asset/`
- `engine/tools/asset/src/`
- `engine/tools/asset/CMakeLists.txt`
- `engine/tools/scene/src/` only when scene-package references are required
- `tools/package-desktop-runtime.ps1`
- `tools/validate-installed-desktop-runtime.ps1`
- `tools/validation-recipe-core.ps1`
- `tests/unit/*asset*tests.cpp`
- `games/sample_desktop_runtime_game/source/`
- `games/sample_desktop_runtime_game/runtime/assets/`
- `games/sample_desktop_runtime_game/game.agent.json`

- [x] Add clean-break source asset descriptors for `GameEngine.TextureSource.v2` and `GameEngine.EnvironmentAssetSource.v1`.
- [x] Reject legacy descriptors explicitly with diagnostics instead of compatibility parsing.
- [ ] For OpenEXR, validate data window, display window, channel list, pixel type, multipart/deep/tiled flags, color metadata, and scene-linear intent through a bootstrapped `asset-importers` lane.
- [ ] For KTX2/Basis, validate container metadata, levels, layers, faces, supercompression, Basis requirement, and intended sampler class.
- [ ] Add device/backend format-selection policy rows for D3D12, Vulkan, Metal, Android Vulkan, and iOS Metal.
- [ ] Cook deterministic `.geasset` metadata with source hash, license/provenance id, color space, compression/transcoding choice, mip count, memory estimate, and unsupported-host diagnostics.
- [ ] Add package smokes proving selected EXR and KTX2/Basis assets can be reviewed, cooked, installed, and referenced by environment packages.
- [ ] Run optional dependency bootstrap validation, focused asset tests, package smoke, and full `tools/validate.ps1` if C++ runtime/build/public contracts changed.

Phase 3 slice 1 implementation notes:

- `MK_assets` now exposes clean-break value contracts for `TextureSourceDocumentV2`, `EnvironmentAssetSourceDocumentV1`, `TextureCookMetadataDocumentV1`, backend policy rows, and deterministic serializers for `GameEngine.TextureSource.v2`, `GameEngine.EnvironmentAssetSource.v1`, and `GameEngine.CookedTextureMetadata.v1`.
- The slice validates descriptor-level OpenEXR scene-linear intent, data/display windows, channel metadata, pixel encoding, multipart/deep exclusion, chromaticity recording, and KTX2/Basis level/layer/face/supercompression/transcode intent. It does not read EXR/KTX binaries, invoke OpenEXR or KTX tools, transcode Basis data, or write cooked payload bytes.
- Cook metadata rows serialize D3D12, Vulkan, macOS Metal, Android Vulkan, and iOS Metal format policies with support, host-validation, estimated GPU byte, and diagnostic fields. These are policy metadata rows only; they do not promote backend readiness, GPU upload readiness, platform readiness, or `environment_asset_pipeline_openexr_ktx_basis_ready`.
- Focused TDD evidence: `MK_asset_environment_source_pipeline_tests` first failed to compile before the new public contracts existed, then passed after implementation with `tools/cmake.ps1 --build --preset dev --target MK_asset_environment_source_pipeline_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_asset_environment_source_pipeline_tests`.

Phase 3 slice 2 implementation notes:

- `MK_tools` now exposes `OpenExrTextureSourceReviewRequest`, `OpenExrTextureSourceReviewResult`, `OpenExrTextureSourceReviewDiagnostic`, `has_openexr_texture_source_review`, and `review_openexr_texture_source_metadata` for OpenEXR environment-radiance source metadata review.
- The adapter uses OpenEXR official C++ APIs behind `MK_HAS_ASSET_IMPORTERS`: `InputFile` for scanline files, `TiledInputFile` for tiled files, `Header` data/display windows, `ChannelList` pixel types, `isMultiPartOpenExrFile`, `isDeepOpenExrFile`, `isTiledOpenExrFile`, and `hasChromaticities`. It maps accepted single-encoding `HALF`/`FLOAT` `R/G/B` plus optional `A` channels into `GameEngine.TextureSource.v2` OpenEXR review metadata and rejects multipart/deep files, missing RGB channels, extra channels, mixed/unsupported channel encodings, missing chromaticities, and missing scene-linear intent.
- Default builds remain dependency-free and fail closed with `OpenExrTextureSourceReviewDiagnosticCode::asset_importers_disabled`; `MK_tools_tests` covers that boundary. This slice does not decode pixels, compute hashes, transcode KTX/Basis, write cooked texture payloads, run GPU/device-format queries, install package assets, or promote `environment_asset_pipeline_openexr_ktx_basis_ready`.
- Focused evidence on 2026-06-14: `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_tools_tests`, and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests` passed. `tools/bootstrap-deps.ps1` was blocked by session command policy (`approval required` with approvals unavailable), and `tools/cmake.ps1 --preset asset-importers` failed before build with missing bootstrapped `SPNGConfig.cmake`; bootstrapped OpenEXR binary-read proof remains a Phase 3 follow-up gate.

Phase 3 slice 3 implementation notes:

- `.github/workflows/validate.yml` now runs `tools/build-asset-importers.ps1` in the hosted Windows MSVC job after `tools/bootstrap-deps.ps1` and default validation, using the official GitHub Actions `run` step with `shell: pwsh`.
- `tools/check-ci-matrix.ps1` now fails closed if the hosted Windows workflow stops running `tools/build-asset-importers.ps1` or stops uploading `out/build/asset-importers/Testing/**/*.log` on failure.
- This is a validation-surface slice only. It proves the optional dependency lane is wired into hosted PR evidence, but it does not add new EXR/KTX/Basis pixel decode behavior, KTX/Basis transcoding, cooked payload writing, package smoke counters, runtime texture upload, backend format selection execution, or `environment_asset_pipeline_openexr_ktx_basis_ready` promotion.

**Done when:** The selected asset pipeline imports and cooks HDR and compressed texture assets with deterministic metadata, legal provenance, backend format decisions, and package evidence.

## Phase 4: AAA Preset Asset Library Governance And First Pack

**Goal:** Ship a production-style environment preset library with license and budget evidence.

**Files:**

- `games/sample_desktop_runtime_game/source/environment/`
- `games/sample_desktop_runtime_game/source/textures/`
- `games/sample_desktop_runtime_game/runtime/assets/`
- `docs/legal-and-licensing.md`
- `THIRD_PARTY_NOTICES.md`
- `docs/dependencies.md`
- `docs/current-capabilities.md`
- `tools/package-desktop-runtime.ps1`
- `tools/validate-installed-desktop-runtime.ps1`
- `tests/unit/*environment*tests.cpp`

- [ ] Define preset-pack schema `GameEngine.EnvironmentPresetPack.v1` with provenance id, art direction, dependencies, quality tier, package budget, memory budget, and required backend feature rows.
- [ ] Create first-party or fully licensed presets for at least clear noon, overcast storm, night moonlit, snowfield, foggy valley, cinematic sunset, and indoor-to-outdoor transition.
- [ ] Each preset must include authored environment profile, sky/atmosphere values, fog/cloud/weather timeline, IBL references, material-weathering references, audio trigger intent, and quality budget.
- [ ] Each preset must include package size, installed size, decoded memory estimate, GPU memory estimate, and expected validation recipe.
- [ ] Add editor/library browsing rows and package sample consumption evidence.
- [ ] Add static checks that fail unlicensed or provenance-missing shipped preset assets.
- [ ] Run package smoke, legal notice checks, focused tests, and full validation if runtime/package contracts changed.

**Done when:** A licensed, budgeted, editor-visible preset pack ships as package evidence and no asset can enter the pack without provenance and budget metadata.

## Phase 5: Strict Vulkan Aggregate Readiness

**Goal:** Promote Vulkan from selected feature lanes to an exact strict aggregate.

**Files:**

- `engine/rhi/vulkan/src/`
- `engine/renderer/src/`
- `engine/renderer/include/mirakana/renderer/`
- `shaders/environment/`
- `tools/compile-*vulkan*.ps1`
- `tools/validation-recipe-core.ps1`
- `tests/unit/backend_scaffold_tests.cpp`
- `games/sample_desktop_runtime_game/game.agent.json`

- [ ] Refresh Context7 `/khronosgroup/vulkan-docs` and official Khronos pages before implementation.
- [ ] Define `desktop-runtime-sample-game-environment-vulkan-strict-aggregate` recipe.
- [ ] Require Vulkan SDK tools, DXC SPIR-V CodeGen, `spirv-val`, synchronization2, dynamic rendering, validation layers, and strict package smoke.
- [ ] Prove image usage/layout compatibility for every attachment, sampled texture, storage buffer, cube map, weather texture, froxel buffer, and readback resource.
- [ ] Record positive synchronization2 barrier counters for transfer-to-shader, compute-to-shader, attachment-to-shader, and readback transitions where used.
- [ ] Record descriptor set binding counters for all selected features.
- [ ] Record positive draw, dispatch, upload, and readback counters for the selected aggregate.
- [ ] Record zero validation diagnostics, zero native-handle access, zero D3D12 fallback, and zero Metal fallback.
- [ ] Add fail-closed tests for missing Vulkan toolchain, missing validation layer, missing SPIR-V validation, and unsupported feature/device rows.

**Done when:** `environment_vulkan_strict_aggregate_ready=1` only on hosts that pass the strict Vulkan aggregate recipe, and all non-Vulkan inference remains blocked.

## Phase 6: Apple-Host Metal Aggregate Readiness

**Goal:** Promote Metal from selected Apple-host feature evidence to an exact aggregate on Apple hosts.

**Files:**

- `engine/rhi/metal/src/`
- `engine/rhi/metal/CMakeLists.txt`
- `engine/renderer/src/`
- `shaders/environment/` or Metal shader-generation source files when required
- `tools/validation-recipe-core.ps1`
- `tests/unit/backend_scaffold_tests.cpp`
- `games/sample_desktop_runtime_game/game.agent.json`

- [ ] Refresh Apple Metal resource, synchronization, texture, and capability documentation before implementation.
- [ ] Define `renderer-metal-environment-aggregate-apple-host-evidence` recipe.
- [ ] Require macOS AppleClang/Xcode toolchain, compiled Metal libraries, real device or approved CI Metal path, and package-visible host evidence.
- [ ] Prove render and compute pipeline creation for selected sky, fog, cloud, precipitation, IBL, material-weathering, and weather simulation visualization rows.
- [ ] Prove texture/cube/HDR resource creation with explicit usage and synchronization/readback where required.
- [ ] Record positive command buffer, pipeline, texture, synchronization, draw/dispatch, and readback counters.
- [ ] Record zero diagnostics, zero native-handle access, zero Vulkan fallback, and zero D3D12 fallback.
- [ ] Add host-gated blockers for non-Apple hosts and fail-closed tests for compile-only evidence attempting to promote readiness.

**Done when:** `environment_metal_host_aggregate_ready=1` only from Apple-host execution evidence, and Windows-side Metal declarations remain non-promoting.

## Phase 7: Backend Parity Matrix

**Goal:** Prove D3D12, strict Vulkan, and Apple-host Metal implement the same selected environment capability set within explicit tolerances.

**Files:**

- `engine/renderer/include/mirakana/renderer/environment_parity.hpp`
- `engine/renderer/src/environment_parity.cpp`
- `tests/unit/renderer_environment_parity_tests.cpp`
- `tools/validation-recipe-core.ps1`
- `docs/current-capabilities.md`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`

- [ ] Define parity rows for feature presence, quality tier, resource class, output tolerance, package counters, unsupported rows, and diagnostics.
- [ ] Require D3D12 primary, strict Vulkan aggregate, and Apple-host Metal aggregate rows to be fresh for the same profile and preset pack revision.
- [ ] Define tolerance classes for scalar counters, image readback hashes, visual metrics, and backend-specific compression formats.
- [ ] Fail parity when any backend silently disables a selected feature, uses an unsupported fallback, exceeds quality budgets, or omits diagnostics.
- [ ] Record `environment_backend_parity_ready=1` only when every selected parity row passes.
- [ ] Keep non-selected platforms host-gated rather than parity-ready.

**Done when:** Backend parity is a checked matrix over exact evidence rows, not a narrative claim.

## Phase 8: All-Platform Readiness Rows

**Goal:** Add exact platform readiness rows without creating an unconditional all-platform claim.

**Files:**

- `games/sample_desktop_runtime_game/game.agent.json`
- mobile package/runtime manifests when selected
- `tools/check-mobile-packaging.ps1`
- `tools/validate-desktop-game-runtime.ps1`
- `tools/validation-recipe-core.ps1`
- `engine/agent/manifest.fragments/009-validationRecipes.json`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- `docs/testing.md`
- `docs/current-capabilities.md`

- [ ] Define exact rows for Windows D3D12, Windows Vulkan, Linux Vulkan, macOS Metal, iOS Metal, Android Vulkan, and any deliberately unsupported platform.
- [ ] For each row, record required host OS, SDK, compiler, runtime sample, backend, feature set, package smoke, profiler or validation tools, and blocker reason.
- [ ] Add CI recipe mappings only for platforms actually available in hosted checks.
- [ ] Add host-gated blocker language for signing, simulator/device, GPU driver, Android SDK/NDK, Vulkan SDK, Xcode, and platform-specific package tools.
- [ ] Fail closed when a platform row is missing, stale, or inferred from another platform.
- [ ] Keep `environment_all_platform_unconditional_ready=0` as a permanent non-claim unless a future architecture decision replaces the row model.

**Done when:** Every selected platform has exact ready or host-gated evidence, and no unconditional all-platform aggregate exists.

## Phase 9: Broad Optimization Measurement Evidence

**Goal:** Prove selected broad environment optimization with reproducible measurements.

**Files:**

- `engine/renderer/include/mirakana/renderer/environment_performance.hpp`
- `engine/renderer/src/environment_performance.cpp`
- `engine/environment/include/mirakana/environment/environment_quality_budget.hpp`
- `engine/environment/src/environment_quality_budget.cpp`
- `tests/unit/*performance*tests.cpp`
- `tools/validation-recipe-core.ps1`
- `tools/check-performance-*.ps1` only if a reusable check is needed
- `docs/testing.md`
- `docs/current-capabilities.md`

- [ ] Use `performance-optimization-change` before implementation.
- [ ] Define workloads: preset-pack flythrough, storm precipitation, dense volumetric fog, volumetric cloud sunset, snowfield material weathering, weather simulation stress, and asset-library cold load.
- [ ] For each workload, define host, CPU, GPU, driver, OS, backend, resolution, frame count, warmup count, quality tier, and package revision.
- [ ] Capture before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, and stutter budget.
- [ ] Use official host tools where available: Windows Performance Toolkit, PIX on Windows, D3D12 debug/GBV, Vulkan validation/profiling tooling, Xcode/Metal tools, and repository counters.
- [ ] Define regression thresholds and fail package validation when thresholds are exceeded on supported proof hosts.
- [ ] Record `environment_broad_optimization_ready=1` only after all selected workloads have before/after measurements and regression guards.

**Done when:** Broad optimization is backed by reproducible workload measurements, profiler artifacts, package counters, and regression thresholds.

## Phase 10: Physical Weather Simulation

**Goal:** Implement a physically modeled weather simulation layer that is distinct from authored VFX.

**Files:**

- `engine/environment/include/mirakana/environment/weather_simulation.hpp`
- `engine/environment/src/weather_simulation.cpp`
- `engine/renderer/include/mirakana/renderer/weather_simulation_policy.hpp`
- `engine/renderer/src/weather_simulation_policy.cpp`
- GPU backend files only after CPU reference behavior is green
- `tests/unit/environment_weather_simulation_tests.cpp`
- `shaders/environment/weather_simulation*.hlsl` only for selected GPU acceleration
- `tools/validation-recipe-core.ps1`

- [ ] Define the numerical model, state variables, units, grid/domain boundaries, timestep limits, forcing inputs, precipitation transfer, fog/cloud coupling, and stability constraints.
- [ ] Add deterministic CPU reference tests for conservation or bounded-error properties, timestep clamping, seeded reproducibility, boundary behavior, and invalid input diagnostics.
- [ ] Record whether the production simulation contract is deterministic, deterministic-per-host, or explicitly nondeterministic; do not mix these states.
- [ ] Add GPU acceleration only after CPU reference behavior and tests are green.
- [ ] Add package counters for simulation steps, cells, timestep, energy/moisture/error bounds, fallback activation, CPU time, GPU time, and diagnostics.
- [ ] Add artist-facing controls that author initial conditions and forces without exposing raw unstable solver internals.
- [ ] Add validation images or datasets for selected canonical conditions.

**Done when:** `environment_physical_weather_simulation_ready=1` is backed by a specified numerical model, stability and error evidence, package counters, performance budgets, and deterministic or explicitly nondeterministic contract.

## Phase 11: Advanced Artist Workflow

**Goal:** Make the environment system usable for real content production through first-party tools.

**Files:**

- `editor/core/include/mirakana/editor/environment_authoring.hpp`
- `editor/core/src/environment_authoring.cpp`
- visible `editor/` shell files after editor-core contracts are green
- `engine/tools/asset/src/`
- `engine/tools/scene/src/`
- `games/sample_desktop_runtime_game/source/`
- `tests/unit/editor_environment_tests.cpp`
- `tools/validation-recipe-core.ps1`
- `docs/workflows.md`
- `docs/current-capabilities.md`

- [ ] Define reviewed command ids for preset import, source asset review, cook preview, profile graph edit, weather timeline edit, local volume edit, simulation parameter edit, quality budget edit, package preview, validation remediation, and publish/package.
- [ ] Every mutation command must support dry-run, revision-checked apply, deterministic report rows, and undo/rollback metadata where the existing editor architecture supports it.
- [ ] Add editor asset-browser rows for preset library, EXR source, KTX2/Basis source, cooked texture, environment profile, simulation preset, validation report, and package artifact.
- [ ] Add preview rows for selected backend, quality tier, missing host gate, package budget, memory budget, diagnostics, and unsupported claim reason.
- [ ] Add production walkthrough validation: import source assets, cook, assemble preset, edit weather timeline, run simulation preview, package sample, run installed validation, and inspect report.
- [ ] Keep backend execution out of editor core unless the visible editor shell has a reviewed runtime-host path.

**Done when:** `environment_artist_workflow_ready=1` proves a reviewed, deterministic, first-party authoring loop for production environment content.

## Phase 12: Commercial Aggregate Closeout

**Goal:** Promote the commercial aggregate only after all selected exact rows pass.

**Files:**

- `engine/agent/manifest.fragments/009-validationRecipes.json`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- composed `engine/agent/manifest.json`
- `docs/current-capabilities.md`
- `docs/roadmap.md`
- `docs/superpowers/plans/README.md`
- this plan
- `tools/check-ai-integration-030-runtime-rendering.ps1`
- `tools/check-json-contracts-040-agent-surfaces.ps1`

- [ ] Require ready rows for strict Vulkan aggregate, Metal host aggregate, backend parity, selected platform rows, broad optimization, asset pipeline, AAA preset library, physical weather simulation, and artist workflow.
- [ ] Require all optional dependency legal records and notices to be current.
- [ ] Require full `tools/validate.ps1` after code/docs/manifest/static checks settle.
- [ ] Require hosted PR checks for the selected platforms and host-gated blocker evidence for unavailable platforms.
- [ ] Set `environment_commercial_ready=1` only in the closeout slice.
- [ ] Set all adjacent broad non-claims explicitly when they remain unsupported.
- [ ] Return `currentActivePlan` to the production master plan or select the next dated plan in the same closeout change.

**Done when:** The commercial aggregate is promoted by evidence, not assertion, and the active plan stack is clean after closeout.

## Validation Ladder

Docs-only plan creation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Manifest selection or validation-recipe changes:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Dependency and asset-pipeline changes:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Runtime, renderer, backend, package, or public-contract changes:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Publication gate for every PR-sized phase:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

## Completion Boundaries

This plan is complete only when Phase 12 closes. Partial phases may land as PR-sized checkpoints, but each checkpoint must keep unsupported adjacent claims explicit and must not broaden readiness wording. A phase may be marked host-gated only with the exact missing host, SDK, tool, account, signing, device, driver, or CI evidence. A phase may be excluded only by updating this plan, registry, manifest fragments, docs, and static checks in the same reviewed change.

## Initial Plan-Creation Evidence

- Created from `origin/main` in an isolated worktree on 2026-06-13.
- Uses current completed environment evidence from `environment-production-excellence-v1`.
- Uses official-source and Context7 references for Vulkan, Metal, D3D12, OpenEXR, KTX/Basis, product environment authoring patterns, and vcpkg dependency gates.
- Does not select itself as active and does not claim any new runtime, backend, asset-pipeline, performance, simulation, platform, or artist-workflow readiness.
