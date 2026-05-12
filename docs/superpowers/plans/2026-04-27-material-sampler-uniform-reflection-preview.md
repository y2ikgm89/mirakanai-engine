# Material Sampler Uniform Reflection Preview Implementation Plan (2026-04-27)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the next material GPU binding slice: explicit sampler descriptors, material factor uniform upload, shader reflection metadata, scene/runtime binding resolution, and editor preview visibility.

**Architecture:** Keep shader/material intent in `MK_assets`, upload and descriptor resolution in `MK_runtime_rhi`, backend-neutral validation in `MK_rhi`, and GUI-independent preview state in `MK_editor_core`. Native backend work must keep D3D12/Vulkan/Metal handles private; this slice adds first-party contracts and Null/native compile-safe bridges without marking tool-gated reflection as complete.

**Tech Stack:** C++23, `MK_assets`, `MK_rhi`, `MK_runtime_rhi`, `MK_editor_core`, `NullRhiDevice`, CTest, PowerShell 7 validation recipes under `tools/`.

---

### Task 1: RHI Sampler Descriptor Contract

**Files:**
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Test: `tests/unit/rhi_tests.cpp`

- [x] Add failing coverage for creating a sampler and writing a sampler descriptor beside a sampled texture.
- [x] Add `SamplerHandle`, `SamplerDesc`, `DescriptorType::sampler`, `DescriptorResource::sampler`, and `IRhiDevice::create_sampler`.
- [x] Track `samplers_created` in `RhiStats`.
- [x] Implement strict Null validation for sampler descriptors and invalid sampler handles.
- [x] Keep native backends compile-safe and descriptor-aware without exposing native handles.

### Task 2: Material Metadata Emits Texture Plus Sampler Bindings

**Files:**
- Modify: `engine/assets/include/mirakana/assets/material.hpp`
- Modify: `engine/assets/src/material.cpp`
- Modify: `tests/unit/runtime_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add failing coverage that material pipeline metadata emits one uniform, one sampled texture, and one sampler binding for a textured material.
- [x] Add `MaterialBindingResourceKind::sampler`.
- [x] Reserve stable sampler binding numbers per texture slot.
- [x] Keep invalid duplicate and unknown-slot validation strict.

### Task 3: Runtime Material Factor Uniform Upload

**Files:**
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`
- Modify: `engine/runtime_rhi/src/runtime_upload.cpp`
- Test: `tests/unit/runtime_rhi_tests.cpp`

- [x] Add failing coverage that runtime material binding uploads base color, emissive, metallic, and roughness into the material uniform buffer.
- [x] Change `create_runtime_material_gpu_binding` to require `MaterialFactors`.
- [x] Upload factors through a copy-source staging buffer into a `uniform | copy_destination` material buffer.
- [x] Create or reuse a sampler for each texture slot and write sampler descriptors.

### Task 4: Shader Reflection Metadata Contract

**Files:**
- Modify: `engine/assets/include/mirakana/assets/shader_metadata.hpp`
- Modify: `engine/assets/src/shader_metadata.cpp`
- Modify: `editor/core/src/shader_artifact_io.cpp`
- Test: `tests/unit/editor_core_tests.cpp`

- [x] Add failing coverage that shader metadata can record descriptor reflection rows and reject duplicate set/binding records.
- [x] Add first-party reflection structs for set, binding, descriptor kind, stage, count, and semantic.
- [x] Preserve manifest IO for existing shader metadata while writing reflection rows deterministically.

### Task 5: Scene GPU Binding Resolution

**Files:**
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`
- Test: `tests/unit/scene_renderer_tests.cpp`

- [x] Add failing coverage that scene mesh/material asset ids resolve into renderer GPU binding payloads.
- [x] Add `SceneGpuBindingPalette` without making `MK_scene` depend on renderer, RHI, or backend handles.
- [x] Count resolved mesh and material GPU bindings in scene render submission results.

### Task 6: Editor Preview Integration

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_pipeline.hpp`
- Modify: `editor/core/src/asset_pipeline.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`

- [x] Add failing coverage that material preview exposes sampler-backed texture binding metadata and material uniform byte size.
- [x] Surface sampler rows in GUI-independent preview models.
- [x] Update the ImGui shell label helper for sampler resources.

### Task 7: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 8: Native Backend Sampler Mapping and Visible Proof

**Files:**
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/agent/manifest.json`
- Test: `tests/unit/d3d12_rhi_tests.cpp`
- Test: `tests/unit/backend_scaffold_tests.cpp`

- [x] Add D3D12 coverage for split CBV/SRV/UAV and sampler root descriptor tables.
- [x] Add D3D12 coverage for native sampler descriptor writes and invalid heap-kind mismatches.
- [x] Implement D3D12 sampler descriptor arena allocation, root-signature table splitting, and descriptor-set binding across both heap kinds.
- [x] Add D3D12 visible readback proof that samples a texture through a sampler descriptor.
- [x] Add Vulkan runtime sampler ownership and native sampled-image/sampler descriptor writes through private runtime descriptor update structures.
- [x] Add environment-gated Vulkan visible texture-sampling proof using `MK_VULKAN_TEST_TEXTURE_VERTEX_SPV` and `MK_VULKAN_TEST_TEXTURE_FRAGMENT_SPV`.
- [x] Update the agent manifest to remove native D3D12/Vulkan sampler descriptor mapping from explicit follow-up work.

### Task 9: Continuation Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 10: Runtime Material Preview and Vulkan Promotion Hardening

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_pipeline.hpp`
- Modify: `editor/core/src/asset_pipeline.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/editor.md`
- Modify: `docs/rhi.md`
- Modify: `docs/testing.md`

- [x] Add GUI-independent `EditorMaterialGpuPreviewPlan` coverage that loads cooked material and texture payloads for GPU binding preview upload.
- [x] Add diagnostics for missing cooked texture artifacts and invalid cooked texture payloads.
- [x] Add backend-neutral `RhiViewportSurface` proof that runtime texture upload plus sampler-aware runtime material descriptor binding can be submitted through the viewport render path.
- [x] Wire the selected material binding preview in `MK_editor` through `MK_runtime_rhi` texture uploads, `create_runtime_material_gpu_binding`, a dedicated preview `RhiViewportSurface`, and D3D12 shared texture or CPU readback display.
- [x] Invalidate the GPU binding preview cache on material selection changes, imports, recooks, and viewport device recreation.
- [x] Gate Vulkan `IRhiDevice` promotion on visible texture-sampling/readback proof, not only clear/draw readback proof.
- [x] Add environment-gated Vulkan `RhiFrameRenderer` runtime material texture-sampling readback proof using `MK_VULKAN_TEST_RUNTIME_MATERIAL_VERTEX_SPV` and `MK_VULKAN_TEST_RUNTIME_MATERIAL_FRAGMENT_SPV`.
- [x] Update manifest and docs to describe the implemented GPU binding preview path and material-preview-specific shader artifact compile/readiness path.
- [x] Add owner-device provenance for runtime material texture resources and material descriptor bindings, and reject cross-device material binding in runtime/renderer tests.
- [x] Allocate runtime material uniform buffers with 256-byte backing while keeping the material factor payload contract at 64 bytes.
- [x] Add material-preview-specific shader compile requests for factor-only and base-color-textured D3D12 DXIL/Vulkan SPIR-V artifacts, shader-id-specific artifact readiness queries, and editor D3D12 preview bytecode selection from the material preview artifacts instead of the default viewport shader.

**Known blockers:** Vulkan SPIR-V validation, DXC SPIR-V CodeGen, Metal tools, Apple/Xcode packaging, and strict clang-tidy compile database remain diagnostic-only or follow-up blockers until the required SDK tools/shader artifacts are installed and promoted. Vulkan editor material-preview display still depends on future native Vulkan viewport/display promotion.
