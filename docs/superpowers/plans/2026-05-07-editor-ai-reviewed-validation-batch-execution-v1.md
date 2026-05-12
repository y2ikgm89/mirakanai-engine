# Editor AI Reviewed Validation Batch Execution v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reviewed batch execution model and visible `MK_editor` control for all currently executable AI validation recipe rows, including host-gated rows only after explicit acknowledgement.

**Architecture:** Build on `make_editor_ai_reviewed_validation_execution_plan` rather than adding a second command validator. `editor/core` will aggregate per-row reviewed plans into a deterministic batch model and retained `MK_ui` summary; the optional `MK_editor` shell will execute only the batch model's `can_execute` plans through the existing platform process runner path.

**Tech Stack:** C++23, `MK_editor_core`, `MK_platform` process contracts, `MK_editor` Dear ImGui adapter, retained `MK_ui`, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the next narrow AI Commands productization gap:

- preserve the existing per-row `Execute` path;
- add a batch review model over reviewed `run-validation-recipe` operator handoff rows;
- include host-gated rows only when their recipe id is explicitly acknowledged in the visible editor session;
- expose retained `ai_commands.execution.batch` summary rows with ready, host-gated, blocked, and executable counts;
- add a visible `Execute Ready` action that executes all currently executable reviewed plans in the displayed order;
- keep arbitrary shell, raw manifest command evaluation, broad package script execution, free-form manifest edits, automatic unacknowledged host-gated execution, dynamic game-module/runtime-host Play-In-Editor execution, renderer/RHI handles, Metal readiness, and broad renderer quality out of scope.

## Context

- `Editor AI Reviewed Validation Execution v1` already validates one operator handoff row and rewrites reviewed dry-run argv into a safe `mirakana::ProcessCommand`.
- `Host-Gated Validation Execution Ack v1` already requires explicit per-row host-gate acknowledgement before a host-gated row can execute.
- The visible `MK_editor` AI Commands panel already stores transient acknowledgement ids and local execution evidence rows.
- The master plan still lists automatic host-gated AI command execution follow-ups; this slice implements a reviewed batch button, not unprompted automatic execution.

## Constraints

- Do not execute anything from `editor/core`; it may only produce reviewed process commands and diagnostics.
- Do not bypass per-row host-gate acknowledgement.
- Do not run unacknowledged host-gated rows from the batch action.
- Do not accept shell strings or raw manifest command text.
- Keep evidence transient in `MK_editor` local state.
- Preserve input order for batch execution so UI order and process evidence are predictable.

## Done When

- RED `MK_editor_core_tests` proves the batch model surface is missing.
- `EditorAiReviewedValidationExecutionBatchModel` exposes per-row plans, counts, executable plan indexes, and retained `ai_commands.execution.batch` labels.
- The visible `MK_editor` AI Commands panel shows an `Execute Ready` button and runs only `can_execute` plans in batch order through the existing process runner.
- Docs, master plan, registry, manifest, skills, and static checks record the boundary truthfully.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Batch Model Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `MK_TEST("editor ai reviewed validation batch execution collects ready acknowledged plans")`.
- [x] Build four `EditorAiPlaytestOperatorHandoffCommandRow` rows:
  - `agent-contract`: ready host-free reviewed dry-run argv.
  - `desktop-runtime-sample-game-scene-gpu-package`: host-gated reviewed dry-run argv with `d3d12-windows-primary`, acknowledged by recipe id.
  - `vulkan-package-smoke`: host-gated reviewed dry-run argv with `vulkan-runtime`, not acknowledged.
  - `blocked-recipe`: blocked row with reviewed-looking argv.
- [x] Call `make_editor_ai_reviewed_validation_execution_batch` with `acknowledged_host_gate_recipe_ids = {"desktop-runtime-sample-game-scene-gpu-package"}`.
- [x] Assert:
  - `plans.size() == 4`,
  - `ready_count == 2`,
  - `host_gated_count == 1`,
  - `blocked_count == 1`,
  - `can_execute_any == true`,
  - `executable_plan_indexes == {0, 1}`,
  - `commands.size() == 2`,
  - the second command contains `-HostGateAcknowledgements d3d12-windows-primary`.
- [x] Assert retained UI ids exist:
  - `ai_commands.execution.batch.ready_count`,
  - `ai_commands.execution.batch.host_gated_count`,
  - `ai_commands.execution.batch.blocked_count`,
  - `ai_commands.execution.batch.executable_count`.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`; confirm it fails before implementation because the batch model symbols are missing.

### Task 2: Editor-Core Batch Execution Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/ai_command_panel.hpp`
- Modify: `editor/core/src/ai_command_panel.cpp`

- [x] Add:

```cpp
struct EditorAiReviewedValidationExecutionBatchDesc {
    std::vector<EditorAiPlaytestOperatorHandoffCommandRow> command_rows;
    std::string working_directory;
    std::vector<std::string> acknowledged_host_gate_recipe_ids;
};

struct EditorAiReviewedValidationExecutionBatchModel {
    std::vector<EditorAiReviewedValidationExecutionModel> plans;
    std::vector<std::size_t> executable_plan_indexes;
    std::vector<mirakana::ProcessCommand> commands;
    std::size_t ready_count{0};
    std::size_t host_gated_count{0};
    std::size_t blocked_count{0};
    bool can_execute_any{false};
    std::vector<std::string> diagnostics;
};
```

- [x] Add declarations for:

```cpp
[[nodiscard]] EditorAiReviewedValidationExecutionBatchModel
make_editor_ai_reviewed_validation_execution_batch(const EditorAiReviewedValidationExecutionBatchDesc& desc);

[[nodiscard]] mirakana::ui::UiDocument
make_ai_reviewed_validation_execution_batch_ui_model(const EditorAiReviewedValidationExecutionBatchModel& model);
```

- [x] Implement `make_editor_ai_reviewed_validation_execution_batch` by iterating `command_rows` in order and calling `make_editor_ai_reviewed_validation_execution_plan`.
- [x] For each row whose recipe id appears in `acknowledged_host_gate_recipe_ids`, set `acknowledge_host_gates=true` and pass that row's `host_gates` as `acknowledged_host_gates`.
- [x] Count `ready`, `host_gated`, and `blocked` statuses from the resulting plans.
- [x] Append only `can_execute` plans to `executable_plan_indexes` and `commands`.
- [x] Add diagnostics:
  - `batch has no reviewed validation rows` when input is empty,
  - `batch has no executable reviewed validation rows` when rows exist but none can execute.
- [x] Implement retained `ai_commands.execution.batch` UI labels for ready, host-gated, blocked, executable counts, and diagnostics.
- [x] Run `cmake --build --preset dev --target MK_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R MK_editor_core_tests`.

### Task 3: Visible AI Commands Batch Control

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add `make_ai_reviewed_execution_batch(std::span<const EditorAiPlaytestOperatorHandoffCommandRow>) const`.
- [x] Build the batch desc from current command rows, the same current working directory used by the single-row plan, and `ai_acknowledged_host_gate_recipe_ids_`.
- [x] Add `execute_ai_reviewed_validation_batch(const EditorAiReviewedValidationExecutionBatchModel&)`.
- [x] Iterate `model.plans` by `executable_plan_indexes` and call the existing `execute_ai_reviewed_validation_recipe(plan)` for each executable plan.
- [x] In `draw_ai_reviewed_execution_controls`, show a compact batch summary above the table and an `Execute Ready` button only when `batch.can_execute_any`.
- [x] Keep per-row `Execute` and host-gate acknowledgement behavior unchanged.
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

- [x] Record `Editor AI Reviewed Validation Batch Execution v1`, `EditorAiReviewedValidationExecutionBatchModel`, `make_editor_ai_reviewed_validation_execution_batch`, retained `ai_commands.execution.batch` ids, and visible `Execute Ready`.
- [x] State that batch execution still requires explicit host-gate acknowledgement and still rejects arbitrary shell, raw manifest command evaluation, broad package script execution, and free-form manifest edits.
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
| RED focused build/test | PASS (expected failure) | `cmake --build --preset dev --target MK_editor_core_tests` failed before implementation on missing batch execution symbols. |
| Focused `MK_editor_core_tests` | PASS | `cmake --build --preset dev --target MK_editor_core_tests` and `ctest --preset dev --output-on-failure -R MK_editor_core_tests` passed after the batch model implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | PASS | Built `MK_editor`; GUI preset reported 46/46 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` records the reviewed validation batch contract across code, docs, manifest, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Reported `unsupported_gaps=11` and preserved `editor-productization` as `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Initial check found clang-format drift in `editor/core/src/ai_command_panel.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and the rerun passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check passed after adding the editor-core batch model surface. |
| `git diff --check` | PASS | No whitespace errors; Git emitted only existing LF-to-CRLF working-copy warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed with 29/29 `dev` CTest tests; Metal and Apple host checks remained diagnostic host gates on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | `tools/build.ps1` configured and built the `dev` preset successfully. |
| Slice-closing commit | Recorded by this slice-closing commit | Stage only the reviewed validation batch execution files; leave unrelated pre-existing guidance changes unstaged. |
