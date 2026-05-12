# Editor Game Module Driver Stable Third-Party ABI 1.0 Exclusion v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1`  
**Gap:** `editor-productization` (hot reload + stable ABI stream — stable ABI track exit via exclusion)  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md)

## Goal

Record an explicit **1.0 product decision**: the engine does **not** ship or support a **stable third-party binary ABI** for out-of-tree editor game module DLLs. Same-repository, **same-engine-build** dynamic loading of `GameEngine.EditorGameModuleDriver.v1` remains the reviewed interchange surface.

## Context

[2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md) requires the stable ABI track to close with either an external SDK contract **or** a **manifest-backed exclusion with rationale**. Implementation slices already document same-build metadata and stopped-state reload transactions; this slice closes the **exclusion** path for 1.0.

## Constraints

- Do not imply third-party SDK stability, LTS C exports, or cross-minor binary compatibility for editor DLLs.
- Do not remove honest `requiredBeforeReadyClaim` wording for **hot reload** or broader editor productization from the manifest gap row; this slice addresses only the **stable third-party ABI** branch of the stream exit criteria.
- ASCII for code paths and public identifiers in tracked docs per repository policy.

## Done when

- [`docs/legal-and-licensing.md`](../../legal-and-licensing.md) contains a dedicated subsection stating the 1.0 exclusion and engineering rationale for editor game module DLLs.
- [`docs/dependencies.md`](../../dependencies.md) cross-references that boundary (no new vcpkg entries required for this policy-only slice).
- Stream ledger [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md) records this slice as completing the **stable ABI track** exclusion path.
- [2026-05-11-production-completion-gap-stream-plans-index-v1.md](2026-05-11-production-completion-gap-stream-plans-index-v1.md) lists this plan under the hot reload + stable ABI stream.
- [`docs/superpowers/plans/README.md`](README.md) active-work notes mention the completed exclusion slice where hot-reload stream progress is summarized.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` `recommendedNextPlan` / gap narrative reflects the exclusion (no change to `unsupportedProductionGaps[].id` list required for a documentation-only 1.0 boundary).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass.

## Validation evidence

| Step | Command / artifact | Result |
| --- | --- | --- |
| Compose manifest | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass |
| Repo gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass |
