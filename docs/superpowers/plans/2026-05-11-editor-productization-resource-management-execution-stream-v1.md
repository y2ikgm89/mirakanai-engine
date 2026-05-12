# Editor Productization Resource Management Execution Stream v1 (2026-05-11)

**Plan ID:** `editor-productization-resource-management-execution-stream-v1`  
**Gap:** `editor-productization` / `renderer-rhi-resource-foundation` (shared diagnostics)  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Active program stream (planning ledger)  

## Goal

Move editor resource tooling from **diagnostics and handoff rows** toward **host-owned execution evidence** (for example PIX / debug-layer captures) while keeping `MK_core` and gameplay layers free of native handle exposure.

## Context

- Existing: `EditorResourcePanelModel`, capture **request** rows, optional capture execution evidence rows without launching tools automatically.
- Gap: `requiredBeforeReadyClaim` requests execution beyond reviewed acknowledgement surfaces.

## Constraints

- Microsoft host diagnostics policy from `AGENTS.md` / manifest `windowsDiagnosticsToolchain`—no bundling PIX or WPT into the repo.
- Unacknowledged automatic execution of host-gated recipes remains out of scope for AI command surfaces.

## Done when (stream exit)

- Documented operator loop from editor rows to **verified** external tool invocation with stdout/stderr or artifact paths recorded in retained rows (where permitted).
- Renderer/RHI registry diagnostics stay consistent with any new execution milestones.

## Next suggested child slices

1. **Completed (capture execution bounded row schema):** `EditorResourceCaptureExecutionInput::host_gates`, `EditorResourceCaptureExecutionRow::phase_code` / `host_gates` / `host_gates_label`, `bound_sanitized_field` caps for artifact and diagnostic text, retained `resources.capture_execution.<id>.phase` and `.host_gates` MK_ui labels, `MK_editor` execution table columns, and `MK_editor_core_tests` coverage (no PIX launch, no native handle exposure).
2. **Completed (PIX host helper script):** [2026-05-11-editor-resource-capture-pix-launch-helper-v1.md](2026-05-11-editor-resource-capture-pix-launch-helper-v1.md) — `tools/launch-pix-host-helper.ps1` creates `%LocalAppData%\MirakanaiEngine\pix-host-helper\` session dirs, optional `MIRAKANA_PIX_EXE`, `-SkipLaunch` for path smoke without starting PIX; no repo writes; no `MK_editor` integration.
3. **Completed (PIX host handoff evidence in MK_editor):** [2026-05-11-editor-resource-capture-pix-host-handoff-evidence-v1.md](2026-05-11-editor-resource-capture-pix-host-handoff-evidence-v1.md) — reviewed `Run helper (-SkipLaunch)` control, `Win32ProcessRunner` + `is_allowed_process_command` pwsh allowlist, retained `resources.capture_execution` stdout/session path summaries; cross-links `renderer-rhi-resource-foundation` teardown scope; default control does not start PIX UI.
4. **Completed (capture execution contract label row):** [2026-05-11-editor-resources-capture-execution-contract-label-v1.md](2026-05-11-editor-resources-capture-execution-contract-label-v1.md) — retained `resources.capture_execution.contract_label` with `ge.editor.resources_capture_execution.v1`, visible MK_editor line before the capture execution evidence table, `MK_editor_core_tests` serialized document check; does not execute capture tooling.
5. **Completed (operator-validated PIX launch workflow):** [2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md](2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md) — retained `resources.capture_execution.operator_validated_launch_workflow_contract_label` with `ge.editor.resources_capture_operator_validated_launch_workflow.v1`, visible MK_editor confirmation modal plus **Run helper (launch PIX)** reviewed five-arg `pwsh` invocation (no `-SkipLaunch`), diagnostics distinguishing skip-launch vs launch-enabled runs, `MK_editor_core_tests` / `MK_core_tests`; no automatic launch or AI command execution.
