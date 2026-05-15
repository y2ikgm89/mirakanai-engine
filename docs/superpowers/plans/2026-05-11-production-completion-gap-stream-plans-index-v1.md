# Production Completion Gap Stream Plans Index 2026-05-11 v1 (2026-05-11)

**Plan ID:** `production-completion-gap-stream-plans-index-2026-05-11-v1`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Orchestration:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Reference index  

## Purpose

Single navigation index for **program-scale stream plans** created to operationalize the Production gaps forward program without replacing per-slice dated implementation plans.

## Next execution pointer

For `editor-productization` with `recommendedNextPlan.id=next-production-gap-selection`, the Windows-tractable 1.0 scope is closed and reclassified by [Editor Productization 1.0 Scope Closeout v1](2026-05-12-editor-productization-1-0-scope-closeout-v1.md). The **resource management execution** stream is closed (including [operator-validated PIX launch workflow](2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md)), and **nested prefab reviewed propagation coverage** is implemented through multi-target late loader drift atomicity in [2026-05-12-editor-scene-nested-prefab-propagation-multi-target-late-loader-drift-atomicity-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-multi-target-late-loader-drift-atomicity-v1.md). The only remaining `editor-productization` required-before-ready claim is Vulkan/Metal material-preview display parity beyond D3D12 host-owned evidence; Metal remains `metal-apple` host-gated. The Windows-default queue has moved past the completed `asset-identity-v2`, `runtime-resource-v2`, and `renderer-rhi-resource-foundation` burn-downs; the next active foundation gap is `frame-graph-v1`.

## Phase 0 — `editor-productization` streams

| Stream | Plan |
| --- | --- |
| Nested prefab propagation / merge UX | [2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md](2026-05-11-editor-productization-nested-prefab-propagation-stream-v1.md) |
| Material preview display parity | [2026-05-11-editor-productization-material-preview-display-parity-stream-v1.md](2026-05-11-editor-productization-material-preview-display-parity-stream-v1.md) |
| Resource management execution | [2026-05-11-editor-productization-resource-management-execution-stream-v1.md](2026-05-11-editor-productization-resource-management-execution-stream-v1.md) |
| Hot reload + stable ABI | [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md) |
| 1.0 scope closeout | [2026-05-12-editor-productization-1-0-scope-closeout-v1.md](2026-05-12-editor-productization-1-0-scope-closeout-v1.md) |

### Hot reload + stable ABI — child implementation plans

| Slice | Plan |
| --- | --- |
| Stable third-party ABI excluded from Engine 1.0 (policy + docs) | [2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md](2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md) |
| Reload transaction validation recipe (`run-validation-recipe`) | [2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md](2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md) |
| MK_editor visible reload transaction recipe argv handoff | [2026-05-11-editor-game-module-driver-mk-editor-visible-reload-transaction-recipe-evidence-v1.md](2026-05-11-editor-game-module-driver-mk-editor-visible-reload-transaction-recipe-evidence-v1.md) |

### Material preview display parity — child implementation plans

| Slice | Plan |
| --- | --- |
| Parity checklist row bundle | [2026-05-11-editor-material-preview-display-parity-checklist-row-bundle-v1.md](2026-05-11-editor-material-preview-display-parity-checklist-row-bundle-v1.md) |
| Vulkan strict validation recipe (material/shader scaffold package) | [2026-05-11-editor-material-preview-vulkan-strict-validation-recipe-v1.md](2026-05-11-editor-material-preview-vulkan-strict-validation-recipe-v1.md) |

### Nested prefab propagation — child implementation plans

| Slice | Plan |
| --- | --- |
| Propagation plan preview rows | [2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md) |
| Reviewed chained propagation apply | [2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md) |
| Nested propagation apply undo/redo test | [2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md) |
| Single-target batch propagation apply undo/redo test | [2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md) |
| Multi-target batch propagation apply + undo/redo | [2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md) |
| Triple-disjoint batch propagation apply + undo/redo | [2026-05-12-editor-scene-nested-prefab-propagation-batch-triple-disjoint-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-triple-disjoint-apply-v1.md) |
| Blocked batch policy label | [2026-05-12-editor-scene-nested-prefab-propagation-batch-blocked-policy-label-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-blocked-policy-label-v1.md) |
| Fail-closed loader edge coverage | [2026-05-12-editor-scene-nested-prefab-propagation-fail-closed-edge-coverage-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-fail-closed-edge-coverage-v1.md) |
| Two-level batch propagation | [2026-05-12-editor-scene-nested-prefab-propagation-two-level-batch-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-two-level-batch-v1.md) |
| Selected nested source-node remap | [2026-05-12-editor-scene-nested-prefab-propagation-selected-added-node-remap-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-selected-added-node-remap-v1.md) |
| Local-child policy during nested propagation | [2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md) |
| Loader drift atomicity | [2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md) |
| Local/stale variant alignment (Prefab Variant `resolution_kind` on local + stale rows) | [2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md) |

### Resource management execution — child implementation plans

| Slice | Plan |
| --- | --- |
| Capture execution contract label | [2026-05-11-editor-resources-capture-execution-contract-label-v1.md](2026-05-11-editor-resources-capture-execution-contract-label-v1.md) |
| Operator-validated PIX launch workflow | [2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md](2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md) |

## Recent implemented slice (Phase 0)

| Slice | Plan |
| --- | --- |
| Editor scene nested prefab propagation multi-target late loader drift atomicity | [2026-05-12-editor-scene-nested-prefab-propagation-multi-target-late-loader-drift-atomicity-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-multi-target-late-loader-drift-atomicity-v1.md) |
| Editor scene nested prefab propagation loader drift atomicity | [2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-loader-drift-atomicity-v1.md) |
| Editor scene nested prefab propagation local-child policy | [2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-local-child-policy-v1.md) |
| Editor scene nested prefab propagation selected source-node remap | [2026-05-12-editor-scene-nested-prefab-propagation-selected-added-node-remap-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-selected-added-node-remap-v1.md) |
| Editor scene nested prefab propagation two-level batch | [2026-05-12-editor-scene-nested-prefab-propagation-two-level-batch-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-two-level-batch-v1.md) |
| Editor scene nested prefab propagation fail-closed loader edge coverage | [2026-05-12-editor-scene-nested-prefab-propagation-fail-closed-edge-coverage-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-fail-closed-edge-coverage-v1.md) |
| Editor scene nested prefab propagation blocked batch policy label | [2026-05-12-editor-scene-nested-prefab-propagation-batch-blocked-policy-label-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-blocked-policy-label-v1.md) |
| Editor scene nested prefab propagation batch triple-disjoint apply | [2026-05-12-editor-scene-nested-prefab-propagation-batch-triple-disjoint-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-triple-disjoint-apply-v1.md) |
| Editor resources capture operator-validated PIX launch workflow | [2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md](2026-05-11-editor-resources-capture-operator-validated-launch-workflow-v1.md) |
| Editor scene nested prefab propagation batch multi-target apply | [2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md](2026-05-12-editor-scene-nested-prefab-propagation-batch-multi-target-apply-v1.md) |
| Editor scene nested prefab propagation batch undo/redo test | [2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-batch-undo-redo-test-v1.md) |
| Editor scene nested prefab propagation undo/redo test | [2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-undo-redo-test-v1.md) |
| Editor game module driver MK editor visible reload transaction recipe evidence | [2026-05-11-editor-game-module-driver-mk-editor-visible-reload-transaction-recipe-evidence-v1.md](2026-05-11-editor-game-module-driver-mk-editor-visible-reload-transaction-recipe-evidence-v1.md) |
| Editor game module driver stable third-party ABI 1.0 exclusion | [2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md](2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md) |
| Editor game module driver reload transaction validation recipe | [2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md](2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md) |
| Editor resources capture execution contract label | [2026-05-11-editor-resources-capture-execution-contract-label-v1.md](2026-05-11-editor-resources-capture-execution-contract-label-v1.md) |
| Editor game module driver reload transaction load tests | [2026-05-11-editor-game-module-driver-reload-transaction-load-tests-v1.md](2026-05-11-editor-game-module-driver-reload-transaction-load-tests-v1.md) |
| PIX host helper (`tools/launch-pix-host-helper.ps1`) | [2026-05-11-editor-resource-capture-pix-launch-helper-v1.md](2026-05-11-editor-resource-capture-pix-launch-helper-v1.md) |
| Resource capture execution bounded row schema | [2026-05-11-editor-productization-resource-management-execution-stream-v1.md](2026-05-11-editor-productization-resource-management-execution-stream-v1.md) (stream item 1) |
| Vulkan strict material/shader scaffold validation recipe | [2026-05-11-editor-material-preview-vulkan-strict-validation-recipe-v1.md](2026-05-11-editor-material-preview-vulkan-strict-validation-recipe-v1.md) |
| Local/stale prefab refresh variant alignment | [2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-local-stale-variant-alignment-v1.md) |
| Nested prefab propagation plan preview rows | [2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-plan-preview-v1.md) |
| Nested prefab propagation reviewed apply | [2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md](2026-05-11-editor-scene-nested-prefab-propagation-apply-v1.md) |
| Material preview display parity checklist row bundle | [2026-05-11-editor-material-preview-display-parity-checklist-row-bundle-v1.md](2026-05-11-editor-material-preview-display-parity-checklist-row-bundle-v1.md) |
| Nested prefab nested-variant alignment (Prefab Variant resolution_kind tooling) | [2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md](2026-05-11-editor-scene-prefab-nested-variant-alignment-v1.md) |
| Nested prefab propagation dry-run counters | [2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md](2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md) |
| Nested prefab summary counters | [2026-05-11-editor-scene-prefab-instance-refresh-nested-summary-counters-v1.md](2026-05-11-editor-scene-prefab-instance-refresh-nested-summary-counters-v1.md) |

## Phase 1 — Foundation gap order

| Plan |
| --- |
| [2026-05-11-phase1-foundation-gaps-coherent-slice-order-v1.md](2026-05-11-phase1-foundation-gaps-coherent-slice-order-v1.md) |

## Phase 2 — Vertical slices

| Plan |
| --- |
| [2026-05-11-phase2-2d-3d-vertical-slice-package-evidence-program-v1.md](2026-05-11-phase2-2d-3d-vertical-slice-package-evidence-program-v1.md) |

## Phase 3 — UI / importer / legal template

| Plan |
| --- |
| [2026-05-11-phase3-production-ui-importer-adapter-vcpkg-legal-template-v1.md](2026-05-11-phase3-production-ui-importer-adapter-vcpkg-legal-template-v1.md) |

## Phase 4 — Quality gate

| Plan |
| --- |
| [2026-05-11-phase4-full-repository-quality-gate-ci-analyzer-expansion-v1.md](2026-05-11-phase4-full-repository-quality-gate-ci-analyzer-expansion-v1.md) |
