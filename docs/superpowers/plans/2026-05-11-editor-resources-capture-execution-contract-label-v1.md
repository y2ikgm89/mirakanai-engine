# Editor Resources Capture Execution Contract Label v1 (2026-05-11)

**Plan ID:** `editor-resources-capture-execution-contract-label-v1`  
**Gap:** `editor-productization` (resource management execution stream follow-up after items 1–3)  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-resource-management-execution-stream-v1.md](2026-05-11-editor-productization-resource-management-execution-stream-v1.md)

## Goal

Add a **stable retained MK_ui contract root** for the Resources capture execution evidence block so agent integration checks and serialized UI documents expose an explicit `ge.editor.*` label alongside existing per-row capture execution ids.

## Context

Capture execution rows, phase codes, PIX host-helper wiring, and `MK_editor_core_tests` already exist. The stream ledger completed bounded rows, helper script, and MK_editor handoff evidence; this slice adds a **single contract label row** under `resources.capture_execution` without launching external tools or widening ready claims.

## Constraints

- No editor-core execution of PIX, debug-layer toggles, or GPU capture.
- No new native handle surfaces; contract text remains ASCII and stable.
- Keep Dear ImGui Resources panel aligned with the retained MK_ui document for the same contract string.

## Done when

- Retained element id `resources.capture_execution.contract_label` exists in `make_resource_panel_ui_model` output with label `ge.editor.resources_capture_execution.v1`.
- `MK_editor` shows the same contract string before the capture execution evidence table when rows are present.
- `MK_editor_core_tests` asserts the serialized document contains the contract label.
- `tools/check-ai-integration.ps1` needles for resource capture execution include the new contract id where appropriate.
- `docs/editor.md` and capability summaries mention the contract id next to existing capture execution documentation.

## Validation evidence

| Step | Command / artifact | Result |
| --- | --- | --- |
| Compose manifest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass (fragment `010-aiOperableProductionLoop.json` updated + composed) |
| Focused tests | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass |
| Repo gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass (includes `tools/build.ps1` + full `ctest --preset dev`) |
