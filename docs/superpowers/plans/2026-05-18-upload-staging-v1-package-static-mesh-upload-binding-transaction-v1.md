# Upload Staging v1 Package Static Mesh Upload Binding Transaction v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for behavior changes. This is a phase under `upload-staging-v1`, not a separate production gap.

**Goal:** Add a host-owned package streaming transaction for resident static mesh payload uploads and renderer mesh bindings.

**Architecture:** Mirror the completed package texture upload transaction boundary for static meshes. The new runtime_rhi entrypoint validates a committed package streaming result, live resident catalog mesh rows, and explicit caller-reviewed `RuntimeMeshPayload` rows before uploading through `upload_runtime_mesh`, returning upload rows, renderer `MeshGpuBinding` rows, aggregate byte/counter evidence, and diagnostics without loading packages or exposing native handles.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_rhi` upload staging helpers, `RuntimeResourceCatalogV2`, NullRHI focused tests, manifest/docs/static checks.

---

## Goal

Narrow `upload-staging-v1` by proving package streaming can materialize resident static mesh rows through the same validated upload/binding transaction pattern already used for package textures.

## Context

- Texture package streaming already has `upload_runtime_package_streaming_frame_graph_texture_bindings`.
- Runtime static mesh uploads already support default staging buffers and caller-owned `RuntimeMeshUploadOptions::upload_ring`.
- Runtime scene GPU binding can upload meshes from a full `RuntimeAssetPackage`, but there is no package-streaming transaction that validates resident catalog rows plus explicit mesh payloads outside scene binding.

## Constraints

- Support static `AssetKind::mesh` only in this slice; skinned and morph mesh package streaming remain follow-ups under the same gap.
- Require committed package streaming state and live resident catalog rows before upload side effects.
- Require caller-reviewed `RuntimeMeshPayload` asset and handle values to match the resident catalog row.
- Preserve `RuntimeMeshUploadResult` Frame Graph/fence counters and allow existing `RuntimeMeshUploadOptions::upload_ring` reuse.
- Do not claim background streaming, skinned/morph package streaming, native async upload execution, staging-pool production adoption, renderer-owned residency, allocator budgets, or public native handles.

## Done When

- RED tests prove a committed resident static mesh transaction returns one upload row, one `MeshGpuBinding` row, aggregate uploaded bytes/counters, and can reuse a caller-owned upload ring.
- RED tests prove missing payloads and non-mesh resident catalog rows fail before upload side effects.
- Focused runtime RHI tests pass, agent-surface drift is reconciled, and the coherent C++/public-contract slice passes full `tools/validate.ps1`.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` after adding RED tests | Failed as expected: `RuntimePackageStreamingMeshUploadSource` and `upload_runtime_package_streaming_mesh_gpu_bindings` were not implemented. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | Passed after implementation. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | Passed: `MK_runtime_rhi_tests`. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_rhi/src/package_streaming_frame_graph.cpp,tests/unit/runtime_rhi_tests.cpp` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. | 2026-05-18 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: `validate: ok`, 65/65 CTest tests passed; Apple/Metal diagnostics remain host-gated. | 2026-05-18 |
