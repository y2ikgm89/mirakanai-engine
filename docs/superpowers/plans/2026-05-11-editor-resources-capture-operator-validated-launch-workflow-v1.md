# Editor Resources Capture Operator Validated Launch Workflow v1 (2026-05-11)

**Plan ID:** `editor-resources-capture-operator-validated-launch-workflow-v1`  
**Gap:** `editor-productization` (resource management execution stream)  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-resource-management-execution-stream-v1.md](2026-05-11-editor-productization-resource-management-execution-stream-v1.md)  
**Gap index:** [2026-05-11-production-completion-gap-stream-plans-index-v1.md](2026-05-11-production-completion-gap-stream-plans-index-v1.md)

## Goal

Close the resource execution stream toward **operator-validated external tool invocation**: after acknowledging the PIX capture request, Windows `MK_editor` offers a **second reviewed control** that runs `tools/launch-pix-host-helper.ps1` **without** `-SkipLaunch` only behind an explicit confirmation modal, records stdout/stderr and session scratch paths in existing `resources.capture_execution` evidence rows, and exposes a retained contract label for agent drift checks—without automatic launch, AI-driven execution, or native RHI handle exposure.

## Context

- Items 1–4 of the resource management execution stream are complete (bounded rows, PIX helper script, `-SkipLaunch` handoff, contract label).
- `mirakana::is_safe_reviewed_pix_host_helper_invocation` already allows exactly five `pwsh` arguments (no `-SkipLaunch`) alongside the six-argument `-SkipLaunch` shape (`engine/platform/src/process.cpp`).
- Stream exit still requires a documented operator loop to **verified** external invocation; the default path remains `-SkipLaunch` for CI and hosts without PIX.

## Constraints

- Microsoft host diagnostics policy: PIX is not bundled; no default automatic PIX start from the first button.
- Confirmation modal text must state that the Microsoft PIX UI process may start; operator must confirm.
- `Win32ProcessRunner` / `run_process_command` only; argv must remain allowlisted.
- No new gameplay or RHI public handle surfaces.

## Done when

- Retained `resources.capture_execution.operator_validated_launch_workflow_contract_label` with `ge.editor.resources_capture_operator_validated_launch_workflow.v1` appears in `make_resource_panel_ui_model` when capture execution rows exist, before per-row items (alongside the existing capture execution contract label).
- Windows Resources **Capture Execution Evidence** table exposes **Run helper (launch PIX)** (or equivalent label) with modal confirm; on confirm runs reviewed `pwsh` with five-arg `launch-pix-host-helper.ps1` (no `-SkipLaunch`).
- Execution diagnostics in the PIX row distinguish skip-launch vs launch-enabled helper runs when `pix_host_helper_last_run_valid_` is true.
- `MK_core_tests` asserts the five-arg PIX helper command is allowed.
- `MK_editor_core_tests` asserts the new retained element id and contract string.
- `tools/check-ai-integration.ps1` needles updated for new symbols and ids.
- `docs/editor.md` Resources section documents both controls and the launch confirmation boundary.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` `editor-productization` notes reference this slice; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` run after fragment edits.

## Validation evidence

| Step | Command / artifact | Expected |
| --- | --- | --- |
| Core process contract | `ctest --preset dev -R MK_core_tests --output-on-failure` | Pass (`process contract accepts reviewed pwsh pix host helper invocations` covers 5-arg) |
| Editor core | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass |
| Compose manifest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass on implementation host |

## Checklist

- [x] Plan + gap index + stream ledger pointers
- [x] Retained MK_ui contract label row + `resource_panel.hpp` constexpr
- [x] `MK_editor` modal + `execute_pix_host_helper_reviewed(bool skip_launch)`
- [x] Tests + `check-ai-integration` needles
- [x] `docs/editor.md` + manifest fragment notes + compose

## Next concrete follow-up (optional)

- Extend operator workflow docs with screenshots or a short **Recommended workflow** subsection tying PIX session dir → attach capture for AI analysis (`docs/ai-integration.md` cross-link only if needed).
- Stream exit: reconcile `requiredBeforeReadyClaim` wording after broader operator evidence is recorded (separate manifest slice if narrowing claims).
