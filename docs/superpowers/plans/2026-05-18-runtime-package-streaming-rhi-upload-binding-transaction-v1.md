# Runtime Package Streaming RHI Upload Binding Transaction v1 - 2026-05-18

## Goal

Add a narrow host-owned transaction helper that validates a committed package streaming safe-point result, live resident texture catalog rows, and caller-reviewed `RuntimeTexturePayload` rows before uploading those textures through `upload_runtime_texture` and returning imported `FrameGraphTextureBinding` rows plus aggregate upload/Frame Graph evidence.

## Context

`Package Streaming Frame Graph Texture Binding Handoff v1` already converts successful caller-owned `RuntimeTextureUploadResult` rows into imported bindings. This phase closes the next narrow gap by letting a host submit explicit texture payload rows and receive the upload results and binding rows in one fail-closed result without adding broad/background streaming, renderer-owned residency, upload staging rings, native async upload execution, production graph ownership, or gameplay-visible `IRhiDevice` access.

## Constraints

- Keep the helper host-owned and explicit: committed streaming result, resident catalog, and caller-reviewed payload pointers are required inputs.
- Reuse `upload_runtime_texture` for actual RHI work and preserve its submitted-fence and Frame Graph transition counters.
- Do not infer payloads from resident catalog metadata; package payload loading remains `MK_runtime`/host-owned.
- Keep `frame-graph-v1` and `upload-staging-v1` at `implemented-foundation-only`.

## Done When

- RED/GREEN tests prove the upload+binding transaction returns uploads, bindings, uploaded byte totals, and Frame Graph transition counters.
- Existing handoff rejects successful-looking texture uploads without owner-device provenance.
- Current capabilities, roadmap, manifest fragments, composed manifest, and static guard needles describe the narrow surface without broad ready claims.
- Focused runtime_rhi validation and the coherent full validation gate pass or have recorded blockers.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` failed before `RuntimePackageStreamingFrameGraphTextureUploadSource` and `upload_runtime_package_streaming_frame_graph_texture_bindings` existed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_tests --output-on-failure` passed.
- Focused static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_rhi/src/package_streaming_frame_graph.cpp,tests/unit/runtime_rhi_tests.cpp` passed.
- Agent/API drift: `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1` passed.
- Slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, with expected diagnostic-only or host-gated Metal/Apple checks on this Windows host.
