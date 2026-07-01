# 2026-07-02 MAVG DirectStorage Zstd Preview Review v1

## Goal

Close `mavg-directstorage-zstd-preview-review-v1` as the DirectStorage 1.4/Zstd/GACL preview ambiguity gate: a first-party, value-only review gate in `MK_runtime_rhi`.

The slice must promote only `mavg_directstorage_zstd_preview_review_ready=1` and `mavg_directstorage_zstd_preview_selected=1`. It must keep runtime preview readiness, Zstd execution, GACL pipeline execution, package-visible backend readiness, performance readiness, broad CPU/GPU/memory optimization, native handle exposure, Unity/Unreal/Godot compatibility, Nanite compatibility/equivalence/superiority, legal advice, and legal approval unclaimed.

## Context

Context7 was checked on 2026-07-02 for `Microsoft DirectStorage`; it did not expose a DirectStorage-specific library id and only the broad Microsoft Learn corpus returned unrelated snippets. The implementation therefore follows current Microsoft official DirectStorage sources directly:

- Microsoft DirectStorage SDK & API landing page: `https://devblogs.microsoft.com/directx/directstorage-api-downloads/`
- Microsoft DirectStorage 1.4/Zstd/GACL public-preview announcement: `https://devblogs.microsoft.com/directx/directstorage-1-4-release-adds-support-for-zstandard/`
- Microsoft.Direct3D.DirectStorage 1.4.0-preview1-2603.504 NuGet changelog: `https://www.nuget.org/packages/Microsoft.Direct3D.DirectStorage/1.4.0-preview1-2603.504`
- Microsoft DirectStorage 1.3 EnqueueRequests release note: `https://devblogs.microsoft.com/directx/directstorage-1-3-is-now-available/`

The official source split is:

- DirectStorage 1.4 public preview adds Zstd compression/decompression, GACL request-level shuffle transform support, and D3D12 CreatorID support.
- Zstd preview support requires explicitly authored and compressed Zstd assets, optional GACL conditioning, and explicit game API usage; replacing redistributable DLLs is not evidence.
- GACL post-processing scope is limited to the reviewed public-preview BC1/BC3/BC4/BC5 texture scope; BC7 remains a future DirectStorage update.
- The NuGet preview changelog records known issues for staging buffers above 256MB on some GPUs and Zstd GPU fallback shader TDR risk for some compressed content.

## Constraints

- Do not include or call DirectStorage, D3D12, or GACL SDK headers.
- Do not import assets, execute GACL, execute DirectStorage, run GPU work, or mutate packages.
- Do not add dependencies or copy Microsoft sample code/shaders.
- Keep public API clean-break C++23 value types under `mirakana::runtime_rhi`.
- Preserve external-engine clean-room boundaries: Unity, Unreal, Godot, and Nanite are named only as explicit non-claim categories.

## Implementation Plan

1. Add `mavg_directstorage_zstd_preview_review.hpp/.cpp` with `RuntimeMavgDirectStorageZstdPreviewRow`, `RuntimeMavgDirectStorageZstdPreviewResult`, and `evaluate_runtime_mavg_directstorage_zstd_preview_review`.
2. Require four reviewed row classes: Zstd codec, GACL shuffle/postprocess, D3D12 CreatorID, and preview known issue.
3. Fail closed for missing preview SDK version, missing DirectStorage 1.4 selection, missing Zstd/GACL/CreatorID review, missing authored-asset or explicit-API requirements, missing preview known issue review, BC7 postprocess claims, DLL replacement claims, native handles, execution claims, performance claims, package-visible backend readiness, broad optimization, Unity/Unreal/Godot compatibility, and Nanite claims.
4. Add `GameEngine.MavgDirectStorageZstdPreviewReview.v1` JSON Schema.
5. Add `tools/validate-mavg-directstorage-zstd-preview-review.ps1` and focused `MK_runtime_rhi_mavg_ds_zstd_preview_tests`.
6. Update docs, manifest fragments, composed `engine/agent/manifest.json`, validation recipe, and static AI integration guard.

## Done When

- Focused C++ tests prove ready review rows, missing-preview host gates, GACL/known-issue gates, unsupported claims, and required feature row coverage.
- Validator emits:
  - `mavg_directstorage_zstd_preview_review_ready=1`
  - `mavg_directstorage_zstd_preview_selected=1`
  - `mavg_directstorage_zstd_preview_ready=0`
  - `mavg_directstorage_zstd_execution_ready=0`
  - `mavg_directstorage_gacl_pipeline_ready=0`
  - `mavg_directstorage_native_handles_exposed=0`
  - `mavg_package_visible_backend_readiness_ready=0`
  - `mavg_directstorage_performance_ready=0`
  - `mavg_broad_cpu_gpu_memory_optimization_ready=0`
  - `mavg_nanite_compatible=0`
  - `mavg_nanite_equivalent=0`
  - `mavg_nanite_superior=0`
  - `mavg_external_engine_compatibility=0`
- Agent surfaces remain synchronized with `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, and composed manifest output.

## Validation Evidence

Completed local validation on 2026-07-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_ds_zstd_preview_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_ds_zstd_preview_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-directstorage-zstd-preview-review.ps1 -RequireReady` passed and emitted the retained ready/non-claim counters listed above.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed with zero files rewritten.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after composing `engine/agent/manifest.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `git diff --check` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and 171/171 tests passed.
