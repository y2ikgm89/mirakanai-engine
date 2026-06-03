# Environment System v1 Design Record

## Status

Active design record for the `environment-system-v1` milestone. This record supports the implementation plan in `docs/superpowers/plans/2026-05-26-environment-system-v1.md`; it is not a completion claim.

## Goal

Add a first-party MIRAIKANAI environment system for sky, sun, moon, ambient/reflection intent, fog, clouds, rain, snow, storms, time-of-day, weather presets, and quality tiers through clean C++23 engine contracts. The implementation is greenfield and does not preserve backward compatibility with ad hoc sky or lighting assumptions.

## Official Source Baseline

Use these sources as design anchors, not code to copy:

| Area | Primary sources | Engine implication |
| --- | --- | --- |
| Integrated environment lighting | Unreal Engine Environmental Light with Fog, Clouds, Sky and Atmosphere: <https://dev.epicgames.com/documentation/en-us/unreal-engine/environmental-light-with-fog-clouds-sky-and-atmosphere-in-unreal-engine> | Treat sky, fog, clouds, atmosphere, sun/moon lights, and sky lighting as a coordinated environment feature family, while keeping MIRAIKANAI contracts first-party and backend-neutral. |
| Sky model taxonomy | Unity HDRP Sky: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/sky.html> | Support explicit sky model families such as gradient, HDRI, and physical atmosphere instead of one catch-all sky setting. |
| Environment background and effects | Godot Environment and post-processing: <https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html> | Keep environment background, ambient/reflection intent, fog, glow/postprocess relationships, and renderer feature readiness as explicit rows rather than hidden scene globals. |
| Atmosphere rendering | Hillaire 2020, A Scalable and Production Ready Sky and Atmosphere Rendering Technique: <https://diglib.eg.org/items/8a3e5350-18b3-46bd-9274-3add5af88c75> | Physical atmosphere work needs reviewed parameter ranges, shader constants, sample budgets, and readback/package evidence before a ready claim. |
| D3D12 binding and barriers | Microsoft D3D12 resource binding overview and `ID3D12GraphicsCommandList::ResourceBarrier`: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding-flow-of-control>, <https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier> | Descriptor heaps, root signatures, command lists, resource states, fences, and barriers stay backend-private behind renderer/RHI plans and D3D12 evidence tests. |
| Vulkan synchronization | Vulkan synchronization examples: <https://docs.vulkan.org/guide/latest/synchronization_examples.html> | Vulkan readiness needs strict synchronization2/layout/barrier proof for selected resources; D3D12 proof must not imply Vulkan readiness. |
| HDR environment sources | OpenEXR documentation: <https://openexr.com/en/latest/index.html> | Optional EXR/HDR import is dependency, license, vcpkg, importer, package, and security evidence work. It is not part of the PR1 value contract. |
| CMake library export | CMake target-based `BUILD_INTERFACE` / `INSTALL_INTERFACE` and install/export documentation via Context7 `/kitware/cmake` | `MK_environment` uses target-scoped include/link usage requirements and participates in the existing install/export target list. |

## Clean-Break Rules

- No compatibility shims, deprecated aliases, duplicate environment APIs, or migration layers.
- No Dear ImGui, SDL3, UI middleware, native OS handles, backend handles, D3D12/Vulkan/Metal types, descriptor objects, command lists, or raw RHI resources in public environment APIs.
- No environment fields are added to `LightComponent`; scene lights remain scene lights.
- No broad `environment_ready` claim. Every ready row must name the exact feature, backend/host gate, validation recipe, package counter, and evidence.
- No Vulkan or Metal readiness may be inferred from D3D12 evidence.
- Optional OpenEXR/KTX-class support requires dependency/legal/vcpkg/package proof before implementation.

## Module Boundaries

| Module | Boundary |
| --- | --- |
| `MK_environment` | Owns backend-neutral value objects, validation, text IO, package rows, time-of-day/weather planning, and diagnostics. No GPU execution. |
| `MK_scene` | References environment profile assets in scene/runtime package metadata only. Does not absorb sky/weather into light components. |
| `MK_runtime_scene` | Resolves cooked environment references from packages after package rows exist. Does not parse source environment files at runtime. |
| `MK_scene_renderer` | Converts scene/runtime environment rows into renderer-neutral packets. No backend execution. |
| `MK_renderer` | Owns environment render policies, pass plans, quality budgets, shader contract layouts, and package-visible renderer evidence. |
| `MK_rhi` and backend modules | Own backend-neutral resource contracts and backend-private D3D12/Vulkan/Metal execution. Native handles stay private. |
| `MK_runtime_host_win32_presentation` | Reports package-visible selected environment status fields through the Windows host lane while keeping presentation resources private. |
| `MK_editor_core` / `MK_editor` | Own reviewed first-party retained authoring models and visible editor panel work after core models pass. No Dear ImGui or SDL3. |

## PR1 Contract

PR1 is the value-contract foundation only:

- `EnvironmentProfileDesc` and subdescs for sky model, weather kind, sun, moon, atmosphere, fog, and precipitation.
- Fail-closed `EnvironmentProfileValidationResult` and deterministic diagnostics.
- Batch validation for duplicate profile ids.
- Public API boundary coverage so environment headers cannot include scene, renderer, RHI, platform, runtime, UI, tools, editor, or backend modules.
- Target-based `MK_environment` CMake library and install/export wiring.

PR1 does not implement text IO, `AssetKind::environment_profile`, package rows, scene binding, runtime scene binding, render packets, shaders, D3D12/Vulkan/Metal proof, editor authoring, or package counters.

## Phase Order

1. `MK_environment` value profile validation.
2. Deterministic text IO, asset kind, and package rows.
3. Scene/runtime scene environment references.
4. Scene-renderer packets and renderer policy planning.
5. Physical sky and sky lighting shader contracts.
6. D3D12 proof and strict Vulkan host/toolchain proof for selected features.
7. Ambient/reflection/IBL policy.
8. Height fog proof.
9. Volumetric fog policy and proof.
10. Cloud layer.
11. Weather and precipitation rows, including rain/snow/wetness/audio handoff values.
12. Volumetric clouds and storm lighting.
13. Time-of-day and weather blending.
14. First-party editor-core authoring and visible retained editor panel.
15. Package validation recipes, manifest sync, and closeout.

## Optimization Boundary

The completed optimization foundation is inherited as narrow evidence only. Current environment work may rely on existing first-party diagnostics, job/scratch/memory/SIMD evidence boundaries, renderer package counters, and host gates, but it must not claim broad CPU/GPU/memory optimization, GPU async overlap, cross-vendor/backend parity, allocator enforcement, CUDA/HIP/SYCL runtime dependency, or automatic GPU residency without a focused measured environment slice.

## Validation

Minimum PR1 evidence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Full `tools/validate.ps1` is required before a C++/public-contract candidate is publication-ready.
