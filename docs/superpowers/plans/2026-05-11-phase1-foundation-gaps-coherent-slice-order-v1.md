# Phase 1 Foundation Gaps Coherent Slice Order v1 (2026-05-11)

**Plan ID:** `phase1-foundation-gaps-coherent-slice-order-v1`  
**Gaps:** `asset-identity-v2`, `runtime-resource-v2`, `renderer-rhi-resource-foundation`, `upload-staging-v1`, `frame-graph-v1`, `scene-component-prefab-schema-v2`  
**Parent:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Active program ledger  

## Goal

Document the **recommended dependency order** for Phase 1 foundation elevation so coherent slices can touch multiple modules without violating manifest **single-gap burn-down** execution (each dated child plan still names one primary `unsupportedProductionGaps` row unless an orchestration-approved milestone bundles explicitly).

## Recommended order (shallow → deep)

1. **`asset-identity-v2`** — canonical keys and editor/tooling surfaces must agree before broad reference rewrite and cook planning.
2. **`runtime-resource-v2`** — mount/streaming semantics and catalogs depend on stable asset identity for preload hints and failure diagnostics.
3. **`renderer-rhi-resource-foundation`** — native lifetime/teardown must align with catalog replacement and safe-point boundaries.
4. **`upload-staging-v1`** — GPU upload rings and staging pools integrate with RHI lifetime and runtime scene GPU binding reports.
5. **`frame-graph-v1`** — production pass ownership and transient policy assume dependable upload completion and resource retirement ordering.
6. **`scene-component-prefab-schema-v2`** — production scene edit + dependency cook pulls asset graph, runtime packages, and renderer-bound scene payloads together.

## Coherent slice patterns (examples)

- **Upload + runtime bind:** `upload-staging-v1` primary, `runtime-resource-v2` + `renderer-rhi-resource-foundation` secondary notes in the same child plan when tests share one package fixture.
- **Frame graph + RHI:** `frame-graph-v1` primary with explicit `IRhiCommandList` transition ownership; `renderer-rhi-resource-foundation` for registry retirement evidence.

## Constraints

- Do not remove `unsupportedProductionGaps` rows from `engine/agent/manifest.json` until the master-plan closeout criteria are met.
- Each implementation slice still requires Goal/Context/Constraints/Done When/validation table in English under `docs/superpowers/plans/`.

## Done when (ledger refresh)

- Each gap row receives at least one **forward-pointing** dated child plan reference from this file’s “Linked slices” table (maintainers append rows as slices land).

## Linked slices

| Gap id | Plan | Status |
| --- | --- | --- |
| `asset-identity-v2` | [2026-05-12-asset-identity-v2-placement-resolution-v1.md](2026-05-12-asset-identity-v2-placement-resolution-v1.md) | Completed; production asset placement resolution |
| `asset-identity-v2` | [2026-05-12-asset-identity-v2-command-apply-surface-evidence-v1.md](2026-05-12-asset-identity-v2-command-apply-surface-evidence-v1.md) | Completed; reviewed command-owned apply-surface evidence closeout |
| `asset-identity-v2` | [2026-05-12-asset-identity-v2-scene-runtime-reference-placement-evidence-v1.md](2026-05-12-asset-identity-v2-scene-runtime-reference-placement-evidence-v1.md) | Completed; Scene v2 runtime package migration placement evidence |
| `asset-identity-v2` | [2026-05-12-editor-content-browser-asset-identity-v2-rows-v1.md](2026-05-12-editor-content-browser-asset-identity-v2-rows-v1.md) | Completed; read-only Content Browser identity rows |
| `asset-identity-v2` | [2026-05-12-editor-content-browser-source-registry-population-v1.md](2026-05-12-editor-content-browser-source-registry-population-v1.md) | Completed; source-registry-backed Content Browser population |
