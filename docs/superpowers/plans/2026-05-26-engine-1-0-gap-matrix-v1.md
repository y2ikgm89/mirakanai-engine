# Engine 1.0 Gap Matrix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reconcile the current Engine 1.0 2D/3D capability matrix, active-plan pointers, and next implementation selection so follow-up engine work starts from the official project surfaces instead of ad hoc feature lists.

**Architecture:** This is a docs/manifest governance slice. It does not add C++ behavior; it makes `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, the plan registry, `docs/current-capabilities.md`, `docs/roadmap.md`, and the production-completion projection chapter agree on one active selection path. Concrete engine implementation starts in a later dated capability plan selected from the canonical backlog.

**Tech Stack:** Markdown plans/docs, JSON manifest fragments, PowerShell 7 repository validation wrappers, composed `engine/agent/manifest.json`.

---

**Plan ID:** `engine-1-0-gap-matrix-v1`

**Status:** Active.

**Context:** `Generated Game Studio v1` has package/evidence orchestration implemented. The next recommended work is not another broad implementation wave; it is a short official selection pass that maps 2D/3D "necessary / useful / future" engine capabilities onto canonical backlog rows and records the next concrete implementation target.

**Constraints:**

- Keep `unsupportedProductionGaps = []`; this plan must not reopen closed Engine 1.0 ready claims.
- Treat this as a docs/governance slice unless a later selected plan changes C++ behavior.
- Keep broad commercial-engine claims out of ready status.
- Use repository PowerShell 7 entrypoints, not Bun or package-script aliases.
- When the next implementation plan touches CMake, vcpkg, SDL3, D3D12, Vulkan, Metal, Android, Apple, middleware, C++ tooling, OpenAI, or Anthropic surfaces, that later plan must cite current official documentation or Context7 evidence before changing behavior.

**Done When:** The active plan registry, current-truth docs, 2D/3D projection matrix, manifest fragment, composed manifest, and focused static checks all agree that this selection slice is active and that the first follow-up implementation should be a narrow dated capability plan, not a broad "everything engine" wave.

## Decision

Use this selection order:

1. Finish this governance slice by syncing the current matrix and pointers.
2. Select `sprite-collision-hitbox-v1` as the first concrete implementation candidate unless the operator explicitly chooses a different row before implementation starts. It is a narrow, high-value 2D production primitive that currently appears as a post-1.x row and improves platformers, action games, adventure games, projectile games, and combat interactions without requiring backend or platform host gates.
3. After `sprite-collision-hitbox-v1`, prefer a short 2D production wave over a broad 3D wave: `sprite-sorting-layer-v1`, `sprite-9slice-and-tiled-v1`, and `sprite-effects-particles-v1`.
4. Start a separate `generated-3d-production-vertical-slice-v1` milestone only after the 2D production wave has a clean package evidence path, or if the operator explicitly prioritizes 3D.
5. Keep runtime/background streaming, runtime UI platform adapters, editor productization, multiplayer execution, and Apple/Metal host evidence as later separate plans because they cross different architecture and validation boundaries.

## File Structure

- Create: `docs/superpowers/plans/2026-05-26-engine-1-0-gap-matrix-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json`

## Task 1: Active Plan Pointer Sync

**Files:**
- Create: `docs/superpowers/plans/2026-05-26-engine-1-0-gap-matrix-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json`

- [ ] **Step 1: Create this dated plan**

Create the file with the header, `Plan ID`, `Status`, `Context`, `Constraints`, `Done When`, decision, file structure, tasks, validation commands, and closeout checklist in this plan.

- [ ] **Step 2: Update the registry active row**

In `docs/superpowers/plans/README.md`, replace the current active milestone row with:

```markdown
| Active slice (`currentActivePlan`) | [Engine 1.0 Gap Matrix v1](2026-05-26-engine-1-0-gap-matrix-v1.md) | Short official selection pass that reconciles the 2D/3D coverage matrix, active plan pointers, current-truth docs, and next implementation candidate while keeping `unsupportedProductionGaps = []` and avoiding broad commercial-engine ready claims. |
```

Move `Generated Game Studio v1` to a completed milestone row with its narrow orchestration claim and explicit non-goals.

- [ ] **Step 3: Update the manifest fragment**

In `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, set:

```json
"currentActivePlan": "docs/superpowers/plans/2026-05-26-engine-1-0-gap-matrix-v1.md",
"recommendedNextPlan": {
  "id": "engine-1-0-gap-matrix-v1",
  "title": "Engine 1.0 Gap Matrix v1",
  "status": "active",
  "path": "docs/superpowers/plans/2026-05-26-engine-1-0-gap-matrix-v1.md",
  "latestCloseoutEvidence": "Engine 1.0 Gap Matrix v1 is the active docs/manifest governance slice after Generated Game Studio v1. It reconciles the 2D/3D coverage matrix, active plan registry, current-truth docs, and next implementation selection while keeping unsupportedProductionGaps empty and broad commercial-engine claims out of ready status.",
  "completedContext": "<retain Generated Game Studio v1 closeout evidence plus historical renderer/RHI, Frame Graph, upload staging, and UI/importer closeout context>",
  "reason": "Engine 1.0 Gap Matrix v1 is the active official selection pass after Generated Game Studio v1. It keeps unsupportedProductionGaps empty, avoids broad commercial-engine ready claims, and selects sprite-collision-hitbox-v1 as the next implementation candidate unless the operator changes the canonical row before implementation starts."
}
```

- [ ] **Step 4: Keep static guards aligned**

If `recommendedNextPlan.id` changes, update the matching `tools/check-ai-integration-*.ps1` and `tools/check-json-contracts-*.ps1` needles so the new active plan is checked directly instead of falling through to stale legacy closeout assumptions.

- [ ] **Step 5: Compose the manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: command exits 0 and updates `engine/agent/manifest.json`.

## Task 2: 2D/3D Matrix Priority Pass

**Files:**
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`

- [ ] **Step 1: Add the current implementation sequence**

Under `## 2D / 3D Capability Coverage Matrix`, add a short `## Current Recommended Implementation Sequence` section that records the sequence from the Decision section of this plan.

- [ ] **Step 2: Keep the matrix canonical-row based**

Do not add new capability ids in this task. Confirm every next-work item points at an existing canonical row in `04-developer-owned-engine-capability-backlog.md`, except `generated-3d-production-vertical-slice-v1`, which must be introduced only by its later dated plan if selected.

- [ ] **Step 3: Preserve narrow ready claims**

Keep 2D and 3D matrix rows in one of the existing statuses: `covered-foundation`, `post-1x-row`, `host-gated`, or `game-owned`. Do not add a `commercial-ready` or `all-done` status.

## Task 3: Current-Truth Docs Sync

**Files:**
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`

- [ ] **Step 1: Update the readiness ledger current verdict**

Change the active-plan statement so it points at `Engine 1.0 Gap Matrix v1`, keeps `unsupportedProductionGaps = []`, and states that this is a governance selection slice rather than a reopened 1.0 blocker.

- [ ] **Step 2: Update current capabilities**

In the current active work paragraph, record that `Generated Game Studio v1` is completed for orchestration and that `Engine 1.0 Gap Matrix v1` is active for selecting the next implementation candidate.

- [ ] **Step 3: Update roadmap**

In the AI-operable production roadmap summary, replace the active `Generated Game Studio v1` wording with `Engine 1.0 Gap Matrix v1` wording and keep the Generated Game Studio sentence as completed evidence.

## Task 4: Validation

**Files:**
- Check only.

- [ ] **Step 1: Verify repository toolchain surface**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
```

Expected: `toolchain-check: ok`.

- [ ] **Step 2: Verify agent configuration surface**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: `agent-config-check: ok`.

- [ ] **Step 3: Verify manifest composition**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: `json-contract-check: ok`.

- [ ] **Step 4: Verify agent-surface drift**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: `ai-integration-check: ok`.

- [ ] **Step 5: Verify production readiness audit**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: audit accepts `unsupportedProductionGaps = []` and does not report malformed active-plan fields.

- [ ] **Step 6: Verify tracked text formatting**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: text format check passes. If it reports newline or whitespace issues in touched Markdown/JSON files, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format-text.ps1
```

Then rerun the check.

## Closeout Checklist

- [ ] `docs/superpowers/plans/README.md` lists this plan as active and Generated Game Studio v1 as completed.
- [ ] `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point to this plan.
- [ ] `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md` records the recommended implementation sequence.
- [ ] `docs/current-capabilities.md`, `docs/roadmap.md`, and the readiness ledger do not contradict the manifest active pointer.
- [ ] Focused checks pass or any blocker is recorded with exact command output.
