# Runtime Mesh RHI Upload Plan (2026-04-27)

## Goal

Bridge cooked first-party mesh vertex/index bytes into backend-neutral RHI buffers without broadening renderer scope.

## Task List

- [x] Inspect existing texture upload and RHI buffer copy contracts.
- [x] Add RED tests for successful mesh buffer upload and invalid payload rejection.
- [x] Add `RuntimeMeshUploadOptions`, `RuntimeMeshUploadResult`, and `upload_runtime_mesh`.
- [x] Implement buffer creation, upload-buffer writes, copy command recording, submit, and optional wait in `runtime_upload.cpp`.
- [x] Add backend-neutral RHI upload-buffer write support for NullRHI, D3D12, and Vulkan `IRhiDevice` bridges.
- [x] Update manifest, roadmap, gap analysis, and game-development guidance.
- [x] Run public API, agent/schema, test, and full validation checks.

## Non-Goals

- Broader editor/game workflow ownership for uploaded mesh buffers beyond the renderer-visible `SceneGpuBindingPalette` bridge.
- General vertex layout, attribute, or shader input metadata beyond the current fixed position-only `float32x3` / `uint32` runtime mesh binding metadata.
- External glTF adapter decoded vertex/index byte output.
- Backend-specific native mesh resource code outside the existing `IRhiDevice` abstraction.
