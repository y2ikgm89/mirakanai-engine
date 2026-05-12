# First-Party Byte Payload Cooking Design

## Goal

Allow first-party texture, mesh, and audio source fixture documents to carry deterministic byte payloads and preserve those payloads in cooked artifacts.

## Context

`mirakana_runtime` already decodes optional cooked `texture.data_hex`, `mesh.vertex_data_hex`, `mesh.index_data_hex`, and `audio.data_hex` fields. `mirakana_runtime_rhi` already records a texture upload path when `RuntimeTexturePayload::bytes` is populated. The missing host-independent slice was source/import production of those bytes.

## Constraints

- Keep this slice first-party and dependency-free.
- Do not expand optional PNG/glTF/common-audio adapters to decoded byte payloads in this slice.
- Do not add mesh GPU upload, renderer mesh binding, or visible material sampling in this slice.
- Validate texture/audio byte payload size against source metadata.
- Treat mesh vertex/index byte payloads as opaque fixture data until a full mesh vertex layout contract is designed.

## Done When

- `TextureSourceDocument`, `MeshSourceDocument`, and `AudioSourceDocument` parse and serialize optional hex byte payload fields.
- `execute_asset_import_plan` preserves first-party source byte payload fields into cooked artifacts.
- Tests cover source document byte round trips and cooked artifact byte preservation.
- Manifest, roadmap, gap analysis, and game-development skill guidance describe the implemented and remaining scope honestly.

