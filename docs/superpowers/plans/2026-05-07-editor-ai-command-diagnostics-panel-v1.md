# Editor AI Command Diagnostics Panel v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a visible editor AI Commands panel backed by a GUI-independent diagnostics model over existing AI package, validation recipe, operator handoff, and evidence summary contracts.

**Architecture:** Keep `editor/core` free of Dear ImGui, SDL3, process execution, JSON manifest mutation, and shell command evaluation by wrapping existing `EditorAi*` playtest/operator models into plain display rows. The optional `mirakana_editor` shell adapts current scene package candidates, reviewed validation dry-run command displays, and external-evidence placeholders into that model, then renders a read-only panel.

**Tech Stack:** C++23, `mirakana_editor_core`, existing `playtest_package_review` AI models, `mirakana_ui` retained panel output, `mirakana_editor` Dear ImGui adapter, `mirakana_editor_core_tests`, `desktop-gui` validation lane.

---

## Context

- `editor-productization` still lists shared AI command diagnostics as missing before broad editor readiness can be claimed.
- `mirakana_editor_core` already has deterministic AI package authoring diagnostics, validation recipe preflight, readiness report, operator handoff, evidence summary, remediation, and operator workflow report models.
- `mirakana_editor` already owns current scene/package candidate state and a command registry, but must not execute validation recipes or evaluate raw manifest command strings from the editor panel.

## Constraints

- Add or update tests before production behavior.
- Keep `editor/core` GUI-independent and process-execution-free.
- The panel is read-only diagnostics. It must not mutate `game.agent.json`, execute `run-validation-recipe`, execute package scripts, evaluate arbitrary shell or raw manifest command strings, expose renderer/RHI handles, productize Play-In-Editor runtime hosting, make Metal ready, or claim general renderer quality.
- Workspace migration must keep the new optional panel hidden by default for old workspace documents.
- The visible editor adapter may show reviewed command display/argv data, but actual execution remains an external operator workflow.

## Done When

- `mirakana_editor_core` exposes `EditorAiCommandPanelModel` and `make_editor_ai_command_panel_model` over existing AI workflow models.
- The model reports workflow status, stage rows, reviewed command rows, evidence rows, host gates, blocked-by rows, unsupported claims, and diagnostics deterministically.
- `make_ai_command_panel_ui_model` emits retained first-party `mirakana_ui` rows for the panel.
- Workspace defaults, serialization, migration, and command toggles include an optional hidden `ai_commands` panel.
- `mirakana_editor` displays an AI Commands panel from current package candidate state and reviewed dry-run command data without executing anything.
- Docs, registry, master plan, manifest, static checks, and Codex/Claude editor guidance describe the read-only visible diagnostics surface without broad editor or execution claims.
- Relevant validation passes, including RED evidence, focused `mirakana_editor_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- The slice closes with a validated commit checkpoint after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passes, staging only files owned by this slice.

## Commit Policy

- Use one slice-closing commit after code, docs, manifest, static checks, GUI build, and validation evidence are complete.
- Do not commit RED-test or otherwise known-broken intermediate states.
- Keep unrelated pre-existing guidance changes out of the commit.

## Tasks

### Task 1: RED Tests For AI Commands Panel And Workspace

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add workspace tests proving the optional `ai_commands` panel exists, is hidden by default, serializes as `panel.ai_commands=...`, migrates into old workspace documents, and toggles independently.
- [x] Add model tests proving `make_editor_ai_command_panel_model` summarizes ready, host-gated, external-action-required, and blocked workflow rows without mutating or executing.
- [x] Add retained UI model tests proving `make_ai_command_panel_ui_model` exposes stage, command, evidence, and diagnostic rows.
- [x] Run focused build/test and confirm failure because `mirakana/editor/ai_command_panel.hpp`, `PanelId::ai_commands`, and `make_editor_ai_command_panel_model` do not exist yet.

### Task 2: Editor-Core AI Command Panel Model

**Files:**
- Create: `editor/core/include/mirakana/editor/ai_command_panel.hpp`
- Create: `editor/core/src/ai_command_panel.cpp`
- Modify: `editor/CMakeLists.txt`

- [x] Add plain row/model types for workflow stages, reviewed command rows, evidence rows, unsupported claims, and diagnostics.
- [x] Implement deterministic status aggregation over `EditorAiPlaytestOperatorWorkflowReportModel`, `EditorAiPlaytestOperatorHandoffModel`, and `EditorAiPlaytestEvidenceSummaryModel`.
- [x] Implement `make_ai_command_panel_ui_model` with first-party `mirakana_ui` retained rows and no GUI/process dependencies.

### Task 3: Workspace And Visible Editor Wiring

**Files:**
- Modify: `editor/core/include/mirakana/editor/workspace.hpp`
- Modify: `editor/core/src/workspace.cpp`
- Modify: `editor/src/main.cpp`

- [x] Add `PanelId::ai_commands`, token `ai_commands`, and hidden-by-default workspace state.
- [x] Register `view.ai_commands`, add it to the View menu, and draw the AI Commands panel when visible.
- [x] Build package diagnostics from current scene package candidate rows and current `runtimePackageFiles` state.
- [x] Build validation preflight from reviewed dry-run command display/argv rows for selected manifest recipe ids.
- [x] Build readiness, operator handoff, evidence summary, remediation, and workflow report models as read-only data before rendering the panel.
- [x] Display stage rows, reviewed command rows, evidence rows, host gates, blockers, unsupported claims, and diagnostics with Dear ImGui tables/sections.

### Task 4: Docs, Manifest, Static Checks

**Files:**
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

- [x] Describe Editor AI Command Diagnostics Panel v1 as a visible read-only diagnostics surface over existing AI workflow models.
- [x] Keep non-ready claims explicit: no validation execution, arbitrary shell, raw manifest command evaluation, free-form manifest edits, package scripts, dynamic game-module/runtime-host PIE execution, renderer/RHI handles, Metal readiness, or broad renderer quality.
- [x] Add static checks for `EditorAiCommandPanelModel`, workspace `ai_commands` panel, visible `mirakana_editor` wiring, docs, manifest, and AI guidance markers.

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
| RED focused build/test | Failed as expected | `cmake --build --preset dev --target mirakana_editor_core_tests` failed on missing `mirakana/editor/ai_command_panel.hpp` before production code existed. |
| Focused `mirakana_editor_core_tests` | Pass | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed 1/1 after implementation and again after docs/static sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Repository formatting check passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check accepted the new `mirakana/editor/ai_command_panel.hpp` surface. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Static AI integration checks passed with `EditorAiCommandPanelModel`, workspace `ai_commands`, visible `mirakana_editor` wiring, docs, manifest, and AI guidance markers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Audit still reports 11 known non-ready `unsupportedProductionGaps`; `editor-productization` remains `partly-ready` with AI Commands diagnostics and remaining unsupported claims explicit. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | `desktop-gui` configured, built `mirakana_editor`, and ran 46/46 GUI preset tests including `mirakana_editor_core_tests`. |
| `git diff --check` | Pass | No whitespace errors; Git reported only expected CRLF normalization warnings for touched files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passed, including agent/static checks, production readiness audit, toolchain checks, tidy smoke, dev build, and 29/29 dev CTest tests. Metal/Apple diagnostics remain host-gated on this Windows host as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default dev configure/build completed after validation. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the Editor AI Command Diagnostics Panel v1 files; leave unrelated pre-existing guidance changes unstaged. |
