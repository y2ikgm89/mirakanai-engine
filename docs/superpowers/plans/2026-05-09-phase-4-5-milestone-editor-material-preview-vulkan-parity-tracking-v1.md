# Phase 4–5 Milestone — Editor Material Preview Vulkan / Metal Parity Tracking v1 (2026-05-09)

**Plan ID:** `phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1`  
**Parent milestone:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)

## Goal

Satisfy **Phase 4 milestone completion definition §3** (editor material preview parity **tracking**): document Vulkan (and Metal where applicable) display parity expectations against the existing **D3D12 host-owned** material preview path, without claiming parity on hosts that have not executed visible validation.

## Context

- Existing evidence plans: [2026-05-07-editor-material-asset-preview-diagnostics-v1.md](2026-05-07-editor-material-asset-preview-diagnostics-v1.md), [2026-05-07-editor-material-gpu-preview-execution-evidence-v1.md](2026-05-07-editor-material-gpu-preview-execution-evidence-v1.md).
- `docs/current-capabilities.md` states preview execution stays in `mirakana_editor`; Vulkan display parity and Metal readiness remain **outside** editor core until host proof exists.
- `engine/agent/manifest.json` `unsupportedProductionGaps.editor-productization` notes remain authoritative for breadth limits.

## Tracking checklist (operator / host-gated)

| Lane | Intent |
| --- | --- |
| D3D12 Windows | Baseline: existing diagnostics + GPU preview execution snapshot rows on cooked materials. |
| Vulkan Windows | Run the same visible Assets material preview workflow on a Vulkan-capable host; compare viewport presentation vs D3D12 for the same cooked material selection (failures must surface through existing preview diagnostics models, not silent fallback). |
| Metal macOS | Host-gated: full Xcode + Metal toolchain; same comparative intent as Vulkan where product policy requires Metal proof. |

## Constraints

- Do not promote `editor-productization` gap status without matching validation logs on the target host class.
- Do not expose native/RHI handles through editor core public APIs.

## Done when

- This tracking plan is registered and referenced from the milestone closure evidence index.
- No documentation row asserts Vulkan or Metal material-preview **ready** without citing executed host validation.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Docs/registry | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
