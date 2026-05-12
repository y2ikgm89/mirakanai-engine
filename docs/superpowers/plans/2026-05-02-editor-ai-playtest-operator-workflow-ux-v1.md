# Editor AI Playtest Operator Workflow UX v1 Implementation Plan (2026-05-02)

> **Status note (2026-05-02):** This is the focused next production gap selected by [AI-Operable Production Loop Clean Uplift v1](2026-05-02-ai-operable-production-loop-clean-uplift-v1.md). It should start only after that milestone records validation evidence.

**Goal:** Turn the consolidated read-only Editor AI playtest operator workflow into a practical editor/operator UX surface without adding another remediation row model.

**Context:** The clean-uplift milestone retired the separate remediation evidence-review and closeout-report candidates. The existing loop already has package diagnostics, validation recipe preflight, readiness report, operator handoff, evidence summary, remediation queue, and remediation handoff models. The next useful production gap is making that loop easier for an operator to inspect and act on, not expanding editor core into command execution or fix automation.

**Constraints:**

- Do not execute validation commands, package scripts, arbitrary shell, raw/free-form command text, fixes, evidence mutation, remediation mutation, or manifest mutation from editor core.
- Do not claim play-in-editor productization, broad editor productization, renderer/RHI/native handle exposure, Metal readiness, or general renderer quality.
- Reuse `EditorAiPackageAuthoringDiagnosticsModel`, `EditorAiValidationRecipePreflightModel`, `EditorAiPlaytestReadinessReportModel`, `EditorAiPlaytestOperatorHandoffModel`, `EditorAiPlaytestEvidenceSummaryModel`, `EditorAiPlaytestRemediationQueueModel`, and `EditorAiPlaytestRemediationHandoffModel`.
- Keep the UX/report surface deterministic and read-only except for existing reviewed apply lanes that are already separate, such as package registration.

**Done When:**

- A focused UX/report design identifies the operator workflow states and the exact existing model rows shown to the user.
- RED checks or tests fail before implementation for the selected operator UX/report behavior.
- GREEN implementation keeps editor core non-executing and non-mutating for validation/evidence/remediation/fixes.
- Docs, manifest, static checks, and validation evidence stay synchronized.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/toolchain blocker.

## Implementation Tasks

### Task 1: Operator UX Inventory

- [x] Inventory current editor panels, package diagnostics, and AI playtest model consumers.
- [x] Choose the smallest operator-facing surface: editor panel, structured report, or both.
- [x] Record the data flow from package diagnostics to closeout through existing evidence summary reruns.

Inventory result: `mirakana_editor` already has the Assets panel package candidate/review/apply path for reviewed package registration. The AI playtest package diagnostics, validation preflight, readiness, operator handoff, evidence summary, remediation queue, and remediation handoff behavior lives in `mirakana_editor_core` models and unit tests rather than a dedicated GUI panel. The smallest useful operator-facing surface is therefore a deterministic structured report model in editor core, with GUI rendering left as a later productization slice.

Selected data flow: `EditorAiPackageAuthoringDiagnosticsModel` -> `EditorAiValidationRecipePreflightModel` -> `EditorAiPlaytestReadinessReportModel` -> `EditorAiPlaytestOperatorHandoffModel` -> `EditorAiPlaytestEvidenceSummaryModel` -> `EditorAiPlaytestRemediationQueueModel` -> `EditorAiPlaytestRemediationHandoffModel` -> closeout through rerun `EditorAiPlaytestEvidenceSummaryModel` plus empty remediation queue/handoff rows.

### Task 2: RED Checks

- [x] Add failing tests/static checks for the selected UX/report behavior.
- [x] Add failing checks that reject validation execution, package script execution, arbitrary shell/raw command evaluation, evidence/remediation mutation, fix execution, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.

### Task 3: GREEN Implementation

- [x] Implement the selected editor/operator UX or report surface over existing read-only models.
- [x] Keep command execution and evidence collection external to editor core.
- [x] Keep closeout represented by rerunning `EditorAiPlaytestEvidenceSummaryModel` and observing no remediation queue/handoff rows.

Implementation result: `EditorAiPlaytestOperatorWorkflowReportModel` exposes eight deterministic stage rows: package diagnostics, validation preflight, readiness, operator handoff, evidence summary, remediation queue, remediation handoff, and closeout. Each row reports the source model, source row count, source row ids, status, host gates, blockers, and diagnostic text. The report never executes commands or mutates manifests/evidence/remediation/fixes; it only rejects non-read-only inputs and unsupported claims as diagnostics.

### Task 4: Docs Manifest Validation

- [x] Update docs, manifest, and static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Validation Evidence

- 2026-05-02: RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: failed as expected because `EditorAiPlaytestOperatorWorkflowReportModel`, `EditorAiPlaytestOperatorWorkflowReportDesc`, `EditorAiPlaytestOperatorWorkflowStageStatus`, `make_editor_ai_playtest_operator_workflow_report_model`, and `editor_ai_playtest_operator_workflow_stage_status_label` did not exist yet.
- 2026-05-02: RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: failed as expected because manifest `editor-ai-playtest-operator-workflow` lacked `structuredReportSurface`.
- 2026-05-02: RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: failed as expected because manifest `editor-ai-playtest-operator-workflow` lacked `structuredReportSurface`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: PASS, CTest reported 28/28 tests passed after adding the structured report model and tests.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS, `public-api-boundary-check: ok`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS, `json-contract-check: ok`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS, `ai-integration-check: ok`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS, emitted `currentActivePlan=docs/superpowers/plans/2026-05-02-editor-ai-playtest-operator-workflow-ux-v1.md` and `recommendedNextPlan.id=next-production-gap-selection`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS, structured result reported `status=passed`, `agent-check` exit code 0, and `schema-check` exit code 0.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, `validate: ok`; diagnostic-only host gates still reported missing local Metal tools and Apple packaging host tools, with default CTest 28/28 passed.
