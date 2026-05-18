# 2026-05-18 Frame Graph Transient Alias Content Initialization v1

## Goal

Fail closed when a Frame Graph transient texture alias is first used by a render pass that tries to load previous attachment contents.

## Context

The master plan keeps `frame-graph-v1` `implemented-foundation-only`. D3D12 and Vulkan now prove backend-private transient alias memory allocation and aliasing-barrier evidence, but neither the backend-neutral executor nor the ready-surface wording should imply data inheritance or content preservation across transient aliases.

Official practice check:

- Microsoft Direct3D 12 documentation for memory aliasing and data inheritance says aliasing barriers are required when resources share physical memory and describes invalidated resources/undefined contents unless narrow data-inheritance conditions are met.
- Microsoft Direct3D 12 render pass documentation defines beginning access such as `CLEAR`, `NO_ACCESS`, and preserve/load-style access as an application request at render pass entry.
- Khronos Vulkan documentation describes memory aliasing for resources bound to overlapping device-memory ranges and separately describes identical `VK_IMAGE_CREATE_ALIAS_BIT` images interpreting shared memory consistently; this does not make a backend-neutral transient Frame Graph content-preservation claim safe by default.

## Constraints

- Keep this as a backend-neutral executor validation slice.
- Do not expose native handles or add backend-specific content-inheritance APIs.
- Do not claim Metal memory alias allocation, data inheritance/content preservation support, production graph ownership, async overlap/performance, or broad renderer readiness.
- Allow explicit clear/dont-care render-pass starts and non-render-pass first writes such as copy destinations.

## Done When

- `execute_frame_graph_rhi_texture_schedule` rejects resource-backed transient texture lifetimes whose first render-pass attachment uses `LoadAction::load`.
- Rejection happens before aliasing barriers, render pass begin/end, or pass callbacks are recorded.
- Existing clear/dont-care first-use render pass paths continue to work.
- Docs, manifest fragments, and static guards describe the new fail-closed content-initialization guard while keeping data inheritance/content preservation unsupported.
- Focused renderer tests and final repository validation pass.

## Validation

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` passed before implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` failed on the new RED test before implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` passed after implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` passed after implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed.
