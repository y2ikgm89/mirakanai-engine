# Renderer Commercial Readiness Evidence Promotion v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote `renderer_commercial_readiness=1` from retained, exact, legally reviewed renderer evidence bundles instead of manual switch assertions, while keeping D3D12, strict Vulkan, Apple-host Metal, package-visible quality, generated-game output, and clean-room/legal gates independently verifiable.

**Architecture:** Add a first-party, value-only renderer commercial readiness evidence promotion layer on top of the completed `RendererCommercialQualityCloseoutPlan`. The promotion layer consumes strict JSON artifacts and existing package/validator counters, maps them to reviewed evidence rows, rejects native handles, cross-backend inference, subjective quality claims, and external-engine material, then feeds `tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady` only after every selected evidence family is present. GPU/API work remains behind backend-private adapters; public engine contracts expose counters, hashes, row ids, diagnostics, and package-visible readiness only.

**Tech Stack:** C++23 `MK_renderer` value APIs, existing D3D12/strict Vulkan/Metal RHI backend validators, PowerShell 7 repository tools, JSON Schema draft 2020-12 schema files, existing test fixtures under `tests/fixtures/`, existing `tools/check-*` static contract chapters, and official vendor documentation. No new third-party runtime dependency is selected by this plan.

---

**Plan ID:** `renderer-commercial-readiness-evidence-promotion-v1`
**Date:** 2026-06-25
**Status:** Active. Implementation started with Task 0 selection on 2026-06-25.
**Owner surface:** `engine/renderer`, backend-private RHI validators, renderer package validation recipes, legal/dependency records, agent manifest fragments.

## Context

Current repository audit on 2026-06-25 shows:

- `main` is aligned with `origin/main` at merge commit `666d1a63` after PR #822.
- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points to this plan.
- `recommendedNextPlan.id = renderer-commercial-readiness-evidence-promotion-v1`.
- `unsupportedProductionGaps = []`.
- `Renderer Commercial Quality Closeout v1` is completed as a fail-closed aggregate. Default `tools/validate-renderer-commercial-quality-closeout.ps1` emits `renderer_backend_parity_ready=0`, `renderer_metal_broad_readiness=0`, `renderer_broad_quality_ready=0`, and `renderer_commercial_readiness=0`.
- Direct `tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady` can emit those four counters as `1` only when all current switches are supplied. This plan replaces that operator-supplied assertion path with retained artifact ingestion and row-level validation.

The target is not "make the renderer look like Unity, Unreal Engine, or Godot." The target is a first-party MIRAIKANAI evidence system that proves selected commercial renderer readiness through official API behavior, deterministic output, validation logs, package counters, profiler artifacts, legal/source review, and reproducible automation.

## Official Source Baseline

Implementation must treat the following sources as the minimum authoritative baseline. If a source changes or becomes unavailable, record the replacement source id in this plan or the implementation PR before changing behavior.

| Area | Required source ids | Required implementation meaning |
| --- | --- | --- |
| D3D12 command/resource correctness | Microsoft Learn Direct3D 12 Programming Guide, resource barriers, timing, `ID3D12Device3::EnqueueMakeResident`, `ID3D12Debug1` / debug layers | Use fences around allocator reuse, explicit resource state transitions, timestamp query/queue frequency conversion, residency evidence, and debug/validation output. |
| Vulkan correctness | Khronos Vulkan spec, Khronos `Vulkan-ValidationLayers`, LunarG synchronization validation documentation | Use `VK_KHR_synchronization2`/Vulkan 1.3 synchronization2 semantics, validation-layer clean logs, strict memory binding/allocation evidence, timestamp/query evidence, and no release-path full-pipeline-barrier shortcuts. |
| Metal correctness | Apple Developer `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, Metal resource fundamentals; Context7 Metal Shading Language documentation | Use Apple-host-only heap/residency/capture artifacts, `metal`/`metallib` toolchain evidence, MSL address-space/function-constant constraints, and backend-private Objective-C++ boundaries. |
| Legal and independence | Unity trademark/brand/legal pages, Unreal Engine EULA and trademark approval docs, Godot license/compliance/trademark policy | Use public documentation only as category research. Do not copy external engine source, sample code, shader code, UI expression, assets, trademarks, project formats, trade dress, logos, marketplace content, API names, compatibility claims, equivalence claims, or parity claims. |

Context7 checks performed for this plan:

- Direct3D 12 docs through `/websites/learn_microsoft_en-us_windows_win32_direct3d12`.
- Vulkan docs through `/khronosgroup/vulkan-docs`.
- Metal Shading Language docs through `/dogukanveziroglu/metal-shading-language-specification`; Apple API docs remain official-web-source gated.

Official web checks performed for this plan on 2026-06-25:

- Microsoft Learn Direct3D 12 resource barriers: https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
- Microsoft Learn Direct3D 12 timing/timestamp frequency: https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing
- Microsoft Learn `ID3D12CommandAllocator::Reset`: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12commandallocator-reset
- Microsoft Learn `ID3D12Device3::EnqueueMakeResident`: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device3-enqueuemakeresident
- Microsoft Learn Direct3D 12 GPU-based validation: https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation
- Khronos Vulkan synchronization chapter: https://docs.vulkan.org/spec/latest/chapters/synchronization.html
- Khronos `vkCmdPipelineBarrier2` reference: https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdPipelineBarrier2.html
- Khronos `VK_KHR_synchronization2` reference: https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_synchronization2.html
- Khronos Vulkan queries chapter: https://docs.vulkan.org/spec/latest/chapters/queries.html
- Khronos Vulkan SPIR-V environment: https://docs.vulkan.org/spec/latest/appendices/spirvenv.html
- Khronos Vulkan Validation Layers: https://github.com/KhronosGroup/Vulkan-ValidationLayers
- Apple `MTLHeap`: https://developer.apple.com/documentation/metal/mtlheap
- Apple `MTLResidencySet`: https://developer.apple.com/documentation/metal/mtlresidencyset
- Apple `MTLCaptureManager`: https://developer.apple.com/documentation/metal/mtlcapturemanager
- Apple Metal workload capture: https://developer.apple.com/documentation/xcode/capturing-a-metal-workload-in-xcode
- Apple Metal shader validation: https://developer.apple.com/documentation/xcode/validating-your-apps-metal-shader-usage
- Unity trademarks: https://unity.com/legal/trademarks
- Unity terms of service trademark terms: https://unity.com/legal/terms-of-service
- Unreal Engine EULA: https://www.unrealengine.com/eula/unreal
- Epic Unreal Engine trademark approval: https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license
- Godot license: https://godotengine.org/license/
- Godot license compliance docs: https://docs.godotengine.org/en/stable/about/complying_with_licenses.html

## Legal And Clean-Room Rules

- Unity, Unreal Engine, and Godot may be named only in internal source-review/legal rows and documentation that explains non-use or nominative comparison constraints.
- No public MIRAIKANAI API, package schema, command id, UI row id, shader function, sample asset, or marketing claim may use Unity, Unreal, UE, Godot, Nanite, Lumen, UXML, USS, Blueprint, SceneTree, or other external-engine product identifiers as a compatibility target.
- Do not copy from external engine source, sample code, starter templates, marketplace content, shader code, UI expression/layouts, assets, trademarks, or trade dress. Godot's MIT license does not make copying acceptable for this slice unless a separate explicit dependency/license decision updates `docs/legal-and-licensing.md`, `docs/dependencies.md`, `THIRD_PARTY_NOTICES.md`, and the source review rows.
- Do not claim "compatible with", "equivalent to", "drop-in", "parity with", or "better than" Unity, Unreal Engine, or Godot. The only allowed claim is a first-party readiness claim backed by MIRAIKANAI evidence rows.
- Any external material row other than public-doc category research must make `renderer_clean_room_legal_ready=0` until explicit legal and technical approval is recorded.
- This plan is an engineering compliance plan, not legal advice. Public release wording and trademark/commercial claims require human legal review before publication.

## Non-Goals

- Do not rewrite the renderer architecture.
- Do not expose `ID3D12*`, `Vk*`, `MTL*`, `CAMetalLayer`, or platform-native handles through public gameplay/editor APIs.
- Do not add Dear ImGui, Qt, Slint, RmlUi, SDL3 UI, or another UI middleware dependency.
- Do not import Unity/Unreal/Godot projects, packages, assets, materials, shaders, scripts, editor workflows, or UI systems.
- Do not make `environment_ready`, all-platform UI parity, broad MAVG optimization, crash upload production readiness, or unselected iOS/Android readiness claims.
- Do not make subjective visual-quality claims without deterministic package-visible evidence and approved review rows.

## Done When

- Default validation still reports the four final readiness counters at `0`.
- A retained evidence bundle can drive `tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady` to emit:
  - `renderer_backend_parity_ready=1`
  - `renderer_metal_broad_readiness=1`
  - `renderer_broad_quality_ready=1`
  - `renderer_commercial_readiness=1`
- Missing, stale, cross-backend-inferred, unreviewed, host-gated, or legally unsafe evidence blocks the promotion with exact diagnostics.
- Windows/D3D12, strict Vulkan, and Apple-host Metal evidence remain independent; no backend can promote another backend.
- `engine/agent/manifest.fragments/`, composed manifest, docs, static checks, validation recipes, CI lane selection, and plan registry all state the same readiness truth.
- Legal/dependency records remain complete for any third-party material; if no external material is used, source-review rows prove zero external engine source, sample code, shader code, UI expression, assets, trademarks, compatibility claims, equivalence claims, and parity claims.

## Task 0: Select The Plan Without Changing Readiness

**Files:**

- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`

Steps:

- [x] Change `currentActivePlan` to this plan only when implementation starts.
- [x] Set `recommendedNextPlan.id = renderer-commercial-readiness-evidence-promotion-v1`.
- [x] Preserve `unsupportedProductionGaps = []`.
- [x] Preserve default non-ready text for `renderer_backend_parity_ready=0`, `renderer_metal_broad_readiness=0`, `renderer_broad_quality_ready=0`, and `renderer_commercial_readiness=0`.
- [x] Compose the manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Evidence captured 2026-06-25:

- `tools/compose-agent-manifest.ps1 -Write`: wrote `engine/agent/manifest.json`.
- `tools/check-json-contracts.ps1`: `json-contract-check: ok`.
- `tools/check-ai-integration.ps1`: `ai-integration-check: ok`.
- `tools/check-format.ps1`: `format-check: ok`.
- `tools/check-agents.ps1`: `agent-config-check: ok`.
- `tools/validate-renderer-commercial-quality-closeout.ps1`: default remains `renderer_commercial_quality_closeout_status=host_evidence_required`, final counters remain `0`, and `-RequireReady` remains blocked until all selected renderer evidence rows exist.

## Task 1: Add Evidence Bundle Contract

**Files:**

- Add: `schemas/renderer-commercial-readiness-evidence.schema.json`
- Add: `tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json`
- Add: `tests/fixtures/renderer/commercial-readiness-evidence/missing_metal/evidence.json`
- Add: `tests/fixtures/renderer/commercial-readiness-evidence/external_engine_rejected/evidence.json`
- Add: `tools/check-json-contracts-074-renderer-commercial-readiness-evidence.ps1`
- Add: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`

Contract:

- `schema_version` is exactly `GameEngine.RendererCommercialReadinessEvidence.v1`.
- `claim_id` is exactly `renderer-commercial-readiness-evidence-promotion-v1`.
- `source_rows` contains dated official source ids for D3D12, Vulkan, Metal, MSL, Unity legal, Unreal legal, and Godot legal review.
- `backend_rows` contains exactly selected rows for `d3d12`, `vulkan_strict`, and `apple_metal`.
- `package_rows` contains exactly selected 3D/UI/environment/generated-game package evidence rows for `visible_3d`, `runtime_ui`, `environment`, and `generated_game`.
- `quality_rows` contains renderer quality matrix and production VFX/profiling rows.
- `metal_memory_profiling_rows` references retained `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` artifacts.
- `clean_room_rows` proves public-doc-only research and zero external engine source, sample code, shader code, UI expression, assets, trademarks, compatibility claims, equivalence claims, parity claims, and API use.
- `non_claims` contains `environment_ready=false`, `native_handles_exposed=false`, `cross_backend_inference=false`, `external_engine_parity=false`, `unity_compatibility=false`, `unreal_compatibility=false`, and `godot_compatibility=false`.
- Manifest field `rendererCommercialReadinessEvidenceBundleContract` records `schemas/renderer-commercial-readiness-evidence.schema.json`, the ready/missing Metal/external-engine rejected fixtures, and `tools/check-json-contracts-074-renderer-commercial-readiness-evidence.ps1` / `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`.

Steps:

- [x] Follow the existing `schemas/renderer-metal-memory-profiling-host-evidence.schema.json` style: `additionalProperties=false`, const source ids, strict booleans, minimum positive row counts, SHA-256 patterns for retained artifacts.
- [x] Reject absolute paths, parent traversal, unapproved artifact locations, stale schema versions, unknown backend ids, unknown package ids, and unknown claim ids.
- [x] Keep fixtures small and deterministic; fixture files are proof of validator behavior, not real host proof.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Evidence captured 2026-06-25:

- `tools/check-json-contracts.ps1`: `json-contract-check: ok`.
- `tools/check-ai-integration.ps1`: `ai-integration-check: ok`.
- `tools/check-format.ps1`: `format-check: ok`.
- `tools/check-agents.ps1`: `agent-config-check: ok`.
- `tools/compose-agent-manifest.ps1 -Write`: wrote `engine/agent/manifest.json` after adding `rendererCommercialReadinessEvidenceBundleContract`.

## Task 2: Add Value-Only Promotion API

**Files:**

- Add: `engine/renderer/include/mirakana/renderer/renderer_commercial_readiness_evidence.hpp`
- Add: `engine/renderer/src/renderer_commercial_readiness_evidence.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Add: `tests/unit/renderer_commercial_readiness_evidence_tests.cpp`
- Modify: `CMakeLists.txt`

Public API intent:

```cpp
RendererCommercialReadinessPromotionPlan
plan_renderer_commercial_readiness_promotion(
    const RendererCommercialReadinessPromotionDesc& desc);
```

Required behavior:

- [x] Consume only value rows, existing `RendererCommercialQualityCloseoutPlan` data, package counters, artifact hashes, validation recipe ids, and legal/source review booleans.
- [x] Never execute GPU commands, native captures, shell commands, package scripts, crash uploads, or network requests.
- [x] Never store or return native handles.
- [x] Require all selected backends before final promotion: D3D12, strict Vulkan, and Apple-host Metal.
- [x] Require Apple Metal memory/profiling rows to come from Apple-host Metal artifacts, not D3D12/Vulkan proof.
- [x] Require selected package evidence for visible 3D, runtime UI, environment, and generated game output.
- [x] Require clean-room/legal rows and complete third-party notices.
- [x] Reject external-engine code/sample/asset/trademark/UI/API/compatibility/equivalence rows unless the row is a forbidden-material diagnostic and the final ready counter stays `0`.
- [x] Produce a deterministic replay hash from accepted row ids, source ids, package counter ids, artifact hashes, backend ids, and readiness booleans.

Minimum tests:

- [x] Ready fixture promotes all four final counters.
- [x] Missing Metal memory/profiling row keeps `renderer_metal_broad_readiness=false`.
- [x] Missing strict Vulkan clean validation row keeps `renderer_backend_parity_ready=false`.
- [x] Stale source id rejects the bundle.
- [x] Cross-backend Metal inference rejects the bundle.
- [x] Native handle exposure rejects the bundle.
- [x] Unity/Unreal/Godot compatibility or equivalence claim rejects the bundle.
- [x] External engine code/sample/asset/UI/trademark rows reject commercial readiness.
- [x] Replay hash changes when accepted evidence details change.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_commercial_readiness_evidence_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_commercial_quality_closeout_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "renderer_commercial"
```

Evidence captured 2026-06-25:

- `tools/cmake.ps1 --preset dev`: configure succeeded.
- `tools/cmake.ps1 --build --preset dev --target MK_renderer_commercial_readiness_evidence_tests`: build succeeded after RED compile failure proved the missing header first.
- `tools/cmake.ps1 --build --preset dev --target MK_renderer_commercial_quality_closeout_tests`: build succeeded for the existing closeout companion target.
- `tools/ctest.ps1 --preset dev --output-on-failure -R "renderer_commercial"`: 2/2 tests passed.
- `tools/check-ai-integration.ps1`: `ai-integration-check: ok`.
- `tools/check-json-contracts.ps1`: `json-contract-check: ok`.
- `tools/check-format.ps1`: `format-check: ok`.
- `tools/check-agents.ps1`: `agent-config-check: ok`.

## Task 3: Add Artifact Validator And Promotion Wrapper

**Files:**

- Add: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Add: `tools/check-renderer-commercial-readiness-evidence-fixture-guard.ps1`
- Add: `tests/fixtures/renderer/commercial-readiness-evidence/ready/*-*.json` fixture-only retained artifact files
- Modify: `tools/validate-renderer-commercial-quality-closeout.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `tools/classify-pr-validation-tier.ps1`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`

Validator contract:

- Default invocation validates schema/fixtures and emits `renderer_commercial_readiness=0`.
- `-ArtifactRootRelative <path>` points to retained evidence under the repository root or approved artifact root.
- `-RequireReady` fails unless the artifact bundle is present and complete.
- The wrapper calls existing focused validators or consumes their retained output; it must not synthesize ready rows from command-line switches alone.

Steps:

- [x] Parse JSON with strict property checks following `tools/check-renderer-metal-memory-profiling-host-evidence.ps1` helper patterns.
- [x] Verify schema source needles before parsing artifacts.
- [x] Verify every referenced artifact exists under the artifact root and every SHA-256 matches.
- [x] Call or consume:
  - `tools/validate-renderer-commercial-quality-closeout.ps1`
  - `tools/check-renderer-metal-memory-profiling-host-evidence.ps1`
  - selected D3D12 renderer quality evidence recipe
  - selected strict Vulkan renderer quality evidence recipe
  - selected Apple Metal renderer quality evidence recipe
  - selected package-visible 3D/UI/environment/generated-game recipe rows
- [x] Emit exact counters for every accepted and missing row.
- [x] Keep `renderer_environment_ready=0`.
- [x] Add the artifact validator as the final readiness promotion path; the historical closeout v1 direct manual switch lane remains retained legacy evidence until a later cleanup updates its completed plan/static guard.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-evidence.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/classify-pr-validation-tier.ps1 -ChangedPath tools/validate-renderer-commercial-readiness-evidence.ps1
```

Expected results:

- Default passes with `renderer_commercial_readiness=0`.
- `-RequireReady` without artifacts fails with exact missing-evidence blockers.
- Ready fixture passes only as fixture validation and must include `fixture_only=1`; real commercial promotion requires retained host artifacts.
- CI classification selects Windows MSVC/D3D12, strict Vulkan, macOS Metal host evidence, full static analysis, and iOS Metal only if selected iOS rows are present.

Validation evidence:

- `tools/validate-renderer-commercial-readiness-evidence.ps1`: passed with `renderer_commercial_readiness=0`, `renderer_environment_ready=0`, and `renderer_commercial_readiness_evidence_blocker=artifact_root_required`.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady`: failed as expected with `require_ready_without_artifact_root+artifact_root_required`.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready`: passed with `fixture_only=1`, 11 artifact rows, zero missing artifacts, zero hash mismatches, and final renderer readiness counters set for fixture validation only.
- `tools/check-renderer-commercial-readiness-evidence-fixture-guard.ps1`: passed after proving copied fixture artifacts under temporary `artifacts/renderer/commercial-readiness-evidence/...` roots are blocked with `fixture_artifact_rejected`, `renderer_commercial_readiness_fixture_artifacts_rejected=11`, and `renderer_commercial_readiness=0`.
- `tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady -ReadinessEvidenceArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready`: passed through the artifact-ingestion wrapper with 12 selected evidence rows ready.
- `tools/classify-pr-validation-tier.ps1 -ChangedPath tools/validate-renderer-commercial-readiness-evidence.ps1`: selected `windows_msvc,linux_vulkan_host,full_static_analysis,macos_metal_cmake,metal_host_evidence` and did not select iOS Metal.
- `tools/check-validation-recipe-runner.ps1`: passed.
- `tools/check-ci-matrix.ps1`: passed.
- `tools/compose-agent-manifest.ps1 -Write`: wrote `engine/agent/manifest.json`.
- `tools/check-json-contracts.ps1`: passed.
- `tools/check-ai-integration.ps1`: passed.
- `tools/check-format.ps1`: passed.
- `tools/check-agents.ps1`: passed.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1`: passed; Windows host reported expected diagnostic-only Metal/Apple packaging host gates.

## Task 4: Bind D3D12 Evidence

**Files:**

- Modify: existing D3D12 renderer quality/package validation recipe files only inside backend-private RHI/test/tool boundaries.
- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json`

Required D3D12 rows:

- [x] Command allocator/list fence row: allocator reuse is fenced.
- [x] Resource barrier row: render, copy, unordered-access, and readback transitions are explicit.
- [x] Timestamp row: `D3D12_QUERY_TYPE_TIMESTAMP`, resolved query data, queue frequency, and CPU/GPU clock calibration evidence.
- [x] Debug validation row: D3D12 debug layer or GPU-based validation clean output for the selected workload.
- [x] Residency row: explicit memory budget/residency evidence using official D3D12 residency APIs where selected by the workload.
- [x] Package-visible readback row: deterministic hash/counter for selected 3D/UI/environment/generated-game output.
- [x] Native handle row: `native_handles_exposed=0`.

No row may infer Vulkan, Metal, broad UI parity, environment readiness, or external-engine parity.

Validation evidence:

- `tests/fixtures/renderer/commercial-readiness-evidence/ready/d3d12-quality.json` now carries fixture-only D3D12 proof rows for `ID3D12Fence`, `D3D12_RESOURCE_BARRIER`, `D3D12_QUERY_TYPE_TIMESTAMP`, debug/GPU validation clean output, `ID3D12Device3::EnqueueMakeResident`, `IDXGIAdapter3::QueryVideoMemoryInfo`, deterministic package readback, and native-handle non-exposure.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready`: passed with `renderer_d3d12_command_allocator_fence_ready=1`, `renderer_d3d12_resource_barrier_ready=1`, `renderer_d3d12_timestamp_ready=1`, `renderer_d3d12_debug_validation_ready=1`, `renderer_d3d12_residency_ready=1`, `renderer_d3d12_package_readback_ready=1`, `renderer_d3d12_native_handles_exposed=0`, and `renderer_environment_ready=0`.
- `tools/validate-renderer-commercial-readiness-evidence.ps1`: passed with default fail-closed D3D12 counters at `0` and `renderer_environment_ready=0`.
- `tools/check-ai-integration.ps1`: passed after adding D3D12 proof-row static needles.
- `tools/check-format.ps1`: passed.
- `tools/check-agents.ps1`: passed.

## Task 5: Bind Strict Vulkan Evidence

**Files:**

- Modify: existing Vulkan renderer quality/package validation recipe files only inside backend-private RHI/test/tool boundaries.
- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/vulkan-strict-quality.json`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`

Required strict Vulkan rows:

- [x] Synchronization2 row: selected workload uses `vkCmdPipelineBarrier2`/`VkDependencyInfo` semantics or Vulkan 1.3 equivalents.
- [x] Validation-layer row: `VK_LAYER_KHRONOS_validation` clean log for selected workload.
- [x] Synchronization validation row: no known sync validation errors for selected workload.
- [x] Memory binding row: buffer/image allocation and binding evidence follows Vulkan spec VUID constraints for the selected resources.
- [x] Timestamp/query row: query pool/timestamp evidence where selected.
- [x] SPIR-V/shader validation row: package shaders are validated for the selected environment.
- [x] Package-visible readback row: deterministic hash/counter for selected 3D/UI/environment/generated-game output.
- [x] Native handle row: `native_handles_exposed=0`.

No row may use a debugging-only full-pipeline barrier as release readiness evidence.

Validation evidence:

- `tests/fixtures/renderer/commercial-readiness-evidence/ready/vulkan-strict-quality.json` now carries fixture-only Vulkan proof rows for `vkCmdPipelineBarrier2`, `VkDependencyInfo`, `VK_LAYER_KHRONOS_validation`, synchronization validation, Vulkan memory binding VUID coverage, timestamp query resolution, SPIR-V validation, deterministic package readback, and native-handle non-exposure.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready`: passed with `renderer_vulkan_synchronization2_ready=1`, `renderer_vulkan_validation_layer_ready=1`, `renderer_vulkan_sync_validation_ready=1`, `renderer_vulkan_memory_binding_ready=1`, `renderer_vulkan_timestamp_ready=1`, `renderer_vulkan_shader_validation_ready=1`, `renderer_vulkan_package_readback_ready=1`, `renderer_vulkan_native_handles_exposed=0`, and `renderer_environment_ready=0`.
- `tools/check-ai-integration.ps1`: passed after adding Vulkan proof-row static needles.

## Task 6: Bind Apple Metal Evidence

**Files:**

- Modify: existing Metal host evidence generator/checker files only inside Apple-private implementation/test/tool boundaries.
- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/apple-metal-host.json`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`

Required Apple-host Metal rows:

- [x] Full Xcode host row: `metal` and `metallib` tools are available.
- [x] MSL shader row: address spaces, function constants, resource bindings, and stage restrictions are compliant with the queried MSL documentation.
- [x] `MTLHeap` row: heap-backed selected resources are created and budgeted.
- [x] `MTLResidencySet` row: residency set creation/request/commit evidence is present when available on the selected host.
- [x] `MTLCaptureManager` row: capture scope/artifact evidence is produced for the selected workload.
- [x] Visible package row: selected 3D material/lighting/postprocess, runtime UI atlas, environment package consumption, and generated-game package output rows are present.
- [x] Native handle row: `MTL*` objects remain private to Objective-C++ boundaries.
- [x] Cross-backend inference row: D3D12/Vulkan proof cannot promote Metal readiness.

Default Windows/Linux validation remains host-gated and must not require Apple tools.

Validation evidence:

- Context7 selected `/dogukanveziroglu/metal-shading-language-specification` for MSL and confirmed `device`, `constant`, `threadgroup`, `[[function_constant]]`, `[[buffer]]`, `[[texture]]`, `[[sampler]]`, `[[vertex]]`, `[[fragment]]`, and `[[kernel]]` terminology for the proof rows. Apple Developer documentation was checked for `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, and `metal`/`metallib` command-line shader library workflows.
- `tests/fixtures/renderer/commercial-readiness-evidence/ready/apple-metal-host.json` now carries fixture-only Apple Metal proof rows for Xcode/Metal tools, MSL shader constraints, `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, visible package coverage, private Objective-C++ native boundaries, and D3D12/Vulkan non-inference.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready`: passed with `renderer_apple_metal_xcode_tools_ready=1`, `renderer_apple_metal_msl_shader_ready=1`, `renderer_apple_metal_heap_ready=1`, `renderer_apple_metal_residency_set_ready=1`, `renderer_apple_metal_capture_ready=1`, `renderer_apple_metal_visible_package_ready=1`, `renderer_apple_metal_native_handles_exposed=0`, `renderer_apple_metal_cross_backend_inference=0`, and `renderer_environment_ready=0`.
- `tools/check-ai-integration.ps1`: passed after adding Apple Metal proof-row static needles.

## Task 7: Package-Visible Renderer Quality Closeout

**Files:**

- Modify: selected `games/*/game.agent.json` only for existing sample/generated packages that actually emit the rows.
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/visible-3d-package.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/runtime-ui-package.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/environment-package.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/generated-game-package.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`

Required package rows:

- [x] `visible_3d`: material, lighting, shadow/postprocess, and readback/hash evidence.
- [x] `runtime_ui`: UI atlas upload/readback or retained upload handoff evidence.
- [x] `environment`: environment renderer package row consumption without promoting `environment_ready`.
- [x] `generated_game`: generated-game package output row consumption.
- [x] Every package row points to a reviewed validation recipe id and package manifest row.
- [x] No package script execution or arbitrary shell execution is introduced.

Validation evidence:

- Package-visible fixture artifacts now carry detailed rows for `material_render`, `lighting_row`, `shadow_postprocess`, `package_visible_readback`, `ui_atlas_upload`, `ui_atlas_readback`, `renderer_handoff`, `environment_renderer_package_consumption`, and `generated_game_output`, plus manifest binding rows and script-execution zero counters.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready`: passed with `renderer_visible_3d_material_ready=1`, `renderer_visible_3d_lighting_ready=1`, `renderer_visible_3d_shadow_postprocess_ready=1`, `renderer_visible_3d_readback_hash_ready=1`, `renderer_runtime_ui_atlas_upload_ready=1`, `renderer_runtime_ui_atlas_readback_ready=1`, `renderer_runtime_ui_handoff_ready=1`, `renderer_environment_package_consumption_ready=1`, `renderer_environment_ready_promoted=0`, `renderer_generated_game_package_output_ready=1`, `renderer_generated_game_manifest_ready=1`, `renderer_package_arbitrary_script_execution=0`, `renderer_package_script_execution=0`, and `renderer_environment_ready=0`.
- `tools/check-ai-integration.ps1`: passed after adding package-visible proof-row static needles.

## Task 8: Legal, Dependency, And Source Review Closeout

**Files:**

- Modify: `docs/legal-and-licensing.md` only if external material or dependencies change.
- Modify: `docs/dependencies.md` only if dependencies change.
- Modify: `THIRD_PARTY_NOTICES.md` only if external material or dependencies change.
- Modify: `vcpkg.json` only if a dependency is explicitly approved.
- Modify: `schemas/renderer-commercial-readiness-evidence.schema.json`
- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/missing_metal/evidence.json`
- Modify: `tests/fixtures/renderer/commercial-readiness-evidence/external_engine_rejected/evidence.json`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`

Steps:

- [x] Add a retained source-review row proving public-doc category research only.
- [x] Add explicit zero rows for Unity/Unreal/Godot code, samples, shaders, UI expression, assets, trademarks, project import, API compatibility, and equivalence claims.
- [x] Add a blocker for any nonzero external-engine material row unless `legal_review_id` and `technical_review_id` are present and notices are complete.
- [x] Verify no new third-party dependency or asset was introduced. If one was introduced, update all dependency/legal files in the same PR.
- [x] Add static-check needles for the legal constraints so future docs/manifest changes cannot silently weaken them.

Validation evidence:

- Official Unity, Epic/Unreal, and Godot legal/trademark pages were checked as public documentation sources for legal category review only; no external-engine code, sample, shader, UI expression, asset, trademark, project import, API compatibility, equivalence, or parity claim is accepted.
- `schemas/renderer-commercial-readiness-evidence.schema.json` now requires `clean_room_rows.external_engine_zero_material_review` and non-claims for `external_engine_shader_used`, `external_engine_project_import_used`, `external_engine_api_used`, `external_engine_compatibility_claim`, `external_engine_equivalence_claim`, and `external_engine_parity_claim`.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/ready`: passed with `renderer_external_engine_zero_material_review_ready=1`, `renderer_external_engine_shader_used=0`, `renderer_external_engine_project_import_used=0`, `renderer_external_engine_api_used=0`, `renderer_external_engine_compatibility_claim=0`, `renderer_external_engine_equivalence_claim=0`, `renderer_external_engine_parity_claim=0`, and `renderer_environment_ready=0`.
- `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative tests/fixtures/renderer/commercial-readiness-evidence/external_engine_rejected`: failed as expected with all 11 artifacts present and valid, `renderer_external_engine_forbidden_material_detected_rows=1`, `renderer_external_engine_forbidden_material_rejected_rows=1`, and `renderer_commercial_readiness_evidence_blocker=external_engine_material_rejected`.
- `tools/check-ai-integration.ps1`: passed after adding legal/source-review static needles.
- No `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, or `THIRD_PARTY_NOTICES.md` update is required because this slice adds no dependency, third-party code, or distributable external asset.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

## Task 9: Manifest, Docs, CI, And Static Contracts

**Files:**

- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `schemas/engine-agent.schema.json` only if manifest shape changes.
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: relevant `tools/check-json-contracts-*.ps1`
- Modify: relevant `tools/check-ai-integration-*.ps1`

Steps:

- [x] Add the new validation recipe and command surfaces.
- [x] Compose manifest after every manifest fragment edit.
- [x] Keep current capabilities honest: commercial readiness remains `0` until real host artifacts pass.
- [x] Add static checks for exact counters, legal non-claims, source ids, and backend independence.
- [x] Update CI lane classification for the new validator path.

Validation evidence:

- `engine/agent/manifest.fragments/002-commands.json` exposes `rendererCommercialReadinessEvidenceCheck` and `rendererCommercialReadinessEvidenceRequireReady`; `engine/agent/manifest.fragments/009-validationRecipes.json` exposes `renderer-commercial-readiness-evidence`; `tools/classify-pr-validation-tier.ps1` and `tools/check-ci-matrix.ps1` select the renderer commercial readiness evidence lane.
- `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/testing.md`, and `docs/superpowers/plans/README.md` now distinguish fixture proof from real retained artifact promotion and keep default commercial readiness at `0`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`: passed and rewrote `engine/agent/manifest.json`.
- `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1` covers exact counters, source ids, legal non-claims, package/script rejection, backend independence, and forbidden broad ready claims.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

## Task 10: Final Commercial Promotion Gate

**Files:**

- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Add: `tools/collect-renderer-commercial-readiness-evidence.ps1`
- Add: `tools/check-renderer-commercial-readiness-evidence-collector.ps1`
- Add: `tools/check-renderer-commercial-readiness-evidence-metal-memory.ps1`
- Add: `tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1`
- Add: `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `tools/validate-renderer-commercial-quality-closeout.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`

Steps:

- [ ] Run focused renderer/RHI/package validators on the required hosts.
- [ ] Collect retained artifacts into the selected artifact root.
- [ ] Run the promotion validator with the retained artifact root.
- [ ] Feed the accepted row set into the existing closeout gate.
- [ ] Emit final four ready counters only when every accepted row is complete and clean.
- [ ] Record real host validation evidence in the plan before marking the plan completed.
- [ ] Return `currentActivePlan` to the master plan or select the next dated plan after closeout.

Current status:

- Host-gated for real commercial promotion. This workspace has validated the retained artifact contract, fixture-ready path, and rejected external-engine path, but has not collected a non-fixture artifact root from the required selected Windows D3D12, strict Vulkan, Apple-host Metal, package, and legal/source-review lanes.
- Retained artifact roots now fail closed when any child artifact still declares `fixture_only=true`; copied fixture artifacts cannot promote real commercial readiness.
- Full retained `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` artifacts now promote only the `memory_residency` and `profiling_capture` rows when their Apple source ids, `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, capture artifact hash, macOS/Xcode tool rows, and broad-readiness non-claims validate. `tools/check-renderer-commercial-readiness-evidence-metal-memory.ps1` proves this row import while keeping `renderer_metal_broad_readiness=0` and `renderer_commercial_readiness=0` until the other retained rows are supplied.
- `tools/collect-renderer-commercial-readiness-evidence.ps1` now assembles already-collected retained artifacts into `GameEngine.RendererCommercialReadinessEvidence.v1` under `artifacts/renderer/commercial-readiness-evidence/<retained-artifact-root>`, computes SHA-256 rows, copies nested Metal capture artifacts for full `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` input, requires explicit clean-room/legal/third-party notice switches, rejects fixture artifacts by default, and emits no final renderer readiness counters by itself. `tools/check-renderer-commercial-readiness-evidence-collector.ps1` proves the collector contract and copied-fixture rejection path.
- Task 10I adds `tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1` and `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1` for GitHub Actions artifacts intake. The importer follows GitHub CLI `gh run download`, can consume a saved GitHub Actions artifacts API JSON list through `-ArtifactListJsonRelative` to report available, missing, and expired workflow artifact names before download, detects the `renderer-commercial-readiness-final-retained-root` artifact or the seven explicit assembler inputs, records `host-gate-summary.json` states such as `mtlresidencyset_unavailable`, writes an intake manifest with `workflow_artifact_list`, `assembler_handoff`, and `final_preflight_handoff` rows, and keeps `renderer_commercial_readiness=0`.
- Task 10L adds `tools/generate-renderer-clean-room-legal-review-input.ps1` as a host-independent retained clean-room/legal review input producer. It writes `GameEngine.RendererCleanRoomLegalReviewInput.v1` and `GameEngine.RendererCleanRoomLegalReviewSourceSummary.v1` from official public Unity Terms/Trademark, Unreal Engine EULA/Trademark, and Godot license/compliance documentation URLs only, records `renderer-clean-room-legal-review-artifacts` for CI retention, and keeps `renderer_commercial_readiness=0`. It is not legal advice, does not use external engine code/samples/assets/UI/shaders/trademarks/API/project schemas, and does not claim Unity, Unreal Engine, or Godot compatibility, equivalence, parity, or replacement.
- Task 10M adds `renderer-commercial-artifact-intake` to `.github/workflows/validate.yml` as a current-run artifact intake handoff. The job waits for `windows-cpp23` when that lane is selected so `windows-packages` is visible before artifact enumeration, uses GitHub Actions `github.run_id` / `github.repository`, `gh api`, and `gh run download` through the importer to save `workflow-artifacts.json`, download diagnostics, `intake-manifest.json`, `assembler_handoff`, and `final_preflight_handoff` rows, then uploads `renderer-commercial-readiness-current-run-artifact-intake`. It uses `contents: read` plus `actions: read`, does not pass `-RequireReady`, and verifies `renderer_commercial_readiness=0`.
- Task 10N strengthens the artifact intake gap diagnostics without promoting readiness. The importer now records `artifact_handoff_strategy` rows that separate the optional `renderer-commercial-readiness-final-retained-root` artifact path from assembler source artifact availability, emits missing assembler source artifact names, and records exact `missing_assembler_inputs` / `renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names` rows so a current-run intake artifact can say whether the next step is final preflight or which of the seven D3D12, strict Vulkan, Apple-host Metal, Metal memory/profiling, package, quality/VFX, or clean-room/legal inputs remains absent.
- Task 10F.1 adds a retained D3D12 host evidence generator in front of the existing D3D12 artifact collector. The new generator consumes selected D3D12 package smoke counters plus a supplemental host proof JSON, writes `GameEngine.RendererD3d12CommercialQualityHostEvidence.v1` only when both are complete, rejects fixture-only supplemental proof, rejects unsafe paths, and keeps `renderer_commercial_readiness=0`, `renderer_broad_quality_ready=0`, `renderer_backend_parity_ready=0`, `renderer_metal_broad_readiness=0`, and `renderer_environment_ready=0`.
- Task 10F.2 adds an isolated D3D12 host supplement probe and producer that can create the `GameEngine.RendererD3d12CommercialQualityHostSupplement.v1` input required by Task 10F.1. It uses Microsoft D3D12/DXGI APIs only, does not consume Unity, Unreal Engine, Godot, blog, sample, shader, asset, UI, or project material, and keeps every broad renderer/commercial/environment readiness counter at 0 unless the isolated host probe records clean debug/GPU validation output.
- `renderer_commercial_readiness=1` is therefore accepted only for `fixture_only=1` validator proof; default validation and real repository state remain non-ready until retained host artifacts are supplied.
- The plan stays active until real host artifacts are collected or a follow-up plan explicitly takes over Task 10.

Remaining producer sequence:

### Task 10F.1: D3D12 Retained Host Evidence Generator

**Files:**

- Add: `tools/generate-renderer-d3d12-commercial-quality-host-evidence.ps1`
- Add: `tools/check-renderer-d3d12-commercial-quality-host-evidence.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Regenerate: `engine/agent/manifest.json`

Steps:

- [x] Add a failing static check that requires a real host evidence generator before retained D3D12 artifact assembly.
- [x] Require `PackageSmokeOutputRelative` for selected package-visible D3D12 counters and `SupplementalHostEvidenceRelative` for proof that package smoke cannot infer: `ID3D12CommandQueue::GetClockCalibration`, zero debug/GPU validation messages, `ID3D12Device3::EnqueueMakeResident`, `IDXGIAdapter3::QueryVideoMemoryInfo`, unordered-access barrier coverage, and `native_handles_exposed=false`.
- [x] Reject fixture-only supplemental host proof, unsafe paths, missing package smoke, missing supplement, Vulkan/Metal inference, external-engine parity, broad UI/environment readiness, and broad commercial readiness promotion.
- [x] Feed generated `GameEngine.RendererD3d12CommercialQualityHostEvidence.v1` into the existing D3D12 artifact collector and commercial readiness validator while the other retained rows remain fixtures.

Task 10F.1 implementation evidence:

- `tools/check-renderer-d3d12-commercial-quality-host-evidence.ps1` first failed RED on the missing `tools/generate-renderer-d3d12-commercial-quality-host-evidence.ps1`.
- `tools/generate-renderer-d3d12-commercial-quality-host-evidence.ps1` now writes `d3d12-host-evidence.json` only when package smoke counters and supplemental host proof are both present, and otherwise writes a host-gate summary / fails `-RequireReady`.
- `tools/check-renderer-d3d12-commercial-quality-host-evidence.ps1` validates the generated host evidence through `tools/collect-renderer-d3d12-commercial-quality-artifact.ps1`, then through `tools/collect-renderer-commercial-readiness-evidence.ps1` and `tools/validate-renderer-commercial-readiness-evidence.ps1`, proving `renderer_d3d12_renderer_quality_ready=1` while `renderer_commercial_readiness=0`.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-d3d12-commercial-quality-host-evidence.ps1
```

Validation evidence:

- `tools/check-renderer-d3d12-commercial-quality-host-evidence.ps1`: passed with `renderer_d3d12_commercial_quality_host_evidence_ready=1`, `renderer_d3d12_commercial_quality_host_evidence_written=1`, `renderer_d3d12_renderer_quality_ready=1`, and `renderer_commercial_readiness=0`.

### Task 10F.2: D3D12 Isolated Host Supplement Probe

**Files:**

- Add: `tools/generate-renderer-d3d12-commercial-quality-host-supplement.ps1`
- Add: `tools/check-renderer-d3d12-commercial-quality-host-supplement.ps1`
- Add: `tests/fixtures/d3d12_commercial_quality_host_supplement_probe.cpp`
- Modify: `CMakeLists.txt`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Regenerate: `engine/agent/manifest.json`

Steps:

- [x] Add a failing C++ test that requires a D3D12 host supplement collection API before the backend can claim supplemental residency/debug/timing proof.
- [x] Implement `collect_commercial_quality_host_supplement` behind the D3D12 backend boundary, keeping native COM handles private and returning value-only proof rows for `ID3D12CommandQueue::GetClockCalibration`, `ID3D12InfoQueue` debug/GPU validation counts, `ID3D12Device3::EnqueueMakeResident`, `IDXGIAdapter3::QueryVideoMemoryInfo`, `D3D12_RESOURCE_BARRIER` unordered-access coverage, and `native_handles_exposed=false`.
- [x] Keep debug layer and GPU-based validation process-isolated in `MK_d3d12_commercial_quality_host_supplement_probe`; the unit test exercises the same value API without enabling process-wide debug/GBV state so the normal `MK_d3d12_rhi_tests` lane remains deterministic.
- [x] Add a host supplement producer that writes `d3d12-host-supplement.json` only when every proof row is ready and clean, writes `host-gate-summary.json` / `probe-summary.json` otherwise, and fails `-RequireReady` when clean host evidence is not present.
- [x] Connect the producer to validate, manifest commands, validation recipes, and AI integration needles without promoting `renderer_commercial_readiness`, backend parity, Metal broad readiness, broad renderer quality, environment, Vulkan, package, generated-game, or external-engine parity readiness.

Task 10F.2 implementation evidence:

- Microsoft D3D12/DXGI documentation was used for `ID3D12Debug1::SetEnableGPUBasedValidation`, `ID3D12InfoQueue`, `ID3D12CommandQueue::GetClockCalibration`, `ID3D12Device3::EnqueueMakeResident`, `IDXGIAdapter3::QueryVideoMemoryInfo`, and `D3D12_RESOURCE_BARRIER`; no external engine code, samples, shaders, assets, UI expression, project formats, or compatibility claims are used.
- `tools/check-renderer-d3d12-commercial-quality-host-supplement.ps1` first failed RED on the missing producer, then passed after `tools/generate-renderer-d3d12-commercial-quality-host-supplement.ps1`, `tests/fixtures/d3d12_commercial_quality_host_supplement_probe.cpp`, and the CMake target were added.
- Local Windows/WARP probe execution created a host-gate summary instead of a retained supplement because `debug_message_count=1` and `gpu_based_validation_message_count=1`; clock calibration, video memory query, `EnqueueMakeResident`, residency fence signaling, unordered-access barrier recording, WARP device creation, and native handle non-exposure were otherwise present. `-RequireReady` correctly failed rather than promoting readiness.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_commercial_quality_host_supplement_probe
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-d3d12-commercial-quality-host-supplement.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-renderer-d3d12-commercial-quality-host-supplement.ps1 -Mode Generate -OutputRootRelative artifacts/renderer/d3d12-commercial-quality-host-evidence/<local-run>/supplement -NoBuild
```

Validation evidence:

- `MK_d3d12_rhi_tests`: passed through `ctest` after keeping debug/GBV in the isolated probe.
- `MK_d3d12_commercial_quality_host_supplement_probe`: built successfully.
- `tools/check-renderer-d3d12-commercial-quality-host-supplement.ps1`: passed.
- Local supplement generation: `renderer_d3d12_commercial_quality_host_supplement_status=host_evidence_required`, `renderer_d3d12_commercial_quality_host_supplement_ready=0`, `renderer_d3d12_commercial_quality_host_supplement_written=0`, `renderer_backend_parity_ready=0`, `renderer_metal_broad_readiness=0`, `renderer_broad_quality_ready=0`, `renderer_commercial_readiness=0`, and `renderer_environment_ready=0`.
- Slice validation: `tools/validate.ps1` passed with the new `check-renderer-d3d12-commercial-quality-host-supplement.ps1` static lane, full CMake build, and 159/159 CTest tests; Windows-host Metal/Apple checks remained diagnostic-only host gates.

### Task 10A: D3D12 Retained Commercial Row Producer

**Files:**

- Add: `tools/collect-renderer-d3d12-commercial-quality-artifact.ps1`
- Add: `tools/check-renderer-d3d12-commercial-quality-artifact.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Regenerate: `engine/agent/manifest.json`

Steps:

- [x] Add a failing static check that requires the producer to reject fixtures, absolute paths, parent traversal, missing hashes, native handle exposure, cross-backend inference, and manual readiness switches.
- [x] Implement `collect-renderer-d3d12-commercial-quality-artifact.ps1` so it consumes only first-party retained D3D12 host evidence and emits `d3d12-quality.json` with `fixture_only=false` only when every row is backed by retained host output.
- [x] Require rows for `command_allocator_list_fence`, `command_allocator_reuse_fenced`, `D3D12_RESOURCE_BARRIER`, `D3D12_QUERY_TYPE_TIMESTAMP`, `resolved_query_data`, `queue_frequency_hz`, `debug_layer_or_gpu_based_validation_clean`, zero debug/GPU validation messages, `IDXGIAdapter3::QueryVideoMemoryInfo`, `ID3D12Device3::EnqueueMakeResident`, deterministic package readback hash, and `native_handles_exposed=false`.
- [x] Keep the producer value-shaping only: it reads retained JSON evidence and computes hashes, but it does not run arbitrary packages, capture tools, network, editor shell, or GPU work by itself.
- [x] Validate the produced artifact through the commercial readiness bundle self-test with the remaining fixture rows still rejected; Task 10A alone leaves `renderer_commercial_readiness=0`, and all-non-fixture `-RequireReady -ArtifactRootRelative <retained-artifact-root>` validation remains Task 10F.

Task 10A implementation evidence:

- `tools/check-renderer-d3d12-commercial-quality-artifact.ps1` now proves the producer rejects fixture-only host evidence and unsafe paths, writes a non-fixture `d3d12-quality.json`, feeds that artifact through `tools/collect-renderer-commercial-readiness-evidence.ps1`, validates `renderer_d3d12_renderer_quality_ready=1`, and keeps `renderer_commercial_readiness=0` because the other retained rows are still fixtures.
- `engine/agent/manifest.fragments/002-commands.json` exposes `rendererD3d12CommercialQualityArtifactCheck` and `rendererD3d12CommercialQualityArtifactCollector`; `engine/agent/manifest.fragments/009-validationRecipes.json` exposes `renderer-d3d12-commercial-quality-artifact`.
- `tools/validate.ps1` runs `check-renderer-d3d12-commercial-quality-artifact.ps1` as a separate static task, preserving the Task 10A producer boundary instead of folding it into the aggregate collector check.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-d3d12-commercial-quality-artifact.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

Validation evidence:

- `tools/check-renderer-d3d12-commercial-quality-artifact.ps1`: passed with `renderer_d3d12_commercial_quality_artifact_written=1`, `renderer_d3d12_renderer_quality_ready=1`, `renderer_commercial_readiness_evidence_collector_fixture_artifacts=10`, `renderer_commercial_readiness=0`, and `renderer_environment_ready=0`.
- `tools/check-renderer-commercial-readiness-evidence-collector.ps1`, `tools/check-json-contracts.ps1`, `tools/check-format.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1`: passed.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 33 static checks, including the new `check-renderer-d3d12-commercial-quality-artifact.ps1` lane; Windows-host Metal/Apple checks remained diagnostic-only host gates.

### Task 10B: Strict Vulkan Retained Commercial Row Producer

**Files:**

- Add: `tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1`
- Add: `tools/check-renderer-vulkan-strict-commercial-quality-artifact.ps1`
- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Regenerate: `engine/agent/manifest.json`

Steps:

- [x] Add a failing static check that proves strict Vulkan cannot reuse D3D12, environment, Android, or Metal rows as renderer commercial proof.
- [x] Implement the producer so it consumes only retained strict Vulkan host evidence when `VK_LAYER_KHRONOS_validation`, synchronization validation, memory binding VUID coverage, timestamp query evidence, `spirv-val` shader validation, selected package readback, and clean validation logs are present in the input.
- [x] Require rows for `vkCmdPipelineBarrier2`, `VkDependencyInfo`, Vulkan synchronization2 semantics, memory binding/allocation VUID coverage, timestamp query pool evidence, SPIR-V validation, deterministic package readback hash, `debugging_only_full_pipeline_barrier=false`, and `native_handles_exposed=false`.
- [x] Keep full-pipeline-barrier shortcuts rejected as a non-claim; they cannot count as release readiness evidence.
- [x] Keep Task 10B non-promoting until D3D12, Apple Metal, quality, package, and legal rows are also present.

Task 10B implementation evidence:

- `tools/check-renderer-vulkan-strict-commercial-quality-artifact.ps1` now proves the producer rejects fixture-only host evidence and unsafe paths, writes a non-fixture `vulkan-strict-quality.json`, feeds that artifact through `tools/collect-renderer-commercial-readiness-evidence.ps1`, validates `renderer_vulkan_strict_renderer_quality_ready=1`, and keeps `renderer_commercial_readiness=0` because the other retained rows are still fixtures.
- `tools/validate-renderer-commercial-readiness-evidence.ps1` accepts strict Vulkan `timestamp_period_ns` as a positive finite numeric value, matching Khronos `VkPhysicalDeviceLimits::timestampPeriod` being a float nanoseconds-per-tick limit instead of an integer counter.
- `engine/agent/manifest.fragments/002-commands.json` exposes `rendererVulkanStrictCommercialQualityArtifactCheck` and `rendererVulkanStrictCommercialQualityArtifactCollector`; `engine/agent/manifest.fragments/009-validationRecipes.json` exposes `renderer-vulkan-strict-commercial-quality-artifact`.
- `tools/validate.ps1` runs `check-renderer-vulkan-strict-commercial-quality-artifact.ps1` as a separate static task, preserving the Task 10B producer boundary instead of folding it into the aggregate collector check.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-vulkan-strict-commercial-quality-artifact.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120
```

Validation evidence:

- `tools/check-renderer-vulkan-strict-commercial-quality-artifact.ps1`: passed with `renderer_vulkan_strict_commercial_quality_artifact_written=1`, `renderer_vulkan_strict_renderer_quality_ready=1`, `renderer_commercial_readiness_evidence_collector_fixture_artifacts=10`, `renderer_commercial_readiness=0`, and `renderer_environment_ready=0`.
- `tools/check-renderer-commercial-readiness-evidence-collector.ps1`: passed.
- `tools/check-json-contracts.ps1`: passed.
- `tools/check-format.ps1`: passed.
- `tools/check-agents.ps1`: passed.
- `tools/check-ai-integration.ps1`: passed.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 34 static checks, including the new `check-renderer-vulkan-strict-commercial-quality-artifact.ps1` lane; Windows-host Metal/Apple checks remained diagnostic-only host gates.

### Task 10C: Apple Metal Retained Commercial Row Producer

**Files:**

- Add: `tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1`
- Add: `tools/check-renderer-apple-metal-commercial-quality-artifact.ps1`
- Modify: `tools/validate-renderer-metal-apple.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Regenerate: `engine/agent/manifest.json`

Steps:

- [x] Add a failing static check that proves Windows/Linux validation remains host-gated and cannot require Apple tools.
- [x] Implement the producer as an Apple-host bridge that shapes existing `renderer-metal-apple-host-evidence`, `renderer-metal-environment-aggregate-apple-host-evidence`, visible package, and full `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` rows into `apple-metal-host.json`.
- [x] Require rows for full Xcode/`xcrun`, `metal`, `metallib`, MSL address-space/function-constant/resource-binding/stage restrictions, `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, capture artifact hash, visible package readback, `cross_backend_inference=false`, and `native_handles_exposed=false`.
- [x] Treat missing `MTLResidencySet` or capture output as host-gated, not ready.
- [x] Keep Objective-C++ and `MTL*` objects backend-private; the artifact records ids, counters, and hashes only.

Task 10C implementation evidence:

- `tools/check-renderer-apple-metal-commercial-quality-artifact.ps1` first failed because the producer did not exist, then proves the producer rejects fixture-only Apple-host evidence and unsafe paths, writes a non-fixture `apple-metal-host.json`, feeds that artifact through `tools/collect-renderer-commercial-readiness-evidence.ps1`, validates `renderer_apple_metal_renderer_quality_ready=1`, and keeps `renderer_commercial_readiness=0` because the other retained rows are still fixtures.
- `tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1` accepts only approved retained Apple-host evidence paths and full `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` paths, requires `renderer-metal-apple-host-evidence` plus `renderer-metal-environment-aggregate-apple-host-evidence` source rows, maps memory/profiling rows into `MTLHeap`, `MTLResidencySet`, and `MTLCaptureManager` proof rows, and leaves backend parity, broad Metal, broad quality, commercial readiness, and broad environment counters at zero.
- `tools/validate-renderer-metal-apple.ps1` now emits a non-promoting Apple Metal commercial host-source row that identifies the Apple-host and Metal memory/profiling source schemas without exposing native handles or promoting commercial readiness.
- `engine/agent/manifest.fragments/002-commands.json` exposes `rendererAppleMetalCommercialQualityArtifactCheck` and `rendererAppleMetalCommercialQualityArtifactCollector`; `engine/agent/manifest.fragments/009-validationRecipes.json` exposes `renderer-apple-metal-commercial-quality-artifact`.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-apple-metal-commercial-quality-artifact.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-metal-memory.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120
```

Validation evidence:

- `tools/check-renderer-apple-metal-commercial-quality-artifact.ps1`: passed with `renderer_apple_metal_commercial_quality_artifact_written=1`, `renderer_apple_metal_renderer_quality_ready=1`, `renderer_commercial_readiness_evidence_collector_fixture_artifacts=10`, `renderer_metal_broad_readiness=0`, `renderer_commercial_readiness=0`, and `renderer_environment_ready=0`.
- `tools/check-renderer-commercial-readiness-evidence-metal-memory.ps1`: passed.
- `tools/check-json-contracts.ps1`: passed.
- `tools/check-format.ps1`: passed.
- `tools/check-agents.ps1`: passed.
- `tools/check-ai-integration.ps1`: passed.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 35 static checks, including the new `check-renderer-apple-metal-commercial-quality-artifact.ps1` lane; Windows-host Metal/Apple checks remained diagnostic-only host gates.

### Task 10D: Package, Quality Matrix, And VFX/Profiling Artifact Producers

**Files:**

- Add: `tools/collect-renderer-package-commercial-quality-artifacts.ps1`
- Add: `tools/check-renderer-package-commercial-quality-artifacts.ps1`
- Add: `tools/collect-renderer-quality-vfx-commercial-artifacts.ps1`
- Add: `tools/check-renderer-quality-vfx-commercial-artifacts.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`

Steps:

- [x] Add failing checks that require package artifacts to come from package-visible counters rather than fixture JSON.
- [x] Shape selected `desktop-3d-package`, `desktop-runtime-ui-package`, `environment-package`, and `generated-game-package` outputs into `visible-3d-package.json`, `runtime-ui-package.json`, `environment-package.json`, and `generated-game-package.json`.
- [x] Shape selected `renderer-quality-matrix` and `renderer-production-vfx-profiling` outputs into `renderer-quality-matrix.json` and `production-vfx-profiling.json`.
- [x] Require `renderer_quality_matrix_status=host_evidence_required`, D3D12 and strict Vulkan ready rows, Metal host evidence rows supplied by Task 10C, no general renderer quality claim, zero GPU command/native capture/crash-upload side effects from value-only matrix planning, deterministic replay hash, and clean diagnostics.
- [x] Require `rendering_vfx_profiling_reviewed=1`, D3D12 and strict Vulkan host evidence ready, Metal host evidence supplied by Task 10C, debug policy rows, memory policy rows, package counter rows, deterministic replay hash, and zero native capture/crash upload side effects unless a retained official profiler artifact row is explicitly selected.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-package-commercial-quality-artifacts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-quality-vfx-commercial-artifacts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

Validation evidence:

- `tools/check-renderer-package-commercial-quality-artifacts.ps1`: passed after proving unsafe path rejection, fixture-only input rejection, four non-fixture package artifact writes, collector integration with seven remaining fixture artifacts, package counters ready, and `renderer_commercial_readiness=0`.
- `tools/check-renderer-quality-vfx-commercial-artifacts.ps1`: passed after proving unsafe path rejection, fixture-only input rejection, two non-fixture quality/VFX artifact writes, collector integration with nine remaining fixture artifacts, quality/VFX counters ready, `renderer_broad_quality_ready=0`, and `renderer_commercial_readiness=0`.
- `tools/check-ai-integration.ps1`: passed after adding Task 10D command, recipe, producer, check, and validation task needles.
- `tools/check-json-contracts.ps1`: passed after manifest fragment composition.
- `tools/check-renderer-commercial-readiness-evidence-collector.ps1`: passed after Task 10D producer registration.
- `tools/check-format.ps1`: passed.
- `tools/check-agents.ps1`: passed.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 37 static checks, including both new Task 10D checks; Windows-host Metal/Apple packaging checks remained diagnostic-only host gates.

### Task 10E: Legal/Source Review Artifact Producer

**Files:**

- Add: `tools/collect-renderer-clean-room-legal-artifact.ps1`
- Add: `tools/check-renderer-clean-room-legal-artifact.ps1`
- Modify: `tools/collect-renderer-commercial-readiness-evidence.ps1`
- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `docs/legal-and-licensing.md` only if a new dependency, source, or asset is introduced.
- Modify: `docs/dependencies.md` only if a dependency changes.
- Modify: `THIRD_PARTY_NOTICES.md` only if notices change.
- Modify: `vcpkg.json` only if an approved dependency is added.

Steps:

- [x] Add a failing check that any Unity, Unreal Engine, or Godot source/sample/shader/UI/layout/asset/trademark/project/API/compatibility/equivalence/parity row keeps `renderer_clean_room_legal_ready=0`.
- [x] Implement a first-party clean-room artifact that records official-doc category research only, no external-engine material, no external-engine API adoption, no public trademark usage, no compatibility/equivalence/parity claim, and complete third-party notices for existing dependencies.
- [x] Require human legal-review placeholders to remain non-promoting unless explicit `legal_review_id` and `technical_review_id` are supplied for any future external material row.
- [x] Do not add dependencies or copy external material for this slice. If a future slice does, update `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` in the same PR and rerun dependency policy checks.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-clean-room-legal-artifact.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

If dependency or notice files change, also run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
```

Validation evidence:

- `tools/check-renderer-clean-room-legal-artifact.ps1`: passed after proving unsafe path rejection, fixture-only review rejection, clean `clean-room-legal.json` production, rejected external-engine material rows for code, sample, shader, asset, trademark, UI expression, API, compatibility, equivalence, and parity, collector integration with 11 remaining fixture artifacts, `renderer_clean_room_legal_ready=1` for clean retained legal evidence, and `renderer_commercial_readiness=0`.
- `tools/check-renderer-commercial-readiness-evidence-collector.ps1`: passed after adding retained `CleanRoomLegalArtifactRelative` integration and preserving the existing fixture/self-test assembly path.
- `tools/check-ai-integration.ps1`: passed after adding Task 10E command, recipe, producer, check, validator counter, and static validation task needles.
- `tools/check-json-contracts.ps1`: passed after manifest fragment composition.
- `tools/check-agents.ps1`: passed.
- `tools/check-format.ps1`: passed.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 38 static checks, including the new `check-renderer-clean-room-legal-artifact.ps1` lane; Windows-host Metal/Apple checks remained diagnostic-only host gates.
- No `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, or `THIRD_PARTY_NOTICES.md` update is required because this slice adds no dependency, third-party code, or distributable external asset.

### Task 10F: Final Non-Fixture Retained Artifact Promotion

**Files:**

- Modify: `tools/validate-renderer-commercial-readiness-evidence.ps1`
- Modify: `tools/validate-renderer-commercial-quality-closeout.ps1`
- Add: `tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1`
- Add: `tools/check-renderer-commercial-readiness-final-promotion-preflight.ps1`
- Add: `tools/assemble-renderer-commercial-readiness-final-retained-root.ps1`
- Add: `tools/check-renderer-commercial-readiness-final-retained-root-assembler.ps1`
- Modify: `.github/workflows/validate.yml`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`

Steps:

- [ ] Assemble all Task 10A-10E artifacts with `tools/collect-renderer-commercial-readiness-evidence.ps1 -Mode Assemble`.
- [x] Add a final retained-root preflight that reports the exact 12 required final files, copied-fixture blockers, missing full Metal host evidence, missing clean-room legal evidence, schema/claim/recipe mismatch blockers, and downstream validator readiness without promoting broad renderer counters.
- [x] Add a final retained-root assembler that requires seven explicit D3D12, strict Vulkan, Apple-host Metal, Metal memory/profiling, package, quality/VFX, and clean-room/legal inputs, stages producer outputs under `_producer-artifacts`, assembles the final root, and runs preflight without executing GPU workloads or promoting readiness by itself.
- [x] Retain any existing final `artifacts/renderer/commercial-readiness-evidence/final-retained/**` output from the Metal host CI lane as `renderer-commercial-readiness-final-retained-root` with `compression-level: 0`, `include-hidden-files: false`, and `if-no-files-found: warn`, without generating evidence or promoting readiness.
- [ ] Run `tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative <retained-artifact-root>` and require every fixture-flagged artifact to have `fixture_only=false`; the full `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` input is accepted by schema/claim id and downstream validator because that host-evidence schema does not carry `fixture_only`.
- [ ] Run `tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady -ReadinessEvidenceArtifactRootRelative <retained-artifact-root>`.
- [ ] Accept `renderer_backend_parity_ready=1`, `renderer_metal_broad_readiness=1`, `renderer_broad_quality_ready=1`, and `renderer_commercial_readiness=1` only when all retained non-fixture row hashes, source ids, recipe ids, package counters, legal rows, and non-claims validate.
- [ ] Record exact local/hosted validation evidence and PR head SHAs in this plan and the registry.
- [ ] Return `currentActivePlan` to the production-completion master plan or select the next active dated plan after closeout.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative <retained-artifact-root>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1 -ArtifactRootRelative <retained-artifact-root> -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/assemble-renderer-commercial-readiness-final-retained-root.ps1 -Mode Assemble -OutputRootRelative artifacts/renderer/commercial-readiness-evidence/<retained-artifact-root> -D3d12HostEvidenceRelative <path> -VulkanStrictHostEvidenceRelative <path> -AppleMetalHostEvidenceRelative <path> -MetalMemoryProfilingHostEvidenceRelative <path> -PackageHostEvidenceRelative <path> -QualityVfxHostEvidenceRelative <path> -CleanRoomLegalReviewRelative <path> -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-final-retained-root-assembler.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady -ReadinessEvidenceArtifactRootRelative <retained-artifact-root>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Final local command sequence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative <retained-artifact-root>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-final-retained-root-assembler.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/assemble-renderer-commercial-readiness-final-retained-root.ps1 -Mode Assemble -OutputRootRelative artifacts/renderer/commercial-readiness-evidence/<retained-artifact-root> -D3d12HostEvidenceRelative <path> -VulkanStrictHostEvidenceRelative <path> -AppleMetalHostEvidenceRelative <path> -MetalMemoryProfilingHostEvidenceRelative <path> -PackageHostEvidenceRelative <path> -QualityVfxHostEvidenceRelative <path> -CleanRoomLegalReviewRelative <path> -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-final-promotion-preflight.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1 -ArtifactRootRelative <retained-artifact-root> -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-fixture-guard.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-commercial-readiness-evidence-metal-memory.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady -ReadinessEvidenceArtifactRootRelative <retained-artifact-root>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Publication command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Task 10F preflight candidate validation evidence:

- Current retained artifact availability audit: `artifacts/renderer/commercial-readiness-evidence/final-retained` is absent. The #833 post-merge `main` Validate run `28200969528` completed successfully and exposed four workflow artifacts through the GitHub Actions artifacts API (`windows-packages`, `linux-vulkan-host-evidence`, `renderer-metal-memory-profiling-host-artifacts`, and `metal-host-optimization-artifacts`), but it did not expose `renderer-commercial-readiness-final-retained-root`; a temporary `out/` download audit found only `GameEngine.RendererMetalMemoryProfilingHostGate.v1` / `mtlresidencyset_unavailable` JSON and zero of the seven final assembler input schemas. Final promotion remains impossible without host-supplied retained artifacts.
- `tools/check-renderer-commercial-readiness-final-promotion-preflight.ps1`: passed. It proves missing-root reporting (`required_files=12`, `present_files=0`, `missing_files=12`), unsafe-root rejection, `-RequireReady` failure with `require_ready_without_complete_retained_artifacts`, copied fixture rejection, missing full Metal host evidence, missing clean-room legal evidence, and `renderer_commercial_readiness=0`.
- `tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1`: passed in diagnostic mode with `renderer_commercial_readiness_final_preflight_status=blocked`, `renderer_commercial_readiness_final_preflight_ready=0`, `renderer_commercial_readiness_final_preflight_present_files=0`, `renderer_commercial_readiness_final_preflight_missing_files=12`, `renderer_commercial_readiness=0`, and blockers for every required final artifact plus `readiness_validator_not_run`.
- `tools/check-renderer-commercial-readiness-final-retained-root-assembler.ps1`: passed. It proves plan mode remains non-promoting, missing seven required host/package/legal inputs fail `-RequireReady`, unsafe output roots are rejected, the assembler stages child outputs under `_producer-artifacts`, and the complete self-test-only retained root reaches `renderer_commercial_readiness_final_preflight_ready=1`, `renderer_commercial_readiness_final_preflight_missing_files=0`, and `renderer_environment_ready=0`. This is a static contract self-test, not production retained host evidence.
- `tools/assemble-renderer-commercial-readiness-final-retained-root.ps1`: passed in default Plan mode with seven required inputs, 12 required final files, `renderer_commercial_readiness_final_assembler_ready=0`, and all broad renderer counters at `0`.
- `.github/workflows/validate.yml`: retains any existing `artifacts/renderer/commercial-readiness-evidence/final-retained/**` output from the Metal host lane as `renderer-commercial-readiness-final-retained-root`. The upload step is retention-only and uses `if-no-files-found: warn`; it does not assemble artifacts or change readiness counters.
- Context7 `/actions/upload-artifact` and the official `actions/upload-artifact` README confirm `if-no-files-found: warn`, `retention-days`, `compression-level`, and `include-hidden-files` are supported inputs; this slice keeps `include-hidden-files: false`, `compression-level: 0`, and job-local retention only.
- `tools/generate-environment-metal-optimization-artifacts.ps1`: now bounds hosted `xcrun xctrace record` with the official `--time-limit` option through default `XctraceTimeLimit=10m`, adds a host-level `XctraceRecordTimeoutSeconds=900` watchdog, and emits `xctrace_time_limit=10m` plus `xctrace_record_timeout_seconds=900`; this keeps the existing Metal host evidence lane fail-closed while preventing one recorder process tree from consuming the full macOS job timeout before surfacing.
- `tools/check-ai-integration.ps1`: passed after adding the final preflight and final retained-root assembler command, recipe, static check, docs, manifest, and composed-manifest needles.
- `tools/check-json-contracts.ps1`: passed after manifest composition.
- `tools/check-agents.ps1`: passed.
- `tools/check-format.ps1`: passed.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 40 static checks, including `check-renderer-commercial-readiness-final-promotion-preflight.ps1` and `check-renderer-commercial-readiness-final-retained-root-assembler.ps1`; Windows-host Apple/Metal tooling remained diagnostic-only host gates.

Task 10I handoff/runner integration evidence:

- Context7 `/websites/cli_github_manual` confirmed `gh run download <run-id> -n <name> -D <dir>` for exact workflow artifact downloads; Context7 `/microsoftdocs/powershell-docs` confirmed `ConvertTo-Json -Depth` for structured manifest output.
- `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1`: passed after requiring `assembler_handoff` and `final_preflight_handoff` manifest rows, exact assembler argument arrays for the seven input paths, a separate `-RequireReady` argument array, and continued `renderer_commercial_readiness=0`.
- `tools/check-validation-recipe-runner.ps1`: passed after adding safe DryRun coverage for `renderer-commercial-readiness-final-promotion-preflight`, `renderer-commercial-readiness-final-retained-root-assembler`, and `renderer-commercial-readiness-final-retained-root-artifact-import`; the assembler/import recipes use `-Mode Plan`, do not require `RunId`, and do not pass `-RequireReady`.
- `tools/check-renderer-commercial-readiness-final-retained-root-assembler.ps1` and `tools/check-renderer-commercial-readiness-final-promotion-preflight.ps1`: passed, preserving fail-closed assembler/preflight behavior.
- `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-format.ps1`, and `tools/check-agents.ps1`: passed after docs, manifest fragments, static checks, and composed manifest were synchronized.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 41 static checks in `out/validation-logs/validate-20260626-075417-54132`; Windows-host Apple/Metal tooling remained diagnostic-only host gates, and broad renderer counters remained non-ready.

Task 10L clean-room/legal review input retention evidence:

- `tools/generate-renderer-clean-room-legal-review-input.ps1` writes retained `clean-room-legal-review.json` and `official-source-summary.json` files under `artifacts/renderer/clean-room-legal-review/renderer-commercial-readiness` by default. The producer uses official public documentation URLs only, records source categories for Unity Terms/Trademark, Unreal Engine EULA/Trademark, and Godot license/compliance review, and preserves explicit non-claims for external-engine code, samples, assets, shaders, UI expression, trademarks, project import, APIs, compatibility, equivalence, parity, and legal advice.
- `.github/workflows/validate.yml` runs the producer in the agent static job and uploads `renderer-clean-room-legal-review-artifacts` with `actions/upload-artifact@043fb46d1a93c77aae656e7c1c64a875d1fc6a0a`, `include-hidden-files: false`, `compression-level: 0`, `retention-days: 14`, and `if-no-files-found: error`. This is retention-only evidence for later human/legal review input and does not promote readiness.
- `tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1` now treats `renderer-clean-room-legal-review-artifacts` as a required workflow artifact before final retained-root import handoff, so CI artifact inventory reports clean-room/legal review input availability, missing, or expiration before real promotion work.
- `run-validation-recipe.ps1 -Mode DryRun -Recipe renderer-clean-room-legal-review-input` plans only `tools/generate-renderer-clean-room-legal-review-input.ps1 -Mode Plan`; it does not write artifacts, download evidence, or pass `-RequireReady`.
- `tools/check-renderer-clean-room-legal-artifact.ps1`, `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1`, `tools/check-validation-recipe-runner.ps1`, and `tools/check-ci-matrix.ps1`: passed after adding producer, recipe, importer, and CI retention coverage.

Task 10M current-run artifact intake handoff evidence:

- Context7 `/websites/cli_github_manual` confirmed `gh run download <run-id> -n <name> -D <dir>` for exact workflow artifact downloads. GitHub Actions documentation confirms `github.repository` and `github.run_id` identify the current repository/run, and the official GitHub REST Actions artifacts endpoint lists workflow run artifacts with Actions read permission.
- `.github/workflows/validate.yml` now runs `renderer-commercial-artifact-intake` after `agent-static`, `windows`, `windows-cpp23`, `linux-vulkan`, and `macos`. Context7 official GitHub Actions documentation confirms `needs` is the workflow-supported way to make a downstream job wait for upstream artifact-producing jobs, `gh run download <run-id> -n <name>` is the supported CLI path for exact workflow artifact downloads, and `upload-artifact` exposes immutable artifact ids/URLs/digests for subsequent jobs. `tools/classify-pr-validation-tier.ps1` now selects `windows_cpp23_release` for renderer commercial closeout/evidence paths so the current-run artifact list does not treat `windows-packages` as missing merely because the selected package lane was skipped. The job records the current run artifacts API response at `artifacts/renderer/commercial-readiness-evidence/current-run-artifact-intake/workflow-artifacts.json`, runs `tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1 -Mode Import -RepoFullName "${{ github.repository }}" -RunId "${{ github.run_id }}"`, asserts the importer still emits `renderer_commercial_readiness=0`, and uploads `renderer-commercial-readiness-current-run-artifact-intake` with `compression-level: 0`, `include-hidden-files: false`, and `if-no-files-found: error`.
- `tools/check-ci-matrix.ps1`: failed first with `missing required job: renderer-commercial-artifact-intake`, then passed after the job, permissions, GitHub API call, importer invocation, artifact upload, PR Gate `needs`, checkout retry contract, and job id were wired.
- `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1`: passed after adding final-root-only handoff proof. The check now proves that an existing final retained root makes `final_preflight_handoff_ready=1` while `assembler_handoff_ready=0`, and that seven explicit assembler inputs make `assembler_handoff_ready=1` while all broad renderer counters remain `0`.
- `tools/check-validation-recipe-runner.ps1`: passed, preserving the artifact import recipe DryRun contract; recipe execution still uses `-Mode Plan`, does not require `RunId`, and does not pass `-RequireReady`.
- Hosted PR #837 run `28209702205` proved the `renderer-commercial-artifact-intake` job captures the current workflow artifact list and uploads the intake artifact, then exposed a native `$LASTEXITCODE` leak from handled `gh run download` misses. `tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1` now clears handled native download failures before returning unless readiness fails through explicit `-RequireReady`, so missing artifacts remain manifest/counter blockers instead of CI process failures.
- Local missing-artifact import reproduction passed with `last_exit=0` after one intentionally missing artifact while still emitting `renderer_commercial_readiness_final_retained_root_artifact_import_ready=0`.
- Task 10N local TDD evidence: `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1` first failed because `missing_assembler_input_names`, `final_root_workflow_artifact_available`, `assembler_source_workflow_artifacts`, and `missing_assembler_source_workflow_artifact_names` were not emitted. It now passes after the importer writes `artifact_handoff_strategy`, `missing_assembler_inputs`, exact final-root-vs-assembler-source workflow artifact counters, and exact missing assembler input names while preserving `renderer_commercial_readiness=0`.
- Task 10N validation evidence: `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-ci-matrix.ps1`, `tools/check-format.ps1`, `tools/check-agents.ps1`, and `tools/check-validation-recipe-runner.ps1` passed after docs, manifest fragments, composed manifest, and static needles were synchronized.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 41 static checks in `out/validation-logs/validate-20260626-104836-41708`; Windows-host Apple/Metal tooling remained diagnostic-only host gates and broad renderer counters stayed non-ready.
- `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-ci-matrix.ps1`, `tools/check-format.ps1`, and `tools/check-agents.ps1`: passed after docs, manifest fragments, composed manifest, workflow, and static needles were synchronized.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 41 static checks in `out/validation-logs/validate-20260626-100131-38844`; Windows-host Apple/Metal tooling remained diagnostic host gates and broad renderer counters stayed non-ready.

Task 10P package host evidence retention evidence:

- `tools/check-renderer-package-commercial-quality-host-evidence.ps1` first failed because `tools/collect-renderer-package-commercial-quality-host-evidence.ps1` did not exist. It now proves unsafe path rejection, broad `environment_ready=1` rejection, conversion of selected generated 3D visible/package-upload, 2D runtime UI atlas handoff, and sample desktop environment profile package status logs into non-fixture `GameEngine.RendererPackageCommercialQualityHostEvidence.v1`, then feeds that JSON through `tools/collect-renderer-package-commercial-quality-artifacts.ps1` while keeping `renderer_commercial_readiness=0`.
- `.github/workflows/validate.yml` now has the selected `windows-cpp23` lane collect three first-party installed package smoke logs and upload `renderer-package-commercial-quality-host-evidence` separately from immutable `windows-packages`. The current-run artifact importer treats that artifact as the package assembler source, so `package_host_evidence` can become present without claiming D3D12, strict Vulkan, Apple Metal, quality/VFX, final retained-root, broad renderer quality, broad Metal, backend parity, or commercial readiness.
- `tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1` now requires `renderer-package-commercial-quality-host-evidence` in addition to `windows-packages`; artifact intake can distinguish release ZIP availability from accepted package host evidence JSON availability.
- `tools/check-renderer-package-commercial-quality-host-evidence.ps1`, `tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1`, `tools/check-ci-matrix.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-validation-recipe-runner.ps1`, `tools/check-format.ps1`, and `tools/check-agents.ps1`: passed after producer, workflow, importer, manifest, validation recipe, and static needles were synchronized.
- `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`: passed with 44 static checks in `out/validation-logs/validate-20260626-141458-38248`; Windows-host Apple/Metal tooling remained diagnostic host gates and broad renderer counters stayed non-ready.

## Host Evidence Matrix

| Host lane | Required proof | Promotion rule |
| --- | --- | --- |
| Windows D3D12 | WARP or hardware D3D12 validation, barriers, timestamps, residency/debug rows, package readback | Can promote only D3D12 rows. |
| Linux or Windows strict Vulkan | Vulkan validation/synchronization clean logs, synchronization2 barriers, memory binding, SPIR-V validation, package readback | Can promote only strict Vulkan rows. |
| macOS full Xcode Metal | `metal`/`metallib`, MSL compliance, `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, visible package readback | Can promote only Apple Metal rows. |
| iOS Metal | Optional selected package evidence only when explicitly required | Cannot promote macOS Metal rows unless the plan selects iOS rows. |
| Docs/legal static | public-doc source register, no external-engine material rows, notices complete | Blocks every final readiness counter when unsafe. |

## Review Checklist

- [ ] Does the plan promote `renderer_commercial_readiness=1` only from retained artifacts, not manual switches?
- [ ] Can a fresh reviewer trace every ready counter to an exact file, row, source id, package counter, host recipe, and hash?
- [ ] Is every backend independently proven without inference?
- [ ] Are native handles absent from public APIs?
- [ ] Are Unity, Unreal Engine, and Godot used only as public-doc category/legal research?
- [ ] Are external-engine code/sample/asset/UI/trademark/API/compatibility/equivalence claims impossible without explicit blocker rows?
- [ ] Does default validation still pass without Apple tools and keep all broad renderer counters at `0`?
- [ ] Do docs, manifest, schemas, static checks, CI lane selection, and plan registry agree?
