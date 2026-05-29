# Renderer Postprocess Tone Mapping Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a clean-break, backend-neutral tone mapping evidence gate for `renderer-postprocess-v1` without GPU execution, SDL3, native handles, or broad postprocess quality claims.

**Architecture:** Extend `MK_renderer` `postprocess_policy` with a value-only `PostprocessToneMappingEvidence*` planner that validates per-backend HDR input, transfer-function, luminance, color-space, synchronization, shader-validation, backend-validation, and host-validation evidence rows. D3D12 and strict Vulkan rows can become ready only with explicit local evidence; Metal rows remain host-gated unless Apple-host evidence is present.

**Tech Stack:** C++23, `MK_renderer`, first-party RHI backend taxonomy, repository PowerShell validation, Context7 Vulkan docs, Microsoft DirectX Advanced Color docs, Apple Metal HDR/resource synchronization docs.

---

**Plan ID:** `renderer-postprocess-tone-mapping-evidence-v1`

**Status:** Active.

**Date:** 2026-05-29

## Context

The production-completion manifest has `unsupportedProductionGaps = []`, so this is not a reopened Engine 1.0 blocker. The backlog records `renderer-postprocess-v1` as `implemented-1x-foundation` while explicitly leaving tone mapping, exposure, bloom, color grading, fog, anti-aliasing selection, and subjective visual quality as future evidence-gated work. This plan selects the smallest follow-up: deterministic tone mapping evidence rows only.

Official practice checks:

- Microsoft DirectX Advanced Color recommends explicit swapchain pixel-format and color-space handling for HDR/WCG output, including FP16 scRGB and explicit HDR10/BT.2100 color-space setup.
- `IDXGISwapChain3::SetColorSpace1` is the Windows API used to set swapchain color space; the engine must keep that native detail backend-private.
- Khronos Vulkan docs require explicit synchronization and validation-layer/shader evidence; postprocess read-after-write paths must account for attachment writes transitioning to shader reads.
- Vulkan SPIR-V is the shader input to `vkCreateShaderModule`; strict Vulkan evidence must not rely on D3D12 DXIL proof.
- Apple Metal HDR content and resource synchronization are Apple-host concerns; Metal readiness must stay host-gated without macOS/Xcode evidence.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/postprocess_policy.hpp`
- Modify: `engine/renderer/src/postprocess_policy.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

## Tasks

### Task 1: Implement Tone Mapping Evidence Contract

- [x] Add `PostprocessToneMappingEvidenceStatus`, `PostprocessToneMappingOperator`, `PostprocessColorTransferFunction`, `PostprocessToneMappingEvidenceDiagnosticCode`, row/request/diagnostic/plan structs, `plan_postprocess_tone_mapping_evidence`, and `has_postprocess_tone_mapping_evidence_diagnostic`.
- [x] Add unit tests proving ready, Metal host-gated, strict evidence rejection, unsafe-claim rejection, replay hash, and no-row behavior.
- [x] Run focused `MK_renderer_tests` build/CTest and fix compile or behavior failures.

### Task 2: Sync Agent and Operator Surfaces

- [x] Update manifest fragments and compose `engine/agent/manifest.json`.
- [x] Update current capabilities, roadmap, backlog/projections, and plan registry so the new contract is discoverable without claiming broad postprocess readiness.
- [x] Update rendering skills for Codex/Claude parity.
- [x] Extend static AI integration checks for the new API, tests, docs, and skill needles.

### Task 3: Validate and Publish

- [x] Run focused checks: `tools/check-ai-integration.ps1`, `tools/check-text-format.ps1`, and `git diff --check`.
- [x] Run C++ slice gate: `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, target build, target CTest, then `tools/validate.ps1` if code/build evidence changed.
- [ ] Commit the validated candidate, push a topic branch, create a PR with evidence, wait for required hosted checks, merge through GitHub Flow, and clean the worktree.

## Done When

- The tone mapping evidence planner accepts per-backend ready rows, keeps Metal host-gated without Apple evidence, and rejects missing color-space, synchronization, shader validation, backend validation, host validation, native-handle, and subjective-quality claims.
- No SDL3, native handles, GPU command execution, native capture, crash upload, or broad visual-quality claims are introduced.
- Manifest/docs/skills/static checks agree with the new API and preserve `unsupportedProductionGaps = []`.
- Local and hosted validation evidence is recorded before publication/merge.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Pass | Reconfigured the linked worktree after restoring the shared local `external/vcpkg/.vcpkg-root` marker. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | Pass | Focused renderer test target built successfully. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` | Pass | `MK_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | Manifest compose, recommended plan pointers, and static line budgets passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Agent/rendering needles and dry-run generated game contracts passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Pass | Tracked text formatting passed. |
| `git diff --check` | Pass | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Pass | Linked worktree, vcpkg junction, CMake, CTest, CPack, MSBuild, and clang-format ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passed, including 19 static checks, full build, tidy smoke, and 79/79 CTest tests. Apple/Metal checks remained host-gated diagnostic-only on Windows. |
