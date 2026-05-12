# Editor AI Reviewed Validation Execution v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the visible `mirakana_editor` AI Commands panel execute host-gate-free reviewed `run-validation-recipe` rows and feed the result into the existing evidence summary without moving process execution into `editor/core`.

**Architecture:** Add an `editor/core` review model that converts an `EditorAiPlaytestOperatorHandoffCommandRow` with reviewed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun ...` argv into a safe `mirakana::ProcessCommand` using `-Mode Execute`, or reports blocked/host-gated diagnostics. The optional `mirakana_editor` shell owns the actual `Win32ProcessRunner` call, maps the process result to a transient `EditorAiPlaytestValidationEvidenceRow`, and leaves host-gated recipes in the external handoff path.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_platform` process contracts, `mirakana_editor` Dear ImGui adapter, `mirakana_editor_core_tests`, `desktop-gui` validation lane.

---

## Goal

Implement a narrow reviewed execution path that:

- accepts only reviewed `run-validation-recipe` argv rows already produced by the AI operator handoff model;
- rewrites `-Mode DryRun` to `-Mode Execute` while preserving safe typed arguments such as `-Recipe`;
- rejects raw shell commands, malformed argv, blocked handoff rows, missing recipe ids, arbitrary manifest command evaluation, free-form manifest edits, and broad package script execution requests;
- treats host-gated recipe rows as still external until a later explicit host-gate acknowledgement UX slice;
- executes only from the optional visible `mirakana_editor` shell through the platform process backend, never from `editor/core`;
- records transient evidence rows from process exit code/stdout/stderr and recomputes the existing AI Commands evidence summary.

## Context

- `run-validation-recipe` already has reviewed dry-run/execute modes and an allowlisted recipe set.
- `EditorAiPlaytestOperatorHandoffModel` already exposes reviewed command rows with argv data.
- `Editor AI Evidence Import Review v1` lets operators paste external evidence; this slice adds a local reviewed execution path for host-gate-free recipes.
- The master plan still lists host-gated AI command execution follow-ups under `editor-productization`.

## Constraints

- Do not add arbitrary shell execution or raw manifest command evaluation.
- Do not execute validation recipes from `editor/core`; it may only return a reviewed `mirakana::ProcessCommand`.
- Do not run host-gated recipes automatically; keep those rows as explicit external/operator work.
- Do not parse the runner JSON in this slice; map process exit code/stdout/stderr to existing evidence rows.
- Keep evidence transient in `mirakana_editor` local state.

## Done When

- Unit tests prove valid host-free execution plan conversion, host-gated blocking, malformed argv rejection, unsafe executable rejection, unsupported-claim diagnostics, and retained execution-plan UI ids.
- `mirakana_editor` shows reviewed execution controls for eligible AI command rows and appends/replaces transient evidence rows after execution.
- Docs, manifest, skills, plan registry, master plan, and static checks describe Editor AI Reviewed Validation Execution v1 and keep host-gated recipe execution plus arbitrary package scripts unsupported.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify `editor/core/include/mirakana/editor/ai_command_panel.hpp` for execution status, desc/model, and reviewed process command declarations.
- Modify `editor/core/src/ai_command_panel.cpp` for argv validation, `DryRun` to `Execute` rewrite, host-gate blocking, diagnostics, and retained `ai_commands.execution` UI output.
- Modify `tests/unit/editor_core_tests.cpp` for RED/GREEN coverage.
- Modify `editor/src/main.cpp` for execution controls, `Win32ProcessRunner` invocation on Windows, process-result to evidence-row mapping, and project-switch reset.
- Update docs/manifest/skills/static checks after behavior is green.

## Tasks

### Task 1: RED Tests For Reviewed Execution Planning

- [x] Add focused `mirakana_editor_core_tests` coverage for host-free `agent-contract` argv conversion to a safe `mirakana::ProcessCommand`.
- [x] Add tests for host-gated rows, blocked rows, malformed argv, unsafe executable, unsupported execution claims, and retained `ai_commands.execution` ids.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm it fails on missing execution-plan symbols.

### Task 2: Editor-Core Execution Plan Model

- [x] Add `EditorAiReviewedValidationExecutionStatus`, desc/model row types, and public declarations.
- [x] Implement strict argv validation for `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe <id>`.
- [x] Rewrite the reviewed argv into a safe `mirakana::ProcessCommand` with `-Mode Execute`.
- [x] Block host-gated rows and unsupported claim requests without mutating or executing.
- [x] Implement retained `ai_commands.execution` UI output.
- [x] Run focused build/test until `mirakana_editor_core_tests` passes.

### Task 3: Visible Editor Execution Wiring

- [x] Add AI Commands execution plan rendering and buttons for executable host-free rows.
- [x] Execute reviewed commands through `mirakana::Win32ProcessRunner` on Windows and report non-Windows as host-gated.
- [x] Convert `ProcessResult` to transient `EditorAiPlaytestValidationEvidenceRow` values and replace rows by recipe id.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Static Checks

- [x] Update current docs, roadmap, master plan, plan registry, manifest, and Codex/Claude editor skills.
- [x] Update `tools/check-ai-integration.ps1` to require the new execution plan model, retained ids, docs, manifest, and GUI wiring.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Passed | `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation on missing reviewed execution-plan symbols. |
| Focused `mirakana_editor_core_tests` | Passed | `cmake --build --preset dev --target mirakana_editor_core_tests; if ($LASTEXITCODE -eq 0) { ctest --preset dev --output-on-failure -R mirakana_editor_core_tests } else { exit $LASTEXITCODE }` passed after implementation and formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Failed once on `ai_command_panel.cpp`, then passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public API boundary check passed for the new editor/core model and `mirakana::ProcessCommand` use. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration static checks passed after manifest/docs/static synchronization. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Unsupported production gap audit passed with `editor-productization` still `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | `desktop-gui` configured, built `mirakana_editor`, and ran 46/46 tests. |
| `git diff --check` | Passed | Whitespace check passed; Git reported only existing CRLF conversion warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | `validate: ok`; host-specific Metal and Apple checks remained diagnostic-only blockers on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Default `dev` preset configured and built successfully after validation. |
| Slice-closing commit | Passed | This commit closes the editor AI reviewed validation execution slice after validation/build gates. |
