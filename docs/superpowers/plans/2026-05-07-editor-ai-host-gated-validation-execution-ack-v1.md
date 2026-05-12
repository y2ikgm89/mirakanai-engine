# Editor AI Host-Gated Validation Execution Ack v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the visible `mirakana_editor` AI Commands panel execute reviewed host-gated `run-validation-recipe` rows only after an explicit per-row host-gate acknowledgement.

**Architecture:** Extend the existing `EditorAiReviewedValidationExecutionDesc` / `EditorAiReviewedValidationExecutionModel` review contract with acknowledgement input and acknowledgement diagnostics. `editor/core` still only validates and returns a safe `mirakana::ProcessCommand`; the optional `mirakana_editor` shell stores transient acknowledgement state, passes acknowledged host gates into the review model, and runs the returned command through the existing platform process runner path.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_platform` process contracts, `mirakana_editor` Dear ImGui adapter, `mirakana_editor_core_tests`, `desktop-gui` validation lane.

---

## Goal

Implement a narrow acknowledgement path that:

- keeps host-gated recipe execution blocked by default;
- requires the caller to explicitly acknowledge every host gate listed on the reviewed operator handoff row;
- appends reviewed `-HostGateAcknowledgements <gate>` arguments only after `-Mode DryRun` has been rewritten to `-Mode Execute`;
- rejects missing acknowledgements, unknown acknowledgements, blocked handoff rows, malformed argv, unsafe tokens, arbitrary shell, raw manifest command evaluation, broad package script execution requests, and free-form manifest edits;
- stores acknowledgement state only in the visible `mirakana_editor` session, not in project files or manifests;
- continues to map process exit code/stdout/stderr into transient AI playtest evidence rows.

## Context

- `Editor AI Reviewed Validation Execution v1` already converts host-gate-free reviewed dry-run rows into safe execute commands.
- `tools/run-validation-recipe.ps1` already enforces `-HostGateAcknowledgements` in `Execute` mode for recipes such as `desktop-runtime-sample-game-scene-gpu-package`.
- The master plan still lists host-gated AI command execution follow-ups under `editor-productization`.

## Constraints

- Do not execute host-gated rows automatically.
- Do not move process execution into `editor/core`.
- Do not add arbitrary shell execution, raw manifest command evaluation, broad package-script execution, or free-form manifest edits.
- Do not persist host-gate acknowledgement state to project documents or manifests in this slice.
- Keep execution limited to reviewed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe <id> ...` argv rows.

## Done When

- Unit tests prove host-gated rows require explicit acknowledgement, exact host-gate coverage, safe command construction with `-HostGateAcknowledgements`, and retained acknowledgement UI ids.
- `mirakana_editor` shows a per-row host-gate acknowledgement checkbox and enables `Execute` only after all host gates on that reviewed row are acknowledged.
- Docs, manifest, skills, plan registry, master plan, and static checks describe Host-Gated Validation Execution Ack v1 and keep automatic host-gated execution plus arbitrary package scripts unsupported.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify `editor/core/include/mirakana/editor/ai_command_panel.hpp` for acknowledgement input/model fields.
- Modify `editor/core/src/ai_command_panel.cpp` for exact host-gate acknowledgement validation, command argument append, diagnostics, and retained `ai_commands.execution` acknowledgement UI output.
- Modify `tests/unit/editor_core_tests.cpp` for RED/GREEN coverage.
- Modify `editor/src/main.cpp` for transient acknowledgement state, checkbox rendering, and reset on project/session switch.
- Update docs/manifest/skills/static checks after behavior is green.

## Tasks

### Task 1: RED Tests For Host-Gated Ack Review

- [x] Add `mirakana_editor_core_tests` coverage named `editor ai reviewed validation execution requires host gate acknowledgement`.

  The test should construct an `EditorAiPlaytestOperatorHandoffCommandRow`:

  ```cpp
  mirakana::editor::EditorAiPlaytestOperatorHandoffCommandRow row{
      "desktop-runtime-sample-game-scene-gpu-package",
      mirakana::editor::EditorAiPackageAuthoringDiagnosticStatus::host_gated,
      "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe desktop-runtime-sample-game-scene-gpu-package -GameTarget sample_desktop_runtime_game",
      {"pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1", "-Mode", "DryRun", "-Recipe",
       "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget", "sample_desktop_runtime_game"},
      {"d3d12-windows-primary"},
      {},
      "EditorAiPlaytestReadinessReportModel.ready_for_operator_validation",
      "D3D12 package handoff is host-gated",
  };
  ```

  First call `make_editor_ai_reviewed_validation_execution_plan` with no acknowledgement and assert `status == host_gated`, `can_execute == false`, `host_gate_acknowledgement_required == true`, `host_gates_acknowledged == false`, and a diagnostic containing `requires host-gate acknowledgement`.

- [x] In the same test, set:

  ```cpp
  mirakana::editor::EditorAiReviewedValidationExecutionDesc desc{row, "G:/workspace/development/GameEngine"};
  desc.acknowledge_host_gates = true;
  desc.acknowledged_host_gates = {"d3d12-windows-primary"};
  ```

  Assert `status == ready`, `can_execute == true`, `host_gates_acknowledged == true`, `command.executable == "pwsh"`, command arguments contain `-Mode Execute`, `-Recipe desktop-runtime-sample-game-scene-gpu-package`, `-GameTarget sample_desktop_runtime_game`, and `-HostGateAcknowledgements d3d12-windows-primary`, and `mirakana::is_safe_process_command(command)` is true.

- [x] Add `mirakana_editor_core_tests` coverage named `editor ai reviewed validation execution rejects mismatched host gate acknowledgement`.

  Use the same row with `acknowledge_host_gates = true` and `acknowledged_host_gates = {"metal-apple"}`. Assert `status == blocked`, `can_execute == false`, diagnostics include `missing host-gate acknowledgement` and `unknown host-gate acknowledgement`, and unsupported claim requests still block execution.

- [x] Add retained UI assertions for:

  ```cpp
  ai_commands.execution.desktop-runtime-sample-game-scene-gpu-package.host_gate_acknowledgement_required
  ai_commands.execution.desktop-runtime-sample-game-scene-gpu-package.host_gates_acknowledged
  ```

- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm it fails on missing acknowledgement fields.

### Task 2: Editor-Core Ack Plan Model

- [x] Add fields to `EditorAiReviewedValidationExecutionDesc`:

  ```cpp
  bool acknowledge_host_gates{false};
  std::vector<std::string> acknowledged_host_gates;
  ```

- [x] Add fields to `EditorAiReviewedValidationExecutionModel`:

  ```cpp
  bool host_gate_acknowledgement_required{false};
  bool host_gates_acknowledged{false};
  std::vector<std::string> acknowledged_host_gates;
  ```

- [x] Update `make_editor_ai_reviewed_validation_execution_plan` so a row with host gates:

  - sets `host_gate_acknowledgement_required = true`;
  - stays `host_gated` with no command when `acknowledge_host_gates == false`;
  - becomes `blocked` if any row host gate is missing from `acknowledged_host_gates`;
  - becomes `blocked` if `acknowledged_host_gates` contains a gate not present on the row;
  - validates and rewrites reviewed argv after host gates are fully acknowledged;
  - appends `-HostGateAcknowledgements <gate>` pairs to the safe process command after rewriting `-Mode Execute`.

- [x] Update `make_ai_reviewed_validation_execution_ui_model` to expose retained labels for acknowledgement-required and acknowledged state.

- [x] Run focused build/test until `mirakana_editor_core_tests` passes.

### Task 3: Visible Editor Ack Wiring

- [x] Add transient editor state:

  ```cpp
  std::vector<std::string> ai_acknowledged_host_gate_recipe_ids_;
  ```

- [x] Add helpers:

  ```cpp
  [[nodiscard]] bool ai_host_gate_acknowledged(std::string_view recipe_id) const;
  void set_ai_host_gate_acknowledged(std::string_view recipe_id, bool acknowledged);
  ```

  Use `std::ranges::find` over the vector, append on true, and erase on false.

- [x] Update `make_ai_reviewed_execution_plan` to pass `acknowledge_host_gates = ai_host_gate_acknowledged(row.recipe_id)` and `acknowledged_host_gates = row.host_gates` only when acknowledged.

- [x] Update `draw_ai_reviewed_execution_controls` to add an `Ack` column. Render a checkbox for rows that require host-gate acknowledgement and leave it unchecked by default. `Execute` must remain unavailable until the next model pass reports `can_execute`.

- [x] Clear `ai_acknowledged_host_gate_recipe_ids_` in the same project/session reset path that clears transient AI evidence rows.

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Static Checks

- [x] Update `docs/editor.md`, `docs/current-capabilities.md`, `docs/ai-game-development.md`, `docs/roadmap.md`, `docs/testing.md`, the master plan, and the plan registry.
- [x] Update `engine/agent/manifest.json` so `recommendedNextPlan`, `unsupportedProductionGaps.editor-productization`, and `gameCodeGuidance` distinguish explicit host-gate acknowledgement from automatic host-gated execution.
- [x] Update `.agents/skills/editor-change/SKILL.md` and `.claude/skills/gameengine-editor/SKILL.md` with the acknowledgement rule.
- [x] Update `tools/check-ai-integration.ps1` to require the new fields, tests, retained ids, docs, manifest, and GUI wiring.
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
| RED focused build/test | PASS (expected fail) | `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing acknowledgement fields. |
| Focused `mirakana_editor_core_tests` | PASS | `cmake --build --preset dev --target mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found one clang-format violation; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting; rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public header change keeps backend/native handle boundaries clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`; unsupported gap count remains 11. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Desktop GUI lane built and ran 46/46 tests. |
| `git diff --check` | PASS | Exit 0; Git reported line-ending conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; Metal/Apple lanes remain diagnostic/host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Default dev build completed. |
| Slice-closing commit | Pending |  |
