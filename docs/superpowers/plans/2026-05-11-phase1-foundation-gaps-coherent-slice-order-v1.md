# Phase 1 Foundation Gaps Coherent Slice Order v1 (2026-05-11)

**Plan ID:** `phase1-foundation-gaps-coherent-slice-order-v1`  
**Gaps:** `asset-identity-v2`, `runtime-resource-v2`, `renderer-rhi-resource-foundation`, `upload-staging-v1`, `frame-graph-v1`, `scene-component-prefab-schema-v2`  
**Parent:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Active program ledger  

## Goal

Document the **recommended dependency order** for Phase 1 foundation elevation so coherent slices can touch multiple modules without violating manifest **single-gap burn-down** execution (each dated child plan still names one primary `unsupportedProductionGaps` row unless an orchestration-approved milestone bundles explicitly).

## Recommended order (shallow → deep)

1. **`asset-identity-v2`** — closed by the 2026-05-12 reference-cleanup milestone.
2. **`runtime-resource-v2`** — closed by the 2026-05-16 Engine 1.0 runtime-resource scope closeout.
3. **`renderer-rhi-resource-foundation`** — closed by the 2026-05-16 Windows-default renderer/RHI foundation scope closeout; Metal parity, allocator enforcement, frame graph, and upload/staging package integration remain separate rows or host gates.
4. **`frame-graph-v1`** — next active foundation gap; production pass ownership and transient policy assume dependable upload completion and resource retirement ordering.
5. **`upload-staging-v1`** — GPU upload rings and staging pools integrate with RHI lifetime and runtime scene GPU binding reports.
6. **`scene-component-prefab-schema-v2`** — production scene edit + dependency cook pulls asset graph, runtime packages, and renderer-bound scene payloads together.

## Coherent slice patterns (examples)

- **Frame graph + RHI:** `frame-graph-v1` primary with explicit `IRhiCommandList` transition ownership; reuse closed `renderer-rhi-resource-foundation` registry/teardown evidence instead of reopening that gap.
- **Upload + runtime bind:** `upload-staging-v1` primary, with `runtime-resource-v2` and `renderer-rhi-resource-foundation` treated as closed boundary evidence when tests share one package fixture.

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
| `runtime-resource-v2` | [2026-05-15-runtime-package-index-discovery-v1.md](2026-05-15-runtime-package-index-discovery-v1.md) | Completed; reviewed `.geindex` candidate discovery without package load/mount mutation |
| `runtime-resource-v2` | [2026-05-15-runtime-package-candidate-load-v1.md](2026-05-15-runtime-package-candidate-load-v1.md) | Completed; reviewed selected `.geindex` candidate load into a typed package load result without mount/cache/streaming mutation |
| `runtime-resource-v2` | [2026-05-15-runtime-package-candidate-resident-mount-v1.md](2026-05-15-runtime-package-candidate-resident-mount-v1.md) | Completed; reviewed selected `.geindex` candidate load plus explicit resident mount/cache commit without package-streaming descriptors, hot reload, renderer/RHI, upload/staging, or native handles |
| `runtime-resource-v2` | [2026-05-15-runtime-package-candidate-resident-replace-v1.md](2026-05-15-runtime-package-candidate-resident-replace-v1.md) | Completed; reviewed selected `.geindex` candidate load plus explicit existing resident mount replacement/cache commit without package-streaming descriptors, hot reload, renderer/RHI, upload/staging, or native handles |
| `runtime-resource-v2` | [2026-05-16-runtime-hot-reload-recook-package-replacement-execution-v1.md](2026-05-16-runtime-hot-reload-recook-package-replacement-execution-v1.md) | Completed; reviewed hot-reload recook replacement safe-point execution |
| `runtime-resource-v2` | [2026-05-16-runtime-hot-reload-registered-asset-watch-tick-v1.md](2026-05-16-runtime-hot-reload-registered-asset-watch-tick-v1.md) | Completed; registered asset watch-tick orchestration without native watcher ownership |
| `runtime-resource-v2` | [2026-05-16-runtime-resource-v2-1-0-scope-closeout-v1.md](2026-05-16-runtime-resource-v2-1-0-scope-closeout-v1.md) | Completed; Engine 1.0 runtime-resource ready surface closed, with renderer/RHI, upload/staging, frame graph, broad streaming, native watcher ownership, native handles, and broad hot reload productization left to separate rows or non-goals |
| `renderer-rhi-resource-foundation` | [2026-05-16-renderer-rhi-resource-foundation-1-0-scope-closeout-v1.md](2026-05-16-renderer-rhi-resource-foundation-1-0-scope-closeout-v1.md) | Completed; Engine 1.0 Windows-default renderer/RHI foundation surface closed, with frame graph, upload/staging package integration, Metal parity, allocator enforcement, package streaming, native handles, and broad renderer quality left to separate rows or non-goals |
