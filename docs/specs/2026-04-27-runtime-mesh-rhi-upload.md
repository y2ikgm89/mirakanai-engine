# Runtime Mesh RHI Upload Design

## Goal

Allow typed cooked `RuntimeMeshPayload` vertex and index byte payloads to create backend-neutral RHI vertex/index buffers through `mirakana_runtime_rhi`.

## Context

First-party mesh source documents can now preserve optional `mesh.vertex_data_hex` and `mesh.index_data_hex` into cooked artifacts, and `mirakana_runtime` decodes those bytes into `RuntimeMeshPayload`. `mirakana_runtime_rhi` already bridges texture payload bytes into RHI resources and material metadata into descriptor resources; this slice extends that bridge so mesh bytes can populate backend-neutral vertex/index buffers.

## Constraints

- Keep the API in `engine/runtime_rhi`; do not expose backend-native handles.
- Use existing `IRhiDevice` buffer creation, upload-buffer write, copy command recording, submit, and wait contracts.
- Treat mesh vertex/index bytes as opaque payloads until a full vertex layout contract is designed.
- Reject metadata-only mesh payloads for upload because there is no byte data to populate GPU buffers.
- Do not add renderer draw binding, shader input layouts, or external glTF decoded byte production in this slice.

## Done When

- `upload_runtime_mesh` creates vertex/index GPU buffers and upload buffers from non-empty `RuntimeMeshPayload` byte arrays.
- The upload path writes bytes into upload buffers, records deterministic buffer copy commands, and optionally waits for completion.
- Invalid metadata and missing byte payloads fail before creating RHI resources.
- Runtime RHI tests cover success and failure paths.
- Manifest, roadmap, gap analysis, and AI guidance describe mesh upload readiness honestly.
