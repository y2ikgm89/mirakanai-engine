# Renderer Visible Mesh And Material Binding Implementation Plan (2026-04-27)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans for follow-up tasks. This slice was implemented in-session with TDD and read-only subagent exploration/review.

**Goal:** Promote the runtime RHI mesh/material bridge into renderer-visible command submission without exposing native graphics handles.

**Architecture:** Preserve scene-to-renderer separation by carrying first-party asset identity through `MeshCommand`, keep backend handles as `mirakana::rhi` first-party handles, and record vertex/index/material descriptor binding through the backend-neutral `IRhiCommandList` contract.

**Tech Stack:** C++23, `mirakana_renderer`, `mirakana_scene_renderer`, `mirakana_rhi`, `mirakana_rhi_d3d12`, `mirakana_rhi_vulkan`, `NullRhiDevice`, CTest.

---

### Task 1: Preserve Scene Mesh Identity

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`
- Modify: `tests/unit/scene_renderer_tests.cpp`

- [x] Add failing coverage that `make_scene_mesh_command` preserves mesh and material ids.
- [x] Extend `MeshCommand` with first-party mesh/material identity.
- [x] Keep transform/color behavior unchanged.

### Task 2: Add Backend-Neutral Indexed Mesh Draw Binding

**Files:**
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/rhi_tests.cpp`

- [x] Add failing coverage for vertex buffer binding, index buffer binding, and indexed draw stats.
- [x] Add `IndexFormat`, `VertexBufferBinding`, `IndexBufferBinding`, `bind_vertex_buffer`, `bind_index_buffer`, and `draw_indexed` to the RHI command-list contract.
- [x] Add explicit graphics-pipeline vertex input layout/attribute metadata so mesh buffer bindings can feed backend PSO input layouts instead of being only command-list side effects.
- [x] Implement validation and stats in `NullRhiDevice`.
- [x] Implement native D3D12 PSO input layouts, IA vertex/index buffer binding, and `DrawIndexedInstanced`.
- [x] Extend Vulkan command loading, graphics pipeline vertex input creation, and dynamic rendering draw recording with private `vkCmdBindVertexBuffers`, `vkCmdBindIndexBuffer`, and `vkCmdDrawIndexed` paths.

### Task 3: Connect Renderer Mesh Commands To RHI Binding

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/renderer.hpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Add failing coverage that `RhiFrameRenderer::draw_mesh` binds material descriptor sets plus mesh vertex/index buffers before an indexed draw.
- [x] Add `MeshGpuBinding` and `MaterialGpuBinding` to `MeshCommand`.
- [x] Release acquired swapchain frames if frame startup fails before command-list ownership can submit or abandon the frame.
- [x] Keep unbound mesh commands on the previous procedural `draw(3, 1)` path.

### Task 4: Review Hardening

**Files:**
- Modify: `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `engine/renderer/src/rhi_frame_renderer.cpp`
- Modify: `tests/unit/backend_scaffold_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] Require a supported `VulkanRhiDeviceMappingPlan` before `create_rhi_device` exposes a Vulkan `IRhiDevice`.
- [x] Validate Vulkan descriptor set binding against the currently bound graphics pipeline layout.
- [x] Cover renderer begin-frame failure cleanup for acquired swapchain frames.

### Task 5: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

**Remaining follow-up:** This is renderer-visible binding, not a full material/shader system. The later material sampler/uniform/reflection slice added public sampler objects, material factor uniform writes, shader reflection metadata, scene GPU binding resolution, native D3D12/Vulkan sampler descriptor writes, D3D12 visible texture-sampling proof, environment-gated Vulkan texture-sampling proof, backend-neutral `RhiFrameRenderer` runtime material descriptor binding coverage, and a D3D12 visible `RhiFrameRenderer` runtime material texture-sampling proof. Production material preview UX and broader material/shader workflows remain follow-up work.

**Follow-up status:** Uploaded runtime mesh buffers can now carry fixed `float32x3` position stride and `uint32` index format metadata, convert to renderer `MeshGpuBinding` through `mirakana::runtime_rhi::make_runtime_mesh_gpu_binding`, register through `SceneGpuBindingPalette::try_add_mesh`, and submit as renderer-visible indexed draws through `RhiFrameRenderer` without native handles. Mesh bindings also carry a non-owning owner-device pointer so `RhiFrameRenderer` rejects cross-device buffer misuse before issuing RHI commands. Broader imported mesh editor UX and richer vertex-layout metadata remain follow-up work.

**Verification note:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on Windows. Diagnostic-only blockers remain for local Vulkan SPIR-V tooling (`spirv-val` and DXC SPIR-V CodeGen), Metal tools, Apple packaging, and strict clang-tidy compile database availability.
