# Phase 4–5 Continuation Wave 1 v1 (2026-05-09)

**Plan ID:** `phase-4-5-continuation-wave-1-v1`  
**Status:** Completed (child plans dated 2026-05-09; see table below).  
**Parent roadmap:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Prior milestone (closed):** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md) — do **not** reopen or extend its ledger.

## Goal

Continue Phase 4–5 domain work using the **Child plan queue** from the closed closure milestone ([recommended order table](2026-05-09-phase-4-5-closure-milestone-v1.md)), without bulk-promoting `engine/agent/manifest.json` `unsupportedProductionGaps` rows without validation evidence. Greenfield / breaking API changes are allowed per `AGENTS.md`; manifest honesty is not optional.

## Wave 1 scope

This wave completes **at least one dated child plan per priority band P0–P3** (implementation + `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`), or records **explicit host-gated blockers** in the child plan validation table without upgrading gap `status`.

| Priority | Area | Primary gap ids | Wave 1 child plan |
| --- | --- | --- | --- |
| P0 | Editor material preview parity evidence | `editor-productization` | [2026-05-09-editor-material-preview-parity-evidence-contract-v1.md](2026-05-09-editor-material-preview-parity-evidence-contract-v1.md) |
| P1 | Frame Graph narrow | `frame-graph-v1` | [2026-05-09-frame-graph-v1-schedule-pass-invoke-order-contract-v1.md](2026-05-09-frame-graph-v1-schedule-pass-invoke-order-contract-v1.md) |
| P1 | RHI native teardown (scoped slice) | `renderer-rhi-resource-foundation` | [2026-05-09-rhi-native-teardown-scoping-v1.md](2026-05-09-rhi-native-teardown-scoping-v1.md) |
| P2 | Upload / streaming boundary | `upload-staging-v1`, `runtime-resource-v2` | [2026-05-09-upload-streaming-boundary-next-v1.md](2026-05-09-upload-streaming-boundary-next-v1.md) |
| P2 | Registered cook + importer | `asset-identity-v2`, `runtime-resource-v2` | [2026-05-09-registered-cook-importer-regression-next-v1.md](2026-05-09-registered-cook-importer-regression-next-v1.md) |
| P3 | UI / importer adapters | `production-ui-importer-platform-adapters` | [2026-05-09-production-ui-importer-adapters-step-v1.md](2026-05-09-production-ui-importer-adapters-step-v1.md) |

Further waves (Wave 2+) should use **new dated milestone files** rather than appending unrelated rows here.

## Constraints

- Do not claim Vulkan/Metal **display parity** without host-run evidence; tracking checklists remain authoritative where execution is gated.
- Dependencies / legal / vcpkg changes follow `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, `THIRD_PARTY_NOTICES.md`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`.
- `docs/current-capabilities.md` must not gain over-broad “ready” language.

## Out of scope (wave-wide)

- Full renderer-wide Frame Graph migration, production upload queues, Unity/UE-class editor UX, Metal shipping parity proofs without macOS hosts.
- Replacing `aiOperableProductionLoop.currentActivePlan` unless maintainers intentionally pivot the active slice to this wave.

## Validation (wave-wide)

| Step | Command | Expected |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK after each merged child slice |

## Done when

- Each row in **Wave 1 scope** has a **Completed** child plan (or documented blocker + unchanged gap promotion rules).
- `docs/superpowers/plans/README.md` lists this wave as a **Completed milestone** row until superseded.
- No contradictory uplift of unsupported-gap `status` without cited validation logs.
