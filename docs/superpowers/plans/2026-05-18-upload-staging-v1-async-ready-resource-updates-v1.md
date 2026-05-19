# Upload Staging v1 Async-Ready Resource Updates Implementation Plan

**Status:** Completed.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the remaining `upload-staging-v1` ready-claim blocker by proving selected package resource updates are ready only after submitted upload fences are available and graphics-queue consumption waits are recorded.

**Architecture:** Add a narrow `MK_runtime_rhi` readiness planner that consumes committed package streaming state, live `RuntimeResourceCatalogV2` rows, and reviewed package upload transaction results. The planner emits first-party metadata rows for texture/static/skinned/morph resource updates and fails closed when a submitted fence is missing, a copy/compute upload lacks a graphics queue wait, the resident catalog row is stale or wrong-kind, or an upload transaction failed. It does not own background streaming, renderer residency, native handles, allocator budgets, or async-overlap claims.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_rhi`, `MK_rhi` NullRHI/D3D12 evidence, PowerShell validation scripts.

---

### Task 1: Resource Update Readiness API

**Files:**
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/package_streaming_frame_graph.hpp`
- Modify: `engine/runtime_rhi/src/package_streaming_frame_graph.cpp`
- Test: `tests/unit/runtime_rhi_tests.cpp`

- [x] **Step 1: Write RED tests**

Add unit coverage that calls `make_runtime_package_resource_update_readiness` after selected package texture/static/skinned/morph upload transactions and verifies four ready update rows, four submitted fences, three graphics waits for copy-queue uploads, and one same-queue graphics update. Add a failure test that clears a copy-queue transaction wait count and requires a `resource-update-upload-not-waited` diagnostic.

- [x] **Step 2: Verify RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests
```

Expected: compile failure because the readiness API is not implemented yet.

- [x] **Step 3: Implement minimal planner**

Add `RuntimePackageResourceUpdateKind`, `RuntimePackageResourceUpdate`, `RuntimePackageResourceUpdateDiagnostic`, `RuntimePackageResourceUpdateReadinessSources`, `RuntimePackageResourceUpdateReadinessResult`, and `make_runtime_package_resource_update_readiness`. Keep source inputs optional so focused tests can validate one transaction class while the selected package evidence validates all four.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_runtime_rhi_tests$"
```

Expected: runtime RHI unit tests pass.

### Task 2: Selected Package Evidence And Installed Validation

**Files:**
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/package_streaming_frame_graph.hpp`
- Modify: `engine/runtime_rhi/src/package_streaming_frame_graph.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`

- [x] **Step 1: Extend evidence counters**

Wire the readiness planner into `execute_runtime_package_upload_staging_evidence` and expose deterministic package smoke counters for `package_upload_staging_resource_updates_ready`, total update rows, submitted fences, graphics-ready rows, same-queue graphics rows, and recorded graphics waits.

- [x] **Step 2: Extend installed validation**

When `--require-package-upload-staging` is present, require the resource update counters in `tools/validate-installed-desktop-runtime.ps1` and the generated 3D package smoke lane.

- [x] **Step 3: Verify package smoke**

Run focused unit tests, the generated package smoke, and `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package`.

### Task 3: Documentation, Manifest, Static Guards, And Gate Validation

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/workflows.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/*.json`
- Generate: `engine/agent/manifest.json`
- Modify: relevant `tools/check-*.ps1` static guards
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`

- [x] **Step 1: Reconcile surfaces**

Record the completed async-ready resource update surface and remove `async-ready resource updates` from `upload-staging-v1.requiredBeforeReadyClaim` if the evidence closes the gap. Keep broad/background streaming, allocator budgets, renderer-owned residency, public native handles, Vulkan/Metal parity, and async-overlap claims unsupported.

- [x] **Step 2: Run agent-surface checks**

Run `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration.ps1`.

- [x] **Step 3: Run final validation and checkpoint**

Run focused tidy/format/static checks plus one full:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Commit, push the existing PR branch, and keep PR #120 updated.

**Validation evidence:**
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` failed before implementation because `make_runtime_package_resource_update_readiness`, `RuntimePackageResourceUpdateReadinessSources`, and `RuntimePackageResourceUpdateKind` were missing.
- Focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests sample_generated_desktop_runtime_3d_package` passed after formatting.
- Focused tests: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_runtime_rhi_tests$|^sample_generated_desktop_runtime_3d_package_smoke$"` passed.
- Focused static checks: `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files engine/runtime_rhi/src/package_streaming_frame_graph.cpp,tests/unit/runtime_rhi_tests.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1` passed.
- Installed package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed with `package_upload_staging_resource_updates_ready=1`, `package_upload_staging_resource_updates=4`, `package_upload_staging_resource_update_submitted_fences=4`, `package_upload_staging_resource_update_graphics_ready_updates=4`, `package_upload_staging_resource_update_graphics_queue_waits_recorded=3`, and `package_upload_staging_resource_update_same_queue_graphics_updates=1`.
- Full gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. `production-readiness-audit-check` reported `unsupported_gaps=6` with `upload-staging-v1` removed and `scene-component-prefab-schema-v2` as the remaining foundation follow-up.
