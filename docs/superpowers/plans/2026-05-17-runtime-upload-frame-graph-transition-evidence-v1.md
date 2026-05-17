# Runtime Upload Frame Graph Transition Evidence v1 (2026-05-17)

**Plan ID:** `runtime-upload-frame-graph-transition-evidence-v1`
**Status:** Completed slice under `frame-graph-v1`.

## Goal

Route runtime texture upload state transitions through the existing Frame Graph RHI texture executor so package/runtime upload paths add production graph ownership evidence without changing native handle boundaries or claiming broad package streaming readiness.

## Architecture

`runtime_rhi::upload_runtime_texture` already creates the texture, staging buffer, command list, copy command, submit fence, and optional wait. This slice keeps that ownership in `MK_runtime_rhi`, but replaces the two direct texture transitions with a tiny Frame Graph schedule: one copy pass that prepares its write target as `copy_destination`, copies the staged bytes, then records a declared final state transition to `shader_read`.

## Official Practice Check

- Microsoft Direct3D 12 documentation (`learn.microsoft.com/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12`) shows texture uploads creating or using a copy-destination texture, issuing the copy, then transitioning from `D3D12_RESOURCE_STATE_COPY_DEST` to shader-resource state before sampling.
- Khronos Vulkan synchronization examples (`github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples`) show image upload paths using layout transitions from undefined to transfer-destination, copy, then a synchronization2 image barrier to read-only/shader-read layout before shader access.

## Constraints

- Keep gameplay, samples, and manifests free of native RHI handles.
- Do not move mesh, skinned mesh, morph, or material uniform buffer copy paths in this slice.
- Do not claim broad/background package streaming, async upload overlap, upload-ring production readiness, Vulkan/Metal alias allocation, or broad renderer quality.
- Keep `upload_runtime_texture` command-list ownership, submit fence behavior, and optional CPU wait behavior stable within this greenfield API.

## Done When

- `RuntimeTextureUploadResult` reports Frame Graph pass target-state, final-state, total barrier, and callback evidence for texture uploads with byte payloads.
- The no-byte texture allocation path still reports no copy and no Frame Graph transition evidence.
- Existing runtime scene package texture upload tests continue to pass.
- Docs, active plan registry/master plan, and manifest fragments/composed manifest reflect the narrowed capability and remaining unsupported scope.
- Focused build/tests/static checks pass, followed by `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the slice gate.

## Tasks

- [x] Add RED runtime RHI test expecting uploaded textures to report one Frame Graph pass target-state barrier, one final-state barrier, two total barriers, and one invoked copy pass callback.
- [x] Implement `upload_runtime_texture` Frame Graph schedule around the existing copy command.
- [x] Update focused tests and any public compile expectations affected by the new result fields.
- [x] Synchronize docs, plan registry/master plan, manifest fragments, and composed manifest.
- [x] Run focused validation, agent/static checks, and full validation.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | RED, expected failure | Failed before implementation because `RuntimeTextureUploadResult` did not yet expose Frame Graph evidence counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | Passed | Focused runtime RHI target after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | Passed | Proves byte-backed texture upload records one pass target-state barrier, one final-state barrier, two total barriers, one pass callback, and metadata-only uploads record zero evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_tests` | Passed | Runtime scene package upload integration target still builds. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scene_rhi_tests` | Passed | Runtime scene RHI package upload tests still pass with the new texture upload evidence fields. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public header change stayed inside allowed API boundaries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed | Applied C++/text formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting/text checks after docs/manifest updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/runtime_rhi/src/runtime_upload.cpp,tests/unit/runtime_rhi_tests.cpp'` | Passed | Focused clang-tidy over touched C++ translation units. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed | Regenerated `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest compose and JSON contract checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent instruction/config surface checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent integration and manifest surface checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full slice gate, including static checks, focused tidy, full dev build, and 65 CTest tests. |
