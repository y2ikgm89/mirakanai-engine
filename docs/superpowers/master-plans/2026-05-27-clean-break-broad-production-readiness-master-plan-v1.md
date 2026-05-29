# Clean Break Broad Production Readiness Master Plan Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:writing-plans to create a focused dated child implementation plan before editing code. Child plans should then use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement task-by-task. Steps in child plans use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the currently unclaimed broad production areas into clean, official-practice, no-backward-compatibility implementation tracks without weakening the Engine 1.0 zero-gap truth.

**Architecture:** This is a master coordination plan, not one implementation slice. Each broad domain gets its own child plan, public first-party contract, adapter boundary, tests, package evidence, host gates, docs, manifest rows, dependency/legal records, and final validation before any ready claim is promoted.

**Tech Stack:** C++23, CMake/CTest, PowerShell validation tools, `MK_renderer`, `MK_rhi`, `MK_runtime_rhi`, `MK_scene_renderer`, `MK_ui`, `MK_ui_renderer`, `MK_assets`, `MK_tools`, `MK_physics`, `MK_navigation`, `MK_audio`, `MK_runtime`, optional vcpkg manifest features, D3D12, Vulkan, Apple Metal, first-party Windows desktop platform backends, Win32, Raw Input, WASAPI, DXGI, HarfBuzz-class shaping, FreeType-class rasterization, ICU-class Unicode services, Khronos glTF/KTX, DXC/SPIR-V tools, Jolt-class physics adapters, Recast/Detour-class navigation adapters, ENet/GameNetworkingSockets-class transport adapters, OpenAL/miniaudio-class audio adapters, and platform accessibility SDKs.

---

**Plan ID:** `clean-break-broad-production-readiness-master-plan-v1`

**Status:** Active child selection for `physics-navigation-commercial-coverage-v1` after the completed `reviewed-importers-codecs-shader-generation-v1` closeout.

**Date:** 2026-05-27

## Master Plan Decision

This work is too broad for one implementation branch. It must be executed through reviewable child plans and milestones:

- Renderer production quality, backend parity, and profiling.
- Runtime UI text shaping, font rasterization, IME, accessibility, and platform parity.
- Reviewed importer, codec, source import, and shader generation execution.
- First-party Windows desktop platform, runtime host, editor, and audio replacement after SDL3 removal.
- Physics and navigation commercial coverage.
- Audio production coverage.
- Networking production coverage.
- Cross-domain package evidence and closeout.

The reviewed importer/codecs/shader-generation child was selected again through an explicit manifest/registry switch after the SDL3 removal milestone and backlog selection-gate reconciliation, then completed selected importer/codec/shader-generation evidence before the manifest returned to the production-completion selection gate. `physics-navigation-commercial-coverage-v1` is now selected through a new manifest/registry switch.

`unsupportedProductionGaps = []` remains the Engine 1.0 ready-surface truth. This plan does not reopen 1.0; it selects post-1.0 / 1.x production breadth.

## Clean-Break Doctrine

- No backward-compatibility shims, deprecated aliases, duplicate public APIs, or transitional adapters unless a future release policy explicitly requires them.
- When a public aggregate, enum, function, manifest literal, package counter, JSON schema, or validation command changes, update every caller, designated initializer, test, sample, doc, manifest fragment, and static check in the same child plan.
- Gameplay-facing APIs stay first-party and value-based. D3D12, Vulkan, Metal, removed SDL3 APIs, HarfBuzz, FreeType, ICU, Jolt, Recast/Detour, ENet, GameNetworkingSockets, OpenAL, miniaudio, codec libraries, platform accessibility objects, and native OS handles stay behind private adapter boundaries.
- Optional dependencies enter only through vcpkg manifest features plus `tools/bootstrap-deps.ps1`. Each dependency child plan updates `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, validation wrappers, and manifest feature rows.
- Official SDK and dependency documentation must be rechecked at the start of each child plan. Record exact URLs or Context7 library IDs in that child plan evidence.
- Broad readiness is fail-closed by default. Package counters distinguish ready rows, host-gated rows, dependency-gated rows, skipped rows, and unsupported claims.

## Official Source Anchors

Re-check exact docs during child-plan execution:

- D3D12 resource states and barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- D3D12 multi-engine synchronization and fences: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>
- D3D12 residency: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency>
- Vulkan synchronization guide: <https://docs.vulkan.org/guide/latest/synchronization.html>
- Vulkan validation layers: <https://docs.vulkan.org/guide/latest/validation_overview.html>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
- Apple Metal capabilities: <https://developer.apple.com/metal/capabilities/>
- HarfBuzz documentation: <https://harfbuzz.github.io/>
- FreeType glyph conventions: <https://freetype.org/freetype2/docs/glyphs/index.html>
- ICU user guide: <https://unicode-org.github.io/icu/userguide/>
- WAI-ARIA Authoring Practices Guide: <https://www.w3.org/WAI/ARIA/apg/>
- Microsoft UI Automation provider guidance: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-providersoverview>
- Khronos glTF 2.0 specification: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html>
- Khronos KTX: <https://www.khronos.org/ktx/>
- DirectX Shader Compiler: <https://github.com/microsoft/DirectXShaderCompiler>
- SPIRV-Tools: <https://github.com/KhronosGroup/SPIRV-Tools>
- Jolt Physics documentation: <https://jrouwe.github.io/JoltPhysics/>
- Recast Navigation documentation: <https://recastnav.com/>
- ENet: <https://github.com/lsalzman/enet>
- GameNetworkingSockets: <https://github.com/ValveSoftware/GameNetworkingSockets>
- OpenAL documentation: <https://www.openal.org/documentation/>
- miniaudio manual: <https://miniaud.io/docs/manual/index.html>

Context7 evidence recorded during this selection pass:

- `/libsdl-org/sdlwiki`: Historical selection-pass evidence only. The retired text-input/clipboard adapter proof was used before the SDL3 removal closeout and must not be treated as current implementation guidance or a reason to reintroduce SDL3 APIs.
- `/jrouwe/joltphysics`: Physics/navigation commercial coverage selection evidence for optional Jolt-class adapter lifecycle, collision layers, query, character, and vehicle capability gates behind first-party value contracts.
- `/websites/recastnav`: Physics/navigation commercial coverage selection evidence for Recast build inputs, Detour navmesh queries, DetourTileCache dynamic obstacles, and DetourCrowd local avoidance gates behind first-party value contracts.

## Child Plan Sequence

### Child Plan 0 - Governance Selection And Baseline Reconciliation

**Status:** This selection pass.

**Purpose:** Create this master plan, select exactly one first child implementation plan, update registry/docs/manifest pointers, and preserve `unsupportedProductionGaps = []`.

**Done When:** The chosen child plan is the only active production implementation plan, the composed manifest points at it, and docs/agent/static checks pass.

### Child Plan 1 - Renderer Production Quality And Backend Parity v1

**Status:** Completed.

**Path:** `docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md`

**Purpose:** Replace selected renderer confidence with backend-local, measurable production quality, parity, profiling, and package evidence gates.

**Closeout:** Completed through PR #261, PR #262, and PR #263. The final checkpoint merged as `97b4b0d8e680b7da723a294ed77555ba9c7c5a8d` with local package/static/full validation and hosted `PR Gate`, `Windows MSVC`, Linux, CodeQL, static analysis, iOS smoke, and macOS Metal CMake evidence. Broad renderer quality, Metal visible parity, and subjective/general performance parity remain host-gated or unclaimed where evidence is absent.

### Child Plan 2 - Runtime UI Text Platform Stack v1

**Status:** Completed.

**Path:** `docs/superpowers/plans/2026-05-27-runtime-ui-text-platform-stack-v1.md`

**Purpose:** Implement real text shaping, font rasterization, IME, accessibility, and platform UI publication lanes behind first-party contracts and optional dependency adapters.

**Closeout:** Completed through PR #264, PR #265, PR #266, PR #267, and PR #268. The final checkpoint merged as `98dbe209bc00a9914f267f2394900d870c572cbc` with local package/static/full validation plus hosted PR Gate, Windows MSVC, Linux, CodeQL, static analysis, iOS smoke, and macOS Metal CMake evidence. Broad runtime UI platform parity, production HarfBuzz/FreeType/ICU dependency adapters, native IME parity, OS accessibility publication, renderer texture upload execution, and broad runtime UI renderer quality remain host/dependency gated or unclaimed.

### Child Plan 3 - Reviewed Importers Codecs And Shader Generation v1

**Status:** Completed.

**Path:** `docs/superpowers/plans/2026-05-27-reviewed-importers-codecs-shader-generation-v1.md`

**Purpose:** Move from value-only import review to selected importer/codec/shader evidence lanes without arbitrary importer, compiler, or source execution.

**Closeout:** Completed selected KTX2/Basis texture review, glTF scene import review, source image/audio codec review, and reviewed shader generation/cache execution evidence. The closeout adds exact DXC D3D12 DXIL, DXC Vulkan SPIR-V, and `spirv-val --target-env vulkan1.3` review rows plus package-visible `shader_generation_cache_*` counters while keeping live shader generation, runtime compiler execution, native cache handles, renderer/RHI residency, package streaming, Metal library generation, and broad import/codec/shader readiness fail-closed.

### Child Plan 4 - Physics And Navigation Commercial Coverage v1

**Purpose:** Expand first-party and optional adapter coverage for controllers, queries, constraints, vehicles, navmesh, dynamic obstacles, crowds, streaming nav data, and genre coverage rows.

**Status:** Active selected slice: [Physics Navigation Commercial Coverage v1](../plans/2026-05-29-physics-navigation-commercial-coverage-v1.md).

### Child Plan 5 - Audio Production Coverage v1

**Purpose:** Expand audio beyond selected mixer/playback evidence into source decode, streaming, DSP, spatialization, device/platform lanes, codec gates, and HRTF/middleware host gates.

### Child Plan 6 - Networking Production Coverage v1

**Purpose:** Build threat-modeled transport/session/replication coverage with loopback and optional adapter proof while leaving auth, encryption, NAT, matchmaking, cloud lobbies, and internet-scale service claims unready until implemented.

### Child Plan 7 - Cross-Domain Package Evidence And Closeout v1

**Purpose:** Promote only evidenced child-plan results into docs, manifest, package recipes, generated-game guidance, static checks, and final validation.

## Global Done When

- Every child plan has official docs checked, tests added or updated, package counters, docs/manifest sync, dependency/legal updates where needed, and validation output.
- Broad renderer, runtime UI, importer/codec/shader, physics/navigation, audio, and networking claims are either implemented with proof or left as explicit non-ready rows.
- No public API exposes backend, platform, or middleware handles.
- No compatibility layer exists solely to preserve old post-1.0 planning names.
- All affected generated-game, editor/operator, validation, and agent surfaces match the implemented contracts.
- Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes for runtime/public-contract closeout, or exact host/tool blockers are recorded.
