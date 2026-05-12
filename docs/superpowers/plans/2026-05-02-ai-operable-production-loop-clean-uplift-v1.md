# AI-Operable Production Loop Clean Uplift v1 Implementation Plan (2026-05-02)

> **For agentic workers:** This is a phase-gated milestone plan. Use focused phases with explicit Goal, Context, Constraints, Done When, and validation evidence. Do not treat this as permission to append unrelated work or weaken ready-claim evidence.

**Goal:** Replace the one-row-model-at-a-time remediation chain with a clean, official-practice-aligned production-loop uplift that reviews docs, skills, rules, subagents, agent manifests, and static checks before selecting the next implementation gap.

**Context:** `core-first-mvp` is closed as historical MVP evidence. Recent Editor AI Playtest slices added read-only diagnostics, preflight, readiness, operator handoff, evidence summary, remediation queue, and remediation handoff models. The next tiny evidence-review and closeout-report plans are candidates only; this milestone decides whether to merge, retire, or implement them based on end-to-end value.

**Constraints:**

- Keep `core-first-mvp` closed and historical.
- Prefer clean greenfield replacement, consolidation, or retirement over compatibility shims unless a future release policy requires compatibility.
- Keep always-loaded instructions concise; put reusable procedures in skills and specialized behavior in subagents.
- Do not claim production readiness without validation evidence.
- Do not execute validation commands, package scripts, arbitrary shell, raw/free-form command text, fixes, manifest mutation, evidence mutation, or remediation mutation from editor core.
- Do not expose renderer/RHI/native handles, claim play-in-editor productization, claim Metal readiness, or claim general renderer quality in this milestone.

## Phase 1: Governance and Static Checks

**Goal:** Make the planning model explicit so agents can use a phase-gated milestone plan when a chain of tiny active slices hides the end-to-end objective.

**Done When:**

- `AGENTS.md`, docs, plan registry, Codex/Claude skills, Claude rules, and read-only subagents describe the phase-gated milestone plan rule.
- Static checks reject stale guidance that lacks the milestone rule.
- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this plan.

- [x] Inventory current docs, skills, rules, subagents, manifest, and active/recommended plans.
- [x] Add RED static checks for missing phase-gated milestone guidance.
- [x] Update docs, skills, rules, subagents, manifest, and old candidate plan notes.
- [x] Run agent/schema/context validation and record evidence.

## Phase 2: Remediation Chain Triage

**Goal:** Decide whether the planned remediation evidence-review and closeout-report slices add real value after remediation handoff.

**Done When:**

- The milestone records whether to implement, merge, or retire the candidate plans.
- Any accepted follow-up remains read-only, deterministic, externally supplied input only, and non-mutating.
- No extra row model is added without a consumer or validation evidence.

- [x] Compare the candidate evidence-review and closeout-report plans against existing `EditorAiPlaytestEvidenceSummaryModel`, `EditorAiPlaytestRemediationQueueModel`, and `EditorAiPlaytestRemediationHandoffModel`.
- [x] Record the smallest valuable next surface, or retire the candidate plans as unnecessary.

Phase 2 decision: retire the separate remediation evidence-review and closeout-report candidates. `EditorAiPlaytestEvidenceSummaryModel` already accepts externally supplied follow-up evidence and reports `passed`, `failed`, `blocked`, `host_gated`, or `missing` rows. `EditorAiPlaytestRemediationQueueModel` already turns non-passing evidence into queue rows, and `EditorAiPlaytestRemediationHandoffModel` already assigns external owner/action handoff. A separate evidence-review model would duplicate evidence summary with a narrower input name; a separate closeout report would duplicate `all_required_evidence_passed`, `remediation_required=false`, and `handoff_required=false` without adding a new operator decision.

## Phase 3: End-to-End Operator Workflow Consolidation

**Goal:** Document and test the current Editor AI playtest operator workflow as one inspectable loop instead of isolated row-model slices.

**Done When:**

- Docs identify the external operator workflow from package diagnostics to preflight, readiness, handoff, evidence summary, remediation queue, and remediation handoff.
- Unsupported claims remain explicit.
- Any code change follows RED -> GREEN and validation evidence.

- [x] Identify the minimum docs/tests needed to make the current loop understandable.
- [x] Avoid adding command execution, fixes, or mutation to editor core.

Phase 3 consolidation: package diagnostics -> validation recipe preflight -> readiness -> operator handoff -> evidence summary -> remediation queue -> remediation handoff is now represented as one inspectable manifest/docs workflow, `editor-ai-playtest-operator-workflow`, over the existing ready models. closeout through existing evidence summary means an external operator supplies new evidence after remediation, reruns `EditorAiPlaytestEvidenceSummaryModel`, and only treats the loop as closed when evidence passes and no remediation queue or handoff rows remain. Editor core still does not execute validation commands, run package scripts, evaluate raw/free-form command text, mutate evidence/remediation, apply fixes, productize play-in-editor, expose renderer/RHI handles, claim Metal readiness, or claim renderer quality.

## Phase 4: Next Production Gap Selection

**Goal:** Select the next real production gap after the AI-operable loop is coherent.

**Done When:**

- The plan registry and manifest point to a focused next plan.
- The chosen plan is based on current capability gaps, validation evidence, and host gates.

Candidate directions:

- Editor productization on top of reviewed authoring surfaces.
- Future 3D playable vertical slice work.
- Package/asset authoring or runtime delivery gap closure.
- Host-gated Apple/Metal validation only on macOS/Xcode evidence.

- [x] Select the next focused production gap and update the manifest/registry recommendation.

Phase 4 selection: choose `docs/superpowers/plans/2026-05-02-editor-ai-playtest-operator-workflow-ux-v1.md` as the next focused gap. This is the smallest host-feasible editor-productization step after the cleanup because it gives an operator a usable view over already reviewed, read-only models instead of adding more backend row models. It must not claim play-in-editor productization or move validation/package execution, evidence mutation, remediation mutation, or fix execution into editor core.

## Validation Evidence

- 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: RED as expected after adding static checks first; failed with `AGENTS.md did not contain expected text: phase-gated milestone plan`.
- 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS, `ai-integration-check: ok`.
- 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS, `json-contract-check: ok`.
- 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS, emitted `currentActivePlan=docs/superpowers/plans/2026-05-02-ai-operable-production-loop-clean-uplift-v1.md`.
- 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS, executed `agent-check` and `schema-check`.
- 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: PASS, CTest `28/28`.
- 2026-05-02: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, `validate: ok`. Diagnostic-only host gates remained explicit: Metal tools missing, Apple packaging requires macOS/Xcode, strict tidy compile database availability gated, and direct `cmake` missing on `PATH`.
- 2026-05-02: RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: failed as expected after adding consolidated workflow static checks first with `engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-operator-workflow review loop`.
- 2026-05-02: RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: failed as expected after adding consolidated workflow static checks first with `engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-operator-workflow review loop`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS, `json-contract-check: ok`, after adding manifest `editor-ai-playtest-operator-workflow`, retiring candidate plans, and selecting `docs/superpowers/plans/2026-05-02-editor-ai-playtest-operator-workflow-ux-v1.md` as `recommendedNextPlan`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS, `ai-integration-check: ok`, after docs/manifest/static checks were synchronized for consolidated operator workflow and retired remediation evidence-review/closeout candidates.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS, emitted `currentActivePlan=docs/superpowers/plans/2026-05-02-ai-operable-production-loop-clean-uplift-v1.md` and `recommendedNextPlan.path=docs/superpowers/plans/2026-05-02-editor-ai-playtest-operator-workflow-ux-v1.md`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS, executed `agent-check` and `schema-check`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: PASS, CTest `28/28`.
- 2026-05-02: GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, `validate: ok`. Diagnostic-only host gates remained explicit: direct `cmake` missing on `PATH`, Metal tools missing, Apple packaging requires macOS/Xcode, and strict tidy compile database availability gated.
