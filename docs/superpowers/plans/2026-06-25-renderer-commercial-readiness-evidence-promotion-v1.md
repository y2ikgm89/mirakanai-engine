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

- `main` is aligned with `origin/main` at merge commit `95d49539`.
- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points to `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`.
- `recommendedNextPlan.id = next-production-gap-selection`.
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
- Add: `tests/fixtures/renderer/commercial-readiness-evidence/ready/*-*.json` fixture-only retained artifact files
- Modify: `tools/validate-renderer-commercial-quality-closeout.ps1`
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

- [ ] Add the new validation recipe and command surfaces.
- [ ] Compose manifest after every manifest fragment edit.
- [ ] Keep current capabilities honest: commercial readiness remains `0` until real host artifacts pass.
- [ ] Add static checks for exact counters, legal non-claims, source ids, and backend independence.
- [ ] Update CI lane classification for the new validator path.

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

Final local command sequence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-evidence.ps1 -RequireReady -ArtifactRootRelative <retained-artifact-root>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Publication command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

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
