# First-Party Byte Payload Cooking Plan (2026-04-27)

## Goal

Bridge first-party source fixture bytes into cooked runtime artifacts without broadening renderer or RHI mesh scope.

## Task List

- [x] Add RED tests for optional texture, mesh, and audio source byte payload parsing/serialization.
- [x] Add RED tests for `execute_asset_import_plan` preserving first-party byte payloads into cooked texture, mesh, and audio artifacts.
- [x] Extend `TextureSourceDocument`, `MeshSourceDocument`, and `AudioSourceDocument` with optional byte vectors.
- [x] Add strict hex parsing/serialization helpers in `asset_source_format.cpp`.
- [x] Validate texture/audio optional byte payload sizes against existing source metadata byte counts.
- [x] Preserve optional first-party byte payloads in cooked artifacts from `asset_import_tool.cpp`.
- [x] Update AI-facing manifest, roadmap, gap analysis, and game-development skill guidance.
- [x] Run public API, agent, schema, and full validation checks.

## Non-Goals

- Optional external PNG/glTF/common-audio adapters emitting decoded byte payloads.
- Mesh vertex/index buffer upload through `mirakana_runtime_rhi`.
- Renderer-visible material sampling or mesh draw binding.
- A full mesh vertex layout contract.

## Follow-Up Status

- [x] Optional PNG adapters now decode RGBA8 pixel bytes into `texture.data_hex`.
- [x] Optional common-audio adapters now decode PCM sample bytes into `audio.data_hex`.
- [x] Optional glTF adapters now cook triangle `POSITION` accessors into deterministic `mesh.vertex_data_hex` and normalize indexed or generated primitive indices into `uint32` `mesh.index_data_hex` for embedded/loaded buffers.
