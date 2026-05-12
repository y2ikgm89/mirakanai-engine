# Editor Productization Material Preview Display Parity Stream v1 (2026-05-11)

**Plan ID:** `editor-productization-material-preview-display-parity-stream-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Active program stream (planning ledger)  

## Goal

Close **Vulkan/Metal material-preview display parity** claims relative to the D3D12 host-owned execution path, respecting `hostGates` (`vulkan-strict`, `metal-apple`) and existing `material_asset_preview.gpu.execution.*` contract rows.

## Context

- Completed: Vulkan and Metal **visible refresh snapshot** evidence slices; display-path contract alignment; viewport Vulkan RHI bootstrap for editor.
- `requiredBeforeReadyClaim` still blocks full parity until reviewed cross-backend display equivalence is proven.

## Constraints

- No public gameplay API exposure of RHI/native handles; editor shell remains host-owned for GPU execution evidence.
- Parity evidence must distinguish **cpu-readback** vs **shared-texture** display paths without overstating ready claims on Windows-only hosts.

## Done when (stream exit)

- `material_asset_preview.gpu.execution` retained rows report pass/fail parity against a documented reference frame set per backend, with CI or documented host recipes where applicable.
- Manifest `editor-productization` notes and `check-ai-integration` needles stay aligned with new row ids.

## Next suggested child slices

1. ~~Parity checklist row bundle (contract ids, backend scope, pending vs complete).~~ Implemented: [2026-05-11-editor-material-preview-display-parity-checklist-row-bundle-v1.md](2026-05-11-editor-material-preview-display-parity-checklist-row-bundle-v1.md).
2. ~~Strict Vulkan editor recipe extension (toolchain-gated) mirroring D3D12 evidence depth.~~ Implemented: [2026-05-11-editor-material-preview-vulkan-strict-validation-recipe-v1.md](2026-05-11-editor-material-preview-vulkan-strict-validation-recipe-v1.md) (`desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict`).
3. Apple-host Metal presentation slice (explicit `metal-apple` gate) when macOS evidence is available; execute on a macOS host with Metal toolchain—Windows agents should complete the Windows resource handoff slice first, then resume this item when `metal-apple` validation is runnable (see `engine/agent/manifest.json` hostGates).

## Host-only execution cycle (`metal-apple`)

Treat **stream item 3** as a **separate dated plan + validation cycle** from Windows-default `editor-productization` work. Author the implementation plan only when the operator host satisfies `metal-apple` (`engine/agent/manifest.json` `aiOperableProductionLoop.hostGates`), run `tools/check-shader-toolchain.ps1` / Apple toolchain probes as required by the slice, attach host evidence (logs, screenshots, or recipe output paths) to the plan validation table, and update manifest `recommendedNextPlan` / gap notes in the same merge so `check-ai-integration` stays aligned. Windows-only agents should not claim this slice complete without that host evidence.
