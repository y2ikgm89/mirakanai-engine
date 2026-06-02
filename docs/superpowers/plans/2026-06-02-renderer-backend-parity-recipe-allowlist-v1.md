# 2026-06-02 Renderer Backend Parity Recipe Allowlist v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `renderer-backend-parity-recipe-allowlist-v1`
**Status:** Completed
**Owner:** `MK_renderer` / agent-surface governance
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)

**Goal:** Make Metal backend parity host evidence fail closed unless its `host_validation_recipe_id` names a reviewed repository validation recipe.

**Architecture:** Keep `BackendRendererParityProofRow` as a first-party value contract and add a narrow reviewed recipe allowlist inside `plan_backend_renderer_parity_policy`. Reject arbitrary syntactically valid Metal host recipe ids with a dedicated diagnostic, while preserving the existing distinction between host-gated Metal rows and host-validated ready rows.

**Tech Stack:** C++23, MK_renderer, PowerShell validation scripts, CMake/CTest presets, Apple Xcode/Metal host gates.

---

## Context

`Renderer Metal CI Host Recipe v1` proved that hosted `macOS Metal CMake` now runs the reviewed `renderer-metal-apple-host-evidence` recipe. The backend parity value contract already requires Metal rows to name a `host_validation_recipe_id`, but today any syntactically valid id can satisfy that field. That leaves ambiguity for AI-generated handoff rows and future backend parity promotion.

Official/current-doc checks used for this slice:

- Apple Xcode command-line docs require Xcode to be installed and selected as the active developer directory for Xcode-only tools such as `xcodebuild`.
- Apple Metal developer tooling docs describe Xcode-bundled Metal debugging, validation, profiling, and shader tooling; Apple Metal library docs cover command-line Metal compiler usage through `xcrun`.
- Apple Metal feature/capability docs make Metal support platform and GPU-family dependent, so Windows or Vulkan proof must not promote Metal readiness.
- Context7 `/kitware/cmake` confirms CTest presets and focused test filters are the supported validation surface for repository CI recipes.

## Constraints

- Do not mark `metal-apple` ready.
- Do not claim broad Metal parity, broad backend parity, or general renderer quality.
- Do not expose public native Metal/RHI handles.
- Keep D3D12 and Vulkan backend-local proof behavior unchanged.
- Keep accepted Metal recipe ids aligned with `engine/agent/manifest.json.aiOperableProductionLoop.recipes` and host gates.

## Tasks

- [x] Add a `BackendRendererParityDiagnosticCode::unreviewed_host_validation_recipe` diagnostic.
- [x] Allow only reviewed Metal recipe ids: `shader-toolchain`, `mobile-packaging`, `ios-simulator-smoke`, and `renderer-metal-apple-host-evidence`.
- [x] Update renderer parity tests to prove arbitrary valid ids fail closed and reviewed ids remain accepted.
- [x] Update current docs, plan registry, manifest fragment/composed manifest, and agent static needles for the new fail-closed diagnostic.
- [x] Run focused renderer tests and agent/static checks.
- [x] Run full `tools/validate.ps1` before publishing because this changes a C++ public value contract.

## Done When

- Arbitrary Metal `host_validation_recipe_id` values produce `unreviewed_host_validation_recipe`.
- Host-gated Metal rows using `renderer-metal-apple-host-evidence` remain host-gated until host validation is explicitly present.
- Host-validated Metal rows can become ready only when all existing backend-local proof requirements are also present.
- `unsupportedProductionGaps = []` remains unchanged and broad Metal/backend/general renderer quality remains unclaimed.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed | Configured the linked worktree `dev` preset after `out/build/dev` was absent. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | Passed | Focused renderer test build. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` | Passed | `MK_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Passed after `tools/format.ps1` normalized one clang-format issue. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent config and skill parity gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest compose/static contract. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent-surface drift gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed | Docs/text formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full slice gate; all 85 CTest tests passed, with Apple/Metal checks remaining diagnostic-only host-gated on Windows. |
