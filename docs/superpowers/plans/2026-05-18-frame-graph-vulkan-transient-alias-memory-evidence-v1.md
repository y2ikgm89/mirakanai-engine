# 2026-05-18 Frame Graph Vulkan Transient Alias Memory Evidence v1

## Goal

Prove a narrow Vulkan backend-private transient texture alias allocation path for Frame Graph alias-group leases without changing public RHI or Frame Graph APIs.

## Context

The master plan keeps `frame-graph-v1` `implemented-foundation-only`. D3D12 already proves backend-private same-offset placed texture alias groups and non-null aliasing barriers. Vulkan previously returned distinct alias-group handles by allocating independent images, so the backend-neutral Frame Graph lease contract could not claim shared native memory allocation evidence on Vulkan.

Khronos Vulkan docs confirm `VK_IMAGE_CREATE_ALIAS_BIT = 0x00000400` and describe it as the flag for images with the same creation parameters aliased to the same memory, subject to Vulkan memory aliasing rules.

## Constraints

- Keep Vulkan handles and memory ownership behind backend-private runtime/PIMPL types.
- Keep public APIs limited to `TextureHandle`, `TransientTextureAliasGroup`, `IRhiCommandList::texture_aliasing_barrier`, and first-party stats.
- Do not claim data inheritance/content preservation, async overlap/performance, Metal readiness, or production render graph ownership.
- Fail closed if Vulkan image memory requirements are unavailable or incompatible.

## Done When

- `VulkanRhiDevice::acquire_transient_texture_alias_group` creates distinct alias-flagged Vulkan images bound to one shared backend-private `VkDeviceMemory` allocation.
- Shared memory lifetime is owned once and remains valid until all alias textures are reset.
- `RhiStats::transient_texture_heap_allocations`, `transient_texture_placed_allocations`, and `transient_texture_placed_resources_alive` report allocation and retirement evidence for Vulkan alias groups.
- `MK_backend_scaffold_tests` covers the Vulkan alias-group allocation/stat path when a Vulkan runtime is available.
- Docs, manifest fragments, skills, and static guards no longer describe Vulkan alias memory allocation as unsupported.

## Validation

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_backend_scaffold_tests` failed on `stats.transient_texture_heap_allocations == 1` before implementation.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` passed.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_backend_scaffold_tests` passed.
- Agent/static sync: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-ai-integration.ps1` passed.
- Coherent phase gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the implementation, docs, manifest, skills, and static guard sync.
- Post-registry text check: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after updating the plan registry.
