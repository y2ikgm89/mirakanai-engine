# Editor Resource Capture Request v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the visible Resources panel with deterministic, reviewed resource/GPU capture request rows without executing capture tooling or exposing renderer/RHI/native handles from `editor/core`.

**Architecture:** Keep `editor/core` as a GUI-independent request/handoff model over plain rows. The optional `mirakana_editor` Dear ImGui shell may display request buttons and transient acknowledgement state, but all actual PIX, debug-layer, GPU validation, ETW, or backend capture execution remains an external host/operator workflow.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_ui`, `mirakana_editor` Dear ImGui adapter, existing Resources panel diagnostics, `mirakana_editor_core_tests`, AI/static validation checks.

---

## Goal

Implement `editor-resource-capture-request-v1` as a narrow follow-up to Resources diagnostics:

- add reviewed capture request input rows for host-owned diagnostics workflows such as PIX GPU capture handoff and D3D12 debug-layer/GPU-validation capture prep;
- expose deterministic request rows with host gates, availability, acknowledgement requirement, reviewed action labels, and blocking diagnostics;
- keep editor-core output as retained `mirakana_ui` labels under `resources.capture_requests`;
- let visible `mirakana_editor` display request rows and record transient reviewed request acknowledgement only after a user action;
- keep all capture execution, process launch/attach, native handles, RHI handles, and platform capture APIs out of `editor/core`.

## Context

- Editor Resource Panel Diagnostics v1 already shows active viewport RHI stats, memory diagnostics, and lifetime summaries.
- The master plan still lists resource management/capture panels beyond read-only diagnostics as a production gap.
- Microsoft PIX guidance treats GPU captures as host/tool-owned workflows where the operator launches or attaches PIX, or uses a dedicated programmatic capture path. It also recommends validating invalid D3D12 usage with the D3D12 debug layer and GPU-based validation when captures fail. This slice turns that into reviewable editor data only; it does not integrate WinPixEventRuntime, D3D12 debug-layer APIs, ETW, PIX launch, or native handles.

## Constraints

- Keep `editor/core` independent from RHI, renderer, SDL3, Dear ImGui, OS APIs, PIX APIs, debug-layer APIs, ETW, and native handles.
- Do not execute capture tools, start processes, attach debuggers/profilers, mutate project files, or persist acknowledgement state.
- Do not expose RHI/backend/native handles or command strings as an editor-core execution contract.
- Host-gated request rows must require explicit acknowledgement before visible editor state reports a request as acknowledged.
- No residency enforcement, allocator policy, eviction, package streaming, backend destruction migration, or broad renderer quality claims.

## Done When

- Unit tests prove resource capture request rows are deterministic, sanitize labels/diagnostics, require acknowledgement when host-gated, and expose retained `resources.capture_requests` UI labels.
- Unit tests prove no-device/resource-unavailable rows are blocked and cannot be acknowledged.
- `mirakana_editor` renders a Capture Requests section with an explicit `Request` button only for available rows that have not yet been acknowledged in the current session.
- Docs, registry, master plan, manifest, skills, and `tools/check-ai-integration.ps1` describe Editor Resource Capture Request v1 without claiming capture execution, native handles, PIX integration, ETW integration, residency management, allocator enforcement, package streaming, or general renderer quality.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify: `editor/core/include/mirakana/editor/resource_panel.hpp`
- Modify: `editor/core/src/resource_panel.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

---

### Task 1: RED Tests For Capture Request Rows

- [x] Add `editor resource capture request model exposes reviewed host gated requests`.
- [x] Build a ready D3D12 resource panel input with PIX and debug-layer capture request rows.
- [x] Assert request rows are sorted deterministically, sanitize text, expose host gates, require acknowledgement, and show unavailable diagnostics only when blocked.
- [x] Assert retained `make_resource_panel_ui_model` output contains `resources.capture_requests.<id>.action`, `.host_gates`, `.acknowledgement`, and `.diagnostic`.
- [x] Add `editor resource capture request model blocks unavailable device requests`.
- [x] Assert no-device requests are blocked, cannot be acknowledged, and expose a diagnostic that capture execution stays host-gated/external.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm failure because capture request fields do not exist yet.

### Task 2: Editor-Core Capture Request Model

- [x] Add `EditorResourceCaptureRequestInput` and `EditorResourceCaptureRequestRow` plain structs.
- [x] Extend `EditorResourcePanelInput` with capture request inputs and `EditorResourcePanelModel` with capture request rows.
- [x] Implement deterministic row ordering, text sanitization, host-gate display labels, acknowledgement labels, and unavailable diagnostics.
- [x] Extend `make_resource_panel_ui_model` with retained `resources.capture_requests` rows.
- [x] Keep the API free of RHI, renderer, OS, PIX, debug-layer, ETW, and native-handle includes.

### Task 3: Visible Editor Wiring

- [x] Add transient `mirakana_editor` session state for acknowledged capture request ids.
- [x] Populate resource capture request inputs from the active viewport resource context using plain backend id/device availability.
- [x] Render a Capture Requests table in the Resources panel.
- [x] Show a `Request` button only when `request_available` is true and the row is not already acknowledged.
- [x] Do not execute any capture/process/API path from the button; record an editor log row and transient acknowledgement only.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Static Checks

- [x] Document Editor Resource Capture Request v1 as reviewed host/operator handoff data, not capture execution.
- [x] Keep non-ready claims explicit: no PIX integration, debug-layer API toggles, ETW capture, native handles, residency/allocator enforcement, package streaming, backend destruction migration, or broad renderer quality.
- [x] Add static checks for new APIs, tests, retained ids, visible editor wiring, docs, manifest, and Codex/Claude skill guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing `EditorResourceCaptureRequestRow`, `EditorResourceCaptureRequestInput`, `EditorResourcePanelInput::capture_requests`, and `EditorResourcePanelModel::capture_request_rows`. |
| Focused `mirakana_editor_core_tests` | PASS | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed after implementing the editor-core request model. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found clang-format issues in `resource_panel.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` fixed them and rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted the editor-core resource capture request additions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after adding resource capture request API, UI, docs, manifest, and skill checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gap count remains 11 with editor-productization still partly-ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Desktop GUI lane built `mirakana_editor` with the Resources Capture Requests table and passed 46/46 CTest entries. |
| `git diff --check` | PASS | No whitespace errors; Git reported line-ending conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; Metal/Apple lanes remained diagnostic/host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev build completed after validation. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Resource Capture Request v1 files; leave unrelated pre-existing guidance changes unstaged. |
