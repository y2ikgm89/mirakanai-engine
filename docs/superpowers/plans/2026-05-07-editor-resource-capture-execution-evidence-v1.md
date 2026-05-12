# Editor Resource Capture Execution Evidence v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add reviewed Resources panel evidence rows for host-owned resource capture execution without letting editor core launch tools, toggle graphics APIs, or expose native handles.

**Architecture:** Extend the GUI-independent `MK_editor_core` resource panel model with caller-supplied capture execution snapshots. The model only normalizes deterministic evidence rows and retained `MK_ui` ids; the optional `MK_editor` shell may surface acknowledged request waiting states, but actual PIX/debug-layer/ETW capture execution remains an external host workflow.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, retained `MK_ui`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow resource-management/capture productization gap:

- Keep `Editor Resource Capture Request v1` handoff rows intact.
- Add host-owned capture execution evidence rows under retained `resources.capture_execution` ids.
- Allow evidence to represent requested, host-gated, running, captured, failed, or blocked states.
- Reject unsupported claims that editor core executed capture tooling or exposed native handles.
- Keep actual PIX launch, D3D12 debug-layer toggles, ETW/performance capture, process execution, residency/allocator policy, package streaming, GPU capture execution, and native handles out of editor core.

## Context

- `EditorResourcePanelInput` already accepts plain diagnostics and capture request inputs.
- The visible `MK_editor` Resources panel tracks transient acknowledgement ids for capture request handoff only.
- The master plan still lists resource management/capture execution beyond reviewed handoff rows as follow-up editor productization work.
- This slice records evidence about a host-owned workflow; it does not perform that workflow.

## Constraints

- Do not add dependencies on PIX, D3D12 SDK Layers, ETW, SDL3, Dear ImGui, concrete RHI backends, or native handles to `editor/core`.
- Do not run process commands from `editor/core`.
- Do not claim capture execution is ready merely because a request is acknowledged.
- Keep evidence rows deterministic, sanitized, sorted, and safe for retained UI.
- Update Codex and Claude editor guidance together.

## Done When

- RED `MK_editor_core_tests` proves the new capture execution evidence surface is missing.
- `EditorResourcePanelModel` exposes `capture_execution_rows`.
- `make_resource_panel_ui_model` emits retained `resources.capture_execution.<id>` rows with status, artifact, and diagnostic labels.
- The visible `MK_editor` Resources panel shows capture execution evidence/waiting rows derived from acknowledged capture requests without executing capture tools.
- Docs, master plan, registry, manifest, skills, and static checks record the boundary truthfully.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Test

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor resource capture execution evidence reports host owned snapshots")`.
- [x] Build an `EditorResourcePanelInput` with `device_available=true`, `backend_id=d3d12`, and three `capture_execution_snapshots`:
  - `pix_gpu_capture`: captured externally with artifact `captures/pix/frame_8.wpix`,
  - `d3d12_debug_validation`: host-gated and waiting on Windows Graphics Tools,
  - `unsafe_native_capture`: claims editor-core execution and native handle exposure.
- [x] Assert `make_editor_resource_panel_model` exposes sorted `capture_execution_rows` with:
  - `d3d12_debug_validation.status_label == "Host-gated"` and `artifact_path == "-"`,
  - `pix_gpu_capture.status_label == "Captured"` and `artifact_path == "captures/pix/frame_8.wpix"`,
  - `unsafe_native_capture.status_label == "Blocked"` and a diagnostic mentioning unsupported editor-core execution or native handle exposure.
- [x] Assert retained UI ids exist:
  - `resources.capture_execution.pix_gpu_capture.status`,
  - `resources.capture_execution.pix_gpu_capture.artifact`,
  - `resources.capture_execution.unsafe_native_capture.diagnostic`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm the build fails before implementation because `capture_execution_snapshots` / `capture_execution_rows` are not declared.

### Task 2: Editor-Core Evidence Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/resource_panel.hpp`
- Modify: `editor/core/src/resource_panel.cpp`

- [x] Add `EditorResourceCaptureExecutionInput` with ids, labels, state booleans, artifact path, diagnostic, and unsupported-claim booleans.
- [x] Add `EditorResourceCaptureExecutionRow` to the public resource panel model.
- [x] Add `capture_execution_snapshots` to `EditorResourcePanelInput` and `capture_execution_rows` to `EditorResourcePanelModel`.
- [x] In `make_editor_resource_panel_model`, sanitize and sort snapshots by id/tool/label.
- [x] Derive status labels:
  - unsupported editor-core execution or native handles: `Blocked`,
  - `capture_failed`: `Failed`,
  - `capture_completed`: `Captured`,
  - `capture_started`: `Running`,
  - `host_gated`: `Host-gated`,
  - `requested`: `Requested`,
  - otherwise `Not requested`.
- [x] Emit safe diagnostics for blocked rows and keep artifact path as `-` when absent.
- [x] In `make_resource_panel_ui_model`, add retained `resources.capture_execution` rows with `label`, `tool`, `status`, `artifact`, and `diagnostic`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible Resources Panel Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add `append_resource_capture_execution_snapshots(EditorResourcePanelInput&)`.
- [x] For acknowledged `pix_gpu_capture` and `d3d12_debug_layer_gpu_validation` requests, add host-gated waiting evidence rows with `requested=true`, `host_gated=true`, no artifact path, and diagnostics that say the editor is waiting for external host capture evidence.
- [x] Add `draw_resource_capture_execution_rows_table` and render it below Capture Requests.
- [x] Keep the table read-only: no process execution, no PIX launch, no debug-layer API toggles, and no native handle display.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Guidance, Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Record `Editor Resource Capture Execution Evidence v1`, `EditorResourceCaptureExecutionInput`, `EditorResourceCaptureExecutionRow`, and retained `resources.capture_execution` ids.
- [x] Keep PIX launch, D3D12 debug-layer toggles, ETW/performance capture, process execution, residency/allocator policy, package streaming, GPU capture execution, native handles, and broad renderer/resource management readiness unsupported.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing `EditorResourceCaptureExecutionRow`, `EditorResourceCaptureExecutionInput`, `EditorResourcePanelInput::capture_execution_snapshots`, and `EditorResourcePanelModel::capture_execution_rows`. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after adding the editor-core evidence model and retained `resources.capture_execution` rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Desktop GUI lane built `MK_editor` with Capture Execution Evidence rows and passed 46/46 CTest entries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after adding resource capture execution evidence API, UI, docs, manifest, and skill checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Unsupported gap audit accepted the updated editor-productization and renderer/resource notes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok` after applying repository clang-format. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted the editor-core resource capture execution evidence additions. |
| `git diff --check` | PASS | No whitespace errors reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; dev CTest passed 29/29. Diagnostic-only host gates remain for Metal/Apple as expected on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Dev build completed through `tools/build.ps1`. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Resource Capture Execution Evidence v1 files; leave unrelated pre-existing guidance changes unstaged. |
