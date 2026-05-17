# Frame Graph Public Null Aliasing Barriers v1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for behavior changes. Keep this slice in `frame-graph-v1`; do not broaden renderer readiness.

**Goal:** Promote public wildcard/null texture aliasing barriers from unsupported to a backend-neutral Frame Graph/RHI contract.

**Architecture:** Treat `rhi::TextureHandle{}` and empty `FrameGraphTextureAliasingBarrier` resource names as explicit wildcard endpoints. Concrete non-zero handles remain validated when present, same concrete before/after handles remain invalid, and backends keep native handles private. D3D12 maps wildcard endpoints to `D3D12_RESOURCE_ALIASING_BARRIER` `pResourceBefore` / `pResourceAfter == nullptr`; Vulkan records the existing conservative synchronization2 memory dependency because Vulkan has memory dependencies rather than a D3D12-style null resource aliasing barrier.

**Tech Stack:** C++23, `MK_rhi`, `MK_rhi_d3d12`, `MK_renderer`, NullRHI/D3D12/Vulkan RHI contracts, PowerShell 7 validation wrappers.

---

## Official Practice Check

- Microsoft Direct3D 12 documents `D3D12_RESOURCE_ALIASING_BARRIER` with `pResourceBefore` and `pResourceAfter`; its resource barrier guidance allows these resources to be `NULL` to express potential aliasing with any tiled/placed resource.
- Khronos Vulkan documents memory aliasing as overlapping bound memory ranges and requires appropriate memory dependencies/barriers between overlapping alias uses; this slice keeps that backend behavior as a conservative synchronization2 memory dependency instead of exposing D3D12-style native handles.

## Context

- `IRhiCommandList::texture_aliasing_barrier` and `record_frame_graph_texture_aliasing_barriers` currently require two concrete distinct texture handles/resource names.
- D3D12 already records backend-private null-resource aliasing barriers for unproven concrete pairs, but the public RHI/Frame Graph contract still rejects wildcard/null endpoints.
- At slice start, manifest `frame-graph-v1` still listed public wildcard/null aliasing barriers as unsupported.

## Constraints

- Keep native resource handles backend-private.
- Do not claim Vulkan/Metal alias memory allocation, data inheritance/content preservation, or broad renderer readiness.
- Do not change texture state tracking when recording aliasing barriers.
- Same non-zero concrete before/after texture remains invalid.

## Tasks

- [x] Add RED tests for public RHI wildcard/null aliasing barriers in NullRHI.
- [x] Add RED tests for Frame Graph empty resource-name wildcard aliasing barriers.
- [x] Add RED tests for D3D12 public wildcard/null aliasing barrier evidence.
- [x] Implement backend-neutral validation and backend mappings.
- [x] Update docs/skills/manifest/static checks for the durable contract.
- [x] Run focused build/test/static checks and full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Validation Evidence

- RED: `MK_rhi_tests` rejected the new NullRHI wildcard barrier case before implementation.
- RED: `MK_renderer_tests` rejected empty Frame Graph aliasing resource names before implementation.
- Focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests MK_renderer_tests MK_d3d12_rhi_tests MK_backend_scaffold_tests`.
- Focused tests: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R "MK_rhi_tests|MK_renderer_tests|MK_d3d12_rhi_tests|MK_backend_scaffold_tests" --output-on-failure` passed 4/4 tests.
- Static checks: `check-public-api-boundaries.ps1`, `check-json-contracts.ps1`, `check-format.ps1`, `check-agents.ps1`, and `check-ai-integration.ps1` passed.
- Full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 65/65 CTest tests; Metal/Apple diagnostics remained host-gated diagnostic-only checks.

## Done When

- Public `TextureHandle{}` endpoints record aliasing barrier intent without changing texture state.
- Empty `FrameGraphTextureAliasingBarrier` resource names map to wildcard RHI endpoints.
- D3D12 records native aliasing barriers with `nullptr` before/after endpoints and evidence counters.
- Vulkan remains conservative and backend-neutral without public native handle exposure.
- `frame-graph-v1` guidance no longer lists public wildcard/null aliasing barriers as unsupported.
