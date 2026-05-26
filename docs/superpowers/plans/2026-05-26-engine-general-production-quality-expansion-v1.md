# 2026-05-26 Engine General Production Quality Expansion v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the current selected-package and foundation proofs into a phased general-production quality track for renderer, runtime UI, asset/import, physics/navigation, networking, and audio without reopening the Engine 1.0 zero-gap ready surface or claiming unsupported broad parity early.

**Architecture:** Keep public contracts first-party, backend-neutral, and value-based until a phase has host/package evidence. Optional middleware and SDK integrations live behind opaque adapters, vcpkg manifest features, dependency/legal records, and explicit validation recipes. Each phase promotes only the specific evidence it proves, with fail-closed diagnostics for broad claims, native handle leakage, backend inheritance, and missing host gates.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_ui`, `MK_ui_renderer`, `MK_assets`, `MK_tools`, `MK_runtime`, `MK_physics`, `MK_navigation`, `MK_audio`, optional adapter modules (`MK_physics_jolt`, `MK_runtime_network_enet`, `MK_audio_sdl3`, future text/font/importer adapters), generated 2D/3D package samples, CMake/CTest, PowerShell validation tools, Direct3D 12, Vulkan, Apple Metal host gates, SDL3, HarfBuzz/FreeType-class text stack, Khronos glTF/KTX validation, Recast/Detour-class navigation, ENet-class transport, and audited audio codec/spatialization dependencies when selected.

---

**Plan ID:** `engine-general-production-quality-expansion-v1`

**Status:** Active.

## Context

The current engine is strong as a clean C++23 foundation with selected package proof, but the user explicitly called out four broad gaps that must remain honest until they are implemented and validated:

- Renderer has strong selected package proof, but not a general production renderer quality claim.
- Runtime UI has first-party contracts and adapters, but production text shaping, font rasterization, IME, accessibility, and broad platform UI parity are still future or dependency-gated.
- Asset/import has a reviewed first-party pipeline, but arbitrary importers, live shader generation, broad codec support, and broad source import are incomplete.
- Physics/navigation/network/audio have foundations and selected proofs, but not commercial-game breadth for all genres.

This plan selects a post-1.0 developer-owned milestone to close those broad gaps in phases. It does not change `unsupportedProductionGaps = []`; that field remains the Engine 1.0 ready-surface truth. Each phase may land as one or more reviewable PRs, but the active plan stays one milestone because the phases share the same promotion rule: no broad production claim without official-practice checks, focused tests, package-visible evidence, manifest sync, and host-gate evidence.

Current implementation anchors:

- Renderer: `engine/renderer/`, `engine/rhi/`, `engine/runtime_rhi/`, `engine/runtime_scene_rhi/`, `engine/scene_renderer/`, `tests/unit/renderer_*`, `games/sample_generated_desktop_runtime_3d_package/`, `tools/validate-installed-desktop-runtime.ps1`.
- Runtime UI: `engine/ui/`, `engine/ui_renderer/`, `engine/platform/sdl3/`, `tests/unit/runtime_ui_workbench_tests.cpp`, `tests/unit/ui_renderer_tests.cpp`, `docs/ui.md`.
- Asset/import/tooling: `engine/assets/`, `engine/tools/asset/`, `engine/tools/gltf/`, `engine/tools/include/mirakana/tools/`, `tests/unit/assets_*`, `tests/unit/tools_*`, sample `game.agent.json` files.
- Physics/navigation: `engine/physics/`, `engine/physics/jolt/`, `engine/navigation/`, `engine/runtime/src/physics_collision_runtime.cpp`, `tests/unit/physics_*`, `tests/unit/navigation_tests.cpp`.
- Networking: `engine/runtime/include/mirakana/runtime/network_transport.hpp`, `engine/runtime/src/network_transport.cpp`, `engine/runtime/network/enet/`, `engine/runtime/src/production_network_replication.cpp`, `tests/unit/runtime_network*`.
- Audio: `engine/audio/`, `engine/audio/sdl3/`, `tests/unit/sdl3_audio_tests.cpp`, selected package audio payloads.
- Agent surfaces: `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`, `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `engine/agent/manifest.json`.

## Official Practice Check

Before changing code in each phase, re-check the exact SDK or dependency docs relevant to that phase and record the checked source in the phase evidence. The planning pass used these current official anchors:

- D3D12 renderer work must require explicit resource-state/barrier, queue/fence, allocator lifetime, and multi-engine synchronization reasoning: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12> and <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>.
- Vulkan renderer work must require synchronization2, image layout/access/stage evidence, queue-family ownership reasoning, validation-layer evidence, and SPIR-V validation evidence: <https://docs.vulkan.org/guide/latest/synchronization.html> and <https://docs.vulkan.org/guide/latest/validation_overview.html>.
- Metal renderer work remains Apple-host-gated until macOS/Xcode/Metal tools validate the exact feature: <https://developer.apple.com/documentation/metal/resource-synchronization> and <https://developer.apple.com/metal/capabilities/>.
- SDL3 platform/UI work must keep `SDL_StartTextInput`, `SDL_StopTextInput`, `SDL_SetTextInputArea`, clipboard, audio stream/device, and native dialog calls in platform/runtime-host adapters, on their documented thread/lifetime boundaries: <https://wiki.libsdl.org/SDL3/>.
- HarfBuzz-class shaping work must separate Unicode text, buffer direction/script/language, glyph ids, clusters, advances, offsets, and feature rows from font rasterization and renderer atlas upload: <https://github.com/harfbuzz/harfbuzz>.
- FreeType-class rasterization work must treat glyph metrics, hinting, bitmap rendering, and atlas placement as separate evidence from shaping: <https://freetype.org/freetype2/docs/glyphs/index.html>.
- Accessibility work must expose first-party semantic roles/states/focus/actions before native OS bridge publication and should be checked against WAI-ARIA APG concepts plus platform accessibility SDKs for each backend: <https://www.w3.org/WAI/ARIA/apg/>.
- Asset import must treat Khronos glTF 2.0 validation, extension support, KTX2/Basis texture handling, and shader compiler execution as reviewed gates rather than arbitrary importer execution: <https://www.khronos.org/gltf> and <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>.
- Navigation work that considers Recast/Detour must keep generated nav data and Detour queries behind first-party assets/adapters, with agent dimensions, polygon corridors, string-pulling, tiled navmesh, dynamic obstacle, and crowd evidence recorded: <https://recastnav.com/>.
- Native physics middleware work must keep Jolt or other middleware behind first-party opaque adapters, with determinism, controller, constraint, collision filtering, and capacity diagnostics recorded before promotion: <https://secondhalfgames.github.io/jolt-docs/5.0.0/>.
- Networking work must start from a threat model and keep transport/session/security/replication evidence separate. ENet-class reliable UDP adapters remain optional and do not imply matchmaking, NAT traversal, encryption, or broad multiplayer readiness: <https://github.com/lsalzman/enet>.
- Audio work must keep decoded asset metadata, streaming, device submission, DSP graph, HRTF/spatialization, and codec dependencies separate. OpenAL/miniaudio/Steam Audio-class dependencies require separate dependency/legal and host-gate review before selection: <https://www.openal.org/documentation/> and <https://miniaud.io/docs/manual/index.html>.

## Constraints

- No backward-compatibility shims, deprecated aliases, duplicate APIs, or migration layers unless a future release policy explicitly requires them.
- No public D3D12, Vulkan, Metal, SDL3, Dear ImGui, PIX, RenderDoc, Xcode, Jolt, Recast/Detour, ENet, OpenAL, miniaudio, HarfBuzz, FreeType, ICU, or middleware-native handles in gameplay-facing APIs.
- No broad renderer, runtime UI, importer, physics, navigation, networking, or audio production-readiness claim until the exact phase has tests, package evidence, docs, manifest rows, static checks, and host-gate evidence.
- Optional dependencies must be vcpkg manifest features with pinned registry baseline impact reviewed through `tools/bootstrap-deps.ps1`, and must update `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, and validation wrappers in the same phase.
- Game-creation agents remain game-surface scoped. Engine internals are changed only by developer-owned engine tasks under this plan.
- Package samples are proof surfaces, not product demos. Counters must report exact evidence and must fail closed on broad unsupported requests.
- Metal, Apple, Android device, real-network, external-service, and platform accessibility evidence remains host-gated unless the phase runs on a suitable host and records the exact command/evidence.

## Done When

- Each domain has a first-party production contract with fail-closed diagnostics, focused tests, package-visible evidence, and no native/middleware leakage.
- Renderer quality has per-backend evidence gates for D3D12, strict Vulkan, and Apple-host Metal, with quality-budget counters for materials, lighting/shadows, postprocess, sprite/UI, scene scale, memory/residency, profiling, and capture handoff.
- Runtime UI has a real text/font/IME/accessibility pipeline: shaping, font loading/rasterization, glyph atlas placement, text edit behavior, platform IME session dispatch, semantic accessibility tree publication, and renderer submission are separated and validated.
- Asset/import has reviewed broad source import gates for glTF/KTX/image/audio/material/shader sources, deterministic cook outputs, dependency/legal records, validation tools, and explicit denial of arbitrary importer execution.
- Physics/navigation has production breadth for controllers, queries, joints, vehicles, navmesh import/bake, path smoothing, crowds, streaming nav data, optional middleware adapters, and selected 2D/3D package proof.
- Networking has a threat-modeled transport/session/replication execution lane with loopback and optional adapter host evidence, without claiming matchmaking/NAT/encryption/cloud readiness before those are implemented.
- Audio has production playback/streaming/mixer/device/spatialization evidence with codec dependencies and package proof, without claiming middleware or HRTF parity before adapter evidence.
- Docs, roadmap, plan registry, master-plan backlog/projection chapters, manifest fragments, composed manifest, schemas/static checks, skills/rules/subagents when durable workflow changes, and validation recipes remain aligned.
- The final milestone closeout returns `currentActivePlan` to the master plan or the next selected child plan, keeps `unsupportedProductionGaps = []` unless a future 1.0 policy reopens it, and records full validation evidence.

## Phase 0 - Select The Milestone

**Files:**

- Create: `docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [x] Create this dated milestone plan and select it as the active developer-owned production quality expansion track.
- [x] Keep `unsupportedProductionGaps = []` because this is post-1.0 production breadth, not a reopened Engine 1.0 blocker.
- [x] Register `engine-general-production-quality-expansion-v1` in the plan registry, roadmap, master plan current verdict, and canonical post-1.0 backlog as `selected-production-slice`.
- [x] Update `recommendedNextPlan` to point at this plan with the first implementation phase as renderer quality matrix work.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

Expected at this checkpoint: PASS for all docs/agent/static checks, with no C++ behavior claim made by the planning slice.

Phase 0 evidence on 2026-05-26:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` wrote `engine/agent/manifest.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `git diff --check` passed.

## Immediate Next Work Queue

Execute phases in this order. Each phase should be a reviewable PR or a small cluster of tightly related PRs. Do not promote a phase from planned to ready until its "Phase Evidence" section is filled with commands, results, and host gates.

## Phase 1 - Renderer General Quality Matrix v1

**Goal:** Replace selected-package renderer confidence with explicit production-quality gates across materials, lighting/shadows, postprocess, sprite/UI, scene scale, GPU memory, profiling, and backend parity.

**Files:**

- Create or modify: `engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp`
- Create or modify: `engine/renderer/src/renderer_quality_matrix.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp`
- Modify: `engine/renderer/src/production_vfx_profiling.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp`
- Modify: `engine/renderer/src/backend_renderer_parity_policy.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp`
- Modify: `engine/renderer/src/gpu_memory_policy.cpp`
- Modify: `tests/unit/renderer_production_vfx_profiling_tests.cpp`
- Create or modify: `tests/unit/renderer_quality_matrix_tests.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify or create: `tools/check-ai-integration-098-renderer-quality-matrix.ps1`

- [x] Re-check D3D12, Vulkan, and Metal official docs for any API touched in this phase and record the exact URLs in this plan.
- [x] Add RED tests requiring a renderer quality matrix to fail closed when any claim lacks backend-local evidence.
- [x] Add RED tests for D3D12 resource-state/barrier/fence evidence, strict Vulkan synchronization2/layout/validation/SPIR-V evidence, and Apple-host-gated Metal evidence as independent rows.
- [x] Add RED tests that reject public native handles, capture execution side effects, crash upload execution, inferred backend parity, and subjective visual-quality claims without evidence.
- [x] Implement backend-neutral `RendererQualityMatrix*` value rows and diagnostics. Keep data explicit: feature id, backend id, proof source, shader/tool validation, resource synchronization, package counter ids, timing budget rows, host gate, and unsupported claim rows.
- [x] Emit selected package counters for D3D12 and strict Vulkan only when their row evidence is ready; emit Metal as host-gated until Apple evidence exists.
- [x] Update generated 3D package smoke and installed validation to require exact renderer quality fields, not a single broad `renderer_ready` flag.
- [x] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests MK_renderer_quality_matrix_tests sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests|MK_renderer_quality_matrix_tests|sample_generated_desktop_runtime_3d_package_smoke"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Phase Evidence:** Phase 1 implementation and local validation complete; GitHub Flow publication is in progress.

- Official docs rechecked on 2026-05-26:
  - Microsoft Learn Direct3D 12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
  - Microsoft Learn Direct3D 12 multi-engine synchronization/fences: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>
  - Microsoft Learn D3D12 GPU-based validation/debug layer: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation>
  - Khronos Vulkan synchronization/cache control: <https://docs.vulkan.org/spec/latest/chapters/synchronization.html>
  - Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
  - Apple Metal feature set tables: <https://developer.apple.com/metal/capabilities/>
- RED test evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_quality_matrix_tests` failed before implementation because `mirakana/renderer/renderer_quality_matrix.hpp` did not exist.
- Focused build evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests MK_renderer_quality_matrix_tests sample_generated_desktop_runtime_3d_package` passed.
- Focused test evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests|MK_renderer_quality_matrix_tests|sample_generated_desktop_runtime_3d_package_smoke"` passed.
- Package evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed; `plan_renderer_quality_matrix` installed smoke required `renderer_quality_matrix_status=host_evidence_required`, `renderer_quality_matrix_rows=21`, `renderer_quality_matrix_ready_rows=14`, `renderer_quality_matrix_host_gated_rows=7`, D3D12/Vulkan ready, Metal host-gated, side effects zero, and `renderer_quality_matrix_general_renderer_quality_ready=0`.
- Full slice validation evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; 88/88 CTest tests passed, and Apple/Metal evidence remained diagnostic host-gated on this Windows host.

## Phase 2 - Runtime UI Text, Font, IME, And Accessibility Production Stack

**Goal:** Move runtime UI from value-only adapter contracts to a production text and platform UI pipeline while keeping SDL3, OS SDKs, font engines, and renderer upload details behind adapters.

**Files:**

- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`
- Modify: `engine/ui/include/mirakana/ui/runtime_ui_workbench.hpp`
- Modify: `engine/ui/src/runtime_ui_workbench.cpp`
- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify: `engine/ui_renderer/src/ui_renderer.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
- Modify: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`
- Create optional adapter folders only when selected: `engine/ui/text/`, `engine/ui/font/`, or equivalent existing ownership-compatible paths.
- Modify: `tests/unit/runtime_ui_workbench_tests.cpp`
- Modify: `tests/unit/ui_renderer_tests.cpp`
- Modify: `docs/ui.md`
- Modify package sample files only after value contracts are green.

- [ ] Re-check SDL3 text input/clipboard, HarfBuzz shaping, FreeType glyph metrics/rasterization, and platform accessibility docs for the exact adapter path selected.
- [ ] Add RED tests for shaping request segmentation, glyph clusters, advances/offsets, fallback-font rows, bidi/line-break boundaries, and unsupported broad text-layout claims.
- [ ] Add RED tests for glyph rasterization request validation, glyph bitmap rows, atlas placement rows, eviction/budget diagnostics, and renderer texture upload handoff without public RHI handles.
- [ ] Add RED tests for IME session begin/update/end, candidate rows, text input area/cursor rows, committed text application, and platform adapter dispatch boundaries.
- [ ] Add RED tests for accessibility semantic tree rows: role, label, state, focus, action, relationships, live-region/update rows, and OS publication host gates.
- [ ] Implement the clean-break first-party text stack contracts. Keep shaping, rasterization, atlas packing, renderer submission, IME, and accessibility publication as separate value rows.
- [ ] Add optional dependency adapter plans only after dependency/legal review. Do not add HarfBuzz, FreeType, ICU, or platform SDK dependencies silently.
- [ ] Add selected package counters for visible runtime UI text/atlas and accessibility/IME readiness. Counters must report adapter invocations and host gates separately.
- [x] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_tests MK_ui_renderer_tests sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_ui|ui_renderer|sample_2d_desktop_runtime_package_smoke|sample_generated_desktop_runtime_3d_package_smoke"
```

**Phase Evidence:** Not started.

## Phase 3 - Broad Reviewed Asset Import And Cook Pipeline

**Goal:** Expand asset/import from first-party reviewed flows to broad, deterministic, audited import/cook surfaces without allowing arbitrary importer execution or live shader generation.

**Files:**

- Modify: `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`
- Modify: `engine/assets/src/asset_import_pipeline.cpp`
- Create: `engine/assets/include/mirakana/assets/asset_import_production_review.hpp`
- Create: `engine/assets/src/asset_import_production_review.cpp`
- Modify: `engine/tools/include/mirakana/tools/asset_import_tool.hpp`
- Modify: `engine/tools/asset/asset_import_tool.cpp`
- Modify: `engine/tools/include/mirakana/tools/asset_import_adapters.hpp`
- Modify: `engine/tools/asset/asset_import_adapters.cpp`
- Modify: `engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp`
- Modify: `engine/tools/gltf/gltf_node_animation_import.cpp`
- Modify or add KTX/image/audio/shader reviewed importer adapter files only after dependency/legal review.
- Modify: `vcpkg.json` only when selecting dependencies.
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `tests/unit/assets_*`
- Modify: `tests/unit/tools_*`
- Create: `tests/unit/asset_import_production_review_tests.cpp`
- Modify: sample `game.agent.json` importer capability rows.

- [x] Re-check Khronos glTF 2.0, glTF Validator, KTX2/Basis, DXC/SPIR-V validation, selected image/audio codec, and vcpkg official docs before dependency changes.
- [x] Add RED tests for reviewed import manifests: allowed source roots, explicit importer id, declared extensions, declared output package rows, license/provenance rows, and deterministic content hashes.
- [x] Add RED tests that reject arbitrary importer plugins, external downloads, live shader generation, source mutation outside reviewed roots, native handles, and compiler execution without reviewed command rows.
- [ ] Add glTF/KTX/source-image/audio import breadth in small sub-phases. Each adapter must have schema validation, source diagnostics, package handoff rows, and legal/dependency records before promotion.
- [x] Add shader generation only as reviewed offline compile-request/package-cache planning unless a separate phase implements actual compiler execution with host/toolchain evidence.
- [x] Update generated 2D/3D game manifests with broad import capability descriptors only for implemented adapters.
- [ ] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "asset|tools|gltf|import"
```

**Phase Evidence:** In progress.

Phase 3 candidate evidence on 2026-05-26:

- Official sources checked before this candidate: Microsoft vcpkg manifest mode / `vcpkg.json` docs, Khronos glTF 2.0 specification and glTF Validator docs, Khronos KTX/KTX2/Basis pages, Context7 `/microsoft/vcpkg`, Context7 `/khronosgroup/ktx-software`, and Context7 `/jkuhlmann/cgltf`.
- RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_production_review_tests` failed while `mirakana/assets/asset_import_production_review.hpp` did not exist.
- GREEN evidence so far: `MK_assets` now exposes `AssetImportProductionReviewRequest`, `AssetImportProductionEvidenceRow`, `AssetImportProductionDiagnosticCode`, `AssetImportProductionReview`, and `review_asset_import_production_readiness` as a value-only review surface. Focused build and CTest passed for `MK_asset_import_production_review_tests`.
- Focused validation passed:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_production_review_tests MK_assets_tests MK_tools_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_identity_runtime_resource_tests MK_tools_runtime_hot_reload_package_tests sample_ui_audio_assets`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "asset|tools|gltf|import"` passed 6/6 tests.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`, `tools/build-asset-importers.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, and `tools/check-format.ps1` passed.
- Full slice validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with `validate: ok` and CTest 88/88 passed. Diagnostic-only host gates remained Metal/Apple host evidence, as expected on this Windows host.
- This candidate intentionally does not implement a KTX2/Basis importer adapter, new image/audio codecs, live shader generation, compiler execution, package mutation, runtime source parsing, external downloads, or broad codec readiness. Adapter sub-phases remain unchecked until dependency/legal records and host/package evidence exist.

## Phase 4 - Physics And Navigation Production Breadth

**Goal:** Move from selected first-party physics/navigation proof to production breadth for controllers, casts, constraints, vehicles, navmesh import/bake, path smoothing, crowds, and streaming nav data.

**Files:**

- Modify: `engine/physics/include/mirakana/physics/physics3d.hpp`
- Modify: `engine/physics/src/physics3d.cpp`
- Modify: `engine/physics/include/mirakana/physics/collision_query.hpp`
- Modify: `engine/physics/include/mirakana/physics/native_adapter.hpp`
- Modify: `engine/physics/src/native_adapter.cpp`
- Create: `engine/physics/include/mirakana/physics/physics_production_breadth.hpp`
- Create: `engine/physics/src/physics_production_breadth.cpp`
- Modify optional: `engine/physics/jolt/`
- Modify: `engine/navigation/include/mirakana/navigation/navigation_navmesh.hpp`
- Modify: `engine/navigation/src/navigation_navmesh.cpp`
- Modify: `engine/navigation/include/mirakana/navigation/navigation_crowd.hpp`
- Modify: `engine/navigation/src/navigation_crowd.cpp`
- Modify: `engine/navigation/include/mirakana/navigation/navigation_hierarchical_world.hpp`
- Modify: `engine/navigation/src/navigation_hierarchical_world.cpp`
- Create: `engine/navigation/include/mirakana/navigation/navigation_production_breadth.hpp`
- Create: `engine/navigation/src/navigation_production_breadth.cpp`
- Create: `tests/unit/physics_navigation_production_breadth_tests.cpp`
- Modify: `tests/unit/physics_*`
- Modify: `tests/unit/navigation_tests.cpp`
- Create or modify: `tools/check-ai-integration-101-physics-navigation-production-breadth.ps1`
- Modify 2D/3D package samples and installed validation after focused APIs are green.

- [x] Re-check Jolt and Recast/Detour documentation before optional adapter or navmesh import changes.
- [x] Add RED tests for oriented box/convex/mesh query review rows, persistent joint asset validation, ragdoll/constraint group diagnostics, controller tuning rows, vehicle policy rows, and deterministic replay signatures.
- [x] Add RED tests for navmesh asset import/bake review rows, agent dimension gates, polygon corridor rows, string-pulling path rows, dynamic obstacle updates, tiled/region nav references, crowd/local-avoidance budgets, and streaming nav data readiness.
- [x] Implement first-party value contracts before optional adapters. Native middleware remains opaque and optional.
- [ ] Add package-visible counters for selected 2D/3D physics/nav production probes with exact body/query/agent/path/crowd budgets.
- [ ] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physics_navigation_production_breadth"
```

**Phase Evidence:** Initial review-gate slice in progress.

Initial Phase 4 evidence on 2026-05-26:

- Official practice re-check: Context7 returned Jolt Physics collision/query documentation around `BroadPhaseQuery`, `NarrowPhaseQuery`, ray casts, shape casts, and point tests from `https://jrouwe.github.io/JoltPhysics/`; Context7 returned Recast Navigation module and tiled-navmesh/crowd/corridor documentation from `https://recastnav.com/`.
- RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests` failed while `mirakana/navigation/navigation_production_breadth.hpp` did not exist.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_physics_navigation_production_breadth_tests` passed.
- GREEN focused test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physics_navigation_production_breadth"` passed.
- Full slice validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 88/88 tests. Apple/Metal checks remained diagnostic-only host gates on this Windows host.
- Implemented scope: first-party value-only `review_physics_production_breadth` and `review_navigation_production_breadth` gates with fail-closed diagnostics for missing review, missing official source, missing budgets, missing required features, native handle exposure, source geometry mutation, arbitrary runtime-bake claims, unsupported broad middleware parity, and host-gated optional adapter rows.
- Remaining Phase 4 work: package-visible 2D/3D counters and any actual optional Jolt/Recast/Detour adapter expansion remain unclaimed until later evidence.

## Phase 5 - Networking Production Execution And Security Gate

**Goal:** Turn network/replication foundations into an execution-capable, threat-modeled, host-gated networking lane without claiming broad online services.

**Files:**

- Modify: `engine/runtime/include/mirakana/runtime/network_transport.hpp`
- Modify: `engine/runtime/src/network_transport.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/production_network_replication.hpp`
- Modify: `engine/runtime/src/production_network_replication.cpp`
- Modify optional: `engine/runtime/network/enet/`
- Modify: `tests/unit/runtime_network_transport_adapter_tests.cpp`
- Modify: `tests/unit/runtime_network_enet_tests.cpp`
- Modify: `tests/unit/runtime_production_network_replication_tests.cpp`
- Create or modify: `docs/specs/2026-05-26-networking-production-security-threat-model.md`
- Modify: `tools/validate-network-enet.ps1`
- Modify package samples only after local loopback/security evidence is green.

- [ ] Write the networking threat model before code. Cover attacker capabilities, trust boundaries, packet tampering/replay, authentication gaps, denial of service, NAT/matchmaking exclusions, and save/rollback abuse.
- [ ] Add RED tests for session lifecycle, connection state, channel policy, reliable/unreliable delivery rows, sequence/replay rejection, snapshot/input-command validation, rollback-window diagnostics, and transport adapter failure modes.
- [ ] Add RED tests that reject encryption/authentication/matchmaking/NAT/cloud claims until implemented with official docs and host evidence.
- [ ] Implement first-party execution-safe networking rows and optional ENet loopback host evidence. Keep internet-facing network execution out of default validation unless a host-gated recipe explicitly opts in.
- [ ] Add package-visible counters for loopback/session/replication execution evidence and host gates.
- [ ] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-network-enet.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_network|production_network_replication"
```

**Phase Evidence:** Not started.

## Phase 6 - Audio Production Playback, Streaming, DSP, And Spatialization

**Goal:** Move audio from gameplay mix planning and selected package counters to production playback/streaming/device/spatialization evidence.

**Files:**

- Modify: `engine/audio/include/mirakana/audio/audio_mixer.hpp`
- Modify: `engine/audio/src/audio_mixer.cpp`
- Modify: `engine/audio/sdl3/include/mirakana/audio/sdl3/sdl_audio_device.hpp`
- Modify: `engine/audio/sdl3/src/sdl_audio_device.cpp`
- Create optional adapter files only after dependency/legal review.
- Modify: `tests/unit/sdl3_audio_tests.cpp`
- Modify package audio payloads and validation scripts after focused APIs are green.

- [ ] Re-check SDL3 audio stream/device docs and selected codec/spatialization dependency docs before dependency changes.
- [ ] Add RED tests for decoded audio source rows, streaming chunk plans, sample-rate/channel conversion policy, voice/bus budgets, DSP graph rows, spatial source/listener rows, HRTF host gates, and device callback lifecycle.
- [ ] Add RED tests that reject broad codec support, middleware parity, native device handles, background streaming, and subjective mix-quality claims until implemented.
- [ ] Implement first-party audio execution contracts and optional adapter rows. Keep codec and spatialization dependencies auditable and feature-gated.
- [ ] Add package-visible counters for selected 2D/3D audio playback, streaming, and spatialization evidence.
- [ ] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_audio_tests MK_audio_sdl3_tests sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "audio|sample_2d_desktop_runtime_package_smoke|sample_generated_desktop_runtime_3d_package_smoke"
```

**Phase Evidence:** Not started.

## Phase 7 - Cross-Domain Package, Generated Game, And Editor Evidence

**Goal:** Prove the production-quality expansions through generated package recipes and reviewed editor/operator surfaces without broad editor productization or arbitrary execution.

**Files:**

- Modify: `tools/new-game.ps1`
- Modify: `tools/new-game-templates.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify editor rows only if selected: `editor/core/`, `editor/src/`, editor tests.
- Modify docs/specs and validation recipes for generated-game guidance.

- [ ] Add package/generator descriptors for only the phases that have landed. Avoid forward-declaring ready recipe ids.
- [ ] Add smoke flags and installed validation fields for renderer quality, runtime UI text/font/IME/accessibility, broad import, physics/nav breadth, networking execution, and audio production evidence as each phase lands.
- [ ] Add editor/operator review rows only for reviewed evidence import, diagnostics, or safe execution. Do not add arbitrary shell or raw manifest command execution.
- [ ] Run package validation lanes selected by the implemented phase set:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package
```

**Phase Evidence:** Not started.

## Phase 8 - Agent Surface Drift, Closeout, And Publication

**Goal:** Close the milestone only after code, tests, docs, manifest, static checks, package evidence, and host gates agree on what is ready and what remains future work.

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/007-importerCapabilities.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`
- Modify tools/static checks and skills/rules/subagents only if durable workflow or validation entrypoints changed.

- [ ] Reconcile every phase with current docs and manifest rows. Do not mark planned or host-gated evidence as ready.
- [ ] Keep AGENTS/skills/rules/subagents unchanged unless this milestone changes durable workflow, permissions, validation entrypoints, or reusable agent behavior.
- [ ] Update `tools/check-ai-integration*.ps1`, schemas, and validation recipes only for machine-checkable supported-surface changes.
- [ ] Run focused agent/static validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

- [ ] Run full validation at each coherent runtime/public-contract slice closeout:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Publish each validated phase through GitHub Flow with a reviewable PR. Do not push directly to `main`, force-push, or merge before hosted checks prove the selected surface.
- [ ] At final milestone closeout, update `currentActivePlan` to the master plan or the next selected dated plan, update `recommendedNextPlan`, keep `unsupportedProductionGaps` honest, compose the manifest, and record final validation evidence here.

**Phase Evidence:** Not started.

## Completion Notes

- This plan intentionally allows breaking changes in public `mirakana::` contracts while the project is greenfield. When a phase changes an aggregate or public API, update all tests and callers in the same task rather than adding compatibility aliases.
- Each phase should prefer small RED/GREEN loops and package-visible evidence over broad prose claims.
- Official docs checked during planning were representative. Before implementation, re-check the current official docs for the exact SDK/dependency versions selected in that phase.
