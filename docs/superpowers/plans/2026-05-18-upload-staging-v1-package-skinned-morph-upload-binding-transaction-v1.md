# Upload Staging v1 Package Skinned/Morph Upload Binding Transaction v1 - 2026-05-18

**Status:** Completed.

**Goal:** Add package-safe-point upload transactions for resident skinned mesh and morph mesh CPU package payloads so hosts can turn explicit reviewed payload rows into renderer GPU binding rows with the same upload/fence evidence as static mesh transactions.

**Context:** `upload-staging-v1` already has ring-backed runtime texture/mesh/skinned/morph uploads, package texture upload transactions, package static mesh upload transactions, RHI no-wait upload batch execution, and runtime/package upload queue waits. The remaining package streaming blocker for animated 3D payloads is the absence of committed-streaming + live-catalog + explicit-payload transaction helpers for `AssetKind::skinned_mesh` and `AssetKind::morph_mesh_cpu`.

**Implementation surface:**
- Add package streaming source/binding/result structs for skinned mesh uploads and morph mesh uploads in `engine/runtime_rhi/include/mirakana/runtime_rhi/package_streaming_frame_graph.hpp`.
- Add `upload_runtime_package_streaming_skinned_mesh_gpu_bindings` and `upload_runtime_package_streaming_morph_mesh_gpu_bindings` in `engine/runtime_rhi/src/package_streaming_frame_graph.cpp`.
- Reuse `upload_runtime_skinned_mesh`, `make_runtime_skinned_mesh_gpu_binding`, `upload_runtime_morph_mesh_cpu`, `make_runtime_morph_mesh_gpu_binding`, caller-owned `upload_ring` options, submitted fences, and `wait_for_runtime_uploads_on_queue`.
- Extend `tests/unit/runtime_rhi_tests.cpp` with RED-first transaction tests for skinned and morph package payloads plus at least one fail-before-upload guard.

**Constraints:**
- Keep the helpers host-owned and package-safe-point scoped: committed streaming result, live `RuntimeResourceCatalogV2` resident row, and explicit caller-reviewed payload are required.
- Do not add background streaming, renderer-owned residency, runtime-wide upload-ring ownership, staging-pool production adoption, allocator/GPU budget enforcement, public native handles, or async-overlap/performance claims.
- Keep `AssetKind::skinned_mesh` and `AssetKind::morph_mesh_cpu` validation distinct from static `AssetKind::mesh`.
- Preserve CPU-wait-free async paths: when upload options use a non-graphics queue with `wait_for_completion=false`, record graphics queue waits through `wait_for_runtime_uploads_on_queue`.

**Done when:**
- RED tests fail for missing package skinned/morph transaction APIs or result fields before implementation.
- Skinned package transactions validate committed streaming, live `skinned_mesh` resident rows, matching `RuntimeSkinnedMeshPayload` handles, caller-owned upload-ring staging, submitted fences, uploaded bytes, command-list counters, graphics queue waits for async copy uploads, and renderer `SkinnedMeshGpuBinding` rows.
- Morph package transactions validate committed streaming, live `morph_mesh_cpu` resident rows, matching `RuntimeMorphMeshCpuPayload` handles, caller-owned upload-ring staging, submitted fences, uploaded bytes, command-list counters, graphics queue waits for async copy uploads, and renderer `MorphMeshGpuBinding` rows.
- Missing payload or wrong resident kind fails before buffer creation, writes, or command-list submission.
- Docs, manifest fragments, and static guard needles describe package skinned/morph upload transactions while leaving staging-pool production adoption, selected 2D/3D package upload smoke evidence, allocator/GPU budgets, background streaming, public native handles, and async-overlap claims unsupported.
- Focused runtime RHI build/test/tidy/format pass, then full `tools/validate.ps1` passes for the coherent public runtime RHI API slice.

**Validation evidence:**
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` failed before implementation because the package skinned/morph transaction API types and functions did not exist.
- Focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` passed after implementation.
- Focused test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` passed.
- Focused static/format: `tools/format.ps1`, `tools/check-format.ps1`, `tools/check-tidy.ps1 -Files engine/runtime_rhi/src/package_streaming_frame_graph.cpp,tests/unit/runtime_rhi_tests.cpp`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1` passed during the slice.
- Final full validation gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed.
