# 2026-06-13 Environment Commercial Excellence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` after the operator explicitly selects this plan for implementation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `environment-commercial-excellence-v1`

**Status:** Active. Phase 0 selected the milestone, Phase 1 added the readiness taxonomy, Phase 2 added optional OpenEXR dependency/legal/bootstrap gates, Phase 3 slice 1 added descriptor-level asset/cook metadata contracts, Phase 3 slice 2 added optional-gated OpenEXR environment source metadata review APIs only, Phase 3 slice 3 added the hosted Windows `asset-importers` validation lane, Phase 3 slice 4 added real OpenEXR binary metadata review evidence, Phase 3 slice 5 added KTX2/Basis container metadata review APIs and real KTX2/Basis fixture evidence, Phase 3 slice 6 added device/backend texture format-selection policy rows for D3D12, Vulkan, macOS Metal, Android Vulkan, and iOS Metal, Phase 3 slice 7 added deterministic metadata-only `.geasset` planning, Phase 3 slice 8 added selected package-visible metadata-only environment texture package smoke evidence, Phase 4 slice 1 added the clean-break `GameEngine.EnvironmentPresetPack.v1` governance schema plus a first-party metadata-only preset pack with provenance/license/budget/static guards, Phase 4 slice 2 adds first-party editor/library browsing rows plus selected sample package consumption counters for that preset pack, Phase 5 slices 1-3 added strict Vulkan aggregate resource/toolchain/validation gates, Phase 6 slice 1 adds the Apple-host-only Metal environment aggregate recipe contract, Phase 7 slice 1 adds the value-only `EnvironmentBackendParityRequest` / `plan_environment_backend_parity` matrix foundation, Phase 7 slice 2 adds the package-visible `desktop-runtime-sample-game-environment-backend-parity` review recipe with `host_evidence_required`, Phase 8 slice 1 adds exact `environmentPlatformReadinessRows` plus package-visible `desktop-runtime-sample-game-environment-platform-readiness` counters, Phase 9 slice 1 adds the value-only `EnvironmentOptimizationMeasurementRequest` / `plan_environment_optimization_measurement` contract plus one D3D12 `preset_pack_flythrough` row, Phase 9 slice 2 adds direct D3D12 `storm_precipitation` before/after package counters, Phase 9 slice 3 adds direct D3D12 `dense_volumetric_fog` before/after package counters, Phase 9 slice 4 adds direct D3D12 `volumetric_cloud_sunset` before/after package counters, Phase 9 slice 5 adds direct D3D12 `snowfield_material_weathering` before/after package counters, Phase 9 slice 6 adds direct D3D12 `weather_simulation_stress` before/after package counters, Phase 9 slice 7 adds direct D3D12 `asset_library_cold_load` before/after package counters to the same package-visible optimization measurement recipe, Phase 10 slice 1 adds the deterministic `MK_environment` CPU reference weather simulation foundation with water-conservation tests and explicit non-claim guards, and Phase 10 slice 2 adds selected package-visible CPU reference counters through `desktop-runtime-sample-game-environment-weather-simulation-package`. This does not claim Windows-derived Metal host aggregate readiness, package-visible backend parity readiness, unconditional all-platform readiness, broad optimization readiness, full OpenEXR/KTX/Basis asset-pipeline readiness, AAA preset-library readiness, complete physical weather simulation readiness, or artist-workflow readiness.

**Goal:** Promote the completed selected D3D12-primary environment aggregate into a commercial environment capability set with strict Vulkan aggregate readiness, Apple-host Metal aggregate readiness, backend parity, exact all-platform readiness rows, measured broad optimization evidence, a licensed AAA preset asset library, OpenEXR/KTX/Basis production asset pipeline, physically modeled weather simulation, and advanced artist workflow support. Every claim must be backed by backend-local package-visible counters, validation recipes, official-source constraints, dependency/legal records, and hosted or host-gated evidence. No broad claim may be inferred from another backend, host, sample, asset, or adjacent row.

**Architecture:** Keep `MK_environment` as the backend-neutral value, profile, validation, weather, quality, asset-reference, and readiness-contract layer. Keep content import and cooking in `MK_tools` under `engine/tools/{asset,scene,shader}` with public headers in `engine/tools/include/mirakana/tools/`. Keep renderer policy in `MK_renderer` and GPU execution in private backend implementations for `MK_rhi_d3d12`, `MK_rhi_vulkan`, and Apple-host-gated `MK_rhi_metal`. Keep first-party editor workflow in `MK_editor_core` and the visible `MK_editor` shell. Put all optional third-party codec dependencies behind vcpkg manifest features and `tools/bootstrap-deps.ps1`. Expose no public native OS, GPU, texture, command queue, or dependency handles. Use clean-break schemas only; do not add compatibility parsers, deprecated aliases, or migration shims for older environment profile or asset-pipeline formats.

**Tech Stack:** C++23, PowerShell 7 repository tools, CMake presets through repository wrappers, vcpkg manifest features, `MK_environment`, `MK_renderer`, `MK_rhi`, `MK_tools`, `MK_editor_core`, `MK_editor`, HLSL, DXC DXIL, DXC SPIR-V CodeGen, SPIR-V validation, Vulkan 1.3 synchronization2 and dynamic rendering, Direct3D 12 barriers/descriptors/debug-layer/GPU-based-validation evidence, Apple Metal resource/texture/pipeline/synchronization evidence on Apple hosts, OpenEXR for HDR scene-linear source images, KTX2/Basis Universal texture containers/transcoding, first-party asset-library metadata, and repository package/runtime validation recipes.

---

## Current Starting Point

Use these current facts as the baseline before implementing this plan:

- `environment-production-excellence-v1` is completed through selected D3D12-primary `environment_ready` aggregate evidence in PR #588 / merge commit `32f053be6ac3bba1948a1d40edf0132120e7adc7`.
- The current aggregate proves `environment_ready=1`, `environment_ready_profile_v2=1`, `environment_ready_d3d12_primary=1`, `environment_ready_vulkan_strict=0`, `environment_ready_metal_host=0`, `environment_ready_backend_parity=0`, `environment_ready_broad_optimization_claimed=0`, `environment_ready_native_handle_access=0`, and `environment_ready_diagnostics=0`.
- Current evidence claims selected D3D12-primary readiness, selected strict Vulkan aggregate readiness, exact platform-readiness row mapping, and seven D3D12 broad-optimization measurement rows only through their package-visible lanes. It still does not claim Metal host aggregate readiness, backend parity readiness, unconditional all-platform readiness, broad optimization readiness, OpenEXR/KTX/Basis production asset import, AAA preset asset-library readiness, physically complete weather simulation, or complete real-production artist workflow.
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
- Do not claim all-platform readiness as a single unconditional row. Use per-platform and per-backend rows such as `windows_d3d12`, `windows_vulkan`, `linux_vulkan`, `macos_metal`, `ios_metal`, and `android_vulkan`.
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
| `environment_platform_windows_vulkan_ready` | Windows Vulkan recipe with Vulkan SDK tools, validation layers, SPIR-V validation, strict runtime package smoke, driver/device-feature evidence, and package counters. | D3D12 evidence, Linux Vulkan evidence, Android Vulkan evidence, or compile-only Vulkan evidence. |
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
- [x] For OpenEXR, validate data window, display window, channel list, pixel type, multipart/deep/tiled flags, color metadata, and scene-linear intent through a bootstrapped `asset-importers` lane.
- [x] For KTX2/Basis, validate container metadata, levels, layers, faces, supercompression, Basis requirement, and intended sampler class.
- [x] Add device/backend format-selection policy rows for D3D12, Vulkan, Metal, Android Vulkan, and iOS Metal.
- [x] Cook deterministic `.geasset` metadata with source hash, license/provenance id, color space, compression/transcoding choice, mip count, memory estimate, and unsupported-host diagnostics.
- [x] Add package smokes proving selected EXR and KTX2/Basis metadata assets can be reviewed, installed, and referenced by environment packages without claiming pixel payload cooking.
- [x] Run focused asset tests and selected package smoke for metadata-only package evidence.
- [ ] Run optional dependency bootstrap validation before promoting binary decode/transcode or full payload-cooker behavior.
- [x] Run full `tools/validate.ps1` for the current C++ runtime/build/public-contract slice before publication.

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

Phase 3 slice 4 implementation notes:

- `MK_tools_tests` now links OpenEXR only when `MK_ENABLE_ASSET_IMPORTERS=ON` and compiles a test-only writer guarded by `MK_TESTS_HAS_ASSET_IMPORTERS`.
- The new asset-importers-only test writes a real 2x2 RGB FLOAT scanline OpenEXR file using the official `Header`, `ChannelList`, `OutputFile`, `FrameBuffer`, `Slice`, and `addChromaticities` APIs, then runs `review_openexr_texture_source_metadata` against that file.
- The test validates `GameEngine.TextureSource.v2` metadata for source path/hash/provenance/license, data window, display window, channel list, float32 pixel encoding, channel count, chromaticities, scene-linear intent, and explicit non-multipart/non-deep/non-tiled flags. It still does not decode OpenEXR pixels into engine texture payloads, transcode KTX/Basis data, cook `.geasset` bytes, run runtime package smokes, upload GPU textures, execute backend format selection, or promote `environment_asset_pipeline_openexr_ktx_basis_ready`.
- Local default validation on 2026-06-14 passed `tools/check-format.ps1`, `tools/check-ci-matrix.ps1`, `git diff --check`, `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_tools_tests`, and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests`. Local `tools/cmake.ps1 --preset asset-importers` remains blocked by the unbootstrapped local optional dependency tree at missing `SPNGConfig.cmake`; hosted PR Windows MSVC `asset-importers` evidence is required for this slice.

Phase 3 slice 5 implementation notes:

- `MK_tools` now exposes `Ktx2BasisTextureSourceReviewRequest`, `Ktx2BasisTextureSourceReviewResult`, `Ktx2BasisTextureSourceReviewDiagnostic`, `has_ktx2_basis_texture_source_review`, and `review_ktx2_basis_texture_source_metadata` for KTX2/Basis source metadata review.
- The adapter uses official KTX-Software APIs behind `MK_HAS_ASSET_IMPORTERS`: `ktxTexture2_CreateFromNamedFile`, public `ktxTexture`/`ktxTexture2` metadata fields, `ktxTexture2_NeedsTranscoding`, `ktxTexture2_GetTransferFunction_e`, `ktxTexture2_GetColorModel_e`, `ktxTexture_Destroy`, and `ktxErrorString`. It maps ETC1S/BasisLZ and UASTC container metadata into `GameEngine.TextureSource.v2`, validates dimensions, mip levels, layers, face count, Basis transcode requirement, transfer-function intent, color-space intent, sampler-class intent, and rejects non-Basis or unsupported KTX2 layouts without exposing libktx handles publicly.
- `MK_tools` links `KTX::ktx` only in the optional `asset-importers` lane. Default builds remain dependency-free and fail closed with `Ktx2BasisTextureSourceReviewDiagnosticCode::asset_importers_disabled`; `MK_tools_tests` covers that boundary.
- The asset-importers-only test writes a real 4x4 RGBA sRGB KTX2 ETC1S/BasisLZ file using official APIs `ktxTexture2_Create`, `ktxTexture_SetImageFromMemory`, `ktxTexture2_CompressBasis`, and `ktxTexture_WriteToNamedFile`, then runs `review_ktx2_basis_texture_source_metadata` against that file.
- The test validates `GameEngine.TextureSource.v2` metadata for source path/hash/provenance/license, width/height, sRGB color-space intent, color sampler class, `VK_FORMAT_UNDEFINED`, level/layer/face counts, BasisLZ supercompression, ETC1S codec, and required transcoding. It still does not transcode Basis data, decode KTX pixels into engine texture payloads, cook `.geasset` bytes, run runtime package smokes, upload GPU textures, execute backend device-format selection, execute KTX command-line tools, or promote `environment_asset_pipeline_openexr_ktx_basis_ready`.
- Local default validation on 2026-06-14 passed `tools/check-format.ps1`, `git diff --check`, `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_tools_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests`, and full `tools/validate.ps1` with 121/121 tests. Local `tools/cmake.ps1 --preset asset-importers` remains blocked by the unbootstrapped local optional dependency tree at missing `SPNGConfig.cmake`; hosted PR Windows MSVC `asset-importers` evidence is required for the real KTX2/Basis fixture lane.

Phase 3 slice 6 implementation notes:

- `MK_tools` now exposes `TextureBackendFormatEvidenceRowV1`, `TextureBackendFormatPolicyRequestV1`, `TextureBackendFormatPolicyResultV1`, `TextureBackendFormatPolicyDiagnostic`, and `plan_texture_backend_format_policy_v1` for value-only backend texture format policy planning.
- The policy requires all five commercial backend rows: D3D12, Vulkan, macOS Metal, Android Vulkan, and iOS Metal. D3D12 evidence must name `ID3D12Device::CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT)`, Vulkan rows must name `vkGetPhysicalDeviceFormatProperties2`, and Metal rows must name Apple `Metal Feature Set Tables` plus `MTLPixelFormat`.
- OpenEXR environment radiance sources select RGBA16 float device formats (`DXGI_FORMAT_R16G16B16A16_FLOAT`, `VK_FORMAT_R16G16B16A16_SFLOAT`, and `MTLPixelFormatRGBA16Float`) with offline policy transcode metadata. KTX2/Basis sources select BC7 for D3D12/desktop Vulkan and ASTC 4x4 for macOS Metal, Android Vulkan, and iOS Metal, matching the official KTX guidance that Basis transcode targets must be chosen from GPU/backend capabilities before upload.
- Tests cover fail-closed missing host evidence and a fully host-validated KTX2/Basis policy row set with deterministic `GameEngine.CookedTextureMetadata.v1` decisions. This still does not call backend APIs, transcode Basis data, decode pixels, write cooked `.geasset` payloads, run package smokes, upload GPU textures, prove backend/platform readiness, or promote `environment_asset_pipeline_openexr_ktx_basis_ready`.
- Local evidence on 2026-06-14 passed `tools/prepare-worktree.ps1`, `tools/format.ps1`, `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_tools_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, `tools/check-json-contracts.ps1`, `git diff --check`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-ci-matrix.ps1`, and full `tools/validate.ps1` with 121/121 tests.

Phase 3 slice 7 implementation notes:

- `MK_assets` now exposes `EnvironmentTextureGeassetMetadataDocumentV1`, `is_valid_environment_texture_geasset_metadata_document_v1`, and `serialize_environment_texture_geasset_metadata_document_v1` for deterministic `GameEngine.EnvironmentTextureGeassetMetadata.v1` rows.
- `MK_tools` now exposes `EnvironmentTextureGeassetMetadataRequestV1`, `EnvironmentTextureGeassetMetadataResultV1`, `EnvironmentTextureGeassetMetadataDiagnostic`, and `plan_environment_texture_geasset_metadata_v1` for metadata-only environment texture `.geasset` planning.
- The metadata document requires a safe package-relative `.geasset` path, valid `GameEngine.CookedTextureMetadata.v1` input, `metadata_only=true`, source hash, provenance id, license id, source kind, color space, sampler class, deterministic mip count, source/decoded/max GPU byte estimates, all five backend policy rows, compression/transcode/device-format choices, support/host-validation flags, and unsupported-host diagnostic count.
- Tests cover deterministic asset serialization, safe path rejection, planner output from fail-closed backend policy metadata, and preservation of unsupported-host diagnostics. This still does not decode EXR/KTX pixels, transcode Basis data, write cooked texture payload bytes, install package assets, run package smokes, call backend APIs, upload GPU textures, prove backend/platform readiness, or promote `environment_asset_pipeline_openexr_ktx_basis_ready`.
- TDD evidence on 2026-06-14: after tests and declarations were added first, `tools/cmake.ps1 --build --preset dev --target MK_asset_environment_source_pipeline_tests MK_tools_tests` failed at link time on missing `is_valid_environment_texture_geasset_metadata_document_v1` / `serialize_environment_texture_geasset_metadata_document_v1`; after implementation, the same build passed and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_asset_environment_source_pipeline_tests|MK_tools_tests"` passed 2/2 tests.
- Local closeout evidence on 2026-06-14 passed `tools/format.ps1`, `tools/cmake.ps1 --build --preset dev --target MK_asset_environment_source_pipeline_tests MK_tools_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_asset_environment_source_pipeline_tests|MK_tools_tests"`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, `tools/check-json-contracts.ps1`, `git diff --check`, `tools/check-public-api-boundaries.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-ci-matrix.ps1`, and full `tools/validate.ps1` with 121/121 tests.

Phase 3 slice 8 implementation notes:

- `sample_desktop_runtime_game` now ships selected metadata-only environment texture package records for `environment_radiance_exr.texture.geasset` and `environment_skybox_basis.texture.geasset`, and the runtime package index records `environment_texture` dependency edges from the environment profile to both texture metadata records.
- `MK_assets` package and source registry parsing now understand the clean-break `environment_texture` dependency kind only for environment-profile to texture-package relationships. This is a package dependency contract; it does not load source images at runtime or expose dependency handles.
- The selected desktop package smoke flag `--require-environment-texture-asset-pipeline-package` requires two package entries, two metadata-only `GameEngine.EnvironmentTextureGeassetMetadata.v1` records, one OpenEXR source-kind row, one KTX2/Basis source-kind row, two source-hash rows, two provenance rows, two license rows, ten backend policy rows, eight unsupported-host diagnostics, two profile dependency references, two package dependency edges, and zero pixel decode, Basis runtime transcode, GPU upload, or broad-ready counters.
- `validate-installed-desktop-runtime.ps1`, `game.agent.json`, static AI/json checks, and package smoke defaults now fail closed if the selected environment texture metadata records or counters drift.
- Focused TDD evidence on 2026-06-14: `MK_assets_tests` first failed because `AssetDependencyKind::environment_texture` did not exist, then passed after the package/source registry dependency contract was implemented. A selected package smoke with `--require-environment-texture-asset-pipeline-package` passed through `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- Local closeout evidence on 2026-06-14 passed `tools/check-format.ps1`, `tools/check-text-format.ps1`, `git diff --check`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-ci-matrix.ps1`, the default `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` package smoke, and full `tools/validate.ps1` with 121/121 tests.
- This slice still does not decode EXR/KTX pixels, transcode Basis data into runtime GPU formats, write cooked texture payload bytes, call backend APIs, upload GPU textures, prove backend/platform readiness, or promote `environment_asset_pipeline_openexr_ktx_basis_ready`.

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

- [x] Define preset-pack schema `GameEngine.EnvironmentPresetPack.v1` with provenance id, art direction, dependencies, quality tier, package budget, memory budget, and required backend feature rows.
- [x] Create first-party or fully licensed presets for at least clear noon, overcast storm, night moonlit, snowfield, foggy valley, cinematic sunset, and indoor-to-outdoor transition.
- [x] Each preset must include authored environment profile, sky/atmosphere values, fog/cloud/weather timeline, IBL references, material-weathering references, audio trigger intent, and quality budget.
- [x] Each preset must include package size, installed size, decoded memory estimate, GPU memory estimate, and expected validation recipe.
- [x] Add editor/library browsing rows and package sample consumption evidence.
- [x] Add static checks that fail unlicensed or provenance-missing shipped preset assets.
- [x] Run package smoke, legal notice checks, focused tests, and full validation if runtime/package contracts changed.

Phase 4 slice 1 implementation notes:

- Official source refresh on 2026-06-14 used Context7 product references for Unity HDRP Visual Environment, Godot Environment/post-processing and volumetric fog, and Unreal Engine environment-lighting/fog/sky atmosphere official pages. The implementation implication is a metadata governance boundary for named authoring presets: sky/atmosphere, fog/cloud/weather timeline, IBL, material-weathering, audio intent, quality tier, backend feature rows, provenance, and budget metadata are required before content can be treated as a library candidate.
- `MK_environment` now exposes `EnvironmentPresetPackDocumentV1`, `EnvironmentPresetPackPresetV1`, `validate_environment_preset_pack_v1`, `serialize_environment_preset_pack_v1`, and `deserialize_environment_preset_pack_v1` for deterministic `GameEngine.EnvironmentPresetPack.v1` validation and text IO.
- `MK_assets` package identity now recognizes `AssetKind::environment_preset_pack`, and `sample_desktop_runtime_game` ships `runtime/assets/desktop_runtime/environment_presets.gepresetpack` plus a `.geindex` package entry for that metadata artifact.
- The first pack is first-party `LicenseRef-Proprietary` metadata. It contains seven named presets, pack and preset budget rows, `provenance.environment.sample_commercial_presets`, `environment_platform_windows_d3d12_ready` as the required backend feature row, and references to existing environment profile, IBL, material-weathering, audio-trigger, and validation recipe ids.
- Static checks in `tools/check-json-contracts-050-generated-games.ps1` and `tools/check-ai-integration-070-production-ledger.ps1` fail closed if the pack file, package index row, format id, provenance id, license id, budget rows, backend feature row, or seven required preset ids drift.
- Focused TDD evidence on 2026-06-14: `MK_environment_tests` first failed because `mirakana/environment/environment_preset_pack.hpp` did not exist, then passed after the schema, validator, serializer, and parser were implemented; `MK_assets_tests` covers `environment_preset_pack` package identity round trip.
- Local closeout evidence on 2026-06-14 passed `tools/cmake.ps1 --build --preset dev --target MK_environment_tests MK_assets_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_environment_tests|MK_assets_tests"`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, `git diff --check`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-ci-matrix.ps1`, `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, and full `tools/validate.ps1` with 121/121 tests.
- Phase 4 slice 1 did not add editor/library browsing, preset preview UI, sample scene consumption counters, install-package smoke counters, third-party preset assets, marketplace licensing, real production artist workflow, or `environment_aaa_preset_library_ready` promotion.

Phase 4 slice 2 implementation notes:

- `MK_editor_core` now exposes `EnvironmentPresetLibraryDesc`, `EnvironmentPresetLibraryModel`, `make_environment_preset_library_model`, `make_environment_preset_library_ui_model`, and `make_environment_preset_library_package_candidate_rows` for read-only first-party preset-library browsing rows over `EnvironmentPresetPack.v1` documents.
- The retained editor rows include pack id, provenance id, license id, art direction, quality tier, package/installed/decoded/GPU budget rows, package index and runtime path evidence, sample consumption evidence, per-preset ids/profile refs/quality tiers/validation recipe ids, and the explicit `environment.readiness.unsupported.environment_aaa_preset_library_ready` non-claim row.
- `sample_desktop_runtime_game` now accepts `--require-environment-preset-library-package` and reports package-visible counters: `environment_preset_library_package_status=ready`, `environment_preset_library_package_ready=1`, package index/file presence, seven preset rows, seven required preset rows, one backend feature row, three dependency refs, `environment_preset_library_sample_consumption_evidence=1`, `environment_preset_library_aaa_ready_claimed=0`, and zero diagnostics.
- Static checks in `tools/check-json-contracts-050-generated-games.ps1` and `tools/check-ai-integration-070-production-ledger.ps1` now require the new runtime flag, recipe, counter fields, and installed-runtime validation guards in the sample manifest, runtime source, and `tools/validate-installed-desktop-runtime.ps1`.
- Focused TDD evidence on 2026-06-14: `MK_editor_environment_tests` first failed because the preset-library editor model and `EnvironmentPackageCandidateKind::preset_pack` did not exist, then passed after the read-only editor rows and candidate rows were implemented. Source-tree package counter smoke passed after matching the runtime asset key to `environment/presets/sample_commercial_pack`.
- Installed package smoke evidence on 2026-06-14 passed `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` with `--require-environment-preset-library-package`, installed `runtime/assets/desktop_runtime/environment_presets.gepresetpack`, and reached `installed-desktop-runtime-validation: ok (sample_desktop_runtime_game)` plus `desktop-runtime-package: ok (sample_desktop_runtime_game)`.
- Local closeout validation on 2026-06-14 passed `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, `git diff --check`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-ci-matrix.ps1`, and full `tools/validate.ps1` with 121/121 tests.
- This slice still does not add visual preset preview rendering, third-party preset assets, marketplace licensing, external asset notices, real production artist workflow, full AAA preset-library closeout, or `environment_aaa_preset_library_ready` promotion.

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

- [x] Refresh Context7 `/khronosgroup/vulkan-docs` and official Khronos pages before implementation.
- [x] Define `desktop-runtime-sample-game-environment-vulkan-strict-aggregate` recipe.
- [x] Require Vulkan SDK tools, DXC SPIR-V CodeGen, `spirv-val`, synchronization2, dynamic rendering, validation layers, and strict package smoke.
- [x] Prove image usage/layout compatibility for every selected attachment, sampled texture, storage buffer, cube map, weather texture, froxel buffer, and readback resource exposed by the strict Vulkan aggregate recipe.
- [x] Record positive synchronization2 barrier counters for transfer-to-shader, compute-to-shader, attachment-to-shader, and readback transitions where used.
- [x] Record descriptor set binding counters for all selected features.
- [x] Record positive draw, dispatch, upload, and readback counters for the selected aggregate.
- [x] Record zero validation diagnostics, zero native-handle access, zero D3D12 fallback, and zero Metal fallback.
- [x] Add fail-closed tests for missing Vulkan toolchain, missing validation layer, missing SPIR-V validation, and unsupported feature/device rows.

**Phase 5 slice 1 evidence (2026-06-14):** `desktop-runtime-sample-game-environment-vulkan-strict-aggregate` is a reviewed `vulkan-strict` host-gated recipe using fixed package `SmokeArgs` and `--require-environment-vulkan-strict-aggregate`. It aggregates selected strict Vulkan physical sky, height fog, IBL, volumetric fog, volumetric cloud, rain precipitation, postprocess/depth input, profile v2, and quality-budget counters into `environment_vulkan_strict_aggregate_ready=1` with `environment_vulkan_strict_aggregate_descriptor_set_bindings=15`, positive synchronization2 barrier/draw/dispatch/upload/readback counters, and zero diagnostics/native-handle/D3D12-fallback/Metal-fallback/backend-parity/broad-optimization counters.

**Phase 5 slice 2 evidence (2026-06-14):** The same strict aggregate now fail-closes on selected resource usage/layout proof rows: `environment_vulkan_strict_aggregate_resource_usage_layout_ready=1`, `environment_vulkan_strict_aggregate_resource_usage_layout_rows=20`, attachment rows `2`, sampled texture rows `6`, storage buffer rows `2`, cube map rows `1`, weather texture rows `3`, froxel buffer rows `1`, and readback resource rows `5`. This is package-visible proof for the selected aggregate resources only; slice 3 adds the strict toolchain, validation-layer, SPIR-V validation, and device-feature gates.

**Phase 5 slice 3 evidence (2026-06-14):** The strict aggregate now fail-closes on package-visible toolchain and device-feature rows: `environment_vulkan_strict_aggregate_toolchain_ready=1`, `environment_vulkan_strict_aggregate_vulkan_sdk_tools_ready=1`, `environment_vulkan_strict_aggregate_dxc_spirv_codegen_ready=1`, `environment_vulkan_strict_aggregate_spirv_validation_ready=1`, `environment_vulkan_strict_aggregate_validation_layers_ready=1`, `environment_vulkan_strict_aggregate_device_features_ready=1`, `environment_vulkan_strict_aggregate_toolchain_rows=6`, and zero missing toolchain, missing validation-layer, missing SPIR-V validation, and unsupported feature/device rows. The runtime host requests `VK_LAYER_KHRONOS_validation` for the strict aggregate scene path, reports validation-layer enablement plus dynamic-rendering and synchronization2 device-feature enablement, and the installed package validation script requires every row exactly. Local TDD evidence: the recipe first failed with the new expected fields absent, then passed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe desktop-runtime-sample-game-environment-vulkan-strict-aggregate -HostGateAcknowledgements vulkan-strict`. This still does not claim Linux Vulkan, Android Vulkan, Metal, backend parity, all-platform readiness, broad optimization, physical simulation, full asset-pipeline readiness, AAA preset-library readiness, artist workflow readiness, broad `environment_ready`, or commercial environment readiness.

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

- [x] Refresh Apple Metal resource, synchronization, texture, shader, and capability documentation before implementation.
- [x] Define `renderer-metal-environment-aggregate-apple-host-evidence` recipe.
- [x] Require macOS AppleClang/Xcode toolchain, compiled Metal libraries, real device or approved CI Metal path, and package-visible host evidence before any aggregate counter is emitted.
- [x] Prove render and compute pipeline creation for selected sky, fog, cloud, precipitation, and IBL rows through the delegated Apple-host renderer Metal evidence recipe.
- [x] Prove texture/cube/HDR/depth/particle resources with explicit synchronization/readback evidence through the delegated Apple-host renderer Metal evidence recipe.
- [x] Record positive command queue/buffer, pipeline, texture, synchronization, draw/dispatch, and readback counters in the dedicated aggregate recipe output.
- [x] Record zero diagnostics, zero native-handle access, zero Vulkan fallback, and zero D3D12 fallback in the dedicated aggregate recipe output.
- [x] Add host-gated blockers for non-Apple hosts and fail-closed tests for compile-only evidence attempting to promote readiness.

**Phase 6 slice 1 evidence (2026-06-14):** Official-source refresh used Apple Metal resource fundamentals, resource synchronization, render/compute pipeline documentation, `MTLBlitCommandEncoder` synchronization documentation, Apple Metal Feature Set Tables dated 2026-05-21, and Context7 Metal Shading Language specification notes for vertex/fragment/kernel entry points plus offline compiler options. The new `renderer-metal-environment-aggregate-apple-host-evidence` recipe is host-gated behind `metal-apple`, runs `tools/validate-environment-metal-host-aggregate.ps1`, delegates to `renderer-metal-apple-host-evidence`, and emits `environment_metal_host_aggregate_ready=1` only after Apple-host renderer Metal evidence succeeds. The package-visible counters include selected backend, host validation recipe id, renderer recipe id, runtime, command queue, command buffer, metallib, render pipeline, compute pipeline, render pass, seven feature rows, four resource rows, cube/HDR/depth/particle resources, synchronization evidence, render and compute readback, zero diagnostics, zero native-handle access, zero Vulkan fallback, zero D3D12 fallback, zero backend-parity readiness, zero broad optimization readiness, and zero commercial readiness. Local Windows evidence remains host-gated and does not promote Metal readiness.

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

- [x] Define parity rows for feature presence, quality tier, resource class, output tolerance, package counters, unsupported rows, and diagnostics.
- [x] Require D3D12 primary, strict Vulkan aggregate, and Apple-host Metal aggregate rows to be fresh for the same profile and preset pack revision.
- [x] Define tolerance classes for scalar counters, image readback hashes, visual metrics, and backend-specific compression formats.
- [x] Fail parity when any backend silently disables a selected feature, uses an unsupported fallback, exceeds quality budgets, or omits diagnostics.
- [x] Record `environment_backend_parity_ready=1` only when every selected parity row passes.
- [x] Keep non-selected platforms host-gated rather than parity-ready.

**Phase 7 slice 1 evidence (2026-06-14):** Official-source refresh used Microsoft Direct3D 12 capability-query/resource-state guidance, Khronos Vulkan 1.3 dynamic rendering, synchronization2, and format-property guidance through Context7, plus Apple Metal Feature Set Tables dated 2026-05-21 and `MTLDevice supportsFamily` guidance. The new backend-neutral `EnvironmentBackendParityRequest` / `plan_environment_backend_parity` value API requires D3D12 primary, strict Vulkan aggregate, and Apple-host Metal rows for the same `GameEngine.EnvironmentProfile.v2`, `GameEngine.EnvironmentPresetPack.v1`, package revision, quality tier, quality-budget class, resource class, output tolerance class, normalized feature id set, package counter id set, counter semantics, and unsupported-row list. It fail-closes on missing required backend rows, stale profile or preset revisions, feature-id mismatch, resource/tolerance mismatch, package counter omission, counter-semantics mismatch, unsupported fallback, native-handle access, cross-backend inference, and nonzero diagnostics; non-selected platform rows stay host-gated. `MK_env_backend_parity_tests` covers ready, missing Metal, feature-count false positives, quality/resource/tolerance/counter mismatches, diagnostics/native-handle/fallback rejection, and host-gated Metal. This is the parity matrix foundation only; package-visible `environment_backend_parity_ready=1`, all-platform readiness, broad optimization, commercial readiness, and broad `environment_ready` remain unclaimed until an actual package recipe feeds fresh backend rows through this matrix.

**Phase 7 slice 2 evidence (2026-06-14):** `desktop-runtime-sample-game-environment-backend-parity` adds a package-visible review recipe and `--require-environment-backend-parity` smoke flag. The sample feeds `plan_environment_backend_parity` with 21 rows: D3D12 primary ready rows from the selected D3D12 aggregate, strict Vulkan ready rows referencing the reviewed strict aggregate recipe, and Apple-host Metal host-gated rows. The installed validator requires `environment_backend_parity_status=host_evidence_required`, `environment_backend_parity_ready=0`, `environment_backend_parity_required_backends=3`, `environment_backend_parity_required_features=7`, `environment_backend_parity_rows=21`, `environment_backend_parity_ready_rows=14`, `environment_backend_parity_host_gated_rows=7`, `environment_backend_parity_host_validated_backends=2`, `environment_backend_parity_d3d12_primary=1`, `environment_backend_parity_vulkan_strict=1`, `environment_backend_parity_metal_host=0`, `environment_backend_parity_requires_metal_host_evidence=1`, zero diagnostics, zero native-handle access, zero invoked GPU commands, and a positive replay hash. This is package-visible host-gated parity review only; `environment_backend_parity_ready=1`, all-platform readiness, commercial readiness, broad optimization readiness, and broad `environment_ready` remain unclaimed until Apple-host Metal ready rows are fed through the matrix.

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

- [x] Define exact rows for Windows D3D12, Windows Vulkan, Linux Vulkan, macOS Metal, iOS Metal, Android Vulkan, and deliberately unsupported unconditional all-platform readiness.
- [x] For each row, record required host OS, SDK, compiler, runtime sample, backend, feature set, package smoke, profiler or validation tools, and blocker reason.
- [x] Add CI recipe mappings only for platforms actually available in hosted checks.
- [x] Add host-gated blocker language for signing, simulator/device, GPU driver, Android SDK/NDK, Vulkan SDK, Xcode, and platform-specific package tools.
- [x] Fail closed when a platform row is missing, stale, or inferred from another platform.
- [x] Keep `environment_all_platform_unconditional_ready=0` as a permanent non-claim unless a future architecture decision replaces the row model.

**Phase 8 slice 1 evidence (2026-06-14):** Official-source refresh used Context7 `/khronosgroup/vulkan-docs` for Vulkan validation layers, synchronization2, dynamic rendering, and SPIR-V validation expectations, Microsoft Direct3D 12 resource-barrier/debug-layer/GBV guidance, Apple Metal resource/synchronization guidance, and Android GameActivity/Vulkan guidance. The manifest now owns `environmentPlatformReadinessRows` for Windows D3D12 ready, Windows Vulkan host-gated, Linux Vulkan host-gated, macOS Metal host-gated, iOS Metal host-gated, Android Vulkan host-gated, and unconditional all-platform unsupported. `desktop-runtime-sample-game-environment-platform-readiness` / `--require-environment-platform-readiness` emits `environment_platform_readiness_status=host_evidence_required`, `environment_platform_readiness_ready=0`, 6 package-visible platform rows, 1 ready row, 5 host-gated rows, `environment_platform_windows_d3d12_ready=1`, every non-D3D12 platform ready counter `0`, `environment_all_platform_unconditional_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. Static guards require the schema field, exact claim matrix row for `environment_platform_windows_vulkan_ready`, exact platform row count/content, recipe allowlist, sample manifest quality gate, installed validator fields, and docs/manifest guidance needles. This slice still does not claim backend parity readiness, unconditional all-platform readiness, commercial readiness, broad optimization readiness, broad `environment_ready`, full asset-pipeline readiness, physical weather simulation readiness, or complete artist-workflow readiness.

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

- [x] Use `performance-optimization-change` before implementation.
- [x] Define workloads: preset-pack flythrough, storm precipitation, dense volumetric fog, volumetric cloud sunset, snowfield material weathering, weather simulation stress, and asset-library cold load.
- [x] For each workload, define host, CPU, GPU, driver, OS, backend, resolution, frame count, warmup count, quality tier, and package revision as required measurement-row fields and host-gated manifest rows.
- [x] Capture before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, and stutter budget for all selected D3D12 workload rows.
- [x] Capture one D3D12 `preset_pack_flythrough` package row with before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, stutter budget, and profiler-artifact evidence.
- [x] Capture one D3D12 `storm_precipitation` package row with before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, stutter budget, and profiler-artifact evidence.
- [x] Capture one D3D12 `dense_volumetric_fog` package row with before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, stutter budget, and profiler-artifact evidence.
- [x] Capture one D3D12 `volumetric_cloud_sunset` package row with before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, stutter budget, and profiler-artifact evidence.
- [x] Capture one D3D12 `snowfield_material_weathering` package row with before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, stutter budget, and profiler-artifact evidence.
- [x] Capture one D3D12 `weather_simulation_stress` package row with before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, stutter budget, and profiler-artifact evidence.
- [x] Capture one D3D12 `asset_library_cold_load` package row with before and after CPU frame time, GPU frame time, memory peak, transient GPU bytes, upload bytes, draw count, dispatch count, barrier count, texture residency, package load time, stutter budget, and profiler-artifact evidence.
- [x] Use official host-tool requirements where available: Windows Performance Toolkit, PIX on Windows, D3D12 timing/query guidance, Vulkan timestamp/performance-query guidance, Xcode/Metal profiling tools, and repository counters.
- [x] Define regression thresholds and fail package validation when thresholds are exceeded on supported proof hosts.
- [x] Record `environment_broad_optimization_ready=1` only after all selected workloads have before/after measurements, regression guards, and backend parity; Phase 9 slice 7 keeps it at `0`.

**Phase 9 slice 1 evidence (2026-06-14):** Official-source refresh used Microsoft Windows Performance Recorder command-line guidance, PIX Timing Captures, Direct3D 12 timing/query/performance-measurement docs, Apple Xcode/Metal debugger and GPU optimization docs, and Context7 `/khronosgroup/vulkan-docs` for Vulkan timestamp and performance-query constraints. `MK_renderer` now exposes `EnvironmentOptimizationMeasurementRequest`, `EnvironmentOptimizationMeasurementRow`, `EnvironmentOptimizationRegressionBudget`, `EnvironmentOptimizationMeasurementPlan`, and `plan_environment_optimization_measurement`. The API is value-only, requires exact host/tool/workload/backend/package fields, rejects broad claims, native handles, GPU command invocation, duplicate/missing workloads, nonfinite metrics, missing before/after metrics, over-budget rows, missing profiler artifacts, cross-backend inference, and nonzero diagnostics. `MK_env_performance_tests` proves the fail-closed contract. `environmentOptimizationMeasurementWorkloadRows` now records seven exact workloads; only `environment_optimization_preset_pack_flythrough_d3d12` is ready, while the six remaining D3D12 workload rows are host-gated. `desktop-runtime-sample-game-environment-optimization-measurement` / `--require-environment-optimization-measurement` emits `environment_optimization_measurement_status=host_evidence_required`, `environment_optimization_measurement_ready=0`, `workload_rows=1`, `required_workloads=7`, `measured_workloads=1`, `before_after_pairs=1`, `backend=d3d12`, `profile=preset_pack_flythrough`, positive before/after metrics, `regression_budget_rows=1`, `over_budget=0`, `backend_parity_ready=0`, `environment_broad_optimization_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. This slice does not claim broad optimization readiness.

**Phase 9 slice 2 evidence (2026-06-15):** Official-source refresh used Microsoft Windows Performance Recorder command-line options, PIX Timing Captures, Direct3D 12 timing/query guidance, and Context7 `/khronosgroup/vulkan-docs` for timestamp/performance-query constraints. `EnvironmentOptimizationMeasurementPlan` now records `d3d12_storm_precipitation_measured` separately from `d3d12_preset_pack_flythrough_measured`. The package-visible `desktop-runtime-sample-game-environment-optimization-measurement` recipe still reports `host_evidence_required`, but now emits two ready D3D12 workload rows for `preset_pack_flythrough` and `storm_precipitation`: `workload_rows=2`, `required_workloads=7`, `measured_workloads=2`, `before_after_pairs=2`, `profiles=preset_pack_flythrough,storm_precipitation`, `regression_budget_rows=2`, `over_budget=0`, `backend_parity_ready=0`, `environment_broad_optimization_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. The storm row includes direct CPU/GPU/memory/transient/upload/draw/dispatch/barrier/texture-residency/package-load/stutter before/after counters and explicit budgets. Dense volumetric fog, volumetric cloud sunset, snowfield material weathering, weather simulation stress, asset-library cold load, backend parity, and broad optimization remain host-gated.

**Phase 9 slice 3 evidence (2026-06-15):** Official-source requirements continue to use Windows Performance Recorder, PIX Timing Captures, Direct3D 12 timestamp-query guidance, repository package counters, and the Context7-backed Vulkan timestamp/performance-query constraints retained from slice 2. `EnvironmentOptimizationMeasurementPlan` now records `d3d12_dense_volumetric_fog_measured` separately from the preset and storm rows. The package-visible `desktop-runtime-sample-game-environment-optimization-measurement` recipe still reports `host_evidence_required`, but now emits three ready D3D12 workload rows for `preset_pack_flythrough`, `storm_precipitation`, and `dense_volumetric_fog`: `workload_rows=3`, `required_workloads=7`, `measured_workloads=3`, `before_after_pairs=3`, `profiles=preset_pack_flythrough,storm_precipitation,dense_volumetric_fog`, `regression_budget_rows=3`, `over_budget=0`, `backend_parity_ready=0`, `environment_broad_optimization_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. The dense fog row includes direct CPU/GPU/memory/transient/upload/draw/dispatch/barrier/texture-residency/package-load/stutter before/after counters and explicit budgets. Volumetric cloud sunset, snowfield material weathering, weather simulation stress, asset-library cold load, backend parity, and broad optimization remain host-gated.

**Phase 9 slice 4 evidence (2026-06-15):** Official-source requirements continue to use Windows Performance Recorder, PIX Timing Captures, Direct3D 12 timestamp-query guidance, repository package counters, and the Context7-backed Vulkan timestamp/performance-query constraints retained from prior slices. `EnvironmentOptimizationMeasurementPlan` now records `d3d12_volumetric_cloud_sunset_measured` separately from the preset, storm, and dense fog rows. The package-visible `desktop-runtime-sample-game-environment-optimization-measurement` recipe still reports `host_evidence_required`, but now emits four ready D3D12 workload rows for `preset_pack_flythrough`, `storm_precipitation`, `dense_volumetric_fog`, and `volumetric_cloud_sunset`: `workload_rows=4`, `required_workloads=7`, `measured_workloads=4`, `before_after_pairs=4`, `profiles=preset_pack_flythrough,storm_precipitation,dense_volumetric_fog,volumetric_cloud_sunset`, `regression_budget_rows=4`, `over_budget=0`, `backend_parity_ready=0`, `environment_broad_optimization_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. The volumetric cloud sunset row includes direct CPU/GPU/memory/transient/upload/draw/dispatch/barrier/texture-residency/package-load/stutter before/after counters and explicit budgets. Snowfield material weathering, weather simulation stress, asset-library cold load, backend parity, and broad optimization remain host-gated.

**Phase 9 slice 5 evidence (2026-06-15):** Official-source requirements continue to use Windows Performance Recorder, PIX Timing Captures, Direct3D 12 timestamp-query guidance, repository package counters, and the Context7-backed Vulkan timestamp/performance-query constraints retained from prior slices. `EnvironmentOptimizationMeasurementPlan` now records `d3d12_snowfield_material_weathering_measured` separately from the preset, storm, dense fog, and cloud rows. The package-visible `desktop-runtime-sample-game-environment-optimization-measurement` recipe still reports `host_evidence_required`, but now emits five ready D3D12 workload rows for `preset_pack_flythrough`, `storm_precipitation`, `dense_volumetric_fog`, `volumetric_cloud_sunset`, and `snowfield_material_weathering`: `workload_rows=5`, `required_workloads=7`, `measured_workloads=5`, `before_after_pairs=5`, `profiles=preset_pack_flythrough,storm_precipitation,dense_volumetric_fog,volumetric_cloud_sunset,snowfield_material_weathering`, `regression_budget_rows=5`, `over_budget=0`, `backend_parity_ready=0`, `environment_broad_optimization_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. The snowfield material weathering row includes direct CPU/GPU/memory/transient/upload/draw/dispatch/barrier/texture-residency/package-load/stutter before/after counters and explicit budgets. Weather simulation stress, asset-library cold load, backend parity, and broad optimization remain host-gated.

**Phase 9 slice 6 evidence (2026-06-15):** Official-source requirements continue to use Windows Performance Recorder, PIX Timing Captures, Direct3D 12 timestamp-query guidance, repository package counters, and the Context7-backed Vulkan timestamp/performance-query constraints retained from prior slices. `EnvironmentOptimizationMeasurementPlan` now records `d3d12_weather_simulation_stress_measured` separately from the preset, storm, dense fog, cloud, and snowfield rows. The package-visible `desktop-runtime-sample-game-environment-optimization-measurement` recipe still reports `host_evidence_required`, but now emits six ready D3D12 workload rows for `preset_pack_flythrough`, `storm_precipitation`, `dense_volumetric_fog`, `volumetric_cloud_sunset`, `snowfield_material_weathering`, and `weather_simulation_stress`: `workload_rows=6`, `required_workloads=7`, `measured_workloads=6`, `before_after_pairs=6`, `profiles=preset_pack_flythrough,storm_precipitation,dense_volumetric_fog,volumetric_cloud_sunset,snowfield_material_weathering,weather_simulation_stress`, `regression_budget_rows=6`, `over_budget=0`, `backend_parity_ready=0`, `environment_broad_optimization_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. The weather simulation stress row includes direct CPU/GPU/memory/transient/upload/draw/dispatch/barrier/texture-residency/package-load/stutter before/after counters and explicit budgets. Asset-library cold load, backend parity, and broad optimization remain host-gated.

**Phase 9 slice 7 evidence (2026-06-15):** Official-source requirements continue to use Windows Performance Recorder, PIX Timing Captures, Direct3D 12 timestamp-query guidance, repository package counters, and the Context7-backed Vulkan timestamp/performance-query constraints retained from prior slices. `EnvironmentOptimizationMeasurementPlan` now records `d3d12_asset_library_cold_load_measured` separately from the six prior D3D12 rows. The package-visible `desktop-runtime-sample-game-environment-optimization-measurement` recipe still reports `host_evidence_required`, but now emits seven ready D3D12 workload rows for `preset_pack_flythrough`, `storm_precipitation`, `dense_volumetric_fog`, `volumetric_cloud_sunset`, `snowfield_material_weathering`, `weather_simulation_stress`, and `asset_library_cold_load`: `workload_rows=7`, `required_workloads=7`, `measured_workloads=7`, `before_after_pairs=7`, `profiles=preset_pack_flythrough,storm_precipitation,dense_volumetric_fog,volumetric_cloud_sunset,snowfield_material_weathering,weather_simulation_stress,asset_library_cold_load`, `regression_budget_rows=7`, `over_budget=0`, `backend_parity_ready=0`, `environment_broad_optimization_ready=0`, zero diagnostics/native-handle/GPU-command side effects, and a positive replay hash. The asset-library cold-load row includes direct CPU/GPU/memory/transient/upload/draw/dispatch/barrier/texture-residency/package-load/stutter before/after counters and explicit budgets. Backend parity and broad optimization remain host-gated.

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

- [x] Define the numerical model, state variables, units, grid/domain boundaries, timestep limits, forcing inputs, precipitation transfer, fog/cloud coupling, and stability constraints for the CPU reference foundation.
- [x] Add deterministic CPU reference tests for conservation or bounded-error properties, timestep clamping, seeded reproducibility, boundary behavior, and invalid input diagnostics.
- [x] Record whether the production simulation contract is deterministic, deterministic-per-host, or explicitly nondeterministic; do not mix these states.
- [ ] Add GPU acceleration only after CPU reference behavior and tests are green.
- [x] Add selected package counters for simulation steps, cells, timestep, moisture/error bounds, fallback activation, side-effect suppression, diagnostics, and replay hash.
- [ ] Add CPU/GPU time, profiler budget, and production solver package counters after the selected CPU reference package lane is green.
  - [x] Add selected CPU reference solver elapsed/budget package counters with explicit GPU solver, profiler artifact, and production solver non-readiness counters.
- [ ] Add artist-facing controls that author initial conditions and forces without exposing raw unstable solver internals.
- [ ] Add validation images or datasets for selected canonical conditions.

**Phase 10 slice 1 evidence (2026-06-15):** Official-source refresh used NOAA's water-cycle overview, NWS vapor-pressure calculator/formula, NOAA NCEI numerical-weather-prediction context, NASA's water-cycle reference, and Context7 `/kitware/cmake` for the CMake test registration pattern. `MK_environment` now exposes `EnvironmentWeatherSimulationDesc`, `EnvironmentWeatherSimulationCellState`, `EnvironmentWeatherSimulationCellForcing`, `EnvironmentWeatherSimulationPlan`, `environment_weather_saturation_vapor_kg_per_m2`, `simulate_environment_weather_cpu_reference`, and `has_environment_weather_simulation_diagnostic`. `MK_environment_weather_simulation_tests` proves deterministic CPU-only stepping, active-water conservation bounds, timestep clamping, replay hash stability, NWS vapor-pressure-derived saturation, invalid input diagnostics, and fail-closed unsupported GPU/backend/native/ready requests. `environmentPhysicalWeatherSimulationRows` records only `environment_weather_simulation_cpu_reference_foundation=ready`; `environment_physical_weather_simulation_ready` remains `unsupported` because package counters, CPU/GPU budgets, profiler evidence, GPU/backend integration, validation datasets/images, and artist controls are still pending.

**Phase 10 slice 2 evidence (2026-06-15):** Context7 `/kitware/cmake` was refreshed for `target_link_libraries(... PRIVATE ...)` usage before linking `sample_desktop_runtime_game` directly to `MK_environment`. `desktop-runtime-sample-game-environment-weather-simulation-package` and `--require-environment-weather-simulation-package` now run the deterministic CPU reference on a selected 2x2 package scenario and emit package-visible counters: `environment_weather_simulation_package_status=ready`, `environment_weather_simulation_package_ready=1`, `environment_weather_simulation_steps=1`, `environment_weather_simulation_cells=4`, `environment_weather_simulation_effective_timestep_ms=500`, `environment_weather_simulation_timestep_clamped=1`, positive total water/evaporation/condensation/precipitation counters, `environment_weather_simulation_water_conservation_error_mg<=1`, `environment_weather_simulation_water_conservation_error_bound_mg=1`, `environment_weather_simulation_max_cell_water_conservation_error_mg_per_m2=0`, `environment_weather_simulation_fallback_cpu_reference_used=1`, zero GPU/backend/native-handle side effects, zero diagnostics, and a positive replay hash. `environmentPhysicalWeatherSimulationRows.environment_weather_simulation_package_counters=ready` records this selected package evidence while `environment_physical_weather_simulation_ready` remains `unsupported` until CPU/GPU budgets, profiler evidence, GPU/backend integration, validation datasets/images, and artist controls land.

**Phase 10 slice 3 evidence (2026-06-15):** Context7 `/microsoftdocs/cpp-docs` was refreshed for `std::chrono::steady_clock::now()` and `duration_cast` elapsed-time measurement guidance before adding package-visible CPU reference solver budget counters. `MK_environment` now exposes `EnvironmentWeatherSimulationSolverBudgetDesc`, `EnvironmentWeatherSimulationSolverBudgetPlan`, `EnvironmentWeatherSimulationSolverBudgetStatus::host_evidence_required`, and `plan_environment_weather_simulation_solver_budget`; `MK_environment_weather_simulation_tests` proves CPU reference budget pass-through, over-budget rejection, missing package rejection, unsupported GPU solver rejection, and unsupported production solver ready claims. `desktop-runtime-sample-game-environment-weather-simulation-package` now emits `environment_weather_simulation_solver_budget_status=host_evidence_required`, `environment_weather_simulation_solver_budget_ready=0`, `environment_weather_simulation_cpu_reference_solver_ready=1`, `environment_weather_simulation_solver_cpu_elapsed_us`, `environment_weather_simulation_solver_cpu_budget_us=50000`, `environment_weather_simulation_solver_cpu_over_budget=0`, `environment_weather_simulation_gpu_solver_ready=0`, `environment_weather_simulation_solver_gpu_elapsed_us=0`, `environment_weather_simulation_solver_gpu_budget_us=0`, `environment_weather_simulation_solver_profiler_artifacts=0`, `environment_weather_simulation_profiler_budget_ready=0`, `environment_weather_simulation_production_solver_ready=0`, and `environment_weather_simulation_solver_budget_diagnostics=0`. `environmentPhysicalWeatherSimulationRows.environment_weather_simulation_solver_budget_counters=ready` records selected CPU budget evidence while GPU solver readiness, retained profiler artifacts, production solver readiness, validation datasets/images, artist controls, and `environment_physical_weather_simulation_ready=1` remain unsupported.

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
