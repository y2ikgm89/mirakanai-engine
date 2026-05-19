# Production Completion v1 Split Index

This directory is the detailed corpus for [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md).

The parent master plan is now the stable, always-small table of contents. These chapter files hold the detailed production-completion evidence, official-practice guidance, and future feature tracks.

## How agents should load this plan

1. Read the parent master plan index first.
2. Read only the chapter that matches the current task.
3. Read `02-official-practice-gates.md` when making or reviewing a plan decision that depends on external SDK/tool/vendor guidance.
4. Read `01-one-dot-zero-readiness-ledger.md` when updating 1.0 status, `currentActivePlan`, `recommendedNextPlan`, or `unsupportedProductionGaps`.
5. Read `99-historical-verdict-archive.md` only when static-check history, retained verdict needles, or old closeout evidence are relevant.

## Chapter map

| File | Scope |
| --- | --- |
| [01-one-dot-zero-readiness-ledger.md](01-one-dot-zero-readiness-ledger.md) | Goal, execution prompt, 100% definition, context, constraints, gap burn-down, Current Verdict, and 1.0 scenario mapping. |
| [02-official-practice-gates.md](02-official-practice-gates.md) | Official Practice Review Gates and documentation authority stack. |
| [03-ai-autonomous-game-creation.md](03-ai-autonomous-game-creation.md) | Codex / Claude Code / Cursor game-creation flow, game-owned mutation boundary, and developer handoff behavior. |
| [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md) | Engine features the developer must add when game creators hit missing capability. |
| [05-2d-3d-capability-coverage-matrix.md](05-2d-3d-capability-coverage-matrix.md) | 2D and 3D foundation/advanced capability coverage. |
| [06-renderer-advanced-production-track.md](06-renderer-advanced-production-track.md) | Renderer high-end production capability track. |
| [07-gameplay-physics-nav-ai-advanced-track.md](07-gameplay-physics-nav-ai-advanced-track.md) | Physics, navigation, and AI gameplay production track. |
| [08-high-freedom-game-creation-track.md](08-high-freedom-game-creation-track.md) | General-purpose and high-freedom game creation capabilities. |
| [09-sprite-production-pipeline-track.md](09-sprite-production-pipeline-track.md) | 2D sprite authoring/import/runtime pipeline. |
| [10-gameplay-archetype-validation.md](10-gameplay-archetype-validation.md) | Gameplay archetype scenarios that prove reusable engine capability families. |
| [99-historical-verdict-archive.md](99-historical-verdict-archive.md) | Historical verdict archive and retained static-check evidence. |

## AI loading rules

- Load the parent index first.
- Load `01-one-dot-zero-readiness-ledger.md` only for 1.0 gap status.
- Load `04-developer-owned-engine-capability-backlog.md` only when a missing reusable engine capability is being promoted for developer work.
- Load `10-gameplay-archetype-validation.md` only to prove broad engine coverage; do not treat archetypes as genre-specific engine goals.
- Do not load `99-historical-verdict-archive.md` unless a static check, old evidence lookup, or migration audit requires it.
## Editing rules

- Keep the parent index concise; do not move full chapter content back into it.
- Keep official guidance in the owning chapter rather than duplicating it across chapters.
- Keep historical static-check needles in `99-historical-verdict-archive.md` unless a check is intentionally retired with validation evidence.
- Update `tools/check-ai-integration*.ps1` and `tools/check-json-contracts*.ps1` when plan discovery rules change.
