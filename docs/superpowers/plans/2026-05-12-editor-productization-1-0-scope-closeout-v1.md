# Editor Productization 1.0 Scope Closeout v1 (2026-05-12)

**Plan ID:** `editor-productization-1-0-scope-closeout-v1`  
**Status:** Completed  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `editor-productization`

## Goal

Reclassify the remaining `editor-productization` 1.0 scope after the recent Windows-tractable streams: implemented reviewed editor contracts stay recorded, Metal material-preview parity stays host-gated, and broad Unity/UE parity, mid-play DLL replacement, automatic fuzzy nested prefab merge/rebase, automatic capture execution, and unacknowledged host-gated AI execution stay explicit 1.0 exclusions.

## Context

- Hot reload plus stable ABI stream is complete for same-engine-build stopped-state reload evidence and stable third-party ABI exclusion.
- Resource management execution stream is complete through operator-validated PIX helper launch evidence, without automatic capture or native handle claims.
- Nested prefab reviewed propagation coverage now includes multi-target apply, deeper chains, fail-closed loader behavior, policy propagation, atomicity, and undo/redo evidence, without fuzzy merge/rebase UX.
- Material preview Metal display parity still requires `metal-apple` host evidence and cannot be closed from this Windows host.

## Constraints

- Do not broaden ready claims beyond reviewed rows and validation evidence.
- Keep `editor-productization` in `unsupportedProductionGaps` while Metal material-preview parity remains host-gated.
- Move Windows-default next selection to the Phase 1 `asset-identity-v2` order only after manifest, master plan, registry, and static checks agree.
- Do not change editor runtime behavior in this slice.

## Done When

- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` keeps only the material-preview parity host-gated claim under `editor-productization.requiredBeforeReadyClaim`.
- The manifest notes explicitly record the implemented streams and 1.0 exclusions.
- The next Windows-default gap selection points to `asset-identity-v2` per [2026-05-11-phase1-foundation-gaps-coherent-slice-order-v1.md](2026-05-11-phase1-foundation-gaps-coherent-slice-order-v1.md).
- Master plan, registry, roadmap, and static checks agree with that scope.
- Validation evidence is recorded for compose, agent/static checks, and full repository validation.

## Validation Evidence

- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (47/47 tests passed; diagnostic-only blockers remain Metal/Apple host gates on this Windows host).

## Next Candidate After Validation

- Start the Phase 1 foundation order with `asset-identity-v2`.
