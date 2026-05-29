# Clean Break Broad Production Readiness Master Plan Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:writing-plans to create a focused dated child implementation plan before editing code. Child plans should then use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement task-by-task. Steps in child plans use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the currently unclaimed broad production areas into clean, official-practice, no-backward-compatibility implementation tracks without weakening the Engine 1.0 zero-gap truth.

**Architecture:** This is a master coordination plan, not one implementation slice. Each broad domain gets its own child plan, public first-party contract, adapter boundary, tests, package evidence, host gates, docs, manifest rows, dependency/legal records, and final validation before any ready claim is promoted.

**Tech Stack:** C++23, CMake/CTest, PowerShell validation tools, `MK_renderer`, `MK_rhi`, `MK_runtime_rhi`, `MK_scene_renderer`, `MK_ui`, `MK_ui_renderer`, first-party native desktop platform adapters, `MK_assets`, `MK_tools`, `MK_physics`, `MK_navigation`, `MK_audio`, `MK_runtime`, optional vcpkg manifest features, D3D12, Vulkan, Apple Metal, Win32/DXGI/WASAPI first-party Windows lanes, HarfBuzz-class shaping, FreeType-class rasterization, ICU-class Unicode services, Khronos glTF/KTX, DXC/SPIR-V tools, Jolt-class physics adapters, Recast/Detour-class navigation adapters, ENet/GameNetworkingSockets-class transport adapters, OpenAL/miniaudio-class audio adapters, and platform accessibility SDKs. Legacy desktop middleware dependencies are removal targets, not future implementation dependencies.

---

**Plan ID:** `clean-break-broad-production-readiness-master-plan-v1`

**Status:** Selected master plan. Not selected as `currentActivePlan`; the first selected child is `renderer-production-quality-backend-parity-v1`.

**Date:** 2026-05-27

## Master Plan Decision

This work is too broad for one implementation branch. It must be executed through reviewable child plans:

- Renderer production quality, backend parity, and profiling.
- Runtime UI text shaping, font rasterization, IME, accessibility, and platform parity.
- Reviewed importer, codec, source import, and shader generation execution.
- Physics and navigation commercial coverage.
- Audio production coverage.
- Networking production coverage.
- Cross-domain package evidence and closeout.

`unsupportedProductionGaps = []` remains the Engine 1.0 ready-surface truth. This plan does not reopen 1.0; it selects post-1.0 / 1.x production breadth.

## Clean-Break Doctrine

- No backward-compatibility shims, deprecated aliases, duplicate public APIs, or transitional adapters unless a future release policy explicitly requires them.
- When a public aggregate, enum, function, manifest literal, package counter, JSON schema, or validation command changes, update every caller, designated initializer, test, sample, doc, manifest fragment, and static check in the same child plan.
- Gameplay-facing APIs stay first-party and value-based. D3D12, Vulkan, Metal, Win32/DXGI/WASAPI objects, HarfBuzz, FreeType, ICU, Jolt, Recast/Detour, ENet, GameNetworkingSockets, OpenAL, miniaudio, codec libraries, platform accessibility objects, and native OS handles stay behind private adapter boundaries.
- Do not add new SDL3 code, dependencies, package recipes, or validation lanes. Existing SDL3 surfaces are legacy replacement/removal work and must be deleted after first-party native replacements are proven.
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
- Win32 windowing and messages: <https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window> and <https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues>
- WASAPI stream management: <https://learn.microsoft.com/en-us/windows/win32/coreaudio/stream-management>
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

- 2026-05-29 update: SDL3 is explicitly removed from future planning. Future desktop work must use first-party native platform lanes, with Windows starting from official Win32 message/window, DXGI presentation, and WASAPI documentation. Retain old SDL3 references only as deletion inventory until their replacement/removal candidate closes.

## Child Plan Sequence

### Child Plan 0 - Governance Selection And Baseline Reconciliation

**Status:** This selection pass.

**Purpose:** Create this master plan, select exactly one first child implementation plan, update registry/docs/manifest pointers, and preserve `unsupportedProductionGaps = []`.

**Done When:** The chosen child plan is the only active production implementation plan, the composed manifest points at it, and docs/agent/static checks pass.

### Child Plan 1 - Renderer Production Quality And Backend Parity v1

**Status:** Selected active child.

**Path:** `docs/superpowers/plans/2026-05-27-renderer-production-quality-backend-parity-v1.md`

**Purpose:** Replace selected renderer confidence with backend-local, measurable production quality, parity, profiling, and package evidence gates.

### Child Plan 2 - First-Party Desktop Platform And Runtime UI Text Stack v1

**Purpose:** Replace remaining SDL3 desktop/runtime-host assumptions with first-party native desktop platform lanes, then implement real text shaping, font rasterization, IME, accessibility, and platform UI publication behind first-party contracts and optional dependency adapters.

### Child Plan 3 - Reviewed Importers Codecs And Shader Generation v1

**Purpose:** Move from value-only import review to selected real importer/codec/shader execution lanes without arbitrary importer, compiler, or source execution.

### Child Plan 4 - Physics And Navigation Commercial Coverage v1

**Purpose:** Expand first-party and optional adapter coverage for controllers, queries, constraints, vehicles, navmesh, dynamic obstacles, crowds, streaming nav data, and genre coverage rows.

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
