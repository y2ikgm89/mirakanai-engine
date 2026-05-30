# Static contract chapter ownership

`tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` dot-source numeric chapter files in one shared PowerShell scope. Load reusable surface text only in the chapter that owns the assertions.

## check-ai-integration.ps1

| Chapter | File | Owns |
| --- | --- | --- |
| 1 | `tools/check-ai-integration-010-agent-baseline.ps1` | Agent baseline, docs, plans, and tooling needles |
| 2 | `tools/check-ai-integration-020-engine-manifest.ps1` | `Get-AgentSurfaceText` for engine/RHI/runtime/renderer contracts consumed by later rendering packs |
| 3+ | `tools/check-ai-integration-03x-*.ps1` and packs | Domain checks; do not preload chapter-2 surfaces in chapter 1 |

`tools/check-json-contracts-040-agent-surfaces.ps1` rejects chapter-1 preload of chapter-2 `Get-AgentSurfaceText` assignments.

## check-json-contracts.ps1

| Chapter | File | Owns |
| --- | --- | --- |
| 4 | `tools/check-json-contracts-040-agent-surfaces.ps1` | Agent surfaces, tooling, and cross-check needles |
| 5 | `tools/check-json-contracts-050-generated-games.ps1` | `Get-JsonContractSurfaceText` for runtime UI/tools C++ contracts |

Chapter 5 guards against chapter-4 preload of those UI/tools reads.

## When editing

- Add Needles in the owning chapter file.
- Use `Get-AgentSurfaceText` / `Get-JsonContractSurfaceText` (cached helpers), not duplicate `Get-Content` loads for the same path.
- Update `tools/static-contract-ledger.ps1` line budgets when a chapter grows.

See **Repository consistency checklist** step 3 in `docs/workflows.md`.
