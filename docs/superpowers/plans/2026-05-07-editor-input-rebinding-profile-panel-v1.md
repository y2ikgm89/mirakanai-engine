# Editor Input Rebinding Profile Panel v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the existing input rebinding profile review contract into a visible read-only editor panel that `mirakana_editor` and AI/editor tooling can share.

**Architecture:** Keep input action maps, rebinding profiles, validation, and formatting in `mirakana_editor_core` over `mirakana_runtime` value types. Keep the visible Dear ImGui panel as a thin adapter over a retained `mirakana_ui` model, without adding SDL3 input APIs, native device handles, runtime input consumption, glyph generation, multiplayer assignment, or file mutation.

**Tech Stack:** C++23, `mirakana_runtime` input contracts, `mirakana_editor_core`, `mirakana_ui`, existing `Workspace` panel state, and visible `mirakana_editor` Dear ImGui adapters.

---

## Goal

Add a hidden-by-default `Input Rebinding` editor panel over `GameEngine.RuntimeInputRebindingProfile.v1` data that shows:

- the reviewed profile id and save readiness;
- base action/axis rows beside profile override rows;
- runtime validation conflicts and unsupported claim diagnostics;
- retained `mirakana_ui` element ids suitable for AI/editor inspection;
- visible `mirakana_editor` menu and panel wiring.

## Context

- `RuntimeInputRebindingProfile` validation/application/serialization already exists in `mirakana_runtime`.
- `EditorInputRebindingProfileReviewModel` already reports save readiness, runtime validation rows, and unsupported mutation/execution/native-handle/UI/glyph claims.
- The master plan still keeps interactive runtime/game rebinding panels, glyphs, UI focus/consumption, and device assignment as follow-up work; this slice narrows that by adding a visible read-only editor profile review panel only.
- Existing editor productization slices add hidden-by-default workspace panels for Resources, AI Commands, Profiler, Project Settings, and Timeline.

## Constraints

- Do not mutate input profile files, execute commands, run validation recipes, call package scripts, or evaluate arbitrary shell.
- Do not add runtime input consumption/bubbling, UI focus routing, capture-record rebinding, glyph generation, multiplayer device assignment, per-device profiles, SDL3 input API exposure, native handles, cloud saves, or binary saves.
- Keep `editor/core` GUI-independent and free of Dear ImGui, SDL3, renderer/RHI, and OS APIs.
- Keep ready claims narrow: this is visible review/readiness UX, not a complete interactive rebinding workflow.

## Done When

- `mirakana_editor_core` exposes `EditorInputRebindingProfilePanelModel`, binding rows, status labels, and `make_input_rebinding_profile_panel_ui_model`.
- Workspace state includes a hidden-by-default `input_rebinding` panel that serializes, restores, and migrates with defaults.
- Unit tests prove clean panel rows, conflict diagnostics, retained `mirakana_ui` ids, and workspace panel persistence.
- `mirakana_editor` exposes View > Input Rebinding and renders the panel from the new retained model.
- Docs, manifest, skills, and `tools/check-ai-integration.ps1` distinguish this visible read-only panel from unsupported interactive capture/glyph/focus/device work.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify: `editor/core/include/mirakana/editor/input_rebinding.hpp`
- Modify: `editor/core/src/input_rebinding.cpp`
- Modify: `editor/core/include/mirakana/editor/workspace.hpp`
- Modify: `editor/core/src/workspace.cpp`
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

### Task 1: RED Tests For Panel Rows And Workspace State

- [x] Add `editor input rebinding panel model exposes reviewed bindings and ui rows` to `tests/unit/editor_core_tests.cpp`.
- [x] Assert a clean profile returns `status_label == "ready"`, action and axis binding rows, override counts, `mutates == false`, `executes == false`, and retained ids under `input_rebinding`.
- [x] Update workspace tests to expect `PanelId::input_rebinding` hidden by default, toggled, serialized, restored, and defaulted during v0 migration.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and record the expected missing symbol/panel-id failure.

### Task 2: Editor-Core Model

- [x] Add `EditorInputRebindingProfilePanelStatus`, `EditorInputRebindingProfileBindingRow`, and `EditorInputRebindingProfilePanelModel`.
- [x] Implement deterministic trigger/source formatting for keyboard, pointer, gamepad button, key-pair axis, and gamepad-axis rows.
- [x] Implement `make_editor_input_rebinding_profile_panel_model` by reusing `make_editor_input_rebinding_profile_review_model` and reading base/profile rows only.
- [x] Implement `make_input_rebinding_profile_panel_ui_model` with stable ids under `input_rebinding`.
- [x] Add `PanelId::input_rebinding` to workspace defaults, tokens, serialization, and migration defaults.
- [x] Run focused `mirakana_editor_core_tests`.

### Task 3: Visible Editor Wiring

- [x] Include `mirakana/editor/input_rebinding.hpp` in `editor/src/main.cpp`.
- [x] Add a shell-owned default base action map and reviewed profile used by the read-only panel.
- [x] Add View menu, command registration, visibility check, and `draw_input_rebinding_panel`.
- [x] Render summary, binding rows, review rows, unsupported claims, and diagnostics without mutating files or capturing input.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Static Checks

- [x] Document Editor Input Rebinding Profile Panel v1 as a visible read-only review panel.
- [x] Keep non-ready claims explicit: no interactive capture, glyphs, focus consumption/bubbling, multiplayer assignment, per-device profiles, SDL3/native handles, cloud/binary saves, execution, or mutation.
- [x] Add static checks for the new editor-core symbols, workspace panel token, visible editor wiring, docs, manifest, and Codex/Claude skill guidance.

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
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Expected failure | `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing `EditorInputRebindingProfileBindingRow`, `PanelId::input_rebinding`, `make_editor_input_rebinding_profile_panel_model`, `EditorInputRebindingProfilePanelStatus`, and `make_input_rebinding_profile_panel_ui_model`. |
| Focused `mirakana_editor_core_tests` | Passed | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` was applied first; `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. The first run caught an over-specific new static needle in `tests/unit/editor_core_tests.cpp`; the check now asserts the actual test API call instead. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `production-readiness-audit-check: ok`; unsupported gap counts remain explicit. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | 46/46 GUI tests passed after visible `mirakana_editor` wiring. |
| `git diff --check` | Passed | Exit code 0. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Exit code 0. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Dev preset configure/build completed. |
| Slice-closing commit | Pending |  |
