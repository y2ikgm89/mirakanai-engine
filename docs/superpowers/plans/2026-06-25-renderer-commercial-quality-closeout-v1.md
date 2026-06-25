# Renderer Commercial Quality Closeout v1 Implementation Plan

**Plan ID:** `renderer-commercial-quality-closeout-v1`

**Path:** `docs/superpowers/plans/2026-06-25-renderer-commercial-quality-closeout-v1.md`

**Status:** Active.

Task 0/1 selection and fail-closed guard work is complete. Task 2 adds the value-only aggregate API and focused tests. Task 3 feature-binds the renderer Metal environment and memory/profiling recipe families. No renderer readiness claim is changed by this status.

**Date:** 2026-06-25

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

## Goal

Close out the renderer commercial-quality gap only after D3D12, strict Vulkan, and Apple-host Metal all prove the same backend-local renderer guarantees without cross-backend inference.

The target claims are:

- `renderer_backend_parity_ready=1`
- `renderer_metal_broad_readiness=1`
- `renderer_broad_quality_ready=1`
- `renderer_commercial_readiness=1`

The closeout must cover the visible surfaces that depend on renderer reliability: 3D scene rendering, runtime UI rendering and texture upload, environment rendering/package evidence, generated-game package output, GPU memory residency, and profiling/capture evidence.

The implementation must be a MIRAIKANAI-first renderer design, not a clone of Unity, Unreal Engine, Godot, or any other engine. External engines may be used only as public category research for market expectations and risk checks; their source code, sample code, shader code, UI layout, API names, asset names, logos, screenshots, proprietary formats, and distinctive product expression must not be copied.

## Current Audit Baseline

- At planning start, live production execution was at `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`, with `recommendedNextPlan.id = next-production-gap-selection`. Task 0/1 selection changes the live pointer to this active child plan while preserving all broad renderer counters at `0`.
- `renderer-backend-parity-v1` remains `host-gated`; required parity features are `synchronization`, `shader_validation`, `memory_residency`, `profiling_capture`, and `package_evidence`.
- The renderer quality matrix has D3D12 and strict Vulkan evidence but remains host-gated because the Metal row set is not complete. Current package evidence is `21` required rows, `14` ready rows, and `7` host-gated rows.
- `production-rendering-vfx-profiling-v1` has D3D12 and Vulkan host evidence but remains host-gated because Metal profiling evidence is not complete. Current package evidence is `3` required rows, `2` ready rows, and `1` host-gated row.
- The completed Metal memory/profiling artifact producer can produce `renderer_metal_memory_profiling_ready=1` on a macOS/full-Xcode host with real `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, `MTLCaptureScope`, and capture artifacts.
- Default Windows/Linux validation must continue to emit `renderer_metal_memory_profiling_status=host_evidence_required`, `renderer_backend_parity_ready=0`, `renderer_metal_broad_readiness=0`, `renderer_commercial_readiness=0`, and `renderer_broad_quality_ready=0` unless exact Apple-host artifacts are supplied.
- Static guards `tools/check-ai-integration-139-renderer-metal-memory-profiling-host-evidence.ps1`, `tools/check-ai-integration-140-renderer-metal-memory-profiling-host-evidence-collector.ps1`, and `tools/check-ai-integration-141-renderer-metal-memory-profiling-host-artifacts.ps1` intentionally block broad renderer promotion today.
- A source audit found a recipe-id boundary that must be resolved explicitly before broad promotion: `BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc` maps memory/profiling proof rows through the existing Apple Metal environment recipe id, while the artifact validator uses the dedicated `renderer-metal-memory-profiling-host-evidence` contract. The implementation must add or review the exact memory/profiling recipe id instead of silently broadening the environment recipe.

Local planning evidence collected on 2026-06-25:

| Command | Planning result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal` | Confirmed selection gate, retained Metal memory/profiling context, and preserved non-claims. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS: `unsupported_gaps=0`, `host_gates=12`, `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1` | Windows host-gated as expected; all broad renderer counters stayed `0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -SkipFocusedRendererBuild -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=host_evidence_required renderer_metal_memory_profiling_ready=0 renderer_backend_parity_ready=0 renderer_commercial_readiness=0 renderer_broad_quality_ready=0` | PASS: default host evidence remains fail-closed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` | D3D12 DXIL and Vulkan SPIR-V tools found; Apple `metal` and `metallib` missing on this Windows host as expected. |

Task 2 implementation evidence collected on 2026-06-25:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS: dev preset configured after adding `MK_renderer_commercial_quality_closeout_tests`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_commercial_quality_closeout_tests` | RED first failed on missing `renderer_commercial_quality_closeout.hpp`, then PASS after adding the value-only aggregate API. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_commercial_quality_closeout_tests"` | PASS: focused aggregate tests prove ready, host-gated, invalid, clean-room, notice, and replay-hash cases. |

Task 3 implementation evidence collected on 2026-06-25:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS: dev preset configured for the recipe-alignment worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | PASS after formatting: focused renderer test target rebuilt with feature-bound Metal recipe checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_tests"` | PASS: renderer parity tests prove environment recipe rows only cover `synchronization`, `shader_validation`, and `package_evidence`; memory/profiling recipe rows only cover `memory_residency` and `profiling_capture`; swapped recipe families are rejected; partial Metal family evidence keeps `metal_parity_ready=false`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -SkipFocusedRendererBuild -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=host_evidence_required renderer_metal_memory_profiling_ready=0 renderer_backend_parity_ready=0 renderer_commercial_readiness=0 renderer_broad_quality_ready=0` | PASS: default Windows host evidence remains fail-closed with all broad renderer counters at `0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-quality-closeout.ps1` | PASS: closeout validator still reports `renderer_commercial_quality_closeout_status=host_evidence_required`, `renderer_commercial_quality_closeout_value_api_ready=1`, and all broad renderer counters at `0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady` | Expected FAIL: still blocks on `validator_integration_and_host_evidence_required`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: all static checks, build, and 158 CTest tests passed; Apple Metal toolchain remains diagnostic-only on this Windows host. |

## Official Source And Context7 Review

Context7 was used for:

- `/websites/learn_microsoft_en-us_windows_win32_direct3d12`
- `/khronosgroup/vulkan-docs`
- `/dogukanveziroglu/metal-shading-language-specification`

Authoritative source constraints for implementation:

- D3D12 readiness rows must prove explicit application-owned resource state transitions, queue/fence synchronization, residency budget handling, and GPU timestamp/profiling evidence. Official Microsoft sources: resource barriers, multi-engine synchronization, residency, and timing.
- Vulkan readiness rows must prove `VK_LAYER_KHRONOS_validation`, synchronization2 barriers or equivalent reviewed synchronization rows, queue ownership/layout transitions where applicable, and explicit memory allocation/budget behavior. Official Khronos sources: Vulkan synchronization guide, validation overview, and memory allocation guide.
- Metal readiness rows must use Apple-host evidence for resource synchronization, feature availability, heap/residency-set behavior, and programmatic capture. Apple Developer Documentation and the Metal Feature Set Tables are authoritative for framework APIs and feature availability; Context7 coverage is limited to Metal Shading Language rules.
- Metal shader evidence must respect MSL restrictions, address spaces, resource binding attributes, shader function categories, and function-constant behavior. Do not infer framework API readiness from MSL-only docs.

Official references used for this plan:

- Microsoft D3D12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- Microsoft D3D12 multi-engine synchronization: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>
- Microsoft D3D12 residency: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency>
- Microsoft D3D12 timing: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing>
- Khronos Vulkan synchronization guide: <https://docs.vulkan.org/guide/latest/synchronization.html>
- Khronos Vulkan validation overview: <https://docs.vulkan.org/guide/latest/validation_overview.html>
- Khronos Vulkan memory allocation guide: <https://docs.vulkan.org/guide/latest/memory_allocation.html>
- Apple Metal capabilities and feature set tables: <https://developer.apple.com/metal/capabilities/>
- Apple `MTLHeap`: <https://developer.apple.com/documentation/metal/mtlheap>
- Apple `MTLResidencySet`: <https://developer.apple.com/documentation/metal/mtlresidencyset>
- Apple `MTLCaptureManager`: <https://developer.apple.com/documentation/metal/mtlcapturemanager>
- Apple programmatic capture guide: <https://developer.apple.com/documentation/xcode/capturing-a-metal-workload-programmatically>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/synchronizing-resource-access-between-multiple-passes>

## Legal And Clean-Room Source Review

This section is an engineering compliance gate, not legal advice. If a release, marketing claim, trademark use, or third-party content use is uncertain, stop and request legal review before shipping.

Official legal and licensing sources checked on 2026-06-25:

- Unity legal hub and Terms of Service: <https://unity.com/legal> and <https://unity.com/legal/terms-of-service>.
- Unity trademark guidelines: <https://unity.com/legal/branding-trademarks>.
- Unreal Engine EULA: <https://www.unrealengine.com/eula/unreal>.
- Unreal Engine release/commercial checklist: <https://www.unrealengine.com/release>.
- Epic Unreal Engine trademark approval page: <https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license>.
- Godot license page: <https://godotengine.org/license/>.
- Godot license-compliance documentation: <https://docs.godotengine.org/en/stable/about/complying_with_licenses.html>.

Clean-room rules for this plan:

- Use official D3D12, Vulkan, Metal, C++/CMake, platform, and profiler documentation for implementation behavior.
- Use Unity, Unreal Engine, and Godot public documentation only to identify broad renderer categories such as render-pipeline selection, UI rendering, profiling, material quality, VFX, mobile renderer constraints, package/export evidence, and license-notice expectations.
- Do not copy or translate external engine source code, shader code, sample code, tutorials, UI layouts, project templates, class names, method names, node names, asset names, icons, logos, screenshots, style guides, proprietary data formats, or branded workflows.
- Do not use Unity Asset Store, Unreal Marketplace/Fab, Epic sample, MetaHuman, Paragon, Godot demo, or third-party marketplace assets as renderer evidence unless a separate license audit records the asset, license, notice text, modification status, and distribution target.
- Do not use Unity, Unreal Engine, Epic, or Godot marks in product branding, marketing, package metadata, splash screens, badges, compatibility claims, or UI labels. Nominative references are allowed only in internal docs or legal-review text when they are accurate, secondary to MIRAIKANAI branding, and not suggestive of endorsement.
- Do not state `compatible with Unity`, `compatible with Unreal Engine`, `powered by Unreal Engine`, `Unity-like`, `Unreal-like`, `Godot-like`, `Blueprint-compatible`, `Shader Graph-compatible`, `Niagara-compatible`, or similar claims unless a separate legal and technical compatibility plan approves the exact wording and evidence.
- If any external code or asset is intentionally introduced later, update `THIRD_PARTY_NOTICES.md`, `docs/legal-and-licensing.md`, `docs/dependencies.md`, and `vcpkg.json` where applicable before the implementation can be considered ready.

MIRAIKANAI-specific product shape:

- The differentiator is evidence-driven renderer readiness: package-visible proof rows, deterministic readback hashes, official-profiler artifacts, backend-local parity rows, and machine-readable agent contracts.
- The renderer API remains first-party and backend-neutral through `mirakana::renderer`, `mirakana::rhi`, `mirakana::ui`, and package manifest counters.
- The editor/runtime experience should expose MIRAIKANAI concepts such as retained evidence rows, backend host gates, generated-game package validation, UI atlas handoff, and environment package proof, rather than copying external engine pipeline names or UI composition.
- Commercial quality is a measurable claim over selected first-party workloads, not a broad subjective comparison to another engine.

## Architecture

Add a fail-closed renderer commercial-quality aggregate in `MK_renderer`. The aggregate consumes existing first-party value plans and package counters; it must not run GPU commands, expose native handles, import SDK types into public headers, execute package scripts, or infer one backend from another backend.

The aggregate should model an explicit row matrix:

- Backend parity rows for D3D12, strict Vulkan, and Apple-host Metal.
- Renderer quality rows for materials, lighting/shadows, postprocess, sprite/UI, scene scale, GPU memory residency, and profiling/capture.
- Production VFX/profiling rows for D3D12, strict Vulkan, and Apple-host Metal.
- Package-visible rows for selected 3D, runtime UI atlas/upload, environment, and generated-game package outputs.
- Claim-control rows proving zero native-handle exposure, zero cross-backend inference, zero external-engine parity claims, zero subjective-only quality claims, zero unresolved host gates, and zero unreviewed recipe ids.
- Clean-room/legal rows proving zero copied external engine code/assets/samples, zero marketplace content, zero trademark/logo usage, zero compatibility/equivalence marketing claims, and complete third-party notices for any approved external material.

Proposed public value API:

- Create: `engine/renderer/include/mirakana/renderer/renderer_commercial_quality_closeout.hpp`
- Create: `engine/renderer/src/renderer_commercial_quality_closeout.cpp`
- Types:
  - `RendererCommercialQualityEvidenceRow`
  - `RendererCommercialQualityCloseoutDesc`
  - `RendererCommercialQualityCloseoutPlan`
  - `plan_renderer_commercial_quality_closeout`

The final ready state is true only when all of these are true:

- `BackendRendererParityPolicyPlan::status == BackendRendererParityPolicyStatus::ready`, `succeeded() == true`, and the existing `d3d12_parity_ready`, `vulkan_parity_ready`, and `metal_parity_ready` flags are all true.
- `RendererQualityMatrixPlan::general_renderer_quality_ready == true`.
- `RendererProductionVfxProfilingPlan::status == RendererProductionVfxProfilingStatus::ready`, `succeeded() == true`, and the existing D3D12, strict Vulkan, and Metal host-evidence flags are all true.
- Metal memory/profiling evidence is imported through the dedicated host evidence contract and mapped to reviewed backend parity rows.
- Selected package outputs prove visible 3D, UI renderer upload, environment rendering, and generated-game package rows.
- All recipe ids are reviewed and host validation recipes are present.
- Diagnostics are empty for missing evidence, unreviewed recipe ids, native handle access, cross-backend inference, subjective-only claims, crash/timeout handoffs, or broad external-engine parity claims.
- Clean-room diagnostics are empty for external API-name copying, copied sample/tutorial code, copied shader code, copied UI/layout expression, unlicensed assets, missing notices, trademark/logo use, and unapproved compatibility claims.

## Non-Goals

- Do not claim external-engine parity, Unreal/Unity/Godot compatibility, or marketplace asset compatibility.
- Do not mimic Unity, Unreal Engine, Godot, or marketplace product expression. Functional renderer categories may overlap because they are standard engine needs; names, APIs, UI expression, assets, samples, and claims must remain first-party.
- Do not claim `environment_ready`; this plan may consume environment package evidence but must not promote broad environment readiness.
- Do not claim Metal readiness from D3D12 or Vulkan proof.
- Do not add public native handle APIs.
- Do not make Apple SDK tools a default Windows/Linux validation dependency.
- Do not require GitHub-hosted macOS to pass `MTLResidencySet` readiness when the host rejects the feature. Hosted diagnostics may remain host-gated unless `-RequireReady` is explicitly selected on a capable Apple host.

## Done When

- A new aggregate value API has RED/GREEN tests proving fail-closed behavior for missing Metal, missing package rows, unreviewed recipe ids, cross-backend inference, native-handle requests, and partial quality matrices.
- The Metal memory/profiling recipe-id boundary is explicit and tested; memory/profiling rows cannot be satisfied by environment-only evidence.
- A selected Apple-host lane can supply Metal synchronization, shader validation, package evidence, memory residency, profiling/capture, and visible renderer package output rows.
- A selected D3D12 lane and strict Vulkan lane supply matching backend/package/quality/profiling rows.
- `tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady` emits `renderer_backend_parity_ready=1`, `renderer_metal_broad_readiness=1`, `renderer_broad_quality_ready=1`, and `renderer_commercial_readiness=1` only when all selected evidence rows are complete.
- Default validation on hosts without Apple Metal evidence remains `host_evidence_required` with all broad renderer counters at `0`.
- Docs, plans, manifest fragments, composed `engine/agent/manifest.json`, schemas, static checks, skills/rules if affected, package manifests, and CI lane selection agree on the new aggregate and preserved non-claims.
- Required focused checks, agent-surface checks, `tools/validate.ps1`, publication preflight, PR checks, ready wrapper, and guarded merge flow pass or record a concrete host blocker.
- A clean-room/legal audit step confirms that all renderer evidence is first-party or properly recorded, no external engine sample/code/asset/trademark was used, and any third-party dependency or asset introduced by the slice has complete notices.

## Task 0: Lock Official Source And Clean-Room Evidence Gate

**Files:**

- Modify: `docs/legal-and-licensing.md` only if new external material is introduced
- Modify: `docs/dependencies.md` only if new dependencies are introduced
- Modify: `THIRD_PARTY_NOTICES.md` only if new third-party code/assets are distributed
- Modify: `vcpkg.json` only if new package-manager dependencies are introduced
- Create or modify: renderer clean-room/static check in the Task 1 static guard

- [x] Record the exact official source set used by the implementation phase: Microsoft D3D12 docs, Khronos Vulkan docs, Apple Metal docs, and any official profiler/tool docs.
- [x] Record external engine docs used only for category research. Allowed engines for this plan are Unity, Unreal Engine, and Godot; allowed use is category taxonomy only.
- [x] Add static guard needles that reject final readiness if any implementation file, package manifest, or public docs introduce unapproved external-engine compatibility claims:

```text
Unity-compatible
Unreal-compatible
Godot-compatible
Unity-like
Unreal-like
Godot-like
Blueprint-compatible
Shader Graph-compatible
Niagara-compatible
powered by Unreal Engine
```

- [x] Add static guard needles requiring the positive clean-room counters in the final validator:

```text
renderer_clean_room_source_review_ready=1
renderer_external_engine_code_used=0
renderer_external_engine_sample_used=0
renderer_external_engine_asset_used=0
renderer_external_engine_trademark_used=0
renderer_external_engine_compatibility_claims=0
renderer_third_party_notices_complete=1
```

- [x] If any new external material is proposed, stop the renderer implementation until `THIRD_PARTY_NOTICES.md`, `docs/legal-and-licensing.md`, `docs/dependencies.md`, and dependency manifests are updated and `tools/check-dependency-policy.ps1` passes.
- [x] Run the legal/static RED gate:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: fail until the clean-room counters and no-claim needles are wired into the implementation/static checks.

## Task 1: Select The Plan And Add RED Guards

**Files:**

- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `docs/superpowers/plans/README.md`
- Create: `tools/check-ai-integration-142-renderer-commercial-quality-closeout.ps1`
- Modify: `tools/check-ai-integration.ps1` or the static-contract ledger entry that owns check discovery

- [x] Select this plan as the active child plan only when implementation begins.
- [x] Add a failing static guard for the new aggregate literals:

```text
renderer-commercial-quality-closeout-v1
RendererCommercialQualityCloseoutDesc
plan_renderer_commercial_quality_closeout
tools/validate-renderer-commercial-quality-closeout.ps1
renderer_commercial_readiness=1
renderer_broad_quality_ready=1
renderer_metal_broad_readiness=1
renderer_backend_parity_ready=1
renderer_external_engine_parity=0
renderer_native_handle_access=0
renderer_clean_room_source_review_ready=1
renderer_external_engine_code_used=0
renderer_external_engine_sample_used=0
renderer_external_engine_asset_used=0
renderer_external_engine_trademark_used=0
renderer_external_engine_compatibility_claims=0
renderer_third_party_notices_complete=1
```

- [x] Require preserved default non-claims in the guard until the ready validator supplies exact evidence:

```text
renderer_commercial_readiness=0
renderer_broad_quality_ready=0
renderer_metal_broad_readiness=0
```

- [x] Run RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: fail on missing aggregate API, validator, docs, manifest fragments, and package counters.

## Task 2: Add The Aggregate Renderer Value API

**Files:**

- Create: `engine/renderer/include/mirakana/renderer/renderer_commercial_quality_closeout.hpp`
- Create: `engine/renderer/src/renderer_commercial_quality_closeout.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Create or modify: `tests/unit/renderer_commercial_quality_closeout_tests.cpp`
- Modify: top-level CMake test registration if needed

- [x] Add RED tests for:
  - all evidence ready emits final ready.
  - missing Metal backend parity keeps `host_evidence_required`.
  - missing Metal quality rows keeps `host_evidence_required`.
  - missing Metal memory/profiling rows keeps `host_evidence_required`.
  - missing selected 3D/UI/environment/generated-game package row keeps `host_evidence_required`.
  - D3D12/Vulkan proof cannot satisfy Metal rows.
  - native-handle access, unreviewed recipe ids, crash handoffs, subjective-only claims, and external-engine parity claims keep readiness false.
  - external engine sample/code/asset/trademark/compatibility claim rows keep readiness false.
  - missing third-party notices keep readiness false when any approved external material row is present.
- [x] Implement the aggregate using only value rows and existing plans.
- [x] Keep row ids stable and package/manifest-friendly.
- [x] Run focused tests:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_commercial_quality_closeout_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_commercial_quality_closeout_tests"
```

## Task 3: Resolve Metal Memory/Profiling Recipe Alignment

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp`
- Modify: `engine/renderer/src/backend_renderer_parity_policy.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp` or the owning renderer parity tests
- Modify: `tools/check-renderer-metal-memory-profiling-host-evidence.ps1` only if the validator contract must expose the reviewed recipe id
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`

- [x] Add or review the dedicated `renderer-metal-memory-profiling-host-evidence` recipe id for `memory_residency` and `profiling_capture`.
- [x] Prove environment evidence can satisfy only `synchronization`, `shader_validation`, and `package_evidence`.
- [x] Prove memory/profiling evidence can satisfy only `memory_residency` and `profiling_capture`.
- [x] Prove broad Metal/backend readiness remains false when either recipe family is missing.
- [x] Run focused renderer parity tests and host-evidence validator defaults.

## Task 4: Add Selected Metal Visible Renderer Package Evidence

**Files:**

- Modify: `engine/rhi/metal/*` only behind Apple-private implementation boundaries
- Modify or create: Apple-only test/probe under the existing Metal evidence pattern
- Modify: `tools/validate-renderer-metal-apple.ps1`
- Modify: `.github/workflows/validate.yml` macOS Metal lane if selected
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: selected UI/runtime/environment package manifests only for evidence counters they actually emit

- [ ] Define exact Apple-host visible rows for:
  - 3D scene material/lighting/postprocess output.
  - UI renderer atlas/upload/readback or package-visible upload evidence.
  - environment renderer package row consumption.
  - generated-game package output row.
- [ ] Use Apple Metal shader/toolchain evidence only from real `metal`/`metallib` capable hosts.
- [ ] Keep Objective-C++ and `MTL*` objects private to Apple implementation/test boundaries.
- [ ] Emit deterministic readback/hash or package counters; do not rely on screenshots or subjective review alone.
- [ ] Keep non-Apple hosts diagnostic/host-gated by default.

## Task 5: Add Final Validator And CI Selection

**Files:**

- Create: `tools/validate-renderer-commercial-quality-closeout.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/classify-pr-validation-tier.ps1`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `.github/workflows/validate.yml`
- Modify: package validators that own selected package counters

- [ ] Default invocation runs focused non-ready checks and emits exact host-gated counters without requiring Apple tools.
- [ ] `-RequireReady` requires selected D3D12, strict Vulkan, Apple-host Metal, quality matrix, VFX/profiling, memory/profiling, package, and static guard evidence.
- [ ] `-RequireReady` requires clean-room source review counters and complete third-party notice counters.
- [ ] CI lane selection requires Windows MSVC/D3D12, Linux or Windows strict Vulkan, macOS Metal host evidence, and iOS Metal only where the selected claim explicitly consumes iOS package evidence.
- [ ] The validator emits the final four ready counters only at the last aggregate gate.
- [ ] The validator fails if any broad claim is present without exact evidence.
- [ ] The validator fails if any external engine API, sample, asset, trademark, UI expression, or compatibility/equivalence claim appears without explicit legal and technical approval.

## Task 6: Synchronize Docs, Manifest, Schemas, And Agent Surfaces

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json` if generated-game guidance changes
- Modify: `schemas/engine-agent.schema.json` only if new manifest shape is introduced
- Modify: `.agents/skills/gameengine-rendering/*`, `.claude/skills/gameengine-rendering/*`, and Cursor pointers only if durable renderer workflow guidance changes

- [ ] Update current capability truth before changing any public readiness text.
- [ ] Compose the manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [ ] Run agent-surface and JSON contract checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

## Task 7: Final Validation And Publication

- [ ] Run focused renderer/RHI/package validators after each behavior phase.
- [ ] Run text and format checks after docs/manifest/static checks settle.
- [ ] Run full validation after code, tests, docs, manifest, and static checks agree:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Run publication preflight before staging/push/PR:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

- [ ] Publish as a focused PR with evidence. Keep the PR draft until selected hosted lanes prove the ready aggregate or record a concrete host blocker.

## Review Checklist

- [ ] Are the final ready counters emitted only by `tools/validate-renderer-commercial-quality-closeout.ps1 -RequireReady`?
- [ ] Can every package-visible row point to a selected validator, package manifest, or retained artifact?
- [ ] Can a fresh reader tell why Metal is ready without looking at D3D12/Vulkan evidence?
- [ ] Does the aggregate reject native handle exposure and external-engine parity claims?
- [ ] Does Windows/Linux default validation still pass without Apple tools and keep broad counters at `0`?
- [ ] Do docs, manifest fragments, static checks, CI selection, and package manifests state the same readiness truth?
