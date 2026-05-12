# Editor Resource Capture PIX Host Handoff Evidence v1 (2026-05-11)

**Plan ID:** `editor-resource-capture-pix-host-handoff-evidence-v1`  
**Gap:** `editor-productization` (shared diagnostics with `renderer-rhi-resource-foundation`)  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-resource-management-execution-stream-v1.md](2026-05-11-editor-productization-resource-management-execution-stream-v1.md) (stream item 3)

## Goal

After the operator acknowledges the retained `pix_gpu_capture` capture request, `MK_editor` on Windows may run a **single reviewed** `pwsh` invocation of `tools/launch-pix-host-helper.ps1` with `-SkipLaunch`, capture stdout/stderr through `Win32ProcessRunner`, and surface **sanitized** session scratch path plus process output summary in existing `resources.capture_execution` / `EditorResourceCaptureExecutionInput` rows. This closes the stream ledger gap between “helper script exists” and “editor-visible verified host-helper execution evidence” without claiming GPU capture completion, automatic AI command execution, or native RHI handle exposure.

## Context

- [2026-05-11-editor-resource-capture-pix-launch-helper-v1.md](2026-05-11-editor-resource-capture-pix-launch-helper-v1.md) added `tools/launch-pix-host-helper.ps1` (no editor integration).
- [editor/core/src/resource_panel.cpp](editor/core/src/resource_panel.cpp) already materializes `resources.capture_execution.*` MK_ui rows with bounded artifact/diagnostic fields.
- `mirakana::run_process_command` and `Win32ProcessRunner` historically allowed only non-shell executables; reviewed `pwsh` allowlists must extend the shared gate alongside `is_safe_reviewed_validation_recipe_invocation`.

## Constraints

- Microsoft host diagnostics policy: do not bundle PIX; do not auto-run host tools from AI command surfaces without reviewed operator acknowledgement rows.
- Default editor action uses **`-SkipLaunch`** so CI and hosts without PIX can still observe deterministic process exit behavior when PIX is discoverable; full PIX UI launch remains an operator choice outside the default button.
- No new public gameplay API for native handles; artifact text stays host file paths under `%LocalAppData%` scratch roots only.
- Unacknowledged automatic execution of host-gated validation recipes remains out of scope.

## Cross-link: `renderer-rhi-resource-foundation`

GPU capture and RHI lifetime evidence must stay aligned with native backend teardown and registry diagnostics tracked under gap `renderer-rhi-resource-foundation` (see `engine/agent/manifest.json` `unsupportedProductionGaps` row and plans such as [2026-05-09-rhi-native-teardown-scoping-v1.md](2026-05-09-rhi-native-teardown-scoping-v1.md)). This slice records **host-helper** evidence only; it does not close RHI teardown migration.

## Done when

- Windows `MK_editor` Resources panel exposes an operator control on the PIX capture execution row that runs the reviewed helper with `-SkipLaunch` when the repo `tools/launch-pix-host-helper.ps1` is discoverable from the process current working directory walk, and refreshes retained execution diagnostics/artifact fields from process output (bounded).
- `mirakana::is_allowed_process_command` (or equivalent) permits `Win32ProcessRunner` + `run_process_command` to execute both `tools/run-validation-recipe.ps1` and `tools/launch-pix-host-helper.ps1` reviewed argv shapes.
- `MK_editor_core_tests` / resource panel contract coverage updated for any new MK_ui element ids.
- `engine/agent/manifest.fragments` + `tools/compose-agent-manifest.ps1 -Write`, plan registry stream item 3 pointer, and `docs/editor.md` operator notes updated.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the implementation host.

## Validation evidence

| Check | Result |
| --- | --- |
| `MK_editor_core_tests` | `resources.capture_execution.pix_gpu_capture.host_helper_hint` contract assertion |
| `MK_core_tests` | `process contract accepts reviewed pwsh pix host helper invocations` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Run on the implementation host at slice close |

## Non-goals

- Launching PIX UI by default from the editor button (operators may run the script manually with launch enabled).
- Clearing Vulkan/Metal material-preview `requiredBeforeReadyClaim` rows.
- Marking `editor-productization` gap ready or removing `resource management/capture execution` follow-ups beyond this reviewed helper evidence slice.
