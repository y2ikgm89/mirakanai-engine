# RHI Native Teardown Scoping v1 (2026-05-09)

**Plan ID:** `rhi-native-teardown-scoping-v1`  
**Parent wave:** [2026-05-09-phase-4-5-continuation-wave-1-v1.md](2026-05-09-phase-4-5-continuation-wave-1-v1.md)  
**Status:** Completed (documentation slice; no native destruction migration in this wave).

## Goal

Record **explicit scope boundaries** for the next per-backend native destruction migration under `renderer-rhi-resource-foundation`, avoiding accidental promises in Wave 1.

## Scoped next slices (recommended)

1. **Buffers/textures** created through `IRhiDevice` public create APIs — destroy native objects at registry retirement when refcount reaches zero (per-backend).
2. **Deferred-release rows** — already coordinated with fence completion; native teardown must remain ordered after last GPU use.
3. **Out of scope for slice 1:** descriptor heaps, swapchain-owned surfaces, editor viewport surfaces owned outside `mirakana_rhi`.

## Constraints

- No change to public `mirakana::rhi` handle ABI without a dedicated migration plan.
- Manifest gap row stays **implemented-foundation-only** until execution proofs exist.

## Done when

- This scoping note is linked from the continuation wave milestone and reviewed against [`renderer-rhi-resource-foundation`](../../../engine/agent/manifest.json) notes.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Docs/registry | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
