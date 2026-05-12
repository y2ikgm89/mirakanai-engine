# Editor Resource Panel Diagnostics v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a visible editor Resources panel backed by a GUI-independent `mirakana_editor_core` diagnostics model for RHI/resource counters, memory diagnostics, and lifetime-ledger summaries.

**Architecture:** Keep `editor/core` independent from RHI, renderer, SDL3, Dear ImGui, OS APIs, and native handles by defining plain editor resource snapshot/input rows. The `mirakana_editor` shell adapts the active viewport `IRhiDevice` stats, memory diagnostics, and optional lifetime registry into those rows, then renders them through Dear ImGui only in the optional GUI target.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_editor`, Dear ImGui adapter, `IRhiDevice` diagnostics as GUI-side input only, `mirakana_editor_core_tests`, `desktop-gui` validation lane.

---

## Context

- `editor-productization` still lists resource/profiler panels and shared AI command diagnostics as missing before broad editor readiness can be claimed.
- Profiler panel models already exist in `mirakana_editor_core`, and `mirakana_editor` already displays a Profiler panel using `DiagnosticsRecorder`.
- RHI devices already expose backend-neutral `stats()`, `memory_diagnostics()`, and an optional `resource_lifetime_registry()`; these are diagnostics only and must not become gameplay/editor-core native handle contracts.
- `mirakana_editor` owns the active viewport RHI device and can safely adapt those diagnostics inside the optional GUI target.

## Constraints

- Add tests before production code.
- Keep `mirakana_editor_core` GUI-free and RHI-free.
- Do not expose `IRhiDevice`, RHI handles, SDL3, Dear ImGui, D3D12, Vulkan, Metal, or OS handles through editor-core contracts.
- The panel is read-only diagnostics. It must not implement residency enforcement, eviction, allocator policies, capture tooling, package streaming, upload execution, or GPU debugging controls.
- Preserve existing workspace migrations: old workspace documents should pick up the new optional panel hidden by default.
- Keep Dear ImGui as an optional developer shell only.

## Done When

- `mirakana_editor_core` exposes deterministic resource panel input/model types and a `make_editor_resource_panel_model` function.
- The model reports device availability, backend label/status, selected RHI counter rows, memory diagnostic rows with unavailable values shown clearly, and lifetime summary rows from caller-supplied plain counts.
- Workspace defaults, serialization, migration, and command toggles include an optional hidden `resources` panel.
- `mirakana_editor` adapts the active viewport RHI device into the resource panel input, displays a Resources panel, and keeps the panel hidden by default.
- Docs, registry, master plan, manifest, and static checks describe Editor Resource Panel Diagnostics v1 without claiming residency management, allocator enforcement, package streaming, GPU captures, native handles, or general renderer quality.
- Relevant validation passes, including RED evidence, focused `mirakana_editor_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- The slice closes with a validated commit checkpoint after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passes, staging only files owned by this slice.

## Commit Policy

- Use one slice-closing commit after code, docs, manifest, static checks, GUI build, and validation evidence are complete.
- Do not commit RED-test or otherwise known-broken intermediate states.
- Keep unrelated pre-existing guidance changes out of the commit.

## Tasks

### Task 1: RED Tests For Resource Panel And Workspace

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add workspace tests proving the optional `resources` panel exists, is hidden by default, serializes as `panel.resources=...`, migrates into old workspace documents, and toggles independently.
- [x] Add model tests for a ready resource diagnostics snapshot, proving deterministic status, backend rows, RHI counter rows, memory rows, and lifetime rows.
- [x] Add model tests for the no-device state, proving unavailable diagnostics are explicit and no native/RHI types are required by the API.
- [x] Run focused build/test and confirm failure because `mirakana/editor/resource_panel.hpp`, `PanelId::resources`, and `make_editor_resource_panel_model` do not exist yet.

### Task 2: Editor-Core Resource Panel Model

**Files:**
- Create: `editor/core/include/mirakana/editor/resource_panel.hpp`
- Create: `editor/core/src/resource_panel.cpp`
- Modify: `editor/CMakeLists.txt`

- [x] Add plain input structs for backend label, selected RHI counters, memory diagnostic values, and lifetime summary counts.
- [x] Add row/model structs for status rows, counter rows, memory rows, and lifetime rows.
- [x] Implement stable formatting for integers, byte counts, percentages, and unavailable values.
- [x] Implement `make_editor_resource_panel_model` with deterministic row ordering and no RHI/renderer/GUI includes.
- [x] Add `make_resource_panel_ui_model` using first-party `mirakana_ui` contracts, matching the profiler-panel retained model pattern.

### Task 3: Workspace And Visible Editor Wiring

**Files:**
- Modify: `editor/core/include/mirakana/editor/workspace.hpp`
- Modify: `editor/core/src/workspace.cpp`
- Modify: `editor/src/main.cpp`

- [x] Add `PanelId::resources`, token `resources`, and hidden-by-default workspace state.
- [x] Register `view.resources`, add it to the View menu, and draw the Resources panel when visible.
- [x] Convert `viewport_device_->stats()`, `viewport_device_->memory_diagnostics()`, and optional `resource_lifetime_registry()->records()` into the resource panel input in `mirakana_editor`.
- [x] Display status, counters, memory rows, and lifetime summary rows with Dear ImGui tables.
- [x] Keep all native/RHI access inside `mirakana_editor`; editor-core receives only plain values.

### Task 4: Docs, Manifest, Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] Describe Editor Resource Panel Diagnostics v1 as read-only editor diagnostics over caller-supplied plain rows.
- [x] Keep non-ready claims explicit: no residency enforcement, allocator policy, package streaming, GPU capture, native handles, backend destruction migration, or general renderer quality.
- [x] Add static checks for `EditorResourcePanelModel`, workspace `resources` panel, visible `mirakana_editor` wiring, docs, manifest, and AI guidance markers.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Failed as expected | `cmake --build --preset dev --target mirakana_editor_core_tests` failed on missing `mirakana/editor/resource_panel.hpp` before production code existed. |
| Focused `mirakana_editor_core_tests` | Pass | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed 1/1. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `format-check: ok` after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok` after adding resource panel contract checks. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; 11 non-ready gap rows remain audited. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | Built `desktop-gui` and passed 46/46 CTest entries including `mirakana_editor_core_tests` and `mirakana_d3d12_rhi_tests`. |
| `git diff --check` | Pass | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Exit 0; all 29 default CTest entries passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Dev preset configured and built. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor Resource Panel Diagnostics v1 files; leave unrelated pre-existing guidance changes unstaged. |
