# Shadow Map Foundation v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first backend-neutral shadow-map foundation now that D3D12 and Vulkan both support private native depth attachments, without claiming production cascaded shadows or exposing native handles.

**Architecture:** Keep this slice renderer/RHI-first and dependency-free. The public surface may add first-party `mirakana::` shadow planning/value contracts only where needed, but must not expose D3D12/Vulkan/Metal handles, SDL3, Dear ImGui, editor APIs, or host-owned backend resources to gameplay. The first implementation should create deterministic shadow pass/light/receiver planning and validation that native backends can consume later, while preserving Lit Material v0 and Frame Graph/Postprocess v0 behavior.

**Tech Stack:** C++23, existing `mirakana_renderer`, `mirakana_rhi`, `mirakana_scene_renderer`, `mirakana_runtime_scene_rhi`, existing depth texture contracts, no new third-party dependencies.

---

## Context

- Lit Material v0 provides one deterministic directional Lambert light but no shadowing.
- RHI Depth Attachment Contract v0 plus D3D12/Vulkan native depth proofs provide the backend foundation for depth-only shadow passes.
- Frame Graph/Postprocess v0 is color-only and not a production render graph; this slice should avoid overclaiming full render-graph scheduling.
- Metal native depth recording remains Apple-host-gated and must not be marked implemented by this slice.

## Constraints

- No new dependencies.
- Do not expose native handles, `IRhiDevice`, swapchain frames, or backend resources to game/runtime public APIs.
- Do not implement cascades, PCF, VSM/EVSM, moment filtering, contact shadows, screen-space shadows, shader graph integration, or editor shadow authoring in this slice.
- Preserve existing headless samples, Lit Material v0 package smokes, and color-only postprocess path.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- Renderer/RHI/shader changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.

## Done When

- [x] A first-party shadow-map foundation contract exists for directional-light shadow planning, depth texture requirements, receiver/caster counts, and deterministic diagnostics.
- [x] Tests cover valid and invalid shadow-map plans: missing light, invalid shadow map extent, unsupported format, missing caster/receiver data, and stable pass ordering.
- [x] Renderer integration can declare or prepare a shadow pass without exposing backend-native resources or changing gameplay APIs.
- [x] Docs, roadmap, gap analysis, manifest, rendering skills, and Codex/Claude guidance describe this as a foundation only, not production shadows.
- [x] Focused renderer/RHI validation, API boundary, shader toolchain diagnostics, schema/agent/format checks, desktop runtime smoke as needed, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Shadow Foundation Tests

**Files:**
- Modify: `tests/unit/renderer_tests.cpp` or adjacent renderer test file
- Modify: RHI tests only if the final design needs RHI helper validation

- [x] Add tests for deterministic directional shadow-map plan creation.
- [x] Add tests for invalid extent/format/light/caster/receiver diagnostics.
- [x] Add tests that shadow pass declaration remains backend-neutral and does not require native handles.

### Task 2: Renderer Shadow Contracts

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/renderer.hpp` or a new renderer public header if narrower
- Modify: `engine/renderer/src/*`

- [x] Add first-party value types for shadow map plan input/output and diagnostics.
- [x] Validate shadow-map depth texture requirements using existing RHI-first terms where appropriate.
- [x] Keep implementation deterministic and independent of SDL3, Dear ImGui, editor, and graphics API headers.

### Task 3: Integration Boundary

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/frame_graph.hpp` or renderer internals only if needed
- Modify: `engine/scene_renderer` only if scene light/caster data must be adapted

- [x] Add a minimal shadow pass declaration or preparation hook that future native renderers can consume.
- [x] Preserve Frame Graph/Postprocess v0 counters and color-only behavior.
- [x] Keep `mirakana_scene` renderer-neutral.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md` or renderer docs as appropriate
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Mark shadow-map foundation honestly as implemented foundation only after tests pass.
- [x] Keep production shadows, cascades, filtering, editor authoring, and Metal native depth as follow-up work.

### Task 5: Verification

- [x] Run focused renderer/RHI tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Implementation Evidence

- Added `mirakana::ShadowMapDesc`, `ShadowMapLightDesc`, `ShadowMapPlan`, `ShadowMapDiagnostic`, `build_shadow_map_plan`, and `has_shadow_map_diagnostic` in `mirakana_renderer`.
- Added `mirakana::build_scene_shadow_map_plan` in `mirakana_scene_renderer` to choose the first directional shadow-casting light and count visible scene meshes as v0 casters/receivers without adding a renderer dependency to `mirakana_scene`.
- Added renderer tests for valid directional planning, invalid light/extent/format/caster/receiver diagnostics, depth texture requirements, and stable shadow-depth-before-receiver ordering.
- Added scene renderer tests for scene-packet adaptation, first directional shadow-light preference, and unsupported point-light diagnostics.
- Synchronized roadmap, RHI/architecture docs, gap analysis, manifest public headers/module purposes/current capability fields, Codex and Claude rendering skills, and rendering-auditor guidance.
- Validation passed: focused `mirakana_renderer_tests` / `mirakana_scene_renderer_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` with existing diagnostic-only Metal `metal` / `metallib` blockers, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` after rerunning outside sandbox for the known vcpkg 7zip `CreateFileW stdin failed with 5` issue, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
