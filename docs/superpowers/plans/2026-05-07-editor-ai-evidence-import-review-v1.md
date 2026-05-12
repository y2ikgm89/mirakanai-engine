# Editor AI Evidence Import Review v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic editor-core evidence import review surface so externally supplied AI validation evidence can be pasted, reviewed, and fed into the existing AI Commands panel without editor-core execution or file mutation.

**Architecture:** Keep `editor/core` GUI-independent by adding a small `GameEngine.EditorAiPlaytestEvidence.v1` text parser and review model beside the existing playtest package review models. The visible `mirakana_editor` shell owns only transient pasted text and imported evidence rows, then passes those rows into `make_editor_ai_playtest_evidence_summary_model`.

**Tech Stack:** C++23, `mirakana_editor_core`, retained `mirakana_ui`, Dear ImGui adapter in `mirakana_editor`, `mirakana_editor_core_tests`, `desktop-gui` validation lane.

---

## Goal

Implement a host-independent evidence import workflow that:

- parses line-oriented `GameEngine.EditorAiPlaytestEvidence.v1` text supplied by an external operator;
- validates recipe ids, status values, exit codes, host gates, blockers, external-supply flags, and editor-core execution claims;
- exposes review rows, imported `EditorAiPlaytestValidationEvidenceRow` values, retained `mirakana_ui` ids under `ai_evidence_import`, and read-only diagnostics;
- lets the visible AI Commands panel import valid evidence into local editor state and recompute the existing evidence summary/workflow rows;
- keeps validation recipe execution, arbitrary shell, package scripts, manifest mutation, dynamic Play-In-Editor execution, renderer/RHI handles, Metal readiness, and broad renderer quality out of scope.

## Context

- `EditorAiPlaytestEvidenceSummaryModel` already maps structured external evidence rows onto reviewed operator handoff command rows.
- `EditorAiCommandPanelModel` already displays command/evidence/workflow rows, but `mirakana_editor` currently supplies no evidence rows.
- The master plan previously grouped AI command execution and evidence import as editor-productization follow-ups; this slice narrows the remaining follow-up to AI command execution.
- This slice implements import/review only. AI command execution remains unsupported.

## Constraints

- Do not execute validation recipes, package scripts, raw command strings, or shell commands.
- Do not mutate manifests, packages, files, remediation data, or project settings from `editor/core`.
- Do not introduce JSON libraries or third-party dependencies for the paste format.
- Keep GUI-specific pasted text buffers and buttons in `mirakana_editor`; keep parsing/review logic in `mirakana_editor_core`.
- Keep evidence imported into the GUI as transient local editor state only.

## Done When

- Unit tests prove valid, missing, malformed, duplicate, unknown-recipe, unsupported-claim, and retained-UI import review behavior.
- `mirakana_editor_core` exposes the evidence import parser/review model and retained `ai_evidence_import` UI model.
- `mirakana_editor` AI Commands panel shows a paste/import review area and feeds accepted rows into the existing evidence summary.
- Docs, manifest, skills, plan registry, master plan, and static checks describe Editor AI Evidence Import Review v1 and keep command execution unsupported.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Modify `editor/core/include/mirakana/editor/playtest_package_review.hpp` for import status, row/model/desc types, parser/review declarations, and retained UI declaration.
- Modify `editor/core/src/playtest_package_review.cpp` for parser, validation, review aggregation, and retained `mirakana_ui` output.
- Modify `editor/src/main.cpp` for transient AI evidence paste/import state and AI Commands panel wiring.
- Modify `tests/unit/editor_core_tests.cpp` for RED/GREEN coverage.
- Update docs/manifest/skills/static checks after behavior is green.

## Tasks

### Task 1: RED Tests For Evidence Import Review

- [x] Add focused tests for valid import text, malformed import text, duplicate fields, unknown recipe rows, unsupported execution claims, and retained `ai_evidence_import` ids.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm it fails on missing evidence import symbols.

### Task 2: Editor-Core Evidence Import Model

- [x] Add import status/row/model/desc types and public declarations.
- [x] Implement line-oriented `GameEngine.EditorAiPlaytestEvidence.v1` parsing.
- [x] Validate status, exit code, duplicate fields, expected recipe ids, external supply, editor-core execution claims, and unsupported request flags.
- [x] Implement retained `make_editor_ai_playtest_evidence_import_ui_model` output under `ai_evidence_import`.
- [x] Run focused build/test until `mirakana_editor_core_tests` passes.

### Task 3: Visible Editor AI Commands Wiring

- [x] Add transient paste buffer and imported evidence rows to `mirakana_editor`.
- [x] Use imported evidence rows when building `EditorAiPlaytestEvidenceSummaryModel`.
- [x] Render evidence import review rows and import/clear buttons in the AI Commands panel.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Static Checks

- [x] Update current docs, roadmap, master plan, plan registry, manifest, and Codex/Claude editor skills.
- [x] Update `tools/check-ai-integration.ps1` to require the new evidence import model, retained ids, docs, manifest, and GUI wiring.
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
| RED focused build/test | Pass | `cmake --build --preset dev --target mirakana_editor_core_tests` failed as expected on missing `EditorAiPlaytestEvidenceImportReviewRow`, `EditorAiPlaytestEvidenceImportDesc`, `EditorAiPlaytestEvidenceImportStatus`, `make_editor_ai_playtest_evidence_import_model`, and `make_editor_ai_playtest_evidence_import_ui_model`. |
| Focused `mirakana_editor_core_tests` | Pass | `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed after preserving import-level blockers when merging imported `blocked_by` rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Formatting checks passed after applying `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` to the C++ changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check accepted the editor-core evidence import review API additions. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Static AI integration checks now require the evidence import symbols, retained `ai_evidence_import` ids, visible editor wiring, manifest/docs coverage, and Codex/Claude editor skill guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `editor-productization` remains `partly-ready`; AI evidence import review is documented as complete while AI command execution remains unsupported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | `desktop-gui` build and 46/46 GUI tests passed after wiring the AI Commands panel evidence import review controls. |
| `git diff --check` | Pass | No whitespace errors were reported; existing CRLF warnings were diagnostic only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; host-specific Metal and Apple checks remained diagnostic-only blockers on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default repository build completed after validation. |
| Slice-closing commit | Pass | This commit closes the editor AI evidence import review slice after validation/build gates. |
